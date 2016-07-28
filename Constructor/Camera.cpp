#include "Stdafx.h"

#include "Common.h"
#include "Camera.h"
#include "Exception.h"

Camera::Camera(Settings &settings, std::string cameraPath, const int id) : m_Id(id), m_Settings(settings), m_CameraPath(cameraPath)
{
	this->m_Initialized = false;

	// Focal length 
	this->m_Fx = 0;
	this->m_Fy = 0;

	// Principal point
	this->m_Px = 0;
	this->m_Py = 0;

	this->m_FrustumDistance = 0.5;

	// Number of frames
	this->m_Frames = 0;
}

Camera::~Camera(void)
{
}

bool Camera::Initialize(void)
{
	// Open the video for this camera
	this->m_Video = cv::VideoCapture(this->m_CameraPath + Common::VideoFile);
	{
		assert(this->m_Video.isOpened());

		// Determine image size
		this->m_FrustumSize.width = (int)this->m_Video.get(CV_CAP_PROP_FRAME_WIDTH);
		this->m_FrustumSize.height = (int)this->m_Video.get(CV_CAP_PROP_FRAME_HEIGHT);
		assert(this->m_FrustumSize.area() > 0);

		// Fetch number of frames
		this->m_Video.set(CV_CAP_PROP_POS_AVI_RATIO, 1);
		this->m_Frames = (long)this->m_Video.get(CV_CAP_PROP_POS_FRAMES);
		assert(this->m_Frames > 1);
		this->m_Video.set(CV_CAP_PROP_POS_AVI_RATIO, 0);

		// Rewind
		this->m_Video.release();
	}
	this->m_Video = cv::VideoCapture(this->m_CameraPath + Common::VideoFile);

	// Load matte video if required
	this->m_MatteVideo = cv::VideoCapture(this->m_CameraPath + Common::MatteVideo);
	{
		assert(this->m_MatteVideo.isOpened());

		// Determine image size
		cv::Size size;
		size.width = (int)this->m_MatteVideo.get(CV_CAP_PROP_FRAME_WIDTH);
		size.height = (int)this->m_MatteVideo.get(CV_CAP_PROP_FRAME_HEIGHT);
		assert(size.area() > 0);

		// Fetch number of frames
		this->m_MatteVideo.set(CV_CAP_PROP_POS_AVI_RATIO, 1);
		const int frames = (long)this->m_MatteVideo.get(CV_CAP_PROP_POS_FRAMES);
		assert(frames > 1);
		this->m_MatteVideo.set(CV_CAP_PROP_POS_AVI_RATIO, 0);

		// Rewind
		this->m_MatteVideo.release();

		// Check if matte video is of same length as normal video
		if (size.width != this->m_FrustumSize.width || size.height != this->m_FrustumSize.height || frames != this->m_Frames)
		{
			throw_line("Matte video is not equal to video input");
		}
	}
	this->m_MatteVideo = cv::VideoCapture(this->m_CameraPath + Common::MatteVideo);

	// Read the camera properties
	cv::FileStorage fs;
	fs.open(this->m_CameraPath + Common::ConfigFile, cv::FileStorage::READ);
	if (fs.isOpened())
	{
		// Reload previous properties
		cv::Mat cameraMatrix, distortionCoefficients, rotation, translation;
		fs["CameraMatrix"] >> cameraMatrix;
		fs["DistortionCoeffs"] >> distortionCoefficients;
		fs["RotationValues"] >> rotation;
		fs["TranslationValues"] >> translation;

		cameraMatrix.convertTo(this->m_CameraMatrix, CV_32F);
		distortionCoefficients.convertTo(this->m_DistortionCoeffs, CV_32F);
		rotation.convertTo(this->m_Rotation, CV_32F);
		translation.convertTo(this->m_Translation, CV_32F);

		fs.release();

		// Reload focal length and principal point
		this->m_Fx = *this->m_CameraMatrix.ptr<float>(0, 0);
		this->m_Fy = *this->m_CameraMatrix.ptr<float>(1, 1);
		this->m_Px = *this->m_CameraMatrix.ptr<float>(0, 2);
		this->m_Py = *this->m_CameraMatrix.ptr<float>(1, 2);
	}
	else
	{
		std::cerr << "Unable to locate: " << this->m_CameraPath << Common::ConfigFile << std::endl;

		// Achtung: Ein fehler
		this->m_Initialized = false;

		return false;
	}

	// Initialize camera parameters
	this->InitializeCameraLocation();
	this->DefineFrustumPoints();

	// Indicate that the camera is initialized
	this->m_Initialized = true;

	return true;
}

