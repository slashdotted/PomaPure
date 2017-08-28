#include "Color.h"

/* Declare module details (name, description, cparams, can be source) */
DEFAULT_EXPORT_ALL(Color, "Color module", "", false)

/* Constructor */
Color::Color(const std::string& mid) : poma::Module<Color, PomaDataType>(mid) {}
  
/* Method for processing incoming data packets */
void Color::on_incoming_data(PomaPacketType& dta, const std::string& channel)
{
	cv::Scalar avg = cv::mean(dta.m_data.m_image);
	dta.m_properties.put("color.blue", avg.val[0]);
	dta.m_properties.put("color.green", avg.val[1]);
	dta.m_properties.put("color.red", avg.val[2]);
    submit_data(dta);
}

