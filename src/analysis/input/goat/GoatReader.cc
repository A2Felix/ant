#include "GoatReader.h"

#include "detail/TreeManager.h"

#include "tree/TEvent.h"
#include "tree/TEventData.h"

#include "base/WrapTFile.h"
#include "base/Logger.h"
#include "base/std_ext/memory.h"
#include "base/Detector_t.h"

#include "TTree.h"

#include <string>
#include <iostream>
#include <memory>


using namespace ant;
using namespace ant::analysis;
using namespace ant::analysis::input;
using namespace std;

/**
 * @brief map goat apparatus numbers to apparatus_t enum values
 * in case unknown values show up: -> exception and do not sliently ignore
 */
Detector_t::Any_t IntToDetector_t(const int& a) {
    auto d = Detector_t::Any_t::None;
    if(a & TrackInput::DETECTOR_NaI) {
        d |= Detector_t::Type_t::CB;
    }
    if(a & TrackInput::DETECTOR_PID) {
        d |= Detector_t::Type_t::PID;
    }
    if(a & TrackInput::DETECTOR_MWPC) {
        d |= Detector_t::Any_t::Tracker;
    }
    if(a & TrackInput::DETECTOR_BaF2) {
        d |= Detector_t::Type_t::TAPS;
    }
    if(a & TrackInput::DETECTOR_PbWO4) {
        d |= Detector_t::Type_t::TAPS;
    }
    if(a & TrackInput::DETECTOR_Veto) {
        d |= Detector_t::Type_t::TAPSVeto;
    }
    return d;
}

void GoatReader::CopyDetectorHits(TEventData& recon)
{
    auto fill_readhits = [] (TEventData& recon,
            const DetectorHitInput::Item_t item,
            Detector_t::Type_t type) {
        for(int i=0;i<item.nHits;i++) {
            const unsigned channel = item.Hits[i];
            const double energy = item.Energy[i];
            const double time = item.Time[i];
            if(isfinite(energy)) {
                recon.DetectorReadHits.emplace_back(
                            LogicalChannel_t{type, Channel_t::Type_t::Integral, channel},
                            vector<double>{energy}
                            );
            }
            if(isfinite(time)) {
                recon.DetectorReadHits.emplace_back(
                            LogicalChannel_t{type, Channel_t::Type_t::Timing, channel},
                            vector<double>{time}
                            );
            }
        }
    };

    fill_readhits(recon, detectorhits.NaI, Detector_t::Type_t::CB);
    fill_readhits(recon, detectorhits.PID, Detector_t::Type_t::PID);
    fill_readhits(recon, detectorhits.BaF2, Detector_t::Type_t::TAPS);
    fill_readhits(recon, detectorhits.Veto, Detector_t::Type_t::TAPSVeto);
    /// \todo think about MWPC stuff here
}

void GoatReader::CopyTagger(TEventData& recon)
{
    for( Int_t i=0; i<tagger.GetNTagged(); ++i) {
        recon.TaggerHits.emplace_back(
                    tagger.GetTaggedChannel(i),
                    tagger.GetTaggedEnergy(i),
                    tagger.GetTaggedTime(i)
                    );
    }
}

void GoatReader::CopyTrigger(TEventData& recon)
{
    TTrigger& ti = recon.Trigger;

    ti.DAQEventID = eventParameters.EventNumber;
    ti.CBEnergySum = trigger.GetEnergySum();
    ti.ClusterMultiplicity = trigger.GetMultiplicity();

    for( int err=0; err < trigger.GetNErrors(); ++err) {
        ti.DAQErrors.emplace_back(
                    trigger.GetErrorModuleID()[err],
                    trigger.GetErrorModuleIndex()[err],
                    trigger.GetErrorCode()[err]
                    );
    }
}

/**
 * @brief map the cluster sizes from goat to unisgend ints
 * negative values mean no hit in the calorimeter
 * map those to 0
 */
clustersize_t GoatReader::MapClusterSize(const int& size) {
    return size < 0 ? 0 : size;
}

