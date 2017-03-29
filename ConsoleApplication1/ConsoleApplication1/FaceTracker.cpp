#include "FaceTracker.h"

FaceTracker::FaceTracker()
:lbp_cascade_frontalface_file_name_("lbpcascade_frontalface.xml"),
haar_cascade_profileface_file_name_("haarcascade_profileface.xml"),
haar_cascade_frontalface_file_name_("haarcascade_frontalface_alt.xml"),
logging_path_("VCam_log.txt"),
my_dll_directory_path_(""),
MAX_FRAMES_TO_WAIT(30), no_frames_waited(0)
{
	my_dll_directory_path_ = GetMyDllDirectory();

	lbp_cascade_frontalface_file_name_ = my_dll_directory_path_ + lbp_cascade_frontalface_file_name_;
	haar_cascade_profileface_file_name_ = my_dll_directory_path_ + haar_cascade_profileface_file_name_;
	haar_cascade_frontalface_file_name_ = my_dll_directory_path_ + haar_cascade_frontalface_file_name_;
	logging_path_ = my_dll_directory_path_ + logging_path_;

	Log(lbp_cascade_frontalface_file_name_);

	if (LoadCascadeClassifiers() == false)
	{
		exit(1);
	}
}

bool FaceTracker::LoadCascadeClassifiers( )
{
	//"E:\\haarcascade_frontalface_alt.xml"
	if ( frontalface_cascade_.load( lbp_cascade_frontalface_file_name_ ) == false )
	{
		Log( "Could not load face cascade file : " + lbp_cascade_frontalface_file_name_);
		return false;
	}

	/*if (profileface_cascade_.load(haar_cascade_profileface_file_name_) == false)
	{
		std::cerr << "Could not load face cascade file : " << haar_cascade_profileface_file_name_ << std::endl;
		return false;
	}*/

	return true;
}

void FaceTracker::DetectAndTrack(Mat & frame)
{
	bool is_face_detected = ((previous_faces_.size() > 0 && previous_faces_[0].width > 0) ? true : false);

	if (frame.empty())
		return;

	if (!is_face_detected)
	{
		is_face_detected = DetectFaces(frontalface_cascade_, frame, false);

		if (is_face_detected)
			Log("DetectAndTrack::face detected");
	}

	if (is_face_detected)
	{
		Mat output_image;
		CamshiftTracker(frame, previous_faces_[0], output_image);
		frame = output_image;
	}
}

void FaceTracker::StartFaceDetector(const std::string & video_file_name)
{
	cv::VideoCapture video_capture(video_file_name);

	if (video_capture.isOpened() == false)
	{
		std::cerr << "Could not open video file " << video_file_name << std::endl;
		exit(1);
	}

	StartTracking(video_capture);
}

void FaceTracker::StartTracking(cv::VideoCapture & video_capture)
{
	namedWindow("face");
	for (;;)
	{
		Mat frame;
		video_capture >> frame;
		if (frame.empty() == true)
		{
			std::cerr << "Frame empty--break" << std::endl;
			continue;
		}

		DetectAndBlur(frame);
		imshow("face", frame);
		if (waitKey(2) >= 0)
			break;
	}
}

void FaceTracker::StartCamShift(const std::string & video_file_name)
{
	VideoCapture video_capture(0);
	StartCamShiftTracker(video_capture);
}

void FaceTracker::StartCamShift()
{
	VideoCapture video_capture(0);
	StartCamShiftTracker(video_capture);
}

void FaceTracker::StartCamShiftTracker(cv::VideoCapture & video_capture)
{
	if (!video_capture.isOpened())
	{
		std::cout << "***Could not initialize capturing...***\n";
		return;
	}

	Mat frame;
	bool is_face_detected = false;
	Rect face;

	while (true)
	{
		video_capture >> frame;
		if (frame.empty())
			continue;

		DetectAndTrack(frame);
		imshow("CamShift", frame);
		//imshow( "Histogram", histimg );

		if (waitKey(30) >= 0)
			break;
	}
}

