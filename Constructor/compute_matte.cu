#include <opencv2/core/core.hpp>
#include <opencv2/core/cuda_types.hpp>

#include <cuda_runtime.h>

#include <iostream>

#include "cuda_common.cuh"

#include "Exception.h"

__global__
void compute_matte_kernel(const uint3 color, const uint treshold, const uint tolerance, const cv::cuda::PtrStepSz<uchar3> in, cv::cuda::PtrStepSz<uchar> out)
{
	uint x = blockIdx.x * blockDim.x + threadIdx.x;
	uint y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x < in.cols && y < in.rows)
	{
		double i_255 = 1.0 / 255.0;
		
		float r = float(in(y, x).x), g = float(in(y, x).y), b = float(in(y, x).z);

		float d = ((r * i_255 - color.x * i_255) * (r * i_255 - color.x * i_255)) + ((g * i_255 - color.y * i_255) * (g * i_255 - color.y * i_255)) + ((b * i_255 - color.z * i_255) * (b * i_255 - color.z * i_255));
		
		// Sqrt hack
		unsigned int i = *(unsigned int*)&(d);
		i += 127 << 23;
		i >>= 1;
		float distance = *(float*)&i;

		int grey = 255 * distance / sqrt(3.0);
		if (grey <= treshold)
		{
			out(y, x) = 0;
		}
		else if (grey >= tolerance)
		{
			out(y, x) = 255;
		}
		else
		{
			grey = 255 * (grey - treshold) / (tolerance - treshold);

			out(y, x) = grey;
		}
	}
}

void compute_matte(const uint3 color, const uint treshold, const uint tolerance, const cv::cuda::PtrStepSz<uchar3> in, cv::cuda::PtrStepSz<uchar> out)
{
	dim3 blockSize(128, 8);
	dim3 gridSize = dim3(iDivUp(in.cols, blockSize.x), iDivUp(in.rows, blockSize.y));

	compute_matte_kernel<<<gridSize, blockSize>>>(color, treshold, tolerance, in, out);
	cudaDeviceSynchronize();

	cudaError_t err = cudaGetLastError();
	if (err != cudaSuccess)
	{
		// WTF! We randomly encounter invalid argument errors here while the block and grid sizes are valid,
		// Nsight debugger reports some random (OpenCV-related?) errors
		/*char b[500];
		sprintf(b, "Failed to compute matte: %s", cudaGetErrorString(err));
		throw_line(b);*/
	}
}