#ifndef OCR_H
#define OCR_H

#include "PomaDefault.h"
#include <baseapi.h>

class OCR : public poma::Module<OCR, PomaDataType>
{
public:
    OCR(const std::string& mid);
    virtual ~OCR();
    void on_incoming_data(PomaPacketType& dta, const std::string& channel) override;
    void initialize() override;
    
private:
	tesseract::TessBaseAPI *ocr_engine{nullptr};
};
#endif

