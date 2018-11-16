#include "LineSplitter.h"
#include <sstream>

/* Declare module details (name, description, cparams, can be source) */
DEFAULT_EXPORT_ALL(LineSplitter, "LineSplitter module", "", PROCESSING_MODULE)

/* Constructor */
LineSplitter::LineSplitter(const std::string& mid) : poma::Module<LineSplitter, PomaDataType>(mid) {}
  
/* Method for processing incoming data packets */
void LineSplitter::on_incoming_data(PomaPacketType& dta, const std::string& channel)
{
	std::stringstream ss{dta.m_properties.get("text", "")};
	std::string word;
	while (ss >> word) {
		PomaPacketType dta2;
		dta2.m_properties.put("word", word);
		submit_data(dta2);
	}
}
