#pragma once

#include "base/WrapTTree.h"
#include "analysis/utils/Fitter.h"

namespace ant {
namespace analysis {

class HistogramFactory;

namespace utils {

class PullsWriter {

public:
    struct PullTree_t : WrapTTree {

        ADD_BRANCH_T(double, FitProb)
        ADD_BRANCH_T(double, TaggW) // for prompt-random subtraction

        ADD_BRANCH_T(double, E)
        ADD_BRANCH_T(double, Theta)
        ADD_BRANCH_T(double, Phi)

        /// \todo Fill with previous values
        ADD_BRANCH_T(double, SigmaE)
        ADD_BRANCH_T(double, SigmaTheta)
        ADD_BRANCH_T(double, SigmaPhi)

        ADD_BRANCH_T(double, PullE)
        ADD_BRANCH_T(double, PullTheta)
        ADD_BRANCH_T(double, PullPhi)

        ADD_BRANCH_T(unsigned, Multiplicity)
    };

protected:
    PullTree_t photons_cb;
    PullTree_t photons_taps;

    PullTree_t proton_cb;
    PullTree_t proton_taps;

    PullTree_t& getPullTree(const ant::analysis::utils::Fitter::FitParticle& particle);

public:

    PullsWriter(ant::analysis::HistogramFactory& histfac);
    ~PullsWriter();

    void Fill(const std::vector<Fitter::FitParticle>& fitParticles,
              double tagger_weight, double fitprob);

};

}
}
}
