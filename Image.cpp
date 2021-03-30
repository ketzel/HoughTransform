#pragma once

#include "Image.h"
#include <cmath>
#include <vector>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

int hist[DICRETE_LEVEL];

std::vector <int> unique;
std::vector <Segment> segments;

bool operator == (const Point& left, const Point& right) {
	return ((left.x == right.x) && (left.y == right.y));
}

Rect::Rect(Point first, Point second) {
	if (first.x <= second.x) {
		this->min = first; this->max = second;
	}
	else {
		this->min = second; this->max = first;
	}
}

Segment::Segment(Point firstPoint, int index) {
	this->xyMax_ = firstPoint; this->xyMin_ = firstPoint; this->count_ = 1; index_ = index;
}

float Segment::getDistortion() {
	int width = this->xyMax_.x - this->xyMin_.x + 1;
	int heigh = this->xyMax_.y - this->xyMin_.y + 1;
	float distortion = ((float)width - (float)heigh) / (float)width;
	return distortion;
}

void Segment::addPoint(Point newPoint) {
	if (newPoint.x < xyMin_.x)
		xyMin_.x = newPoint.x;
	else if (newPoint.x > xyMax_.x)
		xyMax_.x = newPoint.x;
	if (newPoint.y < xyMin_.y)
		xyMin_.y = newPoint.y;
	else if (newPoint.y > xyMax_.y)
		xyMax_.y = newPoint.y;
	++count_;
}

void normalizeValues(GrayImage* image) {

	float min = image->data[0][0];
	float max = min;
	int size = image->getWidth() * image->getHeight();

	for (int i = 0; i < size; ++i)
		if (image->data[0][i] < min)
			min = image->data[0][i];
		else if (image->data[0][i] > max)
			max = image->data[0][i];

	for (int i = 0; i < size; ++i)
		image->data[0][i] = (int)round((((float)image->data[0][i] - min) / (max - min)) * 255.0);
}

void laplacianOfGauss(GrayImage* image) {

	int width = image->getWidth();
	int height = image->getHeight();

	GrayImage* imageLoG = new GrayImage(width, height);

	for (int j = 0; j < height; ++j)
		for (int i = 0; i < width; ++i)

		{
			int temp = 0;
			for (int jk = -2; jk < 3; ++jk)
				for (int ik = -2; ik < 3; ++ik)
				{
					if (i + ik < 0 || i + ik >= width || j + jk < 0 || j + jk >= height)
						continue;
					temp += (image->data[0][(j + jk) * width + (i + ik)] * LOG5[2 + ik][2 + jk]);
				}

			imageLoG->data[0][j * width + i] = temp;
		}

	normalizeValues(imageLoG);

	image->copy(imageLoG);
	delete imageLoG;
}

void getHistogram(GrayImage* image) {

	int size = image->getWidth() * image->getHeight();

	for (int i = 0; i < size; ++i)
		hist[(int)image->data[0][i]]++;
}

int sumValues(GrayImage* image) {

	int sum = 0;
	int size = image->getWidth() * image->getHeight();

	for (int i = 0; i < size; ++i)
		sum += (int)image->data[0][i];

	return sum;
}

int threshold_Otsu(GrayImage* image) {

	int all_pixel_count = image->getWidth() * image->getHeight();
	int all_intensity_sum = sumValues(image);

	int best_thresh = 0;
	double best_sigma = 0.0;

	int first_class_pixel_count = 0;
	int first_class_intensity_sum = 0;

	for (int thresh = 0; thresh < DICRETE_LEVEL - 1; ++thresh) {
		first_class_pixel_count += hist[thresh];
		first_class_intensity_sum += thresh * hist[thresh];

		double first_class_prob = first_class_pixel_count / (double)all_pixel_count;
		double second_class_prob = 1.0 - first_class_prob;

		double first_class_mean = first_class_intensity_sum / (double)first_class_pixel_count;
		double second_class_mean = (all_intensity_sum - first_class_intensity_sum)
			/ (double)(all_pixel_count - first_class_pixel_count);

		double mean_delta = first_class_mean - second_class_mean;

		double sigma = first_class_prob * second_class_prob * mean_delta * mean_delta;

		if (sigma > best_sigma) {
			best_sigma = sigma;
			best_thresh = thresh;
		}
	}

	return best_thresh;
}

void binarize(GrayImage* image, int threshold) {

	int size = image->getWidth() * image->getHeight();
	for (int i = 0; i < size; ++i)
		if (image->data[0][i] >= threshold)
			image->data[0][i] = 255;
		else
			image->data[0][i] = 0;
}

