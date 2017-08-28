#ifndef CAPTURE_H
#define CAPTURE_H

#include "PomaDefault.h"

class Capture : public poma::Module<Capture, PomaDataType>
{
public:
    Capture(const std::string& mid);
    void start_processing() override;
};
#endif

