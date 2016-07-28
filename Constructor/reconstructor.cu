#include <opencv2/core/core.hpp>
#include <opencv2/core/cuda_types.hpp>
#include <opencv2/cudalegacy.hpp>
#include "opencv2/core/cuda/common.hpp"
#include "opencv2/core/cuda/transform.hpp"
#include "opencv2/core/cuda/functional.hpp"
#include "opencv2/core/cuda/reduce.hpp"

#include <cuda_runtime.h>

#include <iostream>
#include <chrono>

#include "VisibleVoxel.h"
#include "cuda_common.cuh"
#include "reconstructor.cuh"

#include "Exception.h"

#define CHECK_ERROR(a) if ((a) != cudaSuccess) { goto error; }

#define DIV 2

static VisibleVoxel *sd_visible_voxel_storage;

static unsigned int sh_num_cameras;

static unsigned int sh_width;
static unsigned int sh_height;
static unsigned int sh_depth;

static unsigned int sh_step;

static int sh_x_l;
static int sh_y_l;
static int sh_z_l;

static unsigned int sh_frustum_width;
static unsigned int sh_frustum_height;

float *sd_r = 0, *sd_t = 0, *sd_a = 0, *sd_k = 0;

static bool s_IsInitialized = false;

__device__ short2 project_points_for_camera_kernel(
	float3 point,
	float *R, 
	float *t,
	float *a,
	float *k,
	int camIdx
	)
{
	float fx, fy, cx, cy;

	fx = a[0]; fy = a[4];
	cx = a[2]; cy = a[5];

	float X = point.x, Y = point.y, Z = point.z;
	float x = R[0] * X + R[1] * Y + R[2] * Z + t[0];
	float y = R[3] * X + R[4] * Y + R[5] * Z + t[1];
	float z = R[6] * X + R[7] * Y + R[8] * Z + t[2];
	float r2, r4, r6, a1, a2, a3, cdist, icdist2;
	float xd, yd;

	z = z ? 1.0f / z : 1;
	x *= z; y *= z;

	r2 = x * x + y * y;
	r4 = r2 * r2;
	r6 = r4 * r2;
	a1 = 2 * x*y;
	a2 = r2 + 2 * x * x;
	a3 = r2 + 2 * y * y;
	cdist = 1 + k[0] * r2 + k[1] * r4 + k[4] * r6;

	float k5 = k[5], k6 = k[6], k7 = k[7];
	icdist2 = 1.0f / (1.0f + k5 * r2 + k6 * r4 + k7 * r6);
	xd = x * cdist * icdist2 + k[2] * a1 + k[3] * a2 + k[8] * r2 + k[9] * r4;
	yd = y * cdist * icdist2 + k[2] * a3 + k[3] * a1 + k[10] * r2 + k[11] * r4;

	return make_short2(__float2int_ru(xd * fx + cx), __float2int_ru(yd * fy + cy));
}

__global__
void update_voxels_kernel(
	VisibleVoxel					  *visible_voxel_storage, //
	const cv::cuda::PtrStepSz<uchar>  foregrounds[], 		 // Array of foreground images from cameras
	const cv::cuda::PtrStepSz<uchar3> frames[], 		     // Array of frames from cameras
	float							  *r,
	float							  *t,
	float							  *a,
	float							  *k,
	const unsigned int				  num_cameras,			 // Number of cameras
	const unsigned int				  width,
	const unsigned int                height,
	const unsigned int                depth,
	const int						  x_l,
	const int						  y_l,
	const int						  z_l,
	const unsigned int				  frustum_width,
	const unsigned int				  frustum_height,
	const unsigned int                step,
	unsigned long long int  	      *voxel_pointer,
	const unsigned int				  m_x,
	const unsigned int				  m_y,
	const unsigned int				  m_z,
	const unsigned int				  part
	)
{
	const unsigned int xIdx = (m_x * (width / part)) + blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int yIdx = (m_y * (height / part)) + blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int zIdx = (m_z * (depth / part)) + blockIdx.z * blockDim.z + threadIdx.z;

	const int x = x_l + xIdx * step;
	const int y = y_l + yIdx * step;
	const int z = z_l + zIdx * step;

	int t_r, t_g, t_b;
	t_r = t_g = t_b = 0;

	int v = 0;
	for (int i = 0 ; i < num_cameras ; ++i)
	{
		float3 p;
		p.x = x;
		p.y = y;
		p.z = z;

		float R[9], T[3], A[9], K[12];
		memcpy(R, r + (i * 9), sizeof(float) * 9);
		memcpy(T, t + (i * 3), sizeof(float) * 3);
		memcpy(A, a + (i * 9), sizeof(float) * 9);
		memcpy(K, k + (i * 12), sizeof(float) * 12);

		short2 point = project_points_for_camera_kernel(p, R, T, A, K, i);
		if ((point.x >= 0 && point.x < frustum_width && point.y >= 0 && point.y < frustum_height))
		{
			// Has white pixel in matte?
			uchar pixel = foregrounds[i](point.y, point.x);
			if (pixel == 255)
			{
				++v;

				t_r += frames[i](point.y, point.x).x;
				t_g += frames[i](point.y, point.x).y;
				t_b += frames[i](point.y, point.x).z;
			}
		}
	}

	if (v >= num_cameras)
	{
		unsigned long long int vIdx = atomicAdd(voxel_pointer, 1);

		// Push the voxel into the set of visible voxels
		visible_voxel_storage[vIdx].X = x;
		visible_voxel_storage[vIdx].Y = y;
		visible_voxel_storage[vIdx].Z = z;

		visible_voxel_storage[vIdx].R = t_r / v;
		visible_voxel_storage[vIdx].G = t_g / v;
		visible_voxel_storage[vIdx].B = t_b / v;
	}
}

