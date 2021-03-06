#include "Reconstruct.h"

#include "Clustering.h"
#include "CandidateBuilder.h"
#include "UpdateableManager.h"

#include "expconfig/ExpConfig.h"

#include "tree/TEventData.h"

#include "base/std_ext/vector.h"
#include "base/Logger.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>
#include <cassert>

using namespace std;
using namespace ant;
using namespace ant::reconstruct;

Reconstruct::Reconstruct() {}

// implement the destructor here,
// makes forward declaration work properly
Reconstruct::~Reconstruct() {}

void Reconstruct::Initialize(const TID& tid)
{
    const auto& setup = ExpConfig::Setup::Get(tid);

    // hooks are usually calibrations, which may also be updateable
    const shared_ptr_list<ReconstructHook::Base>& hooks = setup->GetReconstructHooks();
    for(const auto& hook : hooks) {
        std_ext::AddToSharedPtrList<ReconstructHook::DetectorReadHits, ReconstructHook::Base>
                (hook, hooks_readhits);
        std_ext::AddToSharedPtrList<ReconstructHook::ClusterHits, ReconstructHook::Base>
                (hook, hooks_clusterhits);
        std_ext::AddToSharedPtrList<ReconstructHook::Clusters, ReconstructHook::Base>
                (hook, hooks_clusters);
        std_ext::AddToSharedPtrList<ReconstructHook::EventData, ReconstructHook::Base>
                (hook, hooks_eventdata);
    }

    // put the detectors in a map for convenient access
    const shared_ptr_list<Detector_t>& detectors = setup->GetDetectors();
    for(const auto& detector : detectors) {
        auto ret = sorted_detectors.insert(make_pair(detector->Type, detector));
        if(!ret.second) {
            throw Exception("Reconstruct config provided detector list with two detectors of same type");
        }
    }

    // init clustering
    clustering = std_ext::make_unique<Clustering>(setup);

    // init the candidate builder
    /// \todo Make use of different candidate builders maybe?
    candidatebuilder = std_ext::make_unique<CandidateBuilder>(setup);

    // init the updateable manager
    updateablemanager = std_ext::make_unique<UpdateableManager>(
                            tid, setup->GetUpdateables()
                            );

    initialized = true;
}

void Reconstruct::DoReconstruct(TEventData& reconstructed)
{
    // ignore empty events
    if(reconstructed.DetectorReadHits.empty())
        return;

    if(!initialized) {
        Initialize(reconstructed.ID);
    }

    // update the updateables :)
    updateablemanager->UpdateParameters(reconstructed.ID);

    // apply the hooks for detector read hits (mostly calibrations),
    // note that this also changes the hits itself

    ApplyHooksToReadHits(reconstructed.DetectorReadHits);
    // the detectorReads are now calibrated as far as possible
    // one might return now and detectorRead is just calibrated...


    // do the hit matching, which builds the TClusterHit's
    // put into the AdaptorTClusterHit to track Energy/Timing information
    // for subsequent clustering
    sorted_bydetectortype_t<TClusterHit> sorted_clusterhits;
    BuildHits(sorted_clusterhits, reconstructed.TaggerHits);

    // apply hooks which modify clusterhits
    for(const auto& hook : hooks_clusterhits) {
        hook->ApplyTo(sorted_clusterhits);
    }

    // then build clusters (at least for calorimeters this is not trivial)
    sorted_clusters_t sorted_clusters;
    BuildClusters(move(sorted_clusterhits), sorted_clusters);

    // apply hooks which modify clusters
    for(const auto& hook : hooks_clusters) {
        hook->ApplyTo(sorted_clusters);
    }

    // do the candidate building
    candidatebuilder->Build(move(sorted_clusters),
                            reconstructed.Candidates, reconstructed.Clusters);

    // apply hooks which may modify the whole event
    for(const auto& hook : hooks_eventdata) {
        hook->ApplyTo(reconstructed);
    }

}

void Reconstruct::ApplyHooksToReadHits(
        std::vector<TDetectorReadHit>& detectorReadHits)
{
    // categorize the hits by detector type
    // this is handy for all subsequent reconstruction steps
    // we need to use non-const references because calibrations
    // may change the content (use std::reference_wrapper to hold it in vector)
    sorted_readhits.clear();
    for(TDetectorReadHit& readhit : detectorReadHits) {
        sorted_readhits.add_item(readhit.DetectorType, readhit);
    }

    // apply calibration
    // this may change the given readhits
    for(const auto& hook : hooks_readhits) {
        hook->ApplyTo(sorted_readhits);
    }
}

