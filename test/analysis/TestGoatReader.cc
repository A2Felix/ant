#include "catch.hpp"

#include "analysis/input/goat/GoatReader.h"

#include "base/WrapTFile.h"

#include <iostream>

using namespace std;
using namespace ant;

void dotest();


TEST_CASE("GoatReader", "[analysis]") {
    dotest();
}

void dotest() {


    /// \todo Generate or read some Goat file?

//    auto filemanager = make_shared<WrapTFileInput>();
//    filemanager->OpenFile("NOTTHEREYET");
//    ant::analysis::input::GoatReader g(filemanager);

//    unsigned int n = 0;
//    while(g.hasData() && (n++ < 10)) {
//        auto event = g.ReadNextEvent();
//        cout << *event << endl;
//    }
}
