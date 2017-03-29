#ifndef FACETRACKER_H_
#define FACETRACKER_H_

#include <iostream>
#include <string>
#include <opencv\cv.h>
//#include <cor.h>
#include <opencv2\opencv.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <Windows.h>
#include <string>
#include <fstream>

using namespace cv;
using namespace std;

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

class FaceTracker
{

public:
	void DetectAndTrack(Mat & frame);

	FaceTracker();
	void StartFaceDetector(const std::string & video_file_name);
	void StartFaceDetector();
	void StartCamShift();
	void StartCamShift(const std::string & video_file_name);

private:
	void StartCamShiftTracker(cv::VideoCapture & video_capture);
	void CamshiftTracker(const Mat & frame, Rect face, Mat & output_image);
	bool DetectFaces(cv::CascadeClassifier face_cascade, cv::Mat frame, bool flip_image);

	void StartTracking(cv::VideoCapture & video_capture);
	void DetectAndBlur(cv::Mat & frame);

	void Blur(cv::Mat & image);
	bool FaceTracker::HaarDetector(const cv::Mat & image, cv::Mat & dest);
	bool FaceTracker::LbpFaceDetector(const cv::Mat & image, cv::Mat & dest);

	void FaceTracker::StartEdgeDetection(const std::string & video_file_name);
	char * GetCurrentWorkingDirectory();
	bool LoadCascadeClassifiers();
	bool Log(std::string str);

	std::string GetMyDllDirectory();
private:
	std::vector<cv::Rect> previous_faces_;
	std::string lbp_cascade_frontalface_file_name_;
	std::string haar_cascade_profileface_file_name_;
	std::string haar_cascade_frontalface_file_name_;

	cv::CascadeClassifier frontalface_cascade_;
	cv::CascadeClassifier profileface_cascade_;

	const int MAX_FRAMES_TO_WAIT;
	int no_frames_waited;
	std::string logging_path_;
	std::string my_dll_directory_path_;
};

#endif