#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include "HarrisDetector.h"
#include "ps.h"

auto greaterThan = [](const float x) {
	return [x](const float y)->bool {
		return y > x;
	};
};

int main(int argc, char** argv) {
	// Check if image paths are supplied as arguments
	if (argc < 3) {
		std::cout << "Image URIs need to be applied as arguments." << std::endl;
		return -1;
	}

	// Read images and check if successful
	cv::Mat imgl = cv::imread(argv[1]);
	if (imgl.empty()) {
		std::cout << "Could not open or find the first/left image." << std::endl;
		return -1;
	}
	cv::Mat imgr = cv::imread(argv[2]);
	if (imgr.empty()) {
		std::cout << "Could not open or find the second/right image." << std::endl;
		return -1;
	}

	int heightl = imgl.rows;
	int widthl = imgl.cols;
	int heightr = imgr.rows;
	int widthr = imgr.cols;
	// faust regel for window sizes :-)
	int wsize_sum = (int)((widthl + widthr + heightl + heightr) / 1300. + 0.5);
	if (wsize_sum < 1) {
		wsize_sum = 1;
	}
	int wsize_local_maxima = (int)((widthl + widthr + heightl + heightr) / 500. + 0.5);
	if (wsize_local_maxima < 2) {
		wsize_local_maxima = 2;
	}
	int wsize_match = (int)((widthl + widthr + heightl + heightr) / 90. + 0.5);
	if (wsize_match < 5) {
		wsize_match = 5;
	}
	if (wsize_match > 40) {
		wsize_match = 40;
	}
	std::cout << "Images loaded. wsize_sum=" << wsize_sum << ", wsize_loc=" << wsize_local_maxima << ", wsize_match=" << wsize_match << std::endl;

	// Harris detector
	std::cout << "Start Harris detector ..." << std::endl;
	std::vector<KEYPOINT> pointsl = harris(heightl, widthl, imgl.ptr(0), wsize_sum, wsize_local_maxima, "L");
	//std::vector<KEYPOINT> pointsl = HarrisDetector(imgl).filterKeyPoints(greaterThan(1000));
	std::cout << pointsl.size() << " keypoints in the left image" << std::endl;

	std::vector<KEYPOINT> pointsr = harris(heightr, widthr, imgr.ptr(0), wsize_sum, wsize_local_maxima, "R");
	std::cout << pointsr.size() << " keypoints in the right image" << std::endl;


	//*****************************************************************************************************
	cv::Mat GaussianKernel = (cv::Mat_<float>(5, 5) <<
		1, 4, 7, 4, 1,
		4, 16, 26, 16, 4,
		7, 16, 41, 16, 7,
		4, 16, 26, 16, 4,
		1, 4, 7, 4, 1);
	GaussianKernel = GaussianKernel / 273.0;
	cv::Mat Tmp(imgl.size() / 2, imgl.type());

	for (size_t r = 0; r < Tmp.rows; r++) {
		for (size_t c = 0; c < Tmp.cols; c++) {
			cv::Mat onePixelSourceROI(imgl, cv::Rect(cv::Point(c * 2, r * 2), cv::Size(1, 1)));
			cv::Mat baum;
			cv::filter2D(onePixelSourceROI, baum, imgl.type(), GaussianKernel, cv::Point(-1, -1), 0, cv::BORDER_CONSTANT);
			Tmp.at<cv::Vec3b>(r, c) = baum.at<cv::Vec3b>(0,0);
		}
	}

	cv::Mat GausUp = GaussianKernel.clone();
	GausUp *= 4;
	cv::Mat Stretch(Tmp.size() * 2, imgl.type(), cv::Scalar::all(0));
	for (size_t r = 0; r < Tmp.rows; r++) {
		for (size_t c = 0; c < Tmp.cols; c++) {
			Stretch.at<cv::Vec3b>(r * 2, c * 2) = Tmp.at<cv::Vec3b>(r, c);
		}
	}
	cv::filter2D(Stretch, Stretch, imgl.type(), GausUp, cv::Point(-1, -1), 0, cv::BORDER_CONSTANT);
	//cv::pyrUp(Tmp, Stretch, cv::Size(Tmp.size() * 2));

	cv::imshow("Tmp", Tmp);
	cv::imshow("Stretch", Stretch);
	cv::waitKey(0);


	//*****************************************************************************************************


	// Matching
	std::cout << "Start matching ..." << std::endl;
	std::vector<MATCH> matches = matching(heightl, widthl, imgl.ptr(0), heightr, widthr, imgr.ptr(0), pointsl, pointsr, wsize_match);
	std::cout << matches.size() << " matching pairs found" << std::endl;

	// homography -- OpenCV implementation
	std::cout << "Start standard RANSAC ..." << std::endl;
	std::vector<cv::Point2d> v1;
	std::vector<cv::Point2d> v2;
	for (unsigned int i = 0; i < matches.size(); i++) {
		v1.push_back(cv::Point2d(matches[i].xl, matches[i].yl));
		v2.push_back(cv::Point2d(matches[i].xr, matches[i].yr));
	}
	cv::Mat H = cv::findHomography(v1, v2, CV_RANSAC);
	cv::Mat Hi = H.inv();
	std::cout << H << std::endl;
	std::cout << Hi << std::endl;

	// render the output
	std::cout << "Start simple rendering ..." << std::endl;
	cv::Mat imglt = imgr.clone();
	cv::warpPerspective(imgl, imglt, H, imglt.size(), cv::INTER_LINEAR);
	imglt = imglt * 0.5 + imgr * 0.5;
	cv::imwrite("warpedL.png", imglt);

	cv::Mat imgrt = imgl.clone();
	cv::warpPerspective(imgr, imgrt, Hi, imgrt.size(), cv::INTER_LINEAR);
	imgrt = imgrt * 0.5 + imgl * 0.5;
	cv::imwrite("warpedR.png", imgrt);

	// find two homographies
	std::cout << "Start another RANSAC ..." << std::endl;
	cv::Mat Hl, Hr;
	my_homographies(matches, Hl, Hr);

	// render the output
	std::cout << "Start complex rendering ..." << std::endl;
	render(heightl, widthl, imgl, heightr, widthr, imgr, Hl, Hr, "panorama.png");
}
