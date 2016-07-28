#pragma once

class DistanceKeyer
{
private:
	cv::Vec3f m_KeyColor;

	int m_Treshold, m_Tolerance;

	int m_GarbageTreshold;

	bool m_HasKeyColor;
public:
	DistanceKeyer();
	~DistanceKeyer();

	void SetTreshold(const int treshold) { this->m_Treshold = treshold; }
	void SetTolerance(const int tolerance) { this->m_Tolerance = tolerance; };

	int GetTreshold(void) { return this->m_Treshold; }
	int GetGarbageTreshold(void) { return this->m_GarbageTreshold; }
	int GetTolerance(void) { return this->m_Tolerance; }

	bool HasKeyColor(void) { return this->m_HasKeyColor; }

	const int GetR(void) { return this->m_KeyColor[0]; }
	const int GetG(void) { return this->m_KeyColor[1]; }
	const int GetB(void) { return this->m_KeyColor[2]; }

	void SetR(const int r) { this->m_KeyColor[0] = r; }
	void SetG(const int g) { this->m_KeyColor[1] = g; }
	void SetB(const int b) { this->m_KeyColor[2] = b; }

	void SetGarbageTreshold(const int garbageTreshold) { this->m_GarbageTreshold = garbageTreshold; }

	cv::Vec3f GetKeyColor() { return this->m_KeyColor; }

	void FindKeyColor(const cv::cuda::GpuMat &in);

	void ComputeMatte(const cv::cuda::GpuMat &in, cv::cuda::GpuMat &out);
};