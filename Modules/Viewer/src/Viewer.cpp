#include "Viewer.h"

/* Declare module details (name, description, cparams, can be source) */
DEFAULT_EXPORT_ALL(Viewer, "Viewer module", "", false)

/* Constructor */
Viewer::Viewer(const std::string& mid) : poma::Module<Viewer, PomaDataType>(mid) {}
  
/* Method for processing incoming data packets */
void Viewer::on_incoming_data(PomaPacketType& dta, const std::string& channel)
{
	if (!dta.m_properties.empty()) {
		std::stringstream ss;
		boost::property_tree::write_json(ss, dta.m_properties);
		std::cout << ss.str() << std::endl;
		std::cout.flush();
	}
	if (!dta.m_data.m_image.empty()) {
		cv::imshow("Image", dta.m_data.m_image);
		cv::waitKey(50);
	}	
    submit_data(dta);
}