cv::cuda::GpuMat &Camera::NextVideoFrame(void)
{
	// Fetch new frame
	cv::Mat frame;
	this->m_Video >> frame;
	assert(!frame.empty());

	// Upload to device
	this->m_Frame.upload(frame);

	if (this->m_Settings.UseMatteVideo)
	{
		cv::Mat matteFrame;
		this->m_MatteVideo >> matteFrame;
		assert(!matteFrame.empty());

		// Convert the #$@#%!% to grayscale
		cv::cvtColor(matteFrame, matteFrame, CV_BGR2GRAY);

		// Upload to device
		this->m_ForegroundImage.upload(matteFrame);
	}

	return this->m_Frame;
}

void Camera::SetVideoFrame(int frameNumber)
{
	this->m_Video.set(CV_CAP_PROP_POS_FRAMES, frameNumber);
	this->m_MatteVideo.set(CV_CAP_PROP_POS_FRAMES, frameNumber);
}

void Camera::LoadForegroundFromMatteVideo(void)
{
	// Nothing to do here, lol
}

void Camera::LoadForegroundFromMatteStill(void)
{
	cv::Mat hostMatte = cv::imread(this->m_CameraPath + Common::MatteStill, CV_LOAD_IMAGE_GRAYSCALE);

	if (!hostMatte.data)
	{
		throw_line("Could not read camera still image!");
	}

	this->m_ForegroundImage.upload(hostMatte);
}

cv::cuda::GpuMat Camera::GetVideoFrame(int frameNumber)
{
	this->SetVideoFrame(frameNumber);

	return this->NextVideoFrame();
}

double Camera::ComputeReprojectionErrors(const std::vector<std::vector<cv::Point3f>> &objectPoints, const std::vector<std::vector<cv::Point2f>> &imagePoints, const std::vector<cv::Mat> &rvecs, const std::vector<cv::Mat> &tvecs, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs, std::vector<float> &perViewErrors)
{
	std::vector<cv::Point2f> imagePoints2;
	
	int i, totalPoints = 0;
	double totalErr = 0, err;
	
	perViewErrors.resize(objectPoints.size());

	for (i = 0; i < (int)objectPoints.size(); ++i)
	{
		cv::projectPoints(cv::Mat(objectPoints[i]), rvecs[i], tvecs[i], cameraMatrix, distCoeffs, imagePoints2);
		err = cv::norm(cv::Mat(imagePoints[i]), cv::Mat(imagePoints2), cv::NORM_L2);
		int n = (int)objectPoints[i].size();

		perViewErrors[i] = (float) std::sqrt(err * err / n);
		totalErr += err * err;
		totalPoints += n;
	}

	return std::sqrt(totalErr / totalPoints);
}

