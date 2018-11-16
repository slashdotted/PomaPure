#include "WordCounter.h"

/* Declare module details (name, description, cparams, can be source) */
DEFAULT_EXPORT_ALL(WordCounter, "WordCounter module", "", PROCESSING_MODULE)

/* Constructor */
WordCounter::WordCounter(const std::string& mid) : poma::Module<WordCounter, PomaDataType>(mid) {}
  
/* Method for processing incoming data packets */
void WordCounter::on_incoming_data(PomaPacketType& dta, const std::string& channel)
{
	m_word_counter[dta.m_properties.get("word", "")]++;
    submit_data(dta);
}


void WordCounter::finalize()
{
	std::cout << "Word count results:" << std::endl;
	unsigned long total{0};
	for( const auto& kv : m_word_counter )
	{
		std::cout << kv.first << " : " << kv.second << std::endl;
		total += kv.second;
	}
	std::cout << "Total words: " << total << std::endl;
}