void FaceTracker::CamshiftTracker(const Mat & frame, Rect face, Mat & output_image)
{
	Rect selection = face;
	Rect trackWindow;
	int hsize = 16;

	float hranges[] = { 0, 180 };
	const float* phranges = hranges;

	Mat hsv, hue, mask, hist, histimg = Mat::zeros(200, 320, CV_8UC3), backproj;

	Mat image;
	int vmin = 10, vmax = 256, smin = 30;

	frame.copyTo(image);

	cvtColor(image, hsv, COLOR_BGR2HSV);

	//int _vmin = vmin, _vmax = vmax;

	inRange(hsv, Scalar(0, smin, MIN(vmin, vmax)),
		Scalar(180, 256, MAX(vmin, vmax)), mask);

	int ch[] = { 0, 0 };
	hue.create(hsv.size(), hsv.depth());
	mixChannels(&hsv, 1, &hue, 1, ch, 1);

	Mat roi(hue, selection), maskroi(mask, selection);
	calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
	normalize(hist, hist, 0, 255, CV_MINMAX);

	trackWindow = selection;

	histimg = Scalar::all(0);
	int binW = histimg.cols / hsize;

	Mat buf(1, hsize, CV_8UC3);
	for (int i = 0; i < hsize; i++)
		buf.at<Vec3b>(i) = Vec3b(saturate_cast<uchar>(i*180. / hsize), 255, 255);

	cvtColor(buf, buf, CV_HSV2BGR);

	for (int i = 0; i < hsize; i++)
	{
		int val = saturate_cast<int>(hist.at<float>(i)*histimg.rows / 255);
		rectangle(histimg, Point(i*binW, histimg.rows),
			Point((i + 1)*binW, histimg.rows - val),
			Scalar(buf.at<Vec3b>(i)), -1, 8);
	}

	calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
	backproj &= mask;
	RotatedRect trackBox = CamShift(backproj, trackWindow,
		TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1));

	if (trackWindow.area() <= 1)
	{
		int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5) / 6;
		trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
			trackWindow.x + r, trackWindow.y + r) &
			Rect(0, 0, cols, rows);
	}

	//below commented code is for back-projection view
	//if( backprojMode )
	//cvtColor( backproj, image, COLOR_GRAY2BGR );

	cv::ellipse(image, trackBox, Scalar(0, 0, 255), 3, CV_AA);

	//This line of code is if you want to draw rectangle around tracking area instead of ellipse
	//cv::rectangle( image , trackWindow , CV_RGB( 0 , 255 , 1 ) , 1 );

	//below commented code is for hightlighting the selection/face area
	/*if( selection.width > 0 && selection.height > 0 )
	{
	Mat roi(image, selection);
	bitwise_not(roi, roi);
	}*/

	output_image = image;
}

void FaceTracker::DetectAndBlur(cv::Mat & frame)
{
	bool is_detected = DetectFaces(frontalface_cascade_, frame, false);

	if (!is_detected)
	{
		is_detected = DetectFaces(profileface_cascade_, frame, false);
	}

	if (!is_detected)
	{
		is_detected = DetectFaces(profileface_cascade_, frame, true);
	}

	if (is_detected)
	{
		no_frames_waited = 0;
	}
	else
	{
		++no_frames_waited;
	}

	if (no_frames_waited >= MAX_FRAMES_TO_WAIT)
	{
		previous_faces_.clear();
		Blur(frame);
	}

	for (unsigned int i = 0; i < previous_faces_.size(); i++)
	{
		//		BlurFace( frame( previous_faces_[i] ) );
		cv::rectangle(frame, previous_faces_[i], CV_RGB(0, 255, 1), 1);
	}
}

void FaceTracker::StartFaceDetector()
{
	cv::VideoCapture video_capture(0);

	if (video_capture.isOpened() == false)
	{
		std::cerr << "Could not open camera" << std::endl;
		exit(1);
	}

	StartTracking(video_capture);
}

bool FaceTracker::DetectFaces(cv::CascadeClassifier face_cascade, cv::Mat frame, bool flip_image)
{
	std::vector<cv::Rect> faces;
	cv::Mat frame_gray;

	cv::cvtColor(frame, frame_gray, CV_BGR2GRAY);
	equalizeHist(frame_gray, frame_gray);

	if (flip_image == true)
	{
		cv::flip(frame_gray, frame_gray, 1);
	}

	cv::Size face_size = cv::Size(frame_gray.cols / 4, frame_gray.cols / 4);
	//face_cascade.detectMultiScale( frame_gray , faces );
	face_cascade.detectMultiScale(frame_gray, faces, 1.1, 3, 0 | CV_HAAR_SCALE_IMAGE | CV_HAAR_DO_CANNY_PRUNING, face_size);

	//cv::flip( frame_gray( faces[0] ) , frame_gray( faces[0] ) , 100 );
	if (flip_image)
	{
		for (int i = 0; i < faces.size(); i++)
		{
			int x = faces[i].x;
			int y = faces[i].y;
			int width = faces[i].width;
			int height = faces[i].height;

			int x_distance = x + width;
			int y_distance = y + height;

			int x1 = frame_gray.cols - x_distance;
			int y1 = frame_gray.rows - y_distance;
			int width1 = x1 + width;
			int height1 = y1 + height;
			faces[i] = cv::Rect(x1, y, width, height);
		}
	}

	if (faces.size() > 0)
	{
		previous_faces_ = faces;
		return true;
	}

	return false;
}

