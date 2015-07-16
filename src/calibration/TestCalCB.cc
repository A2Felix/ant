#include "TestCalCB.h"
#include "analysis/plot/HistogramFactories.h"
#include "analysis/data/Event.h"
#include "analysis/utils/combinatorics.h"

using namespace std;
using namespace ant;
using namespace ant::calibration;

TestCalCB::TestCalCB():
    BaseCalibrationModule("TestCalCB")
{
    const BinSettings energybins(1000);

    ggIM = HistFac.makeTH1D("ggIM","2 #gamma IM [MeV]","#",energybins,"ggIM");
}



void TestCalCB::ProcessEvent(const Event &event)
{
    const auto& photons = event.Reconstructed().Particles().Get(ParticleTypeDatabase::Photon);

    for( auto comb = makeCombination(photons,2); !comb.Done(); ++comb ) {

        TLorentzVector gg = *comb.at(0) + *comb.at(1);
        ggIM->Fill(gg.M());
    }
}

void TestCalCB::Finish()
{

}

void TestCalCB::ShowResult()
{

}


void TestCalCB::ApplyTo(unique_ptr<TDetectorRead>&)
{

}

void TestCalCB::BuildRanges(list<TID> &ranges)
{

}

void TestCalCB::Update(const TID &id)
{

}