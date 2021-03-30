#pragma once
#include "BMP.h"

Bmp::Bmp(int32_t width, int32_t height, bool has_alpha) {
	if (width <= 0 || height <= 0) {
		throw std::runtime_error("The image width and height must be positive numbers.");
	}

	bmp_info_header.width = width;
	bmp_info_header.height = height;
	if (has_alpha) {
		bmp_info_header.size = sizeof(BmpInfoHeader) + sizeof(BmpColorHeader);
		file_header.offset_data = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) + sizeof(BmpColorHeader);

		bmp_info_header.bit_count = 32;
		bmp_info_header.compression = 3;
		row_stride = width * 4;
		data.resize(row_stride * height);
		file_header.file_size = file_header.offset_data + data.size();
	}
	else {
		bmp_info_header.size = sizeof(BmpInfoHeader);
		file_header.offset_data = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader);

		bmp_info_header.bit_count = 24;
		bmp_info_header.compression = 0;
		row_stride = width * 3;
		data.resize(row_stride * height);

		uint32_t new_stride = make_stride_aligned(4);
		file_header.file_size = file_header.offset_data + static_cast<uint32_t>(data.size()) + bmp_info_header.height * (new_stride - row_stride);
	}
}

void Bmp::read(const char* fname) {
	std::ifstream inp{ fname, std::ios_base::binary };
	if (inp) {
		inp.read((char*)&file_header, sizeof(file_header));
		if (file_header.file_type != 0x4D42) {
			throw std::runtime_error("Error! Unrecognized file format.");
		}
		inp.read((char*)&bmp_info_header, sizeof(bmp_info_header));

		// The BMPColorHeader is used only for transparent images
		if (bmp_info_header.bit_count == 32) {
			// Check if the file has bit mask color information
			if (bmp_info_header.size >= (sizeof(BmpInfoHeader) + sizeof(BmpColorHeader))) {
				inp.read((char*)&bmp_color_header, sizeof(bmp_color_header));
				// Check if the pixel data is stored as BGRA and if the color space type is sRGB
				check_color_header(bmp_color_header);
			}
			else {
				std::cerr << "Error! The file \"" << fname << "\" does not seem to contain bit mask information\n";
				throw std::runtime_error("Error! Unrecognized file format.");
			}
		}

		// Jump to the pixel data location
		inp.seekg(file_header.offset_data, inp.beg);

		// Adjust the header fields for output.
		// Some editors will put extra info in the image file, we only save the headers and the data.
		if (bmp_info_header.bit_count == 32) {
			bmp_info_header.size = sizeof(BmpInfoHeader) + sizeof(BmpColorHeader);
			file_header.offset_data = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) + sizeof(BmpColorHeader);
		}
		else {
			bmp_info_header.size = sizeof(BmpInfoHeader);
			file_header.offset_data = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader);
		}
		file_header.file_size = file_header.offset_data;

		if (bmp_info_header.height < 0) {
			throw std::runtime_error("The program can treat only BMP images with the origin in the bottom left corner!");
		}

		data.resize(bmp_info_header.width * bmp_info_header.height * bmp_info_header.bit_count / 8);

		// Here we check if we need to take into account row padding
		if (bmp_info_header.width % 4 == 0) {
			inp.read((char*)data.data(), data.size());
			file_header.file_size += static_cast<uint32_t>(data.size());
		}
		else {
			row_stride = bmp_info_header.width * bmp_info_header.bit_count / 8;
			uint32_t new_stride = make_stride_aligned(4);
			std::vector<uint8_t> padding_row(new_stride - row_stride);

			for (int y = 0; y < bmp_info_header.height; ++y) {
				inp.read((char*)(data.data() + row_stride * y), row_stride);
				inp.read((char*)padding_row.data(), padding_row.size());
			}
			file_header.file_size += static_cast<uint32_t>(data.size()) + bmp_info_header.height * static_cast<uint32_t>(padding_row.size());
		}
	}
	else {
		throw std::runtime_error("Unable to open the input image file.");
	}
}

void Bmp::write(const char* fname) {
	std::ofstream of{ fname, std::ios_base::binary };
	if (of) {
		if (bmp_info_header.bit_count == 32) {
			write_headers_and_data(of);
		}
		else if (bmp_info_header.bit_count == 24) {
			if (bmp_info_header.width % 4 == 0) {
				write_headers_and_data(of);
			}
			else {
				uint32_t new_stride = make_stride_aligned(4);
				std::vector<uint8_t> padding_row(new_stride - row_stride);

				write_headers(of);

				for (int y = 0; y < bmp_info_header.height; ++y) {
					of.write((const char*)(data.data() + row_stride * y), row_stride);
					of.write((const char*)padding_row.data(), padding_row.size());
				}
			}
		}
		else {
			throw std::runtime_error("The program can treat only 24 or 32 bits per pixel BMP files");
		}
	}
	else {
		throw std::runtime_error("Unable to open the output image file.");
	}
}

void Bmp::write_headers(std::ofstream& of) {
	of.write((const char*)&file_header, sizeof(file_header));
	of.write((const char*)&bmp_info_header, sizeof(bmp_info_header));
	if (bmp_info_header.bit_count == 32) {
		of.write((const char*)&bmp_color_header, sizeof(bmp_color_header));
	}
}

void Bmp::write_headers_and_data(std::ofstream& of) {
	write_headers(of);
	of.write((const char*)data.data(), data.size());
}