bool Camera::Calibrate(void)
{
	// Check if the intrinsics file exists, if so we don't have do do anything
	std::string intrinsicsFile = this->m_CameraPath + Common::IntrinsicsConfigurationFile;
	if (boost::filesystem::exists(intrinsicsFile))
	{
		// Nothing to do
		return true;
	}
	else
	{
		int boardWidth = 0, boardHeight = 0;
		float boardSquareSize = 0;
		Common::ReadCheckerboardProperties(this->m_CameraPath, boardWidth, boardHeight, boardSquareSize);

		int numFrames = Common::FetchNumberOfFrames(this->m_CameraPath, Common::CalibrationVideoFile);

		cv::Size boardSize = cv::Size(boardWidth, boardHeight);

		// Our camera matrix K and distortion coefficient matrix D
		cv::Mat K, D = cv::Mat::zeros(8, 1, CV_64F);

		// Perform calibration, assume that the calibration video exists
		// Open video files
		std::string videoFile = this->m_Settings.UseCalibrationImages ? this->m_CameraPath + Common::CalibrationImage : this->m_CameraPath + Common::CalibrationVideoFile;
		cv::VideoCapture cap = cv::VideoCapture(videoFile);
		{
			if (!cap.isOpened())
			{
				std::cout << "Could not read file: " << videoFile << std::endl;

				return false;
			}

			// Determine the object points for calibration, we're trying to determine what points in the image (taken from cap) refers to what object point.
			// Since we know that our object is a checkerboard, we can add object points that determine where we can expect the checkboards corners, this
			// data is then used to determine the camera intrinsics, since we know the fixed distance between the checkerboard corners and the distance
			// as found in the frame that depicts the checkerboard in the world. The variable resultsPointsBuffer will contain the corners as they were
			// detected in the frame
			std::vector<cv::Point2f> resultPointsBuffer;
			std::vector<std::vector<cv::Point2f>> imagePoints;
			std::vector<std::vector<cv::Point3f>> objectPoints(1);
			for (int i = 0; i < boardSize.height; ++i)
			{
				for (int j = 0; j < boardSize.width; ++j)
				{
					objectPoints[0].push_back(cv::Point3f(j * boardSquareSize, i * boardSquareSize, 0));
				}
			}

			std::cout << "Finding calibration frames..." << std::endl;

			// We determine the maximum number of frames needed for calibration to be 50, might be more
			const int max = 30;

			cv::Size size;

			// Store chessboard images :-)
			std::vector<cv::Mat> results;

			cv::TermCriteria m = cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, std::numeric_limits<int>::max(), DBL_EPSILON);

			int n = 0;
			cv::Mat frame;
			while (cap.read(frame))
			{
				if (n++ % 2 == 0)
				{
					continue;
				}

				size = frame.size();

				// Convert the frame to grayscale
				cv::Mat viewGray;
				cv::cvtColor(frame, viewGray, cv::COLOR_BGR2GRAY);

				// Try finding checkerboard corners, found will indicate whether or not the checkboard pattern was found
				bool found = cv::findChessboardCorners(
					viewGray, 
					boardSize, 
					resultPointsBuffer, 
					CV_CALIB_CB_FAST_CHECK | CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE | CV_CALIB_CB_FILTER_QUADS
				);

				// In case we've found the pattern we want to locally improve the found solution and store the found corners in imagePoints, which will later be used
				// to determine the camera's intrinsics
				if (found)
				{
					cv::cornerSubPix(viewGray, resultPointsBuffer, cv::Size(11, 11), cv::Size(-1, -1), m);

					// Show the found pattern for a short while
					cv::drawChessboardCorners(frame, boardSize, resultPointsBuffer, found);
					cv::imshow("Calibration", frame);
					cv::waitKey(10);

					results.push_back(frame);
					imagePoints.push_back(resultPointsBuffer);

					int s = (int) imagePoints.size();
					std::cout << s << "/" << max << std::endl;
					if (s >= max)
					{
						break;
					}
				}

				cap.set(CV_CAP_PROP_POS_FRAMES, rand() % numFrames);
			}
			cv::destroyWindow("Calibration");

			// Fill objectPoints to the size of imagePoints
			objectPoints.resize(imagePoints.size(), objectPoints[0]);

#if REPROJECT_OPTIMIZATION
			int reprojectOptimizeIteration = 0;
#endif

calibrate:
			// Start the actual calibration
			std::cout << "Start calibration..." << std::endl;
			std::vector<cv::Mat> rvecs, tvecs;
			double rms = cv::calibrateCamera(
				objectPoints, 
				imagePoints, 
				size, 
				K, 
				D, 
				rvecs, 
				tvecs,
				0,
				m
			);
			std::cout << "Done with calibration!" << std::endl;

			float avgProjectionError;
			std::vector<float> reprojectionErrors;
			std::cout << "Average reprojection errors: " << (avgProjectionError = Camera::ComputeReprojectionErrors(objectPoints, imagePoints, rvecs, tvecs, K, D, reprojectionErrors)) << std::endl;

#if REPROJECT_OPTIMIZATION
			std::vector<int> remove;

			// Determine which frames are belong the average projection error, remove this rubbish from the image points set and
			// recalibrate the camera
			int i = 0;
			n = 0;
			std::vector<float>::const_iterator it;
			std::vector<cv::Mat>::const_iterator rit;
			for (it = reprojectionErrors.begin(), rit = results.begin() ; it != reprojectionErrors.end() ; ++it, ++rit, ++i)
			{
#if 1
				std::stringstream ss;
				ss << "Error: " << *it;

				cv::Mat result;
				(*rit).copyTo(result);
				cv::putText(result, ss.str(), cvPoint(30, 30), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200, 200, 250), 1, CV_AA);

				cv::imshow("Result", result);
				cv::waitKey(100);
#endif

				if (*it > TARGET_AVG_PROJECTION_ERROR)
				{
					remove.push_back(i);

					++n;
				}
			}

			std::sort(remove.begin(), remove.end(), std::greater<int>());
			std::vector<int>::iterator removeIt;
			for (removeIt = remove.begin() ; removeIt != remove.end() ; ++removeIt)
			{
				imagePoints.erase(imagePoints.begin() + *removeIt);
				objectPoints.erase(objectPoints.begin() + *removeIt);
				reprojectionErrors.erase(reprojectionErrors.begin() + *removeIt);
				results.erase(results.begin() + *removeIt);
			}

			if (avgProjectionError > TARGET_AVG_PROJECTION_ERROR)
			{
				std::cout << "Removed " << n << " crappy captures in optimization" << std::endl;
				std::cout << "Captures left: " << imagePoints.size() << std::endl;
				std::cout << "Re-calibrating!" << std::endl;

				++reprojectOptimizeIteration;

				//goto calibrate;
			}

#if DISPLAY_REPROJECTION_OPTIMIZATION_RESULT
			for (rit = results.begin() ; rit != results.end() ; ++rit)
			{
				cv::imshow("Result", *rit);
				cv::waitKey(1000);
			}
#endif
#endif
		}
		cap.release();

		cv::destroyAllWindows();

		// Store the camera matrix and distortion coefficient matrix
		cv::FileStorage fs;
		fs.open(intrinsicsFile, cv::FileStorage::WRITE);
		if (fs.isOpened())
		{
			fs << "CameraMatrix" << K;
			fs << "DistortionCoeffs" << D;

			fs.release();
		}
	}

	return true;
}

