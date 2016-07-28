#pragma once

#include "Camera.h"
#include "VisibleVoxel.h"

class Reconstructor
{
private:
	const std::vector<Camera*> &m_Cameras;

	int m_Step;
	int m_Size;

	int m_TotalVoxels;

	std::vector<cv::Point3f*> m_Corners;

	VisibleVoxel *m_VisibleVoxels;

	unsigned long long int m_NumVisibleVoxels;

	cv::Size m_FrustumSize;
public:
	Reconstructor(const std::vector<Camera*> &cameras);
	virtual ~Reconstructor(void);

	bool Initialize(void);

	void Update(void);

	VisibleVoxel *GetVisibleVoxels(void)
	{
		return this->m_VisibleVoxels;
	};

	unsigned int GetNumVisibleVoxels(void)
	{
		return this->m_NumVisibleVoxels;
	}

	const std::vector<cv::Point3f*> &GetCorners(void) const
	{
		return this->m_Corners;
	}

	int GetSize(void) const
	{
		return this->m_Size;
	}

	const cv::Size &GetFrustumSize(void) const
	{
		return this->m_FrustumSize;
	}
};