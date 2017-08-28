#include "Resize.h"
#include <opencv2/imgproc/imgproc.hpp>

/* Declare module details (name, description, cparams, can be source) */
DEFAULT_EXPORT_ALL(Resize, "Resize module", "", false)

/* Constructor */
Resize::Resize(const std::string& mid) : poma::Module<Resize, PomaDataType>(mid) {}
  
/* Method for processing incoming data packets */
void Resize::on_incoming_data(PomaPacketType& dta, const std::string& channel)
{
	cv::Mat resized;
	cv::resize(dta.m_data.m_image, resized, cv::Size{}, 0.5, 0.5, cv::INTER_LINEAR);
	dta.m_data.m_image = resized;
    submit_data(dta);
}

