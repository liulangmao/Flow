// flow_io.cpp
//
// read and write our simple .flo flow file format

// ".flo" file format used for optical flow evaluation
//
// Stores 2-band float image for horizontal (u) and vertical (v) flow components.
// Floats are stored in little-endian order.
// A flow value is considered "unknown" if either |u| or |v| is greater than 1e9.
//
//  bytes  contents
//
//  0-3     tag: "PIEH" in ASCII, which in little endian happens to be the float 202021.25
//          (just a sanity check that floats are represented correctly)
//  4-7     width as an integer
//  8-11    height as an integer
//  12-end  data (width*height*2*4 bytes total)
//          the float values for u and v, interleaved, in row order, i.e.,
//          u[row0,col0], v[row0,col0], u[row0,col1], v[row0,col1], ...
//


// first four bytes, should be the same in little endian

#include "stdafx.h"
#include <opencv2/opencv.hpp>
using namespace cv;
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <math.h>
#include "imageLib.h"
#include "flowIO.h"
#define TAG_FLOAT 202021.25  // check for this when READING the file
#define TAG_STRING "PIEH"    // use this when WRITING the file
#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
// return whether flow vector is unknown
bool unknown_flow(float u, float v) {
    return (fabs(u) >  UNKNOWN_FLOW_THRESH) 
	|| (fabs(v) >  UNKNOWN_FLOW_THRESH)
	|| isnan(u) || isnan(v);
}

bool unknown_flow(float *f) {
    return unknown_flow(f[0], f[1]);
}

// read a flow file into 2-band image
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

    if ((int)fread(&tag,    sizeof(float), 1, stream) != 1 ||
	(int)fread(&width,  sizeof(int),   1, stream) != 1 ||
	(int)fread(&height, sizeof(int),   1, stream) != 1)
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
    if ((int)fwrite(&width,  sizeof(int),   1, stream) != 1 ||
	(int)fwrite(&height, sizeof(int),   1, stream) != 1)
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

#define WriteFile 1
int main() {

    try {
	//CShape sh(5, 1, 2);
	//CFloatImage img(sh);
	CFloatImage img;
	Mat tempTrainA,tempTrainB,resultC;
	tempTrainA = cv::imread("imm1.png", CV_LOAD_IMAGE_COLOR);
	tempTrainB = cv::imread("imm2.png", CV_LOAD_IMAGE_COLOR);
	tempTrainB.convertTo(tempTrainB, CV_32FC3);
	tempTrainA.convertTo(tempTrainA, CV_32FC3);
	resultC = Mat::zeros(tempTrainA.rows, tempTrainA.cols, CV_32FC3);;

	//char *filename = "flotest.flo";//get the originial result
	char *filename = "flo.flo";
	//WriteFlowFile(img, filename);
	ReadFlowFile(img, filename);
	CShape sh = img.Shape(); 
	int width = sh.width;
	int height = sh.height;
	int band = sh.nBands;
	cout << "width:" << width << "height:" << height << "band:" << band << endl;
#ifdef WriteFile
	ofstream xtxt("x.txt", std::ofstream::out | std::ofstream::app);
	for (int c = 0; c < width; c++)
	{
		for (int r = 0; r < height; r++)
		{
			xtxt << img.Pixel(c, r, 0) << " ";
		}
	}
	xtxt.close();
	ofstream ytxt("y.txt", std::ofstream::out | std::ofstream::app);
	for (int c = 0; c < width; c++)
	{
		for (int r = 0; r < height; r++)
		{
			ytxt << img.Pixel(c, r, 1) << " ";
		}
	}
	ytxt.close();
#endif // WriteFile
	
	for (int c = 0; c < width; c++)
	{
		for (int r = 0; r < height; r++)
		{
	//int c = 248, r = 135;//test corner1
	//int c = 188, r = 192;//test corner2
			
				float uu = img.Pixel(c, r, 0);
				float vv = img.Pixel(c, r, 1);
				//cout << "c:" << c << "r:" << r << ";" << uu << " " << vv << "" << endl;
				
				//int u =round(img.Pixel(c, r, 0)+c);
				//int v = round(img.Pixel(c, r, 1)+r);//this value has negative values
				int u = 0;
				int v = 0;
				if (abs(uu)>=1e9 || abs(vv) >=1e9)
				{
					u = 0;
					v = 0;
				}
				else{
					u = (round(img.Pixel(c, r, 0) + c) >= width) ? width - 1 : round(img.Pixel(c, r, 0) + c);
					v = (round(img.Pixel(c, r, 1) + r) >= height) ? height - 1 : round(img.Pixel(c, r, 1) + r);
					u = (round(img.Pixel(c, r, 0) + c)  < 0) ? 0 : u;
					v = (round(img.Pixel(c, r, 1) + r)  < 0) ? 0 : v;
				}
				
		    	//cout << "c:" << c << "r:" << r << ";" << u << " " << v << "" << endl;
				for (int i = 0; i < 3; i++)
				{
				//resultC.at<Vec3f>(r, c)[i] = tempTrainB.at<Vec3f>(v, u)[i];//(y,x)   r horizontalshuipingde==x (u) and vertical (v) flow components.
					resultC.at<Vec3f>(r, c)[i] = tempTrainB.at<Vec3f>(v, u)[i];//(y,x)   r horizontalshuipingde==x (u) and vertical (v) flow components.
			    }
				/*Point pt1(c, r);
				Point pt2(u, v);
				line(tempTrainA, pt1, pt2, Scalar(0, 255, 255), 1, 8, 0);*/
		}
	}
	cv::imwrite("result.png", resultC);
    }
    catch (CError &err) {
	fprintf(stderr, err.message);
	fprintf(stderr, "\n");
	exit(1);
    }

    return 0;
}
