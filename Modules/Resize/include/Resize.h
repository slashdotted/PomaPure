#ifndef RESIZE_H
#define RESIZE_H

#include "PomaDefault.h"

class Resize : public poma::Module<Resize, PomaDataType>
{
public:
    Resize(const std::string& mid);
    void on_incoming_data(PomaPacketType& dta, const std::string& channel) override;
};
#endif

