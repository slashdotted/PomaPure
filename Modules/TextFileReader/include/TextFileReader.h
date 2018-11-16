#ifndef TEXTFILEREADER_H
#define TEXTFILEREADER_H

#include "PomaDefault.h"
#include <fstream>

class TextFileReader : public poma::Module<TextFileReader, PomaDataType>
{
public:
    TextFileReader(const std::string& mid);
    void start_processing() override;
    void setup_cli(boost::program_options::options_description& desc) const override;
    void process_cli(boost::program_options::variables_map& vm) override;
    void initialize() override;
    
private:
	std::string m_file_path;
};
#endif

