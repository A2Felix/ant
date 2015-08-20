#pragma once

#include "Calibration.h"

class TGraph;

namespace ant{
namespace calibration{

}
}

namespace ant {


namespace expconfig {
namespace detector {
class CB;
}}

namespace calibration {

class DataManager;

namespace gui {
class FitTimewalk;
}


class CB_TimeWalk :
        public Calibration::Module,
        public ReconstructHook::ClusterHits
{
public:
    CB_TimeWalk(
            const std::shared_ptr<expconfig::detector::CB>& cb,
            const std::shared_ptr<DataManager>& calmgr
            );
    virtual ~CB_TimeWalk();

    class ThePhysics : public analysis::Physics {
    protected:
        std::shared_ptr<expconfig::detector::CB> cb_detector;
        TH3D* h_timewalk;

    public:
        ThePhysics(const std::string& name, const std::shared_ptr<expconfig::detector::CB>& cb);

        virtual void ProcessEvent(const analysis::data::Event& event) override;
        virtual void Finish() override ;
        virtual void ShowResult() override;
    }; // ThePhysics

    class TheGUI : public gui::Manager_traits {
    protected:
        std::shared_ptr<DataManager> calibrationManager;
        std::shared_ptr<expconfig::detector::CB> cb_detector;
        std::vector< std::shared_ptr<gui::FitTimewalk> >& timewalks;
        std::shared_ptr<gui::FitTimewalk> last_timewalk;

        gui::CalCanvas* canvas;
        TH1D* means;

    public:
        TheGUI(const std::string& basename,
               const std::shared_ptr<DataManager>& calmgr,
               const std::shared_ptr<expconfig::detector::CB>& cb,
               std::vector<std::shared_ptr<gui::FitTimewalk> >& timewalks_);

        virtual std::string GetHistogramName() const override;
        virtual unsigned GetNumberOfChannels() const override;
        virtual void InitGUI();
        virtual std::list<gui::CalCanvas*> GetCanvases() const;

        virtual void StartRange(const interval<TID>& range);
        virtual DoFitReturn_t DoFit(TH1* hist, unsigned ch);
        virtual void DisplayFit();
        virtual void StoreFit(unsigned channel);
        virtual bool FinishRange();
        virtual void StoreFinishRange(const interval<TID>& range);
    }; // TheGUI

    virtual std::unique_ptr<analysis::Physics> GetPhysicsModule() override;
    virtual void GetGUIs(std::list<std::unique_ptr<calibration::gui::Manager_traits> >& guis) override;

    virtual void ApplyTo(clusterhits_t& sorted_clusterhits) override;

    // Updateable_traits interface
    virtual std::vector<std::list<TID> > GetChangePoints() const override;
    virtual void Update(std::size_t index, const TID& id) override;

protected:
    std::vector< std::shared_ptr<gui::FitTimewalk> > timewalks;

    std::shared_ptr<expconfig::detector::CB> cb_detector;
    std::shared_ptr<DataManager> calibrationManager;





};

}}