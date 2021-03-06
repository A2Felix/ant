#pragma once

#include "tree/TEventData.h"
#include "expconfig/ExpConfig.h"
#include "A2GeoAcceptance.h"

namespace ant {

namespace expconfig {
namespace detector {
struct CB;
struct PID;
struct TAPS;
struct TAPSVeto;
}
} // namespace ant::expconfig::detector

namespace analysis {
namespace utils {

class MCFakeReconstructed {
public:
    MCFakeReconstructed();
    virtual ~MCFakeReconstructed();

    /**
     * @brief Get returns best-effort faked reconstructed from the given mctrue
     * @param mctrue the mctrue information of the event
     * @return faked reconstructed TEventData from mctrue
     */
    const TEventData& Get(const TEventData& mctrue);
protected:
    std::unique_ptr<TEventData> dataptr;
    A2SimpleGeometry geo;

    const std::shared_ptr<expconfig::detector::CB>  cb;
    const std::shared_ptr<expconfig::detector::PID> pid;
    const std::shared_ptr<expconfig::detector::TAPS> taps;
    const std::shared_ptr<expconfig::detector::TAPSVeto> tapsveto;

};


}}} // namespace ant::analysis::utils