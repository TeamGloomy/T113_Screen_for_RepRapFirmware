#include "DebugLevels.h"
#define DEBUG_LEVEL DEBUG_LEVEL_DBG
#include "Debug.h"

#include "bmp.h"
#include <algorithm>
#include <stdio.h>

constexpr int BYTES_PER_PIXEL = sizeof(rgba_t); /// blue, green, red, alpha
constexpr int FILE_HEADER_SIZE = 14;
constexpr int INFO_HEADER_SIZE = 40;

const static unsigned char padding[BYTES_PER_PIXEL] = {0};

BMP::BMP(int width, int height, const char* imageFileName)
	: m_width(width), m_height(height), m_imageFileName(imageFileName),
	  m_paddingSize((4 - (width * BYTES_PER_PIXEL) % 4) % 4), m_stride((width * BYTES_PER_PIXEL) + m_paddingSize),
	  m_pixelIndex(0)
{
	m_imageFile = fopen(imageFileName, "wb");

	m_widthInBytes = m_width * BYTES_PER_PIXEL;
	m_paddingSize = (4 - (m_widthInBytes) % 4) % 4;
	m_stride = (m_widthInBytes) + m_paddingSize;
	info("BMP: %s, width(%d), height(%d), paddingSize(%d), stride(%d)",
		 imageFileName,
		 m_width,
		 m_height,
		 m_paddingSize,
		 m_stride);
}

BMP::~BMP()
{
	Close();
}

bool BMP::New(int width, int height, const char* imageFileName)
{
	if (m_imageFile != nullptr)
	{
		Close();
	}
	m_width = width;
	m_height = height;
	m_imageFileName = imageFileName;
	m_paddingSize = (4 - (width * BYTES_PER_PIXEL) % 4) % 4;
	m_stride = (width * BYTES_PER_PIXEL) + m_paddingSize;
	m_pixelIndex = 0;
	m_pixelBuffer.clear();
	return Open();
}

bool BMP::Open()
{
	m_imageFile = fopen(m_imageFileName, "wb");
	return m_imageFile != nullptr;
}

bool BMP::Close()
{
	return fclose(m_imageFile) == 0;
}

void BMP::generateBitmapImage(rgba_t* pixels)
{
	generateBitmapHeaders();

	int i;
	for (i = 0; i < m_height; i++)
	{
		writeRow((unsigned char*)pixels + (i * m_widthInBytes));
	}
}

void BMP::generateBitmapHeaders()
{
	unsigned char* fileHeader = createBitmapFileHeader();
	dbg("fileHeader: %.*s", FILE_HEADER_SIZE, fileHeader);
	fwrite(fileHeader, 1, FILE_HEADER_SIZE, m_imageFile);

	unsigned char* infoHeader = createBitmapInfoHeader();
	dbg("infoHeader: %.*s", INFO_HEADER_SIZE, infoHeader);
	fwrite(infoHeader, 1, INFO_HEADER_SIZE, m_imageFile);
}

void BMP::writeRow(unsigned char* pixels)
{
	dbg("Writing row");
	fwrite(pixels, BYTES_PER_PIXEL, m_width, m_imageFile);
	fwrite(padding, 1, m_paddingSize, m_imageFile);
}

void BMP::appendPixels(rgba_t* pixels, int count)
{
	static int bytesPerPixel = sizeof(rgba_t);
	info("count: %d", count);
	int pixelsToAdd = std::min(m_width - (int)m_pixelBuffer.size(), count);
	m_pixelBuffer.reserve(m_width);
	while (pixelsToAdd > 0)
	{
		dbg("Adding %d pixels", pixelsToAdd);
		for (int i = 0; i < pixelsToAdd; i++)
		{
			m_pixelBuffer.push_back(pixels[i]);
		}
		dbg("pixelBuffer.size: %d", m_pixelBuffer.size());
		if ((int)m_pixelBuffer.size() >= m_width)
		{
			writeRow((unsigned char*)m_pixelBuffer.begin());
			m_pixelBuffer.clear();
			m_pixelBuffer.reserve(m_width);
		}
		pixels += pixelsToAdd * bytesPerPixel;
		count -= pixelsToAdd;
		pixelsToAdd = std::min(m_width, count);
	}
}

void BMP::pad()
{
	int paddingLength = m_stride - (m_pixelIndex * BYTES_PER_PIXEL);
	fwrite(padding, 1, paddingLength, m_imageFile);
}

unsigned char* BMP::createBitmapFileHeader()
{
	int fileSize = FILE_HEADER_SIZE + INFO_HEADER_SIZE + (m_stride * m_height);
	dbg("%s fileSize: %d", m_imageFileName, fileSize);
	static unsigned char fileHeader[] = {
		0, 0, /// signature
		0, 0, 0, 0, /// image file size in bytes
		0, 0, 0, 0, /// reserved
		0, 0, 0, 0, /// start of pixel array
	};

	fileHeader[0] = (unsigned char)('B');
	fileHeader[1] = (unsigned char)('M');
	fileHeader[2] = (unsigned char)(fileSize);
	fileHeader[3] = (unsigned char)(fileSize >> 8);
	fileHeader[4] = (unsigned char)(fileSize >> 16);
	fileHeader[5] = (unsigned char)(fileSize >> 24);
	fileHeader[10] = (unsigned char)(FILE_HEADER_SIZE + INFO_HEADER_SIZE);

	return fileHeader;
}

unsigned char* BMP::createBitmapInfoHeader()
{
	static unsigned char infoHeader[] = {
		0, 0, 0, 0, /// header size
		0, 0, 0, 0, /// image width
		0, 0, 0, 0, /// image height
		0, 0,		/// number of color planes
		0, 0,		/// bits per pixel
		0, 0, 0, 0, /// compression
		0, 0, 0, 0, /// image size
		0, 0, 0, 0, /// horizontal resolution
		0, 0, 0, 0, /// vertical resolution
		0, 0, 0, 0, /// colors in color table
		0, 0, 0, 0, /// important color count
	};

	infoHeader[0] = (unsigned char)(INFO_HEADER_SIZE);
	infoHeader[4] = (unsigned char)(m_width);
	infoHeader[5] = (unsigned char)(m_width >> 8);
	infoHeader[6] = (unsigned char)(m_width >> 16);
	infoHeader[7] = (unsigned char)(m_width >> 24);
	infoHeader[8] = (unsigned char)(m_height);
	infoHeader[9] = (unsigned char)(m_height >> 8);
	infoHeader[10] = (unsigned char)(m_height >> 16);
	infoHeader[11] = (unsigned char)(m_height >> 24);
	infoHeader[12] = (unsigned char)(1);
	infoHeader[14] = (unsigned char)(BYTES_PER_PIXEL * 8);

	return infoHeader;
}
