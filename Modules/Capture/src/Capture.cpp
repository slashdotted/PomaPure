#include "Capture.h"

/* Declare module details (name, description, cparams, can be source) */
DEFAULT_EXPORT_ALL(Capture, "Capture module", "", true)

/* Constructor */
Capture::Capture(const std::string& mid) : poma::Module<Capture, PomaDataType>(mid) {}

void Capture::start_processing()
{
    for(;;) {
        cv::VideoCapture stream {0};
        PomaPacketType du;
        if(!stream.isOpened()) {
            std::cerr << this->m_module_id << ": got an error opening device" << std::endl;
            return;
        }
        while (true) {
            if (!(stream.read(du.m_data.m_image))) {
                std::cerr << this->m_module_id << ": got an error reading frame from device... aborting" << std::endl;
                break;
            }
            submit_data(du);
        }
    }
}
  