bool update_voxels(
	const cv::cuda::GpuMat *h_gputmat_foregrounds,
	const cv::cuda::GpuMat *h_gputmat_frames,
	unsigned long long int *h_num_voxels,
	VisibleVoxel		   **h_visible_voxels
	)
{
	cv::cuda::PtrStepSz<uchar> *h_foregrounds = new cv::cuda::PtrStepSz<uchar>[sh_num_cameras];
	cv::cuda::PtrStepSz<uchar3> *h_frames = new cv::cuda::PtrStepSz<uchar3>[sh_num_cameras];
	for (int i = 0 ; i < sh_num_cameras ; ++i)
	{
		h_foregrounds[i] = h_gputmat_foregrounds[i];
		h_frames[i] = h_gputmat_frames[i];
	}

	unsigned long long int h_voxel_pointer, *d_voxel_pointer;
	cudaMalloc((void**)&d_voxel_pointer, sizeof(int));
	h_voxel_pointer = 0;
	cudaMemcpy(d_voxel_pointer, &h_voxel_pointer, sizeof(unsigned long long int), cudaMemcpyHostToDevice);

	cv::cuda::PtrStepSz<uchar> *d_foregrounds = 0;
	CHECK_ERROR(cudaMalloc((void**)&d_foregrounds, sizeof(cv::cuda::PtrStepSz<uchar>) * sh_num_cameras));
	CHECK_ERROR(cudaMemcpy(d_foregrounds, h_foregrounds, sizeof(cv::cuda::PtrStepSz<uchar>) * sh_num_cameras, cudaMemcpyHostToDevice));

	cv::cuda::PtrStepSz<uchar3> *d_frames = 0;
	CHECK_ERROR(cudaMalloc((void**)&d_frames, sizeof(cv::cuda::PtrStepSz<uchar3>) * sh_num_cameras));
	CHECK_ERROR(cudaMemcpy(d_frames, h_frames, sizeof(cv::cuda::PtrStepSz<uchar3>) * sh_num_cameras, cudaMemcpyHostToDevice));

	*h_visible_voxels = NULL;

	// Divide the voxel space into equal divisions to reducs vram usage
	long total_voxels = 0;
	for (int x = 0 ; x < DIV ; ++x)
	{
		for (int y = 0 ; y < DIV ; ++y)
		{
			for (int z = 0 ; z < DIV ; ++z)
			{
				dim3 block_size(16, 8, 8);
				dim3 grid_size = dim3(iDivUp(sh_width / DIV, block_size.x), iDivUp(sh_height / DIV, block_size.y), iDivUp(sh_depth / DIV, block_size.z));
				update_voxels_kernel <<<grid_size, block_size>>>(
					sd_visible_voxel_storage,
					d_foregrounds,
					d_frames,
					sd_r,
					sd_t,
					sd_a,
					sd_k,
					sh_num_cameras,
					sh_width,
					sh_height,
					sh_depth,
					sh_x_l,
					sh_y_l,
					sh_z_l,
					sh_frustum_width,
					sh_frustum_height,
					sh_step,
					d_voxel_pointer,
					x,
					y,
					z,
					DIV
				);

				if (cudaDeviceSynchronize() != cudaSuccess)
				{
					std::cout << "Failed to initialize voxels..." << std::endl;
					goto error;
				}

				// Fetch number of visible voxels from kernel
				cudaMemcpy(&h_voxel_pointer, d_voxel_pointer, sizeof(int), cudaMemcpyDeviceToHost);

				// Create memory to store the visible voxels
				*h_visible_voxels = (VisibleVoxel*) realloc(*h_visible_voxels, sizeof(VisibleVoxel) * (h_voxel_pointer + total_voxels));

				//std::cout << "Reallocating " << (sizeof(VisibleVoxel) * (h_voxel_pointer + total_voxels)) / 1000000 << " MB" << std::endl; 

				// Create memory and download visible voxels, store with offset
				cudaMemcpy(*h_visible_voxels + total_voxels, sd_visible_voxel_storage, sizeof(VisibleVoxel) * h_voxel_pointer, cudaMemcpyDeviceToHost);

				total_voxels += h_voxel_pointer;
			}
		}
	}

	*h_num_voxels = total_voxels;

	// House keeping
	delete[] h_foregrounds;
	delete[] h_frames;

	cudaFree(d_frames);
	cudaFree(d_foregrounds);
	cudaFree(d_voxel_pointer);

	return EXIT_SUCCESS;
error:
	cudaError_t err = cudaGetLastError();

	char b[500];
	sprintf(b, "Failed to update voxels: %s", cudaGetErrorString(err));
	throw_line(b);

	return EXIT_FAILURE;
}

