#include "TaggerScalers.h"

#include "SlowControlProcessors.h"

#include "expconfig/ExpConfig.h"

using namespace std;
using namespace ant::analysis::slowcontrol;
using namespace ant::analysis::slowcontrol::variable;

list<Variable::ProcessorPtr> TaggerScalers::GetNeededProcessors()
{
    auto taggerdetector = ExpConfig::Setup::GetDetector<TaggerDetector_t>();
    if(!taggerdetector)
        return {};
    if(taggerdetector->Type == Detector_t::Type_t::EPT) {
        /// \todo check how EPT_2012 scalers were recorded
        /// for 2014, we know that EPT_Scalers are in Beampolmon VUPROMs
        mode = mode_t::EPT_2014;
        nChannels = taggerdetector->GetNChannels();
        return {Processors::EPT_Scalers, Processors::Beampolmon};
    }

    return {};
}

std::vector<double> TaggerScalers::Get() const
{
    vector<double> scalers(nChannels, std::numeric_limits<double>::quiet_NaN());
    if(mode == mode_t::EPT_2014) {
        const double reference = Processors::Beampolmon->Reference_1MHz.Get();
        for(const auto& kv : Processors::EPT_Scalers->Get()) {
            if(kv.Key<scalers.size())
                scalers[kv.Key] = 1.0e6*kv.Value/reference;
        }
    }
    return scalers;
}

