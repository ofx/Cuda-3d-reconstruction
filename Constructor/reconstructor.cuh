#ifndef RECONSTRUCTOR_H
#define RECONSTRUCTOR_H

bool destroy_voxels(void);

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
	const unsigned int     plane_width,
	const unsigned int     plane_height,
	int					   *total_voxels
);

bool update_voxels(
	const cv::cuda::GpuMat *h_gputmat_foregrounds,
	const cv::cuda::GpuMat *h_gputmat_frames,
	unsigned long long int *h_num_voxels,
	VisibleVoxel		   **h_visible_voxels
);

#endif /* VOXEL_H */