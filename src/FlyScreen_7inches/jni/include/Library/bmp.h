/*
 * bmp.h
 *
 *  Created on: 20 Feb 2024
 *      Author: andy
 */

#ifndef JNI_INCLUDE_LIBRARY_BMP_H_
#define JNI_INCLUDE_LIBRARY_BMP_H_

#include "Pixel.h"
#include <stdio.h>

class BMP
{
  public:
	BMP(int width, int height, const char* imageFileName);
	~BMP();
	bool Open();
	bool IsOpen() { return m_imageFile != nullptr; }
	bool Close();
	void generateBitmapImage(unsigned char* image);
	void generateBitmapHeaders();
	void appendPixels(unsigned char* pixels, int count);
	void pad();

  private:
	unsigned char* createBitmapFileHeader();
	unsigned char* createBitmapInfoHeader();
	int m_width;
	int m_height;
	const char* m_imageFileName;
	int m_widthInBytes;
	int m_paddingSize;
	int m_stride;
	FILE* m_imageFile;
	int m_pixelIndex;
};

#endif /* JNI_INCLUDE_LIBRARY_BMP_H_ */