cv::Mat Camera::GetConcatenateForegroundFrame(void)
{
	// We want to concatenate the actual frame with the actual foreground, such that we have a resulting canvas with
	// the frame to the left and the foreground to the right
	cv::Mat hostFrame;
	this->m_Frame.download(hostFrame);

	cv::Mat hostForeground;
	this->m_ForegroundImage.download(hostForeground);

	cv::Mat canvas;

	cv::Mat hostForegroundBgr;
	cv::cvtColor(hostForeground, hostForegroundBgr, CV_GRAY2BGR);
	cv::hconcat(hostFrame, hostForegroundBgr, canvas);

	return canvas;
}

bool Camera::ComputeExtrinsics(void)
{
	// Since our storage for corners is static (since this method is static) we want to make sure the vector is empty
	// before we start
	Camera::s_Corners.clear();

	int boardWidth = 0, boardHeight = 0;
	float boardSquareSize = 0;
	Common::ReadCheckerboardProperties(this->m_CameraPath, boardWidth, boardHeight, boardSquareSize);

	const cv::Size boardSize(boardWidth, boardHeight);
	const float sideLength = boardSquareSize;

	// Read the camera matrix and distortion coefficients
	cv::FileStorage fs;
	cv::Mat cameraMatrix, distortionCoefficients;
	fs.open(this->m_CameraPath + Common::IntrinsicsConfigurationFile, cv::FileStorage::READ);
	if (fs.isOpened())
	{
		cv::Mat k, d;
		fs["CameraMatrix"] >> k;
		fs["DistortionCoeffs"] >> d;

		k.convertTo(cameraMatrix, CV_32F);
		d.convertTo(distortionCoefficients, CV_32F);
		fs.release();
	}
	else
	{
		std::cerr << "Unable to read camera intrinsics from: " << this->m_CameraPath << Common::IntrinsicsConfigurationFile << std::endl;
		return false;
	}

	// Open the video file
	cv::VideoCapture cap(this->m_CameraPath + Common::CheckerboardVideoFile);
	{
		if (!cap.isOpened())
		{
			std::cerr << "Unable to open: " << this->m_CameraPath + Common::CheckerboardVideoFile << std::endl;
			if (boost::filesystem::exists(this->m_CameraPath + Common::ConfigFile))
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		// Read the first frame since we want to show our origin later
		cv::Mat frame;
		cap >> frame;

		// In case we already have saved the corners read them
		std::string cornersFile = this->m_CameraPath + Common::CheckerboadCornersFile;
		if (boost::filesystem::exists(cornersFile))
		{
			cv::FileStorage fs;
			fs.open(cornersFile, cv::FileStorage::READ);
			if (fs.isOpened())
			{
				int corners_amount;
				fs["CornersAmount"] >> corners_amount;

				for (int b = 0; b < corners_amount; ++b)
				{
					std::stringstream corner_id;
					corner_id << "Corner_" << b;

					std::vector<int> corner;
					fs[corner_id.str()] >> corner;
					assert(corner.size() == 2);
					Camera::s_Corners.push_back(cv::Point2f(corner[0], corner[1]));
				}

				assert((int)Camera::s_Corners.size() == boardSize.area());

				fs.release();
			}
		}
		else
		{
			// Find a suitable frame
			int n = 0;
			while (true)
			{
				cap >> frame;

				// Check if we can find board_size.area() corners
				bool found = cv::findChessboardCorners(frame, boardSize, Camera::s_Corners);
				cv::drawChessboardCorners(frame, boardSize, Camera::s_Corners, found);

				// Draw the result
				cv::imshow("Corners Validation", frame);
				cv::waitKey(500);

				if (found)
				{
					// Convert the frame to grayscale
					cv::Mat viewGray;
					cv::cvtColor(frame, viewGray, cv::COLOR_BGR2GRAY);

					cv::cornerSubPix(viewGray, Camera::s_Corners, cv::Size(11, 11), cv::Size(-1, -1), cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));

					// Done!
					if (Camera::s_Corners.size() == boardSize.area())
					{
						break;
					}
				}
			}

			// Store the corners
			cv::FileStorage fs;
			fs.open(cornersFile, cv::FileStorage::WRITE);
			if (fs.isOpened())
			{
				fs << "CornersAmount" << (int)Camera::s_Corners.size();
				for (size_t b = 0; b < Camera::s_Corners.size(); ++b)
				{
					std::stringstream corner_id;
					corner_id << "Corner_" << b;
					fs << corner_id.str() << Camera::s_Corners.at(b);
				}
				fs.release();
			}
		}

		std::vector<cv::Point3f> objectPoints;
		std::vector<cv::Point2f> imagePoints;

		// Save the object points and the image points as we found them above, we need these to determine our extrinsics (R|T)
		// by PNP
		for (int s = 0; s < boardSize.area(); ++s)
		{
			float x = float(s / boardSize.width * sideLength);
			float y = float(s % boardSize.width * sideLength);
			float z = 0;

			objectPoints.push_back(cv::Point3f(x, y, z));
			imagePoints.push_back(Camera::s_Corners.at(s));
		}

		// Solve PNP ransac
		cv::Mat rotation, translation;
		cv::solvePnP(objectPoints, imagePoints, cameraMatrix, distortionCoefficients, rotation, translation);

		cv::Mat rotation_values, translation_values;
		rotation.convertTo(rotation_values, CV_32F);
		translation.convertTo(translation_values, CV_32F);

		// Render the origin for validation
		{
			cv::Mat canvas = frame.clone();

			const float xL = float(sideLength * (boardSize.height - 1));
			const float yL = float(sideLength * (boardSize.width - 1));
			const float zL = float(sideLength * 3);

			cv::Point o = Camera::Project(cv::Point3f(0, 0, 0), rotation, translation, cameraMatrix, distortionCoefficients);
			cv::Point x = Camera::Project(cv::Point3f(xL, 0, 0), rotation, translation, cameraMatrix, distortionCoefficients);
			cv::Point y = Camera::Project(cv::Point3f(0, yL, 0), rotation, translation, cameraMatrix, distortionCoefficients);
			cv::Point z = Camera::Project(cv::Point3f(0, 0, zL), rotation, translation, cameraMatrix, distortionCoefficients);

			line(canvas, o, x, Color_BLUE, 2, CV_AA);
			line(canvas, o, y, Color_GREEN, 2, CV_AA);
			line(canvas, o, z, Color_RED, 2, CV_AA);
			circle(canvas, o, 3, Color_YELLOW, -1, CV_AA);

			/*cv::namedWindow("Origin", CV_WINDOW_NORMAL);
			imshow("Origin", canvas);
			cv::waitKey(500);*/
		}

		// Store in- and extrinsics
		fs.open(this->m_CameraPath + Common::ConfigFile, cv::FileStorage::WRITE);
		{
			if (fs.isOpened())
			{
				fs << "CameraMatrix" << cameraMatrix;
				fs << "DistortionCoeffs" << distortionCoefficients;
				fs << "RotationValues" << rotation_values;
				fs << "TranslationValues" << translation_values;
			}
			else
			{
				std::cerr << "Unable to write camera intrinsics+extrinsics to: " << this->m_CameraPath << Common::ConfigFile << std::endl;
				return false;
			}
		}
		fs.release();
	}
	cap.release();

	return true;
}

