#pragma once

const static int NUM_THREADS = cv::getNumberOfCPUs();

const static cv::Scalar Color_BLUE    = cv::Scalar(255, 0, 0);
const static cv::Scalar Color_GREEN   = cv::Scalar(0, 200, 0);
const static cv::Scalar Color_RED     = cv::Scalar(0, 0, 255);
const static cv::Scalar Color_YELLOW  = cv::Scalar(0, 255, 255);
const static cv::Scalar Color_MAGENTA = cv::Scalar(255, 0, 255);
const static cv::Scalar Color_CYAN    = cv::Scalar(255, 255, 0);
const static cv::Scalar Color_WHITE   = cv::Scalar(255, 255, 255);
const static cv::Scalar Color_BLACK   = cv::Scalar(0, 0, 0);

class Common
{
public:
	static const std::string CheckboardConfigurationFile;

	static const std::string IntrinsicsConfigurationFile;

	static const std::string CalibrationVideoFile;

	static const std::string CheckerboardVideoFile;

	static const std::string CheckerboadCornersFile;

	static const std::string VideoFile;

	static const std::string ConfigFile;

	static const std::string CalibrationImage;

	static const std::string MatteStill;

	static const std::string MatteVideo;

	static void MatToFloatArray(const cv::Mat &in, float *out);

	static void ProjectPoints(cv::Point3f point, const cv::Mat r_vec, const cv::Mat t_vec, const cv::Mat A, const cv::Mat distCoeffs, cv::Point &projectedPoint);

	static bool ReadCheckerboardProperties(std::string dataPath, int &boardWidth, int &boardHeight, float &boardSquareSize);

	static long FetchNumberOfFrames(std::string dataPath, std::string videoFile);

	static cv::Mat Common::MakeCanvas(std::vector<cv::Mat>& vecMat, int windowHeight, int nRows);
};