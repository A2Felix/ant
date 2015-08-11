#pragma once
#include "analysis/plot/root_draw.h"
#include "base/interval.h"
#include "DataManager.h"

#include <memory>
#include <string>
#include <vector>

class TH2D;

namespace ant
{

namespace calibration
{

class Editor
{

private:
    DataBase dman;

    bool getIDRange(const std::string& calibrationID, interval<TID>& IDinterval) const;
    std::vector<std::pair<std::uint32_t,ant::TCalibrationData>> getValidData(const std::string& calibrationID) const;

public:
    Editor(): dman(){}

    //File Operations
    bool AddFromFile(const std::string& fileName) {return dman.ReadData(fileName); }
    void SaveToFile(const std::string& fileName) const {dman.WriteData(fileName);}

    //Element Access
    void Add(const TCalibrationData& cdata) {dman.DataMap[cdata.CalibrationID].push_back(cdata);}
    bool Remove(const std::string& calibrationID, const std::uint32_t& index);
    bool Remove(const std::string& calibrationID,
                const std::uint32_t& index1, const std::uint32_t& index2);

    //queries
    std::list<std::string> GetListOfCalibrations() const;
    bool Has(const std::string& calibrationID) const {return dman.Has(calibrationID);}
    std::uint32_t GetNumberOfSteps(const std::string& calibrationID) const;

    //changes to db
    bool ReduceToValid(const std::string&  calibrationID);
    bool ExpandToMax(const std::string& calibrationID, std::uint32_t index);

    //visualisation
    std::list<std::pair<std::uint32_t,IntervalD>> GetAllRanges(const std::string& calibrationID) const;
    std::list<std::pair<std::uint32_t,IntervalD>> GetAllValidRanges(const std::string& calibrationID) const;
    std::pair<std::uint32_t, IntervalD> GetRange(const std::string &calibrationID, std::uint32_t index) const;




    // === soon obsolete
    interval<TID> GetMaxInt(const std::string& calibrationID) const;
    // === soon obsolete
    bool ShowHistory(const std::string& calibrationID) const;
    /**
     * @brief ShowValid shows only calibration steps which are used
     * @param calibrationID
     */
    // === soon obsolete
    bool ShowValid(const std::string&  calibrationID) const;



};

}
}