void Camera::InitializeCameraLocation(void)
{
	// Member rotation is a (3x1) rotation vector, Rodrigues converts this vector into a
	// conventional rotation matrix
	cv::Mat r;
	cv::Rodrigues(this->m_Rotation, r);

	cv::Mat rotation = cv::Mat::zeros(4, 4, CV_32F);
	*rotation.ptr<float>(3, 3) = 1.0;

	cv::Mat r_sub = rotation(cv::Rect(0, 0, 3, 3));
	r.copyTo(r_sub);

	// The translation vector we got as a result from PNP is in form (tx, ty, tz), we can represent
	// the translation as a conventional translation matrix:
	// | 1  0  0 0|
	// | 0  1  0 0|
	// | 0  0  1 0|
	// |tx ty tz 1|
	// Since the translation vector represent the origin of the checkerboard in camera space, we invert the vector to
	// find the origin of the camera. When multiplying R * T we get the camera matrix, since this matrix already accounts
	// for rotation (since we multiplied), we 
	cv::Mat translation = cv::Mat::eye(4, 4, CV_32F);
	*translation.ptr<float>(3, 0) = (*this->m_Translation.ptr<float>(0, 0)) * -1.0f;
	*translation.ptr<float>(3, 1) = (*this->m_Translation.ptr<float>(1, 0)) * -1.0f;
	*translation.ptr<float>(3, 2) = (*this->m_Translation.ptr<float>(2, 0)) * -1.0f;

	// RT_4,1, R_4,2, R_4,3 now respectively have the translation over rotation
	cv::Mat cameraMatrix = translation * rotation;
	this->m_CameraLocation = cv::Point3f(*cameraMatrix.ptr<float>(3, 0), *cameraMatrix.ptr<float>(3, 1), *cameraMatrix.ptr<float>(3, 2));

	std::cout << "Camera " << m_Id + 1 << " " << this->m_CameraLocation << std::endl;

	// Store the rotation
	this->m_Rt = rotation;

	// Store the inverse rotation
	cv::invert(this->m_Rt, this->m_iRt);
}

