#ifndef PLUTOINPUT_H
#define PLUTOINPUT_H

#include "FileManager.h"
#include "InputModule.h"
#include <vector>

class TClonesArray;
class TTree;
class PParticle;

namespace ant {
namespace input {

class PlutoInput: public BaseInputModule {
public:
    using PParticleVector = std::vector<const PParticle*>;

protected:
    TTree*          data = nullptr;
    TClonesArray*   PlutoMCTrue = nullptr;
    int64_t         plutoID = 0;
    int64_t         plutoRandomID = 0;

    PParticleVector particles;
public:

    PlutoInput();
    virtual ~PlutoInput();

    bool SetupBranches(TreeRequestManager&& input_files);
    void GetEntry();

    const PParticleVector&  Particles() const   { return particles; }
    const int64_t           GetPlutoID() const  { return plutoID; }
    const int64_t           GetRandomID() const { return plutoRandomID; }
};
}
}

#endif