void Reconstruct::BuildHits(sorted_bydetectortype_t<TClusterHit>& sorted_clusterhits,
        vector<TTaggerHit>& taggerhits)
{
    auto insert_hint = sorted_clusterhits.cbegin();

    for(const auto& it_hit : sorted_readhits) {
        const Detector_t::Type_t detectortype = it_hit.first;
        const auto& readhits = it_hit.second;

        // find the detector instance for this type
        const auto& it_detector = sorted_detectors.find(detectortype);
        if(it_detector == sorted_detectors.end())
            continue;
        const detector_ptr_t& detector = it_detector->second;

        // for tagger detectors, we do not match the hits by channel at all
        if(detector.TaggerDetector != nullptr) {
            HandleTagger(detector.TaggerDetector, readhits, taggerhits);
            continue;
        }

        map<unsigned, TClusterHit> hits;

        for(const TDetectorReadHit& readhit : readhits) {
            // ignore uncalibrated items
            if(readhit.Values.empty())
                continue;

            /// \todo think about multi hit handling here?
            TClusterHit& hit = hits[readhit.Channel];
            hit.Data.emplace_back(readhit.ChannelType, readhit.Values.front());
            hit.Channel = readhit.Channel;

            if(readhit.ChannelType == Channel_t::Type_t::Integral)
                hit.Energy = readhit.Values.front();
            else if(readhit.ChannelType == Channel_t::Type_t::Timing)
                hit.Time = readhit.Values.front();
        }

        TClusterHitList clusterhits;
        for(const auto& hit : hits)
            clusterhits.emplace_back(move(hit.second));


        // The trigger or tagger detectors don't fill anything
        // so skip it
        if(clusterhits.empty())
            continue;

        // insert the clusterhits
        insert_hint =
                sorted_clusterhits.insert(insert_hint,
                                          make_pair(detectortype, move(clusterhits)));
    }
}

void Reconstruct::HandleTagger(const shared_ptr<TaggerDetector_t>& taggerdetector,
                               const std::vector<std::reference_wrapper<TDetectorReadHit> >& readhits,
                               std::vector<TTaggerHit>& taggerhits
                               )
{

    // gather electron hits by channel
    struct taggerhit_t {
        std::vector<double> Timings;
        std::vector<double> Energies;
    };
    map<unsigned, taggerhit_t > hits;

    for(const TDetectorReadHit& readhit : readhits) {
        // ignore uncalibrated items
        if(readhit.Values.empty())
            continue;

        auto& item = hits[readhit.Channel];
        if(readhit.ChannelType == Channel_t::Type_t::Timing) {
            std_ext::concatenate(item.Timings, readhit.Values);
        }
        else if(readhit.ChannelType == Channel_t::Type_t::Integral) {
            std_ext::concatenate(item.Energies, readhit.Values);
        }
    }

    for(const auto& hit : hits) {
        const auto channel = hit.first;
        const auto& item = hit.second;
        // create a taggerhit from each timing for now
        /// \todo handle double hits here?
        /// \todo handle energies here better? (actually test with appropiate QDC run)
        const auto qdc_energy = item.Energies.empty() ? std_ext::NaN : item.Energies.front();
        for(const auto timing : item.Timings) {
            taggerhits.emplace_back(channel,
                                    taggerdetector->GetPhotonEnergy(channel),
                                    timing,
                                    qdc_energy
                                    );
        }
    }
}

void Reconstruct::BuildClusters(
        const sorted_clusterhits_t& sorted_clusterhits,
        sorted_clusters_t& sorted_clusters)
{
    auto insert_hint = sorted_clusters.begin();

    for(const auto& it_clusterhits : sorted_clusterhits) {
        const Detector_t::Type_t detectortype = it_clusterhits.first;
        const TClusterHitList& clusterhits = it_clusterhits.second;

        // find the detector instance for this type
        const auto& it_detector = sorted_detectors.find(detectortype);
        if(it_detector == sorted_detectors.end())
            continue;
        const detector_ptr_t& detector = it_detector->second;

        TClusterList clusters;

        // check if detector supports clustering
        if(detector.ClusterDetector != nullptr) {
            // yes, then hand over to clustering algorithm
            clustering->Build(detector.ClusterDetector, clusterhits, clusters);
        }
        else {
            // in case of no clustering detector,
            // build simple "cluster" consisting of single TClusterHit
            for(const TClusterHit& hit : clusterhits) {

                // ignore hits with time and energy information
                if(!hit.IsSane())
                    continue;


                clusters.emplace_back(
                                          detector.Detector->GetPosition(hit.Channel),
                                          hit.Energy,
                                          hit.Time,
                                          detector.Detector->Type,
                                          hit.Channel,
                                          vector<TClusterHit>{hit}

                                      );

            }
        }

        // insert the clusters (if any)
        if(!clusters.empty()) {
            insert_hint =
                    sorted_clusters.insert(insert_hint,
                                           make_pair(detectortype, move(clusters)));
        }
    }
}