void Camera::DefineFrustumPoints(void)
{
	this->m_Frustum.clear();
	this->m_Frustum.push_back(this->m_CameraLocation);

	// Calculate the fustrum of the camera according to:
	//
	// -px,-py______px*2,py*2
	//    |             |
	//    |             |
	//    |      p      |
	//    |             |
	// -px,2*py_____2*px,2*py
	//
	// Define z to be half the focal length

	// Upper left
	cv::Point3f ul = Camera3dToWorld(cv::Point3f(-this->m_Px, -this->m_Py, (this->m_Fx + this->m_Fy) * this->m_FrustumDistance));
	this->m_Frustum.push_back(ul);

	// Upper right
	cv::Point3f ur = Camera3dToWorld(cv::Point3f(this->m_Px * 2, -this->m_Py, (this->m_Fx + this->m_Fy) * this->m_FrustumDistance));
	this->m_Frustum.push_back(ur);

	// Lower right
	cv::Point3f lr = Camera3dToWorld(cv::Point3f(this->m_Px * 2, this->m_Py * 2, (this->m_Fx + this->m_Fy) * this->m_FrustumDistance));
	this->m_Frustum.push_back(lr);

	// Lower left
	cv::Point3f ll = Camera3dToWorld(cv::Point3f(-this->m_Px, this->m_Py * 2, (this->m_Fx + this->m_Fy) * this->m_FrustumDistance));
	this->m_Frustum.push_back(ll);

	// Principal point p
	cv::Point3f p = Camera3dToWorld(cv::Point3f(this->m_Px, this->m_Py, (this->m_Fx + this->m_Fy) * this->m_FrustumDistance));
	this->m_Frustum.push_back(p);
}

