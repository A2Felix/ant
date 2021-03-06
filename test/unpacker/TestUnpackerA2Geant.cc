#include "catch.hpp"
#include "catch_config.h"
#include "expconfig_helpers.h"

#include "Unpacker.h"
#include "tree/TEvent.h"
#include "tree/TEventData.h"

#include <string>

using namespace std;
using namespace ant;

void dotest_readall();
void dotest_single();

TEST_CASE("UnpackerA2Geant: Read all", "[unpacker]") {
    dotest_readall();
}

TEST_CASE("UnpackerA2Geant: Single Event", "[unpacker]") {
    dotest_single();
}


void dotest_readall() {
    test::EnsureSetup();

    std::unique_ptr<Unpacker::Module> unpacker;
    REQUIRE_NOTHROW(unpacker = ant::Unpacker::Get(string(TEST_BLOBS_DIRECTORY)+"/Geant_with_TID.root"));
    REQUIRE(unpacker != nullptr);

    unsigned nEvents = 0;
    while(auto event = unpacker->NextEvent()) {
        nEvents++;
        REQUIRE(event.HasReconstructed());
    }

    REQUIRE(nEvents==10);
}

void dotest_single() {
    std::unique_ptr<Unpacker::Module> unpacker;
    REQUIRE_NOTHROW(unpacker = ant::Unpacker::Get(string(TEST_BLOBS_DIRECTORY)+"/Geant_with_TID.root"));
    REQUIRE(unpacker != nullptr);

    auto event = unpacker->NextEvent();
    REQUIRE(event);

    REQUIRE(event.HasMCTrue());
    REQUIRE(event.HasReconstructed());

    TEventData& recon = event.Reconstructed();
    TEventData& mctrue = event.MCTrue();

    REQUIRE(mctrue.Target.Vertex.z == Approx(-1.77843));
    REQUIRE(std::isnan(recon.Target.Vertex.z));
}