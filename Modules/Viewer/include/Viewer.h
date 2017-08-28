#ifndef VIEWER_H
#define VIEWER_H

#include "PomaDefault.h"

class Viewer : public poma::Module<Viewer, PomaDataType>
{
public:
    Viewer(const std::string& mid);
    void on_incoming_data(PomaPacketType& dta, const std::string& channel) override;
};
#endif

