#include "Stdafx.h"

#include "Common.h"
#include "Constructor.h"
#include "Exception.h"

#include "init.cuh"

#pragma warning(disable)

int main(int argc, char **argv)
{
	int numCudaDevices = cv::cuda::getCudaEnabledDeviceCount();
    std::cout << "Cuda devices: " << numCudaDevices << std::endl;
	if (numCudaDevices <= 0)
	{
		std::cout << "At least one CUDA supporting device should be present in the system, bye." << std::endl;
		return EXIT_FAILURE;
	}

	init_cuda();

	try
	{
		Constructor c;
		c.Run(argc, argv);
	}
	catch (ConstructorException &e)
	{
		std::cout << "Exception occurred, halting execution:" << std::endl << e.what() << std::endl;
	}
	catch (StopException &e)
	{
		std::cout << "CTRL+C, halt." << std::endl;
	}

	system("pause");

	return EXIT_SUCCESS;
}