cv::Point3f Camera::CameraSpaceToWorld(const cv::Point &point)
{
	// Translate a 2d camera point to a 3d camera point by offsetting with the camera principal point and z half focal length
	return this->Camera3dToWorld(cv::Point3f(float(point.x - this->m_Px), float(point.y - this->m_Py), (this->m_Fx + this->m_Fy) * this->m_FrustumDistance));
}

cv::Point3f Camera::Camera3dToWorld(const cv::Point3f &cameraPoint)
{
	// cameraPoint is a point in camera space, since RT describes the rotation and translation (extrinsics) from
	// world space to camera space, its inverse describes the transofrmation from camera space to world space.
	// Multiplying the camera point vector (in homogeneous coordinates) we thus find the coordinates in world space
	cv::Mat Xc(4, 1, CV_32F);
	*Xc.ptr<float>(0, 0) = cameraPoint.x;
	*Xc.ptr<float>(1, 0) = cameraPoint.y;
	*Xc.ptr<float>(2, 0) = cameraPoint.z;
	*Xc.ptr<float>(3, 0) = 1;

	// Transform
	cv::Mat Xw = this->m_iRt * Xc;

	return cv::Point3f(*Xw.ptr<float>(0, 0), *Xw.ptr<float>(1, 0), *Xw.ptr<float>(2, 0));
}

cv::Point Camera::Project(const cv::Point3f &coords, const cv::Mat &rotation, const cv::Mat &translation, const cv::Mat &cameraMatrix, const cv::Mat &distortionCoefficients)
{
	// By using the camera matrix, RT and distortion coefficients, we can perform a projection of the coordinates (coords)
	// onto the camera in 2d camera space, this method is also implemented in CUDA for the reconstruction process
	std::vector<cv::Point3f> objectPoints;
	objectPoints.push_back(coords);

	std::vector<cv::Point2f> imagePoints;
	cv::projectPoints(objectPoints, rotation, translation, cameraMatrix, distortionCoefficients, imagePoints);

	cv::Point pointone = imagePoints.front();

	cv::Point pointtwo;
	Common::ProjectPoints(coords, rotation, translation, cameraMatrix, distortionCoefficients, pointtwo);

	return pointone;
}

cv::Point Camera::Project(const cv::Point3f &coords)
{
	return this->Project(coords, m_Rotation, m_Translation, m_CameraMatrix, m_DistortionCoeffs);
}