void thresholdImage(GrayImage* image, float multiplier) {

	getHistogram(image);
	int thresh = threshold_Otsu(image);
	binarize(image, (int)(multiplier * thresh));
}

void inverseValues(GrayImage* image) {

	int size = image->getWidth() * image->getHeight();

	for (int i = 0; i < size; ++i)
		image->data[0][i] = (image->data[0][i] > 0) ? 0 : 255;
}

void paintBorders(GrayImage* image, int width) {
	for (int i = 0; i < image->getWidth(); ++i)
		for (int j = 0; j < width; ++j)
		{
			image->data[j][i] = 0;
			image->data[image->getHeight() - j - 1][i] = 0;
		}

	for (int j = 0; j < image->getHeight(); ++j)
		for (int i = 0; i < width; ++i)
		{
			image->data[j][i] = 0;
			image->data[j][image->getWidth() - i - 1] = 0;
		}

}

void morphDilation(GrayImage* image, int size) {
	int spc = (size % 2 == 1) ? ((size - 1) / 2) : (size / 2); //spacing

	GrayImage* dilated = new GrayImage(image->getWidth(), image->getHeight());
	dilated->copy(image);

	for (int i = spc; i < image->getWidth() - spc; ++i)
		for (int j = spc; j < image->getHeight() - spc; ++j) {
			if (image->data[j][i] > 0)
				for (int di = -spc; di < spc; ++di)
					for (int dj = -spc; dj < spc; ++dj)
						dilated->data[j + dj][i + di] = 255;
		}
	image->copy(dilated);
	delete dilated;
}

void morphErosion(GrayImage* image, int size) {
	int spc = (size % 2 == 1) ? ((size - 1) / 2) : (size / 2); //spacing

	GrayImage* eroded = new GrayImage(image->getWidth(), image->getHeight());
	eroded->copy(image);

	for (int i = spc; i < image->getWidth() - spc; ++i)
		for (int j = spc; j < image->getHeight() - spc; ++j) {
			if (image->data[j][i] == 0)
				for (int di = -spc; di < spc; ++di)
					for (int dj = -spc; dj < spc; ++dj)
						eroded->data[j + dj][i + di] = 0;
		}
	image->copy(eroded);
	delete eroded;
}

bool topDown(GrayImage* image) {
	bool isChanged = false;
	for (int i = 1; i < image->getWidth() - 1; ++i)
		for (int j = 1; j < image->getHeight() - 1; ++j)
			if (image->data[j][i] > 0) {
				int min = image->data[j][i];
				for (int di = -1; di <= 1; ++di)
					for (int dj = -1; dj <= 1; ++dj)
						if (image->data[j + dj][i + di] != 0 && image->data[j + dj][i + di] < min)
							min = image->data[j + dj][i + di];
				if (!isChanged)
					if (min != image->data[j][i])
						isChanged = true;
				image->data[j][i] = min;
			}
	return isChanged;
}

bool bottomUp(GrayImage* image) {
	bool isChanged = false;
	for (int i = image->getWidth() - 2; i > 0; --i)
		for (int j = image->getHeight() - 2; j > 0; --j)
			if (image->data[j][i] > 0) {
				int min = image->data[j][i];
				for (int di = -1; di <= 1; ++di)
					for (int dj = -1; dj <= 1; ++dj)
						if (image->data[j + dj][i + di] != 0 && image->data[j + dj][i + di] < min)
							min = image->data[j + dj][i + di];
				if (!isChanged)
					if (min != image->data[j][i])
						isChanged = true;
				image->data[j][i] = min;
			}
	return isChanged;
}

void uniqueValues(GrayImage* image) {

	unique.push_back(0);
	for (int i = 0; i < image->getWidth(); ++i)
		for (int j = 0; j < image->getHeight(); ++j) {
			bool isFound = false;
			for (int k = 0; k < unique.size(); ++k)
				if (image->data[j][i] == unique[k])
					isFound = true;
			if (!isFound)
				unique.push_back(image->data[j][i]);
		}

}

void relabeling(GrayImage* image) {

	int n = unique.size();
	int size = image->getWidth() * image->getHeight();

	for (int i = 0; i < size; ++i)
		for (int k = 1; k < n; ++k)
			if (image->data[0][i] == unique[k]) {
				image->data[0][i] = k;
				break;
			}
}

void segmentTopDownBottomUp(GrayImage* image) {
	int  k = 1;
	int size = image->getWidth() * image->getHeight();
	for (int i = 0; i < size; ++i)
		image->data[0][i] = (image->data[0][i] > 0 ? k++ : 0);

	bool isChanged;
	do {
		isChanged = topDown(image) | bottomUp(image);
	} while (isChanged);

	uniqueValues(image);
	relabeling(image);
	unique.clear();
}

void findSegments(GrayImage* image) {
	for (int i = 0; i < image->getWidth(); ++i)
		for (int j = 0; j < image->getHeight(); ++j)
			if (image->data[j][i] != 0)
			{
				if (segments.size() > 0)
				{
					bool isFound = false;
					int segInd = 0;
					for (int k = 0; k < segments.size(); ++k)
						if (segments[k].getIndex() == image->data[j][i]) {
							isFound = true;
							segInd = k;
							break;
						}
					if (isFound)
						segments[segInd].addPoint(Point(i, j));
					else {
						segments.push_back(Segment(Point(i, j), image->data[j][i]));
					}
				}
				else {
					segments.push_back(Segment(Point(i, j), image->data[j][i]));
				}

			}
}

void removeSegmentByIndex(GrayImage* image, int index) {

	int size = image->getWidth() * image->getHeight();

	for (int i = 0; i < size; ++i)
		if (image->data[0][i] == index)
			image->data[0][i] = 0;
}

void eraseSegments(GrayImage* image, float sizeMultiplier = 0.4, float maxDistortion = 0.4, int pointsLimit = 50) {

	int size = image->getWidth() * image->getHeight();

	for (int i = 0; i < segments.size(); ++i)
	{
		if (segments[i].getArea() > (sizeMultiplier * size)) {
			removeSegmentByIndex(image, segments[i].getIndex());
			segments.erase(segments.begin() + i);
			--i;
		}

		else if (segments[i].howMuch() < pointsLimit) {
			removeSegmentByIndex(image, segments[i].getIndex());
			segments.erase(segments.begin() + i);
			--i;
		}

		else if (abs(segments[i].getDistortion()) > maxDistortion) {
			removeSegmentByIndex(image, segments[i].getIndex());
			segments.erase(segments.begin() + i);
			--i;
		}

	}

}

void binaryNormalize(GrayImage* image) {

	int size = image->getWidth() * image->getHeight();

	for (int i = 0; i < size; ++i)
		if (image->data[0][i] >= 1)
			image->data[0][i] = 255;
}

void removeExceptCircles(GrayImage* image) {
	findSegments(image);
	eraseSegments(image);
	binaryNormalize(image);
}

#pragma region filter multithreaded

std::atomic<bool> start = false;

//CentersPoint centerForRadius(GrayImage* image, Rect borders, int radius) {
//
//	while (!start) std::this_thread::yield();
//
//	std::vector <CentersPoint> circle;
//
//	for (int x0 = borders.min.x; x0 < borders.max.x; ++x0)
//		for (int y0 = borders.min.y; y0 < borders.max.y; ++y0)
//			if (image->data[y0][x0] > 0)
//				for (int alpha = 0; alpha < 360; ++alpha) {
//					int x = (int)round((float)x0 + radius * cos(alpha * PI / 180));
//					int y = (int)round((float)y0 + radius * sin(alpha * PI / 180));
//					Point temp(x, y);
//					if (x > borders.min.x && x < borders.max.x && y > borders.min.y && y < borders.max.y)
//						if (!circle.empty()) {
//							bool isFound = false;
//							if (circle[circle.size() - 1].point == temp || circle[0].point == temp)
//								continue;
//							for (int m = 0; m < circle.size(); ++m)
//								if (circle[m].point == temp) {
//									isFound = true;
//									circle[m].count++;
//									break;
//								}
//							if (!isFound)
//								circle.push_back(CentersPoint(Point(x, y), radius));
//						}
//						else
//							circle.push_back(CentersPoint(Point(x, y), radius));
//
//				}
//
//	int index = 0;
//	int max = circle[0].count;
//
//	for (int k = 1; k < circle.size(); ++k)
//		if (circle[k].count > max) {
//			max = circle[k].count;
//			index = k;
//		}
//
//	CentersPoint out(circle[index].point, circle[index].radius);
//	out.count = circle[index].count;
//	circle.clear();
//
//	return out;
//}

void centerForRadius(GrayImage* image, Rect borders, int radius, std::vector <CentersPoint>* point) {

	while (!start) std::this_thread::yield();

	std::vector <CentersPoint> circle;

	int dAlpha = (int)floor(180.0 / (PI * (float)radius)) - 1;
	if (dAlpha < 1) dAlpha = 1;

	for (int x0 = borders.min.x; x0 < borders.max.x; ++x0)
		for (int y0 = borders.min.y; y0 < borders.max.y; ++y0)
			if (image->data[y0][x0] > 0)
				for (int alpha = 0; alpha < 360; alpha += dAlpha) {
					int x = (int)round((float)x0 + radius * cos(alpha * PI / 180));
					int y = (int)round((float)y0 + radius * sin(alpha * PI / 180));
					Point temp(x, y);
					if (x > borders.min.x && x < borders.max.x && y > borders.min.y && y < borders.max.y)
						if (!circle.empty()) {
							bool isFound = false;
							if (circle[circle.size() - 1].point == temp || circle[0].point == temp)
								continue;
							for (int m = 0; m < circle.size(); ++m)
								if (circle[m].point == temp) {
									isFound = true;
									circle[m].count++;
									break;
								}
							if (!isFound)
								circle.push_back(CentersPoint(Point(x, y), radius));
						}
						else
							circle.push_back(CentersPoint(Point(x, y), radius));

				}

	int index = 0;
	int max = circle[0].count;

	for (int k = 1; k < circle.size(); ++k)
		if (circle[k].count > max) {
			max = circle[k].count;
			index = k;
		}

	CentersPoint out(circle[index].point, circle[index].radius);
	out.count = circle[index].count;
	circle.clear();

	point->push_back(out);
}

double findCircles(GrayImage* image, GrayImage* circles) {
	auto tic = std::chrono::steady_clock::now();

	auto threads_count = std::thread::hardware_concurrency();	// pobranie liczby dostêpnych rdzeni obliczeniowych
	std::vector<std::thread> threads;

	std::vector <CentersPoint> center;

	for (int i = 0; i < segments.size(); ++i)
	{
		int boundMin = segments[i].getWidth() / 2 - 10;
		if (boundMin < 3)
			boundMin = 3;
		int boundMax = segments[i].getWidth() / 2 + 10;
		boundMin = 15;
		boundMax = 45;
		Rect borders = segments[i].getBorders();
		borders.max.x += 5;
		borders.max.y += 5;
		borders.min.x -= 5;
		borders.min.y -= 5;
		std::vector <CentersPoint> circles;
		start = false;
		CentersPoint point(Point(0, 0), 0);
		int kLast = boundMin;
		while (kLast < boundMax) {
			for (int k = kLast; k < (kLast + threads_count) && k < boundMax; ++k) {
				threads.push_back(std::thread(centerForRadius, image, borders, k, &circles));
				//circles.push_back(centerForRadius(image, borders, k));
				//circles.push_back(point);

			}
			kLast += threads_count;
			{
				//time_counter tc;
				start = true;
				for (auto& t : threads) {
					t.join();
				}
				threads.clear();
			}
		}



		int index = 0;
		int max = circles[0].count;
		for (int k = 1; k < circles.size(); ++k)
			if (circles[k].count > max) {
				max = circles[k].count;
				index = k;
			}

		center.push_back(circles[index]);

	}

	for (int i = 0; i < center.size(); ++i)
		for (int alpha = 0; alpha < 360; ++alpha) {
			int x = (int)round((float)center[i].point.x + (center[i].radius) * cos(alpha * PI / 180));
			int y = (int)round((float)center[i].point.y + (center[i].radius) * sin(alpha * PI / 180));
			circles->data[y][x] = 255;
		}

	auto toc = std::chrono::steady_clock::now();
	std::chrono::steady_clock::duration period = toc - tic;
	double time = std::chrono::duration_cast<std::chrono::nanoseconds>(period).count() / (1000.0 * 1000.0);
	return time;
}

#pragma endregion

void drawCircles(RgbImage* rgbImage, GrayImage* circles) {

	int size = rgbImage->getWidth() * rgbImage->getHeight();

	for (int i = 0; i < size; ++i)
		if (circles->data[0][i] > 0)
		{
			rgbImage->data[0][i].g = 0.0;
			rgbImage->data[0][i].r = 255.0;
			rgbImage->data[0][i].b = 0.0;
		}
		else
		{
			rgbImage->data[0][i].g /= 3.0;
			rgbImage->data[0][i].r /= 3.0;
			rgbImage->data[0][i].b /= 3.0;
		}

}