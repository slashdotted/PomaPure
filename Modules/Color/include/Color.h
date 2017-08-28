#ifndef COLOR_H
#define COLOR_H

#include "PomaDefault.h"

class Color : public poma::Module<Color, PomaDataType>
{
public:
    Color(const std::string& mid);
    void on_incoming_data(PomaPacketType& dta, const std::string& channel) override;
};
#endif

