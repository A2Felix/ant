#pragma once

#include "Variable.h"

#include "base/Detector_t.h"

#include <memory>

namespace ant {
namespace analysis {
namespace slowcontrol {
namespace variable {

struct TaggerScalers : Variable {

    virtual std::list<ProcessorPtr> GetNeededProcessors() override;

    /**
     * @brief Get returns the tagger current scaler frequencies
     * @return
     */
    std::vector<double> Get() const;

protected:

    enum class mode_t {
        EPT_2014
    };

    mode_t mode;
    unsigned nChannels;

};

}}}} // namespace ant::analysis::slowcontrol::processor