bool initialize_voxels(
	float			       *h_r,
	float                  *h_t,
	float				   *h_a,
	float				   *h_k,
	const unsigned int	   num_cameras,
	const int			   x_l,
	const int			   x_r,
	const int			   y_l,
	const int              y_r, 
	const int              z_l,
	const int              z_r,
	const unsigned int     step,
	const unsigned int     frustum_width,
	const unsigned int     frustum_height,
	int					   *total_voxels
	)
{
	// Compute the dimensions of the voxel space
	const unsigned int width = x_r - x_l;
	const unsigned int height = y_r - y_l;
	const unsigned int depth = z_r - z_l;

	// Compute the storage dimensions of the voxel space
	const unsigned int voxel_space_width = width / step;
	const unsigned int voxel_space_height = height / step;
	const unsigned int voxel_space_depth = depth / step;

	sh_width = voxel_space_width;
	sh_height = voxel_space_height;
	sh_depth = voxel_space_depth;

	sh_frustum_width = frustum_width;
	sh_frustum_height = frustum_height;

	sh_step = step;

	sh_x_l = x_l;
	sh_y_l = y_l;
	sh_z_l = z_l;

	unsigned long long int num_voxels = voxel_space_width;
	num_voxels *= voxel_space_height;
	num_voxels *= voxel_space_depth;
	num_voxels /= pow(DIV, 3);

	*total_voxels = num_voxels;

	std::cout << "Number of voxels per CUDA kernel: " << num_voxels << std::endl;
	std::cout << "Total number of voxels: " << num_voxels * pow(DIV, 3) << std::endl;

	sh_num_cameras = num_cameras;

	// Since when we're destroying we're deallocating memory of voxel storage, we state that we're intitialized at this point
	s_IsInitialized = true;

	// Create storage for visible voxels (device)
	std::cout << "Allocating " << (num_voxels * sizeof(VisibleVoxel)) / 1000000 << " MB of memory for visible voxel storage" << std::endl;
	if (cudaMalloc((void**)&sd_visible_voxel_storage, (num_voxels * sizeof(VisibleVoxel))) != cudaSuccess)
	{
		goto error;
	}

	// Copy host R, T, K and D to device
	cudaMalloc((void**)&sd_r, sizeof(float) * num_cameras * 9);
	cudaMalloc((void**)&sd_t, sizeof(float) * num_cameras * 3);
	cudaMalloc((void**)&sd_a, sizeof(float) * num_cameras * 9);
	cudaMalloc((void**)&sd_k, sizeof(float) * num_cameras * 12);

	// Copy
	cudaMemcpy(sd_r, h_r, sizeof(float) * num_cameras * 9, cudaMemcpyHostToDevice);
	cudaMemcpy(sd_t, h_t, sizeof(float) * num_cameras * 3, cudaMemcpyHostToDevice);
	cudaMemcpy(sd_a, h_a, sizeof(float) * num_cameras * 9, cudaMemcpyHostToDevice);
	cudaMemcpy(sd_k, h_k, sizeof(float) * num_cameras * 12, cudaMemcpyHostToDevice);

	return EXIT_SUCCESS;
error:
	cudaError_t err = cudaGetLastError();

	// Clean up
	destroy_voxels();

	char b[500];
	sprintf(b, "Failed to initialize voxels: %s\nVoxel storage is destroyed", cudaGetErrorString(err));
	throw_line(b);

	return EXIT_FAILURE;
}

bool destroy_voxels(void)
{
	if (!s_IsInitialized)
	{
		std::cout << "Nothing to destroy!" << std::endl;
		return EXIT_FAILURE;
	}

	// Free the voxel storage
	cudaFree(sd_visible_voxel_storage);

	cudaFree(sd_a);
	cudaFree(sd_k);
	cudaFree(sd_t);
	cudaFree(sd_r);

	return EXIT_SUCCESS;
}