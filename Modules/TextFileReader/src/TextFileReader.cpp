#include "TextFileReader.h"

/* Declare module details (name, description, cparams, can be source) */
DEFAULT_EXPORT_ALL(TextFileReader, "TextFileReader module", "", SOURCE_MODULE)

/* Constructor */
TextFileReader::TextFileReader(const std::string& mid) : poma::Module<TextFileReader, PomaDataType>(mid) {}
  
/* Method for processing incoming data packets */
void TextFileReader::start_processing()
{
	std::ifstream ifile{m_file_path};
	std::string line;
	if (!ifile.is_open()) {
		throw std::runtime_error{std::string{"cannot open input file "} + m_file_path};
	}
	for (std::string line; std::getline(ifile, line);) {
		PomaPacketType dta;
		dta.m_properties.put("text", line);
		submit_data(dta);
	}
	ifile.close();
}

void TextFileReader::setup_cli(boost::program_options::options_description& desc) const
{
	boost::program_options::options_description TextFileReaderOptions("TextFileReader options");
    TextFileReaderOptions.add_options()
	("path", boost::program_options::value<std::string>()->required(), "input file path");
    desc.add(TextFileReaderOptions);
}

void TextFileReader::process_cli(boost::program_options::variables_map& vm)
{
	 m_file_path = vm["path"].as<std::string>();
}


void TextFileReader::initialize()
{
}

