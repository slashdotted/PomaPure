#include "OCR.h"
#include <opencv2/imgproc/imgproc.hpp>

/* Declare module details (name, description, cparams, can be source) */
DEFAULT_EXPORT_ALL(OCR, "OCR module", "", false)

/* Constructor */
OCR::OCR(const std::string& mid) : poma::Module<OCR, PomaDataType>(mid) {}
  
/* Method for processing incoming data packets */
void OCR::on_incoming_data(PomaPacketType& dta, const std::string& channel)
{
	cv::Size size{3,3};
	cv::Mat preprocessed;
	cv::cvtColor(dta.m_data.m_image, preprocessed, cv::COLOR_BGR2GRAY);
	cv::GaussianBlur(preprocessed, preprocessed, size, 0);
	cv::adaptiveThreshold(preprocessed, preprocessed, 255, CV_ADAPTIVE_THRESH_MEAN_C,
		CV_THRESH_BINARY, 75, 10);
	cv::bitwise_not(preprocessed, preprocessed);
	ocr_engine->TesseractRect(preprocessed.data, 1, preprocessed.step1(),
		0, 0, preprocessed.size().width, preprocessed.size().height);
	const char * text=ocr_engine->GetUTF8Text();
	dta.m_properties.put("ocr.text", text);	
    submit_data(dta);
}

OCR::~OCR()
{
	if (!ocr_engine) {
		ocr_engine->Clear();
		ocr_engine->End();
		delete ocr_engine;
	}
}

void OCR::initialize()
{
	ocr_engine = new tesseract::TessBaseAPI();
	if(ocr_engine->Init(NULL,"eng")) {
		std::cerr << "Failed to initialize Tesseract" << std::endl;
		exit(1);
	}
	ocr_engine->SetPageSegMode(static_cast<tesseract::PageSegMode>(7));
}

