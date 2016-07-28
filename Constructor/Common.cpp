#include "Stdafx.h"

#include "Common.h"

const std::string Common::CheckboardConfigurationFile = "checkerboard.xml";
const std::string Common::CalibrationVideoFile = "calibration20x15.mkv";
const std::string Common::CheckerboardVideoFile = "checkerboard20x15.mkv";
const std::string Common::VideoFile = "video.mkv";
const std::string Common::IntrinsicsConfigurationFile = "intrinsics.xml";
const std::string Common::CheckerboadCornersFile = "boardcorners.xml";
const std::string Common::ConfigFile = "config.xml";
const std::string Common::CalibrationImage = "calibration_%03d.png";
const std::string Common::MatteStill = "matte.png";
const std::string Common::MatteVideo = "matte.mp4";

void Common::MatToFloatArray(const cv::Mat &in, float *out)
{
	int n = 0;
	for (int r = 0; r < in.rows; ++r)
	{
		for (int c = 0; c < in.cols; ++c)
		{
			out[n++] = in.ptr<float>(r)[c];
		}
	}
}

void Common::ProjectPoints(cv::Point3f point, const cv::Mat r_vec, const cv::Mat t_vec, const cv::Mat A, const cv::Mat distCoeffs, cv::Point &projectedPoint)
{
	float R[9], t[3], a[9], k[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, fx, fy, cx, cy;

	cv::Mat matR(3, 3, CV_32F, R);
	cv::Rodrigues(r_vec, matR);

	Common::MatToFloatArray(t_vec, t);
	Common::MatToFloatArray(A, a);
	Common::MatToFloatArray(distCoeffs, k);

	fx = a[0]; fy = a[4];
	cx = a[2]; cy = a[5];

	float X = point.x, Y = point.y, Z = point.z;
	float x = R[0] * X + R[1] * Y + R[2] * Z + t[0];
	float y = R[3] * X + R[4] * Y + R[5] * Z + t[1];
	float z = R[6] * X + R[7] * Y + R[8] * Z + t[2];
	float r2, r4, r6, a1, a2, a3, cdist, icdist2;
	float xd, yd;

	z = z ? 1. / z : 1;
	x *= z; y *= z;

	r2 = x * x + y * y;
	r4 = r2 * r2;
	r6 = r4 * r2;
	a1 = 2 * x*y;
	a2 = r2 + 2 * x * x;
	a3 = r2 + 2 * y * y;
	cdist = 1 + k[0] * r2 + k[1] * r4 + k[4] * r6;

	float k5 = k[5], k6 = k[6], k7 = k[7];
	icdist2 = 1.0f / (1 + k[5] * r2 + k[6] * r4 + k[7] * r6);
	xd = x * cdist * icdist2 + k[2] * a1 + k[3] * a2 + k[8] * r2 + k[9] * r4;
	yd = y * cdist * icdist2 + k[2] * a3 + k[3] * a1 + k[10] * r2 + k[11] * r4;

	projectedPoint.x = xd * fx + cx;
	projectedPoint.y = yd * fy + cy;
}

bool Common::ReadCheckerboardProperties(std::string dataPath, int &boardWidth, int &boardHeight, float &boardSquareSize)
{
	// Read the checkerboard properties
	cv::FileStorage fs;
	fs.open(dataPath + "../" + Common::CheckboardConfigurationFile, cv::FileStorage::READ);
	if (fs.isOpened())
	{
		fs["CheckerBoardWidth"] >> boardWidth;
		fs["CheckerBoardHeight"] >> boardHeight;
		fs["CheckerBoardSquareSize"] >> boardSquareSize;
	}
	else
	{
		return false;
	}

	fs.release();

	return true;
}

long Common::FetchNumberOfFrames(std::string dataPath, std::string videoFile)
{
	long frames;

	cv::VideoCapture cap(dataPath + videoFile);
	{
		assert(cap.isOpened());

		// Fetch number of frames
		cap.set(CV_CAP_PROP_POS_AVI_RATIO, 1);
		frames = (long) cap.get(CV_CAP_PROP_POS_FRAMES);
		assert(frames > 1);
		cap.set(CV_CAP_PROP_POS_AVI_RATIO, 0);
	}
	cap.release();

	return frames;
}

cv::Mat Common::MakeCanvas(std::vector<cv::Mat> &vecMat, int windowHeight, int nRows) 
{
	int N = vecMat.size();
	nRows = nRows > N ? N : nRows;
	int edgeThickness = 3;
	int imagesPerRow = ceil(double(N) / nRows);
	int resizeHeight = floor(2.0 * ((floor(double(windowHeight - edgeThickness) / nRows)) / 2.0)) - edgeThickness;
	int maxRowLength = 0;

	std::vector<int> resizeWidth;
	for (int i = 0; i < N;) 
	{
		int thisRowLen = 0;
		for (int k = 0; k < imagesPerRow; ++k) 
		{
			double aspectRatio = double(vecMat[i].cols) / vecMat[i].rows;
			int temp = int(ceil(resizeHeight * aspectRatio));
			resizeWidth.push_back(temp);
			thisRowLen += temp;

			if (++i == N)
			{
				break;
			}
		}
		if ((thisRowLen + edgeThickness * (imagesPerRow + 1)) > maxRowLength) 
		{
			maxRowLength = thisRowLen + edgeThickness * (imagesPerRow + 1);
		}
	}

	int windowWidth = maxRowLength;
	cv::Mat canvasImage(windowHeight, windowWidth, CV_8UC3, cv::Scalar(0, 0, 0));

	for (int k = 0, i = 0; i < nRows; ++i) 
	{
		int y = i * resizeHeight + (i + 1) * edgeThickness;
		int x_end = edgeThickness;

		for (int j = 0; j < imagesPerRow && k < N; ++k, ++j) 
		{
			int x = x_end;
			cv::Rect roi(x, y, resizeWidth[k], resizeHeight);
			cv::Size s = canvasImage(roi).size();

			// Change the number of channels to three
			cv::Mat target_ROI(s, CV_8UC3);
			if (vecMat[k].channels() != canvasImage.channels()) 
			{
				if (vecMat[k].channels() == 1) 
				{
					cv::cvtColor(vecMat[k], target_ROI, CV_GRAY2BGR);
				}
			}
			else
			{
				target_ROI = vecMat[k];
			}
			
			cv::resize(target_ROI, target_ROI, s);
			if (target_ROI.type() != canvasImage.type()) 
			{
				target_ROI.convertTo(target_ROI, canvasImage.type());
			}

			target_ROI.copyTo(canvasImage(roi));
			x_end += resizeWidth[k] + edgeThickness;
		}
	}
	return canvasImage;
}