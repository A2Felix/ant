#include "EventDisplayHists.h"

#include "base/std_ext/memory.h"
#include "base/std_ext/string.h"

#include "root-addons/cbtaps_display/TH2TAPS.h"
#include "root-addons/cbtaps_display/TH2CB.h"

#include "expconfig/ExpConfig.h"
#include "expconfig/detectors/TAPS.h"

#include "TCanvas.h"
#include "TMarker.h"

using namespace ant;
using namespace ant::std_ext;
using namespace ant::analysis::physics;
using namespace std;


EventDisplayHists::EventDisplayHists(const string& name, OptionsPtr opts):
    Physics(name, opts)
{
    const auto setup = ExpConfig::Setup::GetLastFound();

    if(!setup)
        throw std::runtime_error("No Setup found");

    const auto det = setup->GetDetector(Detector_t::Type_t::TAPS);

    if(det) {
        const auto taps = dynamic_pointer_cast<expconfig::detector::TAPS>(det);
        if(taps) {
            tapsZ = taps->GetZPosition();
        } else
            throw std::runtime_error("TAPS detector not found");

    } else
        throw std::runtime_error("TAPS detector not found");
}

EventDisplayHists::~EventDisplayHists()
{}

void EventDisplayHists::ProcessEvent(const TEvent& event, manager_t&)
{

    const auto& candidates = event.Reconstructed().Candidates;

    auto taps_cands = candidates.get_ptr_list(
                          [] (const TCandidate& cand) {
        return cand.Detector & Detector_t::Type_t::TAPS;
    });

    if(taps_cands.empty())
        return;

    auto tapsCal = HistFac.make<TH2TAPS>(formatter() << "taps" << n++, formatter() << taps_cands.size() << " Candidates (TAPS)");

    for(const auto& c : taps_cands) {
        const auto cluster = c->FindCaloCluster();

        for(const auto& hit : cluster->Hits) {
            tapsCal->SetElement(hit.Channel, hit.Energy);
        }

        tapsCal->CreateMarker(cluster->Position.XY(), kFullCircle, kOpenCircle);

        tapsCal->CreateMarker(cluster->CentralElement);

    }

    const auto true_particles = event.MCTrue().Particles.GetAll();

    for(const auto& p : true_particles) {
        if(p->Theta() < degree_to_radian(30.0)) {
            const auto xy = p->p.XY() * tapsZ / p->p.z;

            tapsCal->CreateMarker(xy, kFullSquare, kOpenSquare);

        }
    }
}


AUTO_REGISTER_PHYSICS(EventDisplayHists)
