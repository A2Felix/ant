#pragma once

#include "base/interval.h"
#include "calibration/gui/GUIbase.h"

#include <memory>

#include "TF1Knobs.h"

#include <list>
#include <string>
#include "base/std_ext.h"

#include "Rtypes.h"

class TH1;
class TF1;

namespace ant {
namespace calibration {
namespace gui {


class FitFunction {
public:
    using knoblist_t = std::list<std::unique_ptr<VirtualKnob>>;

protected:
    knoblist_t knobs;

    template <typename T, typename ... Args_t>
    void Addknob(Args_t&& ... args) {
        knobs.emplace_back(std_ext::make_unique<T>(std::forward<Args_t>(args)...));
    }

public:

    virtual ~FitFunction();
    virtual void Draw() =0;
    knoblist_t& getKnobs() { return knobs; }
    virtual void Fit(TH1* hist) =0;
};



class FitFunctionGaus: public FitFunction {
protected:
    TF1* func = nullptr;

    class MyWKnob: public VirtualKnob {
    protected:
        TF1* func = nullptr;
    public:

        MyWKnob(const std::string& n, TF1* Func);

        virtual double get() const override;
        virtual void set(double a) override;

    };

public:
    FitFunctionGaus(double A, double x0, double sigma, ant::interval<double> range);

    virtual ~FitFunctionGaus();

    virtual void Draw() override;

    virtual void Fit(TH1* hist) override;

};

}
}
}

