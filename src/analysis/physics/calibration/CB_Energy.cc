#include "CB_Energy.h"

#include "utils/combinatorics.h"

#include "expconfig/ExpConfig.h"
#include "expconfig/detectors/CB.h"

using namespace std;
using namespace ant;
using namespace ant::analysis::physics;

CB_Energy::CB_Energy(const string& name, OptionsPtr opts) :
    Physics(name, opts)
{
    auto detector = ExpConfig::Setup::GetDetector(Detector_t::Type_t::CB);

    const BinSettings cb_channels(detector->GetNChannels());
    const BinSettings energybins(1000);

    ggIM = HistFac.makeTH2D("2 neutral IM (CB,CB)", "IM [MeV]", "#",
                            energybins, cb_channels, "ggIM");
    h_cbdisplay = HistFac.make<TH2CB>("h_cbdisplay","Number of entries");
}

void CB_Energy::ProcessEvent(const TEvent& event, manager_t&)
{
    const auto& cands = event.Reconstructed().Candidates;

    for( auto comb = analysis::utils::makeCombination(cands.get_ptr_list(),2); !comb.Done(); ++comb ) {
        const TCandidatePtr& p1 = comb.at(0);
        const TCandidatePtr& p2 = comb.at(1);

        if(p1->VetoEnergy==0 && p2->VetoEnergy==0
           && (p1->Detector & Detector_t::Type_t::CB)
           && (p2->Detector & Detector_t::Type_t::CB)) {
            const TParticle a(ParticleTypeDatabase::Photon,comb.at(0));
            const TParticle b(ParticleTypeDatabase::Photon,comb.at(1));
            const auto& gg = a + b;

            auto cl1 = p1->FindCaloCluster();
            if(cl1)
                ggIM->Fill(gg.M(),cl1->CentralElement);

            auto cl2 = p2->FindCaloCluster();
            if(cl2)
                ggIM->Fill(gg.M(),cl2->CentralElement);
        }
    }
}

void CB_Energy::ShowResult()
{
    h_cbdisplay->SetElements(*ggIM->ProjectionY());
    canvas(GetName()) << drawoption("colz") << ggIM
                      << h_cbdisplay
                      << endc;
}

AUTO_REGISTER_PHYSICS(CB_Energy)