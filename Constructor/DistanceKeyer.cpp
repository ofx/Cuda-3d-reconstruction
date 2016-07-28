#include "Stdafx.h"

#include "Common.h"
#include "DistanceKeyer.h"

#include "compute_matte.cuh"

DistanceKeyer::DistanceKeyer()
{
	this->m_HasKeyColor = false;
}

DistanceKeyer::~DistanceKeyer()
{
}

void DistanceKeyer::FindKeyColor(const cv::cuda::GpuMat &in)
{
	cv::Mat frame;
	in.download(frame);

	int size = frame.rows * frame.cols;
	int r = 0, g = 0, b = 0;

	for (int i = 0; i < frame.rows; ++i)
	{
		const cv::Vec3b* pixel = frame.ptr<cv::Vec3b>(i);

		for (int j = 0; j < frame.cols; ++j)
		{
			r = r + pixel[j][0];
			g = g + pixel[j][1];
			b = b + pixel[j][2];
		}
	}

	r /= size;
	g /= size;
	b /= size;

	this->m_KeyColor = cv::Vec3f(r, g, b);

	std::cout << "Key color: (" << r << ", " << g << ", " << b << ")" << std::endl;

	this->m_HasKeyColor = true;
}

void DistanceKeyer::ComputeMatte(const cv::cuda::GpuMat &in, cv::cuda::GpuMat &out)
{
	uint3 color;
	color.x = this->m_KeyColor[0];
	color.y = this->m_KeyColor[1];
	color.z = this->m_KeyColor[2];

	/*cv::Ptr<cv::cuda::Filter> filter = cv::cuda::createGaussianFilter(in.type(), in.type(), cv::Size(21, 21), 0);
	filter->apply(in, in);*/

	out.create(in.size(), CV_8UC1);

	compute_matte(color, this->m_Treshold, this->m_Tolerance, in, out);
}