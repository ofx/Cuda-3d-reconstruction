#include <cuda_runtime.h>

#include <iostream>

#include "Exception.h"

#include "init.cuh"

bool init_cuda(void)
{
	size_t size;
	cudaDeviceGetLimit(&size, cudaLimitMallocHeapSize);
	printf("CUDA heap size found to be: %d bytes\n", (int)size);

	return true;
}