void GoatReader::CopyTracks(TEventData& recon)
{


    for(Int_t i=0; i< tracks.GetNTracks(); ++i) {

        const Detector_t::Any_t det = IntToDetector_t(tracks.GetDetectors(i));

        // Goat does not provide clusters,
        // so simulate some with fuzzy logic...
        TClusterList clusters;
        /// \todo how does this work with MWPC?

        if(det & Detector_t::Any_t::Calo) {

            clusters.emplace_back(
                        vec3::RThetaPhi(1.0, tracks.GetTheta(i), tracks.GetPhi(i)),
                        tracks.GetClusterEnergy(i),
                        tracks.GetTime(i),
                        det & Detector_t::Type_t::CB ? Detector_t::Type_t::CB : Detector_t::Type_t::TAPS ,
                        tracks.GetCentralCrystal(i)
                        );
            auto& calo_cluster = clusters.back();
            if(tracks.GetShortEnergy(i)>0)
                calo_cluster.ShortEnergy = tracks.GetShortEnergy(i);

            double vetoEnergy = 0.0;
            if(det & Detector_t::Any_t::Veto) {
                vetoEnergy =  tracks.GetVetoEnergy(i);
                clusters.emplace_back(
                            vec3(std_ext::NaN, std_ext::NaN, std_ext::NaN), // no veto position available
                            vetoEnergy,
                            std_ext::NaN, // no veto timing available
                            det & Detector_t::Type_t::PID ? Detector_t::Type_t::PID : Detector_t::Type_t::TAPSVeto,
                            tracks.GetCentralVeto(i)
                            );

            }

            recon.Candidates.emplace_back(
                        det,
                        tracks.GetClusterEnergy(i),
                        tracks.GetTheta(i),
                        tracks.GetPhi(i),
                        tracks.GetTime(i),
                        MapClusterSize(tracks.GetClusterSize(i)),
                        vetoEnergy,
                        //tracks.GetMWPC0Energy(i)+tracks.GetMWPC1Energy(i),
                        std_ext::NaN, // MWPC not handled correctly at the moment
                        TClusterList(clusters.begin(), clusters.end())
                        );
        }
        else if(det & Detector_t::Any_t::Veto) {
            // veto-only track is just a cluster in Ant
            // don't know if such tracks actually exist in GoAT/Acqu...
            const double vetoEnergy =  tracks.GetVetoEnergy(i);
            clusters.emplace_back(
                        vec3(std_ext::NaN, std_ext::NaN, std_ext::NaN), // no veto position available
                        vetoEnergy,
                        std_ext::NaN, // no veto timing available
                        det & Detector_t::Type_t::PID ? Detector_t::Type_t::PID : Detector_t::Type_t::TAPSVeto,
                        tracks.GetCentralVeto(i)
                        );
        }

        // always add clusters...
        recon.Clusters.insert(recon.Clusters.end(), clusters.begin(), clusters.end());
    }
}


void GoatReader::CopyParticles(TEventData& recon, ParticleInput& input_module,
                               const ParticleTypeDatabase::Type& type)
{
    for(Int_t i=0; i < input_module.GetNParticles(); ++i) {

        const auto trackIndex = input_module.GeTCandidateIndex(i);
        if(trackIndex == -1) {
            LOG(ERROR) << "No Track for this particle!!" << endl;
        } else {
            const auto& track = recon.Candidates.get_ptr_at(trackIndex);
            recon.Particles.Add(std::make_shared<TParticle>(type,track));
        }

    }
}

class MyTreeRequestMgr: public TreeRequestManager {
protected:
    WrapTFileInput& m;
    TreeManager& t;

public:
    MyTreeRequestMgr(WrapTFileInput& _m, TreeManager& _t):
        m(_m), t(_t) {}

    TTree *GetTree(const std::string &name) {
        TTree* tree = nullptr;
        if( m.GetObject(name, tree) ) {
            t.AddTree(tree);
            VLOG(6) << "TTree " << name << " opened";
            return tree;
        } else
            VLOG(7) << "Could not find TTree " << name << " in any of the provided files";
            return nullptr;
    }

};

GoatReader::GoatReader(const std::shared_ptr<WrapTFileInput>& rootfiles):
    files(rootfiles),
    trees(std_ext::make_unique<TreeManager>())
{
    /// \todo find a smart way to manage trees and modules:
    //   if module does not init or gets removed-> remove also the tree from the list
    //   two modules use same tree?
    //   reset branch addresses ?

    for(auto module = active_modules.begin(); module != active_modules.end(); ) {

        if( (*module)->SetupBranches( MyTreeRequestMgr(*files, *trees))) {
            module++;
        } else {
            module = active_modules.erase(module);
            VLOG(7) << "Not activating GoAT Input module";
        }
    }
}

GoatReader::~GoatReader() {}

bool GoatReader::IsSource() { return trees->GetEntries()>0; }


bool GoatReader::ReadNextEvent(TEvent& event)
{
    if(current_entry>=trees->GetEntries())
        return false;

    trees->GetEntry(current_entry);

    active_modules.GetEntry();

    if(!event.HasReconstructed()) {
        /// \todo think of some better timestamp?
        const TID tid(
                    static_cast<std::uint32_t>(std::time(nullptr)),
                    static_cast<std::uint32_t>(current_entry),
                    std::list<TID::Flags_t>{TID::Flags_t::AdHoc}
                    );
        event.MakeReconstructed(tid);
    }

    auto& recon = event.Reconstructed();

    CopyDetectorHits(recon);
    CopyTrigger(recon);
    CopyTagger(recon);
    CopyTracks(recon);

    CopyParticles(recon, photons, ParticleTypeDatabase::Photon);
    CopyParticles(recon, protons, ParticleTypeDatabase::Proton);
    CopyParticles(recon, pichagred, ParticleTypeDatabase::eCharged);
    CopyParticles(recon, echarged, ParticleTypeDatabase::PiCharged);
    CopyParticles(recon, neutrons, ParticleTypeDatabase::Neutron);

    ++current_entry;
    return true;
}

double GoatReader::PercentDone() const
{
    return double(current_entry)/double(trees->GetEntries());
}
