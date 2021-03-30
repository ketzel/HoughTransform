#include "BMP.h"
#include "Image.h"
#include <chrono>

int main() {

	auto tic = std::chrono::steady_clock::now();
	Bmp* bmpImage = new Bmp("image.bmp");
	RgbImage* rgbImage = new RgbImage(bmpImage);
	delete bmpImage;
	GrayImage* grayImage = new GrayImage(rgbImage);

	laplacianOfGauss(grayImage);

	thresholdImage(grayImage);
	inverseValues(grayImage);
	paintBorders(grayImage);

	GrayImage* contours = new GrayImage(grayImage->getWidth(), grayImage->getHeight());
	contours->copy(grayImage);

	morphDilation(grayImage, 5);
	morphDilation(grayImage, 5);
	morphErosion(grayImage, 7);
	segmentTopDownBottomUp(grayImage);
	removeExceptCircles(grayImage);
	grayImage->setValues();

	auto toc = std::chrono::steady_clock::now();
	std::chrono::steady_clock::duration period = toc - tic;
	double time = std::chrono::duration_cast<std::chrono::nanoseconds>(period).count() / (1000.0 * 1000.0);

	double timeCircle = findCircles(contours, grayImage);
	delete contours;

	std::cout << time << std::endl << timeCircle;
	drawCircles(rgbImage, grayImage);
	delete grayImage;
	Bmp* resultImage = new Bmp(rgbImage->getWidth(), rgbImage->getHeight(), false);
	rgbToBmp(rgbImage, resultImage);
	delete rgbImage;

	resultImage->write("im1.bmp");

	delete resultImage;

	return 0;
}