#ifndef WORDCOUNTER_H
#define WORDCOUNTER_H

#include "PomaDefault.h"
#include <map>

class WordCounter : public poma::Module<WordCounter, PomaDataType>
{
public:
    WordCounter(const std::string& mid);
    void on_incoming_data(PomaPacketType& dta, const std::string& channel) override;
    void finalize() override;
    
private:
	std::map<std::string,unsigned long> m_word_counter;
};
#endif

