#pragma once

#include "Settings.h"

#define REPROJECT_OPTIMIZATION 1
#define DISPLAY_REPROJECTION_OPTIMIZATION_RESULT 1
#define TARGET_AVG_PROJECTION_ERROR 0.25

class Camera
{
	bool m_Initialized;

	Settings &m_Settings;

	const int m_Id;

	float m_FrustumDistance;

	const std::string m_CameraPath;

	cv::cuda::GpuMat m_ForegroundImage;

	cv::VideoCapture m_Video;
	cv::VideoCapture m_MatteVideo;

	cv::Size m_FrustumSize;

	long m_Frames;

	cv::Mat m_CameraMatrix, m_DistortionCoeffs;
	cv::Mat m_Rotation, m_Translation;

	float m_Fx, m_Fy, m_Px, m_Py;

	cv::Mat m_Rt;
	cv::Mat m_iRt;

	cv::Point3f m_CameraLocation;

	std::vector<cv::Point3f> m_Frustum;
	std::vector<cv::Point3f> m_CameraFloor;

	cv::cuda::GpuMat m_Frame;

	std::vector<cv::Point2f> s_Corners;

	void InitializeCameraLocation(void);

	void DefineFrustumPoints(void);

	cv::Point3f CameraSpaceToWorld(const cv::Point &);

	cv::Point3f Camera3dToWorld(const cv::Point3f &);
public:
	Camera(Settings &settings, std::string cameraPath, const int id);
	~Camera(void);

	bool Initialize(void);

	cv::Point Project(const cv::Point3f &coords);

	cv::cuda::GpuMat &NextVideoFrame(void);
	
	bool IsInitialized(void) const
	{
		return this->m_Initialized;
	}

	static double ComputeReprojectionErrors(const std::vector<std::vector<cv::Point3f>> &objectPoints, const std::vector<std::vector<cv::Point2f>> &imagePoints, const std::vector<cv::Mat> &rvecs, const std::vector<cv::Mat> &tvecs, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs, std::vector<float> &perViewErrors);

	bool Calibrate(void);
	bool ComputeExtrinsics(void);

	static cv::Point Project(const cv::Point3f &, const cv::Mat &, const cv::Mat &, const cv::Mat &, const cv::Mat &);
	
	cv::cuda::GpuMat GetVideoFrame(int);

	cv::Mat GetConcatenateForegroundFrame(void);

	void LoadForegroundFromMatteVideo(void);
	void LoadForegroundFromMatteStill(void);

	const std::string GetCameraPath(void)
	{
		return this->m_CameraPath;
	}

	const int GetId(void) const
	{
		return this->m_Id;
	}

	cv::Mat GetCameraMatrix(void)
	{
		return this->m_CameraMatrix;
	}

	cv::Mat GetDistortionCoefficients(void)
	{
		return this->m_DistortionCoeffs;
	}

	cv::Mat GetRotation(void)
	{
		return this->m_Rotation;
	}

	cv::Mat GetTranslation(void)
	{
		return this->m_Translation;
	}

	const cv::VideoCapture &GetVideo(void)
	{
		return this->m_Video;
	}

	long GetFrames(void)
	{
		return this->m_Frames;
	}

	const cv::Size &GetFrustumSize(void)
	{
		return this->m_FrustumSize;
	}

	const cv::cuda::GpuMat &GetForegroundImage(void)
	{
		return this->m_ForegroundImage;
	}

	const cv::cuda::GpuMat &GetFrame(void)
	{
		return this->m_Frame;
	}

	const std::vector<cv::Point3f> &GetCameraFloor(void)
	{
		return this->m_CameraFloor;
	}

	const cv::Point3f &GetCameraLocation(void)
	{
		return this->m_CameraLocation;
	}

	const std::vector<cv::Point3f> &GetCameraPlane(void)
	{
		return this->m_Frustum;
	}

	void SetVideoFrame(int);

	void SetVideo(const cv::VideoCapture &video)
	{
		this->m_Video = video;
	}

	void SetForegroundImage(const cv::cuda::GpuMat &foregroundImage)
	{
		this->m_ForegroundImage = foregroundImage;
	}
};
