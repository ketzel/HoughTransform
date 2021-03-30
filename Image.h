#pragma once

#include "BMP.h"

const float PI = 3.14159265;

const int DICRETE_LEVEL = 256;

const short LOG5[5][5] = { 0, 0, 1, 0, 0,
						0, 1, 2, 1, 0,
						1, 2, -16, 2, 1,
						0, 1, 2, 1, 0,
						0, 0, 1, 0, 0 };

const short LOG3[3][3] = { 0, 1, 0,
						1, -4, 1,
						0, 1, 0, };

struct Point {

	Point(int x = 0, int y = 0) { this->x = x; this->y = y; }

	int x;
	int y;
};

struct Rect {
	Rect(Point a, Point b);
	Point min;
	Point max;
};

struct Segment {

	Segment(Point firstPoint, int index);

	void addPoint(Point newPoint);

	Rect getBorders() { Rect borders(xyMin_, xyMax_); return borders; }

	int getIndex() { return index_; }

	int howMuch() { return count_; }

	int getArea() { return ((this->xyMax_.x - this->xyMin_.x) * (this->xyMax_.y - this->xyMin_.y)); }

	int getHeigh() { return (this->xyMax_.y - this->xyMin_.y); }

	int getWidth() { return (this->xyMax_.x - this->xyMin_.x); }

	float getDistortion();

private:
	int index_;
	int count_;
	Point xyMin_;
	Point xyMax_;
};

struct CentersPoint {
	CentersPoint(Point point, int radius) { this->point = point; this->radius = radius; count = 1; }

	Point point;
	int count;
	int radius;
};

void laplacianOfGauss(GrayImage* imgGr);

void normalizeValues(GrayImage* imgGr);

void getHistogram(GrayImage* img);

void thresholdImage(GrayImage* img, float multiplier = 1.0);

void inverseValues(GrayImage* img);

void paintBorders(GrayImage* img, int width = 2);

void morphDilation(GrayImage* img, int size = 5);

void morphErosion(GrayImage* img, int size = 5);

void segmentTopDownBottomUp(GrayImage* imBin);

void removeExceptCircles(GrayImage* img);

double findCircles(GrayImage* image, GrayImage* circles);

void drawCircles(RgbImage* img, GrayImage* circles);