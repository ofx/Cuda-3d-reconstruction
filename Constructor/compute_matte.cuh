#ifndef RECONSTRUCTOR_H
#define RECONSTRUCTOR_H

void compute_matte(
	const uint3 color, 
	const uint treshold, 
	const uint tolerance,
	const cv::cuda::PtrStepSz<uchar3> in, 
	cv::cuda::PtrStepSz<uchar> out
);

#endif /* VOXEL_H */