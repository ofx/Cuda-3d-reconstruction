#include "Stdafx.h"

#include "Common.h"
#include "Reconstructor.h"

#include "reconstructor.cuh"
#include "VisibleVoxel.h"

Reconstructor::Reconstructor(const std::vector<Camera*> &cs) : m_Cameras(cs)
{
	// Define the size of the frustum
	for (size_t c = 0; c < this->m_Cameras.size(); ++c)
	{
		if (this->m_FrustumSize.area() > 0)
		{
			assert(this->m_FrustumSize.width == this->m_Cameras[c]->GetFrustumSize().width && this->m_FrustumSize.height == this->m_Cameras[c]->GetFrustumSize().height);
		}
		else
		{
			this->m_FrustumSize = this->m_Cameras[c]->GetFrustumSize();
		}
	}

	this->m_VisibleVoxels = 0;
	this->m_NumVisibleVoxels = 0;

	this->m_Step = 1;
	this->m_Size = 128;
}

Reconstructor::~Reconstructor()
{
	// Delete corners
	for (size_t c = 0; c < this->m_Corners.size(); ++c)
	{
		delete this->m_Corners.at(c);
	}

	// Clean up the old set of visible voxels
	if (this->m_VisibleVoxels != 0)
	{
		delete this->m_VisibleVoxels;
		this->m_NumVisibleVoxels = 0;
	}

	destroy_voxels();
}

bool Reconstructor::Initialize(void)
{
	// Define the bounds of the voxel space
	const int halfEdge = this->m_Size * 4;
	const int xL = -halfEdge;
	const int xR = halfEdge;
	const int yL = -halfEdge;
	const int yR = halfEdge;
	const int zL = 0;
	const int zR = halfEdge;

	// Store the corners of the voxel volume
	this->m_Corners.push_back(new cv::Point3f((float)xL, (float)yL, (float)zL));
	this->m_Corners.push_back(new cv::Point3f((float)xL, (float)yR, (float)zL));
	this->m_Corners.push_back(new cv::Point3f((float)xR, (float)yR, (float)zL));
	this->m_Corners.push_back(new cv::Point3f((float)xR, (float)yL, (float)zL));
	this->m_Corners.push_back(new cv::Point3f((float)xL, (float)yL, (float)zR));
	this->m_Corners.push_back(new cv::Point3f((float)xL, (float)yR, (float)zR));
	this->m_Corners.push_back(new cv::Point3f((float)xR, (float)yR, (float)zR));
	this->m_Corners.push_back(new cv::Point3f((float)xR, (float)yL, (float)zR));

	// Create some storage for the Rotation, Translation, cAmera matrix and distortion (c)koefficients
	float *R = new float[this->m_Cameras.size() * 9];
	float *T = new float[this->m_Cameras.size() * 3];
	float *A = new float[this->m_Cameras.size() * 9];
	float *K = new float[this->m_Cameras.size() * 12];

	memset(R, 0, sizeof(float)* this->m_Cameras.size() * 9);
	memset(T, 0, sizeof(float)* this->m_Cameras.size() * 3);
	memset(A, 0, sizeof(float)* this->m_Cameras.size() * 9);
	memset(K, 0, sizeof(float)* this->m_Cameras.size() * 12);

	int i = 0;
	std::vector<Camera*>::const_iterator it;
	for (it = this->m_Cameras.begin() ; it != this->m_Cameras.end() ; ++it)
	{
		cv::Mat r = (*it)->GetRotation();
		cv::Mat t = (*it)->GetTranslation();
		cv::Mat a = (*it)->GetCameraMatrix();
		cv::Mat k = (*it)->GetDistortionCoefficients();

		cv::Mat matR(3, 3, CV_32F, R + (i * 9));
		cv::Rodrigues(r, matR);

		Common::MatToFloatArray(t, T + (i * 3));
		Common::MatToFloatArray(a, A + (i * 9));
		Common::MatToFloatArray(k, K + (i * 12));

		++i;
	}

	bool success = initialize_voxels(R, T, A, K, this->m_Cameras.size(), xL, xR, yL, yR, zL, zR, this->m_Step, this->m_FrustumSize.width, this->m_FrustumSize.height, &this->m_TotalVoxels) == EXIT_SUCCESS;

	delete[] R;
	delete[] T;
	delete[] A;
	delete[] K;

	return success;
}

void Reconstructor::Update()
{
	// Fetch set of foregrounds from cameras
	cv::cuda::GpuMat *foregrounds = new cv::cuda::GpuMat[this->m_Cameras.size()];
	cv::cuda::GpuMat *frames = new cv::cuda::GpuMat[this->m_Cameras.size()];
	int i = 0;
	std::vector<Camera*>::const_iterator it;
	for (it = this->m_Cameras.begin() ; it != this->m_Cameras.end() ; ++it)
	{
		foregrounds[i] = cv::cuda::GpuMat((*it)->GetForegroundImage());
		frames[i] = cv::cuda::GpuMat((*it)->GetFrame());

		/*cv::Mat fg, f;
		foregrounds[i].download(fg);
		frames[i].download(f);

		cv::imshow("FG", fg);
		cv::imshow("F", f);
		cv::waitKey();*/

		++i;
	}

	// Clean up the old set of visible voxels
	if (this->m_VisibleVoxels != 0)
	{
		free(this->m_VisibleVoxels);
		this->m_NumVisibleVoxels = 0;
	}

	// Update voxels, call CUDA kernel
	update_voxels(foregrounds, frames, &this->m_NumVisibleVoxels, &this->m_VisibleVoxels);

	delete[] foregrounds;
	delete[] frames;
}