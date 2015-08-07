#include "Manager.h"

#include "calibration/gui/CalCanvas.h"

#include "tree/TDataRecord.h"
#include "tree/TAntHeader.h"

#include "base/interval.h"
#include "base/std_ext.h"
#include "base/WrapTFile.h"
#include "base/Logger.h"

#include "TH1D.h"
#include "TH2D.h"

#include <memory>

using namespace std;
using namespace ant;
using namespace ant::calibration;
using namespace ant::calibration::gui;


void Manager::BuildInputFiles(const vector<string>& filenames)
{
    if(filenames.empty())
        return;

    for(const auto& filename : filenames) {

        try {

            WrapTFile file(filename, WrapTFile::mode_t::read, false);

            TAntHeader* header = nullptr;
            file.GetObject("AntHeader", header);

            if(!header) {
                LOG(WARNING) << "No TAntHeader found in " << filename;
                continue;
            }

            if(SetupName.empty()) {
                SetupName = header->SetupName;
            }
            else if(SetupName != header->SetupName) {
                LOG(WARNING) << "Previously found setup name '" << SetupName
                             << "' does not match '" << header->SetupName << "' of file "
                             << filename;
                continue;
            }

            auto range = interval<TID>(header->FirstID, header->LastID);
            if(!range.IsSane()) {
                LOG(WARNING) << "Range " << range << " not sane in " << filename;
                continue;
            }
            input_files.emplace_back(filename, range);

        } catch (const std::runtime_error& e) {
            LOG(WARNING) << "Can't open " << filename << " " << e.what();
        }
    }

    input_files.sort();
    LOG(INFO) << "Loaded " << input_files.size()
              << " files from " << filenames.size() << " provided filenames ("
              << 100*(double)input_files.size()/filenames.size() << " %)";
}



void Manager::FillWorklistFromFiles()
{
    while(buffer.Worklist().empty() && state.it_file != input_files.end()) {
        const input_file_t& file_input = *state.it_file;
        try
        {
            WrapTFile file(file_input.filename, WrapTFile::mode_t::read, false);

            auto hist = file.GetSharedTH2(module->GetHistogramName());

            if(!hist) {
                LOG(WARNING) << "Histogram " << module->GetHistogramName() << " not found in " << file_input.filename;
            } else {
                buffer.Push(hist, file_input.range);
            }

        }
        catch (const std::runtime_error& e) {
            LOG(WARNING) << "Can't open " << file_input.filename << ": " << e.what();
        }
        state.it_file++;
    }

    if(state.it_file == input_files.end()) {
        VLOG(7) << "Reached end of files, processing remaining buffer";
        buffer.PushRestToWorklist();
    }
}

Manager::Manager(const std::vector<std::string>& inputfiles, unsigned avglength):
    buffer(avglength),
    state(),
    mode(std_ext::make_unique<CalCanvasMode>())
{
    BuildInputFiles(inputfiles);
}
void Manager::InitGUI(const char* receiver_class, void* receiver, const char* slot)
{
    module->InitGUI();
    for(CalCanvas* canvas : module->GetCanvases()) {
        canvas->LinkGUIMode(mode.get());
    }
    for(CalCanvas* canvas : module->GetCanvases()) {
        canvas->ConnectReturnFunc(receiver_class, receiver, slot);
    }
}

bool Manager::input_file_t::operator <(const Manager::input_file_t& o) const {
    return range.Start() < o.range.Start();
}

bool Manager::Run()
{
    if(!state.is_init) {
        state.channel = 0;
        state.it_buffer = buffer.begin();
        state.it_file = input_files.begin();
        FillWorklistFromFiles();
        if(buffer.Worklist().empty()) {
            LOG(WARNING) << "Did not process anything";
            return false;
        }
        state.is_init = true;
        module->StartRange(buffer.Worklist().front());
    }

    VLOG(8) << "Worklist size " << buffer.Worklist().size();

    if(!state.stop_finish) {
        if(!state.stop_fit) {
            const string& title = std_ext::formatter() << "Channel=" << state.channel
                                                       << " " << buffer.Worklist().front();
            buffer.Average()->SetTitle(title.c_str());

            const bool stop = module->DoFit(buffer.Average(), state.channel);

            if(stop || mode->alwaysDisplayFit) {
                VLOG(7) << "Open GUI...";
                module->DisplayFit();
                state.stop_fit = true;
                return false;
            }
        }
        module->StoreFit(state.channel);
        state.stop_fit = false;
    }

    if(state.stop_finish
       || (state.channel >= (int)module->GetNumberOfChannels() && mode->gotoNextRange))
    {

        if(!state.stop_finish) {
            VLOG(7) << "Finish module first";
            if(module->FinishRange()) {
                VLOG(7) << "GUI Opened (finish range)";
                state.stop_finish = true;
                return false;
            }
        }

        module->StoreFinishRange(buffer.Worklist().front());
        state.stop_finish = false;
        state.channel = 0;

        buffer.Worklist().pop();

        // try refilling the worklist
        FillWorklistFromFiles();
        if(buffer.Worklist().empty()) {
            /// \todo give module a chance to do something again here???
            for(CalCanvas* canvas : module->GetCanvases()) {
                canvas->Close();
            }
            LOG(INFO) << "Finished processing whole buffer";
            return false;
        }

        module->StartRange(buffer.Worklist().front());

    }
    else
    {
        state.channel += mode->channelStep;
        if(state.channel<0)
            state.channel = 0;
        else if(state.channel>(int)module->GetNumberOfChannels())
            state.channel = module->GetNumberOfChannels();
    }

    // continue running by default
    return true;
}

Manager::~Manager()
{

}