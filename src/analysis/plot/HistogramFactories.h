#pragma once

#include "base/interval.h"

#include <string>

class TDirectory;
class TH1D;
class TH2D;
class TH3D;
class TTree;

namespace ant {
namespace analysis {


class BinSettings: public interval<double>  {
protected:
    unsigned int bins;
public:

    BinSettings(unsigned int number_of_bins, double minimum, double maximum) noexcept:
        interval<double>(minimum,maximum),
        bins(number_of_bins)
    {}

    BinSettings(unsigned int number_of_bins) noexcept:
        interval<double>(0,number_of_bins),
        bins(number_of_bins)
    {}

    BinSettings(unsigned int number_of_bins, const interval<double>& i) noexcept:
        interval<double>(i),
        bins(number_of_bins)
    {}

    unsigned int Bins() const noexcept { return bins; }
    unsigned int& Bins()   noexcept { return bins; }

    double BinWidth() const { return Length() / bins; }

    /**
     * @brief Produces BinSettings close to the supplied one, but with a given bin size.
     *        Slightly modifies the number of bins and boundaries.
     *        Useful to to counteract binning effects if intrinsic spacing of data is known (for example ADC channel widths).
     * @param bins Target settings
     * @param binSize size to match
     * @return new settings with the given bin size
     */
    static BinSettings RoundToBinSize(const BinSettings& bins, const double binSize);
};

class HistogramFactory {
private:

    TDirectory* my_directory = nullptr;
    void goto_dir() const;

    struct DirStackPush {
        TDirectory* dir;
        DirStackPush();
        ~DirStackPush();
    };

    std::string title_prefix;

    std::string MakeTitle(const std::string& title) const;

    /**
     * @brief create a new TDirectory. If a TDirectory already exists, append a number (1, 2, 3, ...) at the end
     * @param name Name of the directory to create
     * @param rootdir where to create
     * @return new created directory
     */
    static TDirectory* mkDirNumbered(const std::string& name, TDirectory* rootdir);

    mutable unsigned n_unnamed = 0;
    std::string GetNextHistName(const std::string& name) const;


public:
    HistogramFactory(const std::string& directory_name, TDirectory* root=nullptr, const std::string& title_prefix_ = "");
    HistogramFactory(const std::string& directory_name, const HistogramFactory &parent, const std::string& title_prefix_ = "");

    void SetRootDir(TDirectory* root_dir=nullptr);
    void SetTitlePrefix(const std::string& title_prefix_);
    std::string GetTitlePrefix() const;
    void SetDirDescription(const std::string& desc);

    TH1D* makeTH1D(
            const std::string& title,
            const std::string& xlabel,
            const std::string& ylabel,
            const BinSettings& bins,
            const std::string& name="") const;

    TH2D* makeTH2D(
            const std::string& title,
            const std::string& xlabel,
            const std::string& ylabel,
            const BinSettings& xbins,
            const BinSettings& ybins,
            const std::string& name="") const;

    TH3D* makeTH3D(const std::string& title,
            const std::string& xlabel,
            const std::string& ylabel,
            const std::string& zlabel,
            const BinSettings& xbins,
            const BinSettings& ybins,
            const BinSettings& zbins,
            const std::string& name="") const;

    TTree* makeTTree(const std::string& name);

    template<class T, typename... Args>
    T* make(Args&&... args) const {

        // save current dir and cd back to it on exit
        DirStackPush dirstack;
        goto_dir();

        auto t = new T(std::forward<Args>(args)...);
        return t;
    }

};

}
}