uint32_t Bmp::make_stride_aligned(uint32_t align_stride) {
	uint32_t new_stride = row_stride;
	while (new_stride % align_stride != 0) {
		new_stride++;
	}
	return new_stride;
}

void Bmp::check_color_header(BmpColorHeader& bmp_color_header) {
	BmpColorHeader expected_color_header;
	if (expected_color_header.red_mask != bmp_color_header.red_mask ||
		expected_color_header.blue_mask != bmp_color_header.blue_mask ||
		expected_color_header.green_mask != bmp_color_header.green_mask ||
		expected_color_header.alpha_mask != bmp_color_header.alpha_mask) {
		throw std::runtime_error("Unexpected color mask format! The program expects the pixel data to be in the BGRA format");
	}
	if (expected_color_header.color_space_type != bmp_color_header.color_space_type) {
		throw std::runtime_error("Unexpected color space type! The program expects sRGB values");
	}
}

RgbImage::RgbImage(Bmp* bmpImage)
{
	this->width_ = bmpImage->bmp_info_header.width;
	this->height_ = bmpImage->bmp_info_header.height;
	data = new RgbPixel * [this->width_];
	data[0] = new RgbPixel[this->width_ * this->height_];
	uint32_t channels = bmpImage->bmp_info_header.bit_count / 8;
	for (uint32_t y = 0; y < this->height_; ++y) {
		for (uint32_t x = 0; x < this->width_; ++x) {
			data[0][y * this->width_ + x].b = bmpImage->data[channels * (y * this->width_ + x) + 0];
			data[0][y * this->width_ + x].g = bmpImage->data[channels * (y * this->width_ + x) + 1];
			data[0][y * this->width_ + x].r = bmpImage->data[channels * (y * this->width_ + x) + 2];
		}
	}
	for (int i = 1; i < this->width_; ++i)
		data[i] = data[i - 1] + this->height_;
}

RgbImage::RgbImage(int width, int height, float value) {
	this->width_ = width;
	this->height_ = height;
	data = new RgbPixel * [width];
	data[0] = new RgbPixel[width * height];
	for (int i = 0; i < width * height; ++i)
	{
		data[0][i].r = value;
		data[0][i].g = value;
		data[0][i].b = value;
	}

	for (int i = 1; i < width; ++i)
		data[i] = data[i - 1] + height;
}

GrayImage::GrayImage(RgbImage* rgbImage) {
	this->width_ = rgbImage->getWidth();
	this->height_ = rgbImage->getHeight();
	data = new int* [this->width_];
	data[0] = new int[this->width_ * this->height_];
	for (int i = 0; i < this->width_ * this->height_; ++i)
	{
		data[0][i] = (int)round((rgbImage->data[0][i].r + rgbImage->data[0][i].g + rgbImage->data[0][i].b) / 3.0);
	}
	for (int i = 1; i < this->height_; ++i)
		data[i] = data[i - 1] + this->width_;
}

GrayImage::GrayImage(int width, int height, int value) {
	this->width_ = width;
	this->height_ = height;
	data = new int* [width];
	data[0] = new int[width * height];
	for (int i = 0; i < width * height; ++i)
		data[0][i] = value;
	for (int i = 1; i < height; ++i)
		data[i] = data[i - 1] + width;
}

void GrayImage::copy(GrayImage* grayImage) {
	this->width_ = grayImage->getWidth();
	this->height_ = grayImage->getHeight();
	delete[] data[0];
	delete[] data;
	data = new int* [this->width_];
	data[0] = new int[this->width_ * this->height_];
	for (int i = 0; i < this->width_ * this->height_; ++i)
	{
		data[0][i] = grayImage->data[0][i];
	}
	for (int i = 1; i < this->height_; ++i)
		data[i] = data[i - 1] + this->width_;
}

void GrayImage::setValues(int value, int width, int height) {
	if (width == 0 || height == 0)
	{
		int size = this->width_ * this->height_;
		for (int i = 0; i < size; ++i)
			this->data[0][i] = value;
	}
	else
	{
		delete[] data[0];
		delete[] data;
		this->width_ = width;
		this->height_ = height;
		data = new int* [width];
		data[0] = new int[width * height];
		for (int i = 0; i < width * height; ++i)
			data[0][i] = value;
		for (int i = 1; i < height; ++i)
			data[i] = data[i - 1] + width;
	}

}

int grayToRgb(GrayImage* grayImage, RgbImage* rgbImage) {

	if (grayImage->getWidth() != rgbImage->getWidth() || grayImage->getHeight() != rgbImage->getHeight())
		return 1;
	for (int i = 0; i < grayImage->getWidth() * grayImage->getHeight(); ++i)
	{
		rgbImage->data[0][i].r = (float)grayImage->data[0][i];
		rgbImage->data[0][i].g = (float)grayImage->data[0][i];
		rgbImage->data[0][i].b = (float)grayImage->data[0][i];
	}
	return 0;
}

void rgbToBmp(RgbImage* rgbImage, Bmp* bmpImage) {

	int width = rgbImage->getWidth();
	int height = rgbImage->getHeight();

	for (uint32_t y = 0; y < height; ++y) {
		for (uint32_t x = 0; x < width; ++x) {
			bmpImage->data[3 * (y * width + x) + 0] = (uint8_t)rgbImage->data[0][y * width + x].b;
			bmpImage->data[3 * (y * width + x) + 1] = (uint8_t)rgbImage->data[0][y * width + x].g;
			bmpImage->data[3 * (y * width + x) + 2] = (uint8_t)rgbImage->data[0][y * width + x].r;
		}
	}
}