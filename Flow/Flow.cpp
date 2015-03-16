// Flow.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
// first four bytes, should be the same in little endian
#define TAG_FLOAT 202021.25  // check for this when READING the file
#define TAG_STRING "PIEH"    // use this when WRITING the file


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "imageLib.h"
#include "flowIO.h"
void ReadFlowFile(CFloatImage& img, const char* filename)
{
	if (filename == NULL)
		throw CError("ReadFlowFile: empty filename");

	const char *dot = strrchr(filename, '.');
	if (strcmp(dot, ".flo") != 0)
		throw CError("ReadFlowFile (%s): extension .flo expected", filename);

	FILE *stream = fopen(filename, "rb");
	if (stream == 0)
		throw CError("ReadFlowFile: could not open %s", filename);

	int width, height;
	float tag;

	if ((int)fread(&tag, sizeof(float), 1, stream) != 1 ||
		(int)fread(&width, sizeof(int), 1, stream) != 1 ||
		(int)fread(&height, sizeof(int), 1, stream) != 1)
		throw CError("ReadFlowFile: problem reading file %s", filename);

	if (tag != TAG_FLOAT) // simple test for correct endian-ness
		throw CError("ReadFlowFile(%s): wrong tag (possibly due to big-endian machine?)", filename);

	// another sanity check to see that integers were read correctly (99999 should do the trick...)
	if (width < 1 || width > 99999)
		throw CError("ReadFlowFile(%s): illegal width %d", filename, width);

	if (height < 1 || height > 99999)
		throw CError("ReadFlowFile(%s): illegal height %d", filename, height);

	int nBands = 2;
	CShape sh(width, height, nBands);
	img.ReAllocate(sh);

	//printf("reading %d x %d x 2 = %d floats\n", width, height, width*height*2);
	int n = nBands * width;
	for (int y = 0; y < height; y++) {
		float* ptr = &img.Pixel(0, y, 0);
		if ((int)fread(ptr, sizeof(float), n, stream) != n)
			throw CError("ReadFlowFile(%s): file is too short", filename);
	}

	if (fgetc(stream) != EOF)
		throw CError("ReadFlowFile(%s): file is too long", filename);

	fclose(stream);
}
// write a 2-band image into flow file 
void WriteFlowFile(CFloatImage img, const char* filename)
{
	if (filename == NULL)
		throw CError("WriteFlowFile: empty filename");

	const char *dot = strrchr(filename, '.');
	if (dot == NULL)
		throw CError("WriteFlowFile: extension required in filename '%s'", filename);

	if (strcmp(dot, ".flo") != 0)
		throw CError("WriteFlowFile: filename '%s' should have extension '.flo'", filename);

	CShape sh = img.Shape();
	int width = sh.width, height = sh.height, nBands = sh.nBands;

	if (nBands != 2)
		throw CError("WriteFlowFile(%s): image must have 2 bands", filename);

	FILE *stream = fopen(filename, "wb");
	if (stream == 0)
		throw CError("WriteFlowFile: could not open %s", filename);

	// write the header
	fprintf(stream, TAG_STRING);
	if ((int)fwrite(&width, sizeof(int), 1, stream) != 1 ||
		(int)fwrite(&height, sizeof(int), 1, stream) != 1)
		throw CError("WriteFlowFile(%s): problem writing header", filename);

	// write the rows
	int n = nBands * width;
	for (int y = 0; y < height; y++) {
		float* ptr = &img.Pixel(0, y, 0);
		if ((int)fwrite(ptr, sizeof(float), n, stream) != n)
			throw CError("WriteFlowFile(%s): problem writing data", filename);
	}

	fclose(stream);
}
int main() {

	try {
		CShape sh(5, 1, 2);
		CFloatImage img(sh);
		img.ClearPixels();
		img.Pixel(0, 0, 0) = -5.0f;
		char *filename = "test.flo";

		WriteFlowFile(img, filename);
		ReadFlowFile(img, filename);
	}
	catch (CError &err) {
		fprintf(stderr, err.message);
		fprintf(stderr, "\n");
		exit(1);
	}

	return 0;
}