bool FaceTracker::HaarDetector(const cv::Mat & image, cv::Mat & dest)
{
	std::string faceCascadeName = "haarcascade_frontalface_alt.xml";
	cv::CascadeClassifier classifier;

	if (classifier.load(faceCascadeName) == 0)
	{
		std::cout << "unable to load training data for classifier" << std::endl;
		return false;
	}

	cv::Mat grayImage;
	if (image.channels() == 3)
		cvtColor(image, grayImage, CV_BGR2GRAY);
	else if (image.channels() == 4)
		cvtColor(image, grayImage, CV_BGRA2GRAY);
	else
		grayImage = image;

	cv::equalizeHist(grayImage, grayImage);
	std::vector<cv::Rect> faces;
	classifier.detectMultiScale(grayImage, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE | CV_HAAR_FIND_BIGGEST_OBJECT, cv::Size(80, 80));

	if (faces.size() == 0)
	{
		std::cout << "No face detected" << std::endl;
		return false;
	}

	dest = grayImage(faces[0]);
	return true;
}

bool FaceTracker::LbpFaceDetector(const cv::Mat & image, cv::Mat & dest)
{
	std::string faceCascadeName = "lbpcascade_frontalface.xml";
	cv::CascadeClassifier classifier;

	if (classifier.load(faceCascadeName) == 0)
	{
		std::cout << "unable to load training data for classifier" << std::endl;
		return false;
	}

	cv::Mat grayImage;
	if (image.channels() == 3)
		cvtColor(image, grayImage, CV_BGR2GRAY);
	else if (image.channels() == 4)
		cvtColor(image, grayImage, CV_BGRA2GRAY);
	else
		grayImage = image;

	equalizeHist(grayImage, grayImage);
	std::vector<cv::Rect> faces;
	classifier.detectMultiScale(grayImage, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, cv::Size(80, 80));

	if (faces.size() == 0)
	{
		std::cout << "No face detected" << std::endl;
		return false;
	}


	dest = grayImage(faces[0]);
	return true;
}

void FaceTracker::StartEdgeDetection(const std::string & video_file_name)
{
	cv::VideoCapture video_capture(video_file_name);

	if (video_capture.isOpened() == false)
	{
		std::cerr << "Could not open video file" << std::endl;
		exit(1);
	}

	namedWindow("edges");
	Mat edges;
	for (;;)
	{
		Mat frame;
		video_capture >> frame;
		cvtColor(frame, edges, CV_BGR2GRAY);

		GaussianBlur(edges, edges, Size(7, 7), 1.5, 1.5);
		Canny(edges, edges, 0, 30, 3);
		imshow("edges", edges);

		if (waitKey(30) >= 0)
			break;
	}
}

void FaceTracker::Blur(cv::Mat & image)
{
	//GaussianBlur( face , face , Size( 7 , 7 ) , 100 , 100 );
	medianBlur(image, image, 51);
}

char * FaceTracker::GetCurrentWorkingDirectory()
{
	char * buffer = NULL;
	if ((buffer = _getcwd(NULL, 0)) == NULL)
	{
		std::cerr << "Unable to get current working directory" << std::endl;
		exit(1);
	}

	return buffer;
}

bool FaceTracker::Log(std::string str)
{
	static int count = 0;
	++count;

	std::ofstream output_file( logging_path_, std::ios::app);

	if (output_file.is_open() == false)
	{
		return false;
	}

	output_file << count << ": " << str << std::endl;
	output_file.flush();

	output_file.close();

	return true;
}

std::string FaceTracker::GetMyDllDirectory()
{
	WCHAR   dll_path[MAX_PATH] = { 0 };
	GetModuleFileNameW((HINSTANCE)&__ImageBase, dll_path, _countof(dll_path));

	// convert from wide char to narrow char array
	char ch[260];
	char DefChar = ' ';
	WideCharToMultiByte(CP_ACP, 0, dll_path, -1, ch, 260, &DefChar, NULL);

	//convert char * to string
	std::string str(ch);

	if (str.length() == 0)
	{
		return str;
	}

	int i = str.length() - 1;
	while (i > 0 && str[i] != '\\')
	{
		i--;
	}

	str.resize(i + 1);
	return str;
}