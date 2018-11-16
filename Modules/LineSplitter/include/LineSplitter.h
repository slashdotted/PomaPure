#ifndef LINESPLITTER_H
#define LINESPLITTER_H

#include "PomaDefault.h"

class LineSplitter : public poma::Module<LineSplitter, PomaDataType>
{
public:
    LineSplitter(const std::string& mid);
    void on_incoming_data(PomaPacketType& dta, const std::string& channel) override;
};
#endif

