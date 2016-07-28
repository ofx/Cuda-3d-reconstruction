#include "Stdafx.h"

#include "Constructor.h"
#include "Exception.h"
#include "Getopt.h"

Constructor::Constructor(void) {}

Constructor::~Constructor(void)
{
	for (int v = 0; v < this->m_Cameras.size(); ++v)
	{
		delete this->m_Cameras[v];
	}
}

void Constructor::PrintUsage(void)
{
	std::cout << "Available parameters:" << std::endl;
	std::cout << "n			  : Number of cameras (numeric), should correspond with the camera file in data path" << std::endl;
	std::cout << "d			  : Data path (directory location)" << std::endl;
	std::cout << "o			  : Compressed file output location" << std::endl;
	std::cout << "i			  : Flag indicating that calibration images should be used in stead of videos" << std::endl;
	std::cout << "s			  : Flag indicating if a matte still should be used in stead of using the keyer" << std::endl;
	std::cout << "m			  : Flag indicating if a matte video should be used in stead of using the keyer" << std::endl;
	std::cout << "h			  : This usage information" << std::endl;
}

bool Constructor::ParseArguments(int argc, char **argv)
{
	bool hasNumCameras = false, hasDataPath = false, hasCompressedFileName = false;

	int opt;
	while ((opt = getopt(argc, argv, "n:d:o:hism")) != -1) 
	{
		switch (opt) 
		{
		// Number of cameras
		case 'n':
			hasNumCameras = true;
			this->m_Settings.NumCameras = atoi(optarg);
			break;
		// Data path
		case 'd':
			hasDataPath = true;
			this->m_Settings.DataPath = std::string(optarg);
			break;
		// Help
		case 'h':
			Constructor::PrintUsage();
			return false;
		// Compressed file output
		case 'o':
			hasCompressedFileName = true;
			this->m_Settings.CompressedFileName = std::string(optarg);
			break;
		// Calibration images?
		case 'i':
			this->m_Settings.UseCalibrationImages = true;
			break;
		// Matte still?
		case 's':
			this->m_Settings.UseMatteStill = true;
			break;
		// Matte video?
		case 'm':
			this->m_Settings.UseMatteVideo = true;
			break;
		default:
			std::cout << "Unknown option: " << (char) opt << std::endl << std::endl;

			Constructor::PrintUsage();
			return false;
		}
	}

	// Check requirements
	if (!hasNumCameras)
	{
		std::cout << "Parameter 'n' is required!" << std::endl << std::endl;

		Constructor::PrintUsage();
		return false;
	}
	if (!hasDataPath)
	{
		std::cout << "Parameter 'd' is required!" << std::endl << std::endl;

		Constructor::PrintUsage();
		return false;
	}
	if (!hasCompressedFileName)
	{
		std::cout << "Parameter 'o' is required!" << std::endl << std::endl;

		Constructor::PrintUsage();
		return false;
	}

	// All OK, show settings
	this->m_Settings.Print();

	return true;
}

void Constructor::Run(int argc, char** argv)
{
	// Parse arguments
	if (!this->ParseArguments(argc, argv))
	{
		return;
	}

	// Create cameras
	const std::string cameraPath = this->m_Settings.DataPath + "cam";
	for (int v = 0; v < this->m_Settings.NumCameras; ++v)
	{
		std::stringstream fullPath;
		fullPath << cameraPath << (v + 1) << "/";

		if (!boost::filesystem::exists(fullPath.str() + Common::ConfigFile))
		{
			if (boost::filesystem::exists(fullPath.str() + Common::IntrinsicsConfigurationFile) && boost::filesystem::exists(fullPath.str() + Common::CheckerboardVideoFile) == false)
			{
				throw_line("Invalid configuration, expecting intrinsics and checkerboard video file!");
			}
		}

		Camera *c = new Camera(this->m_Settings, fullPath.str(), v);

		bool camOk = false;

		// Calibrate camera
		camOk = c->Calibrate();

		// Compute existinsics
		camOk &= c->ComputeExtrinsics();

		// Check if we should initialize (if we've not failed)
		if (camOk)
		{
			camOk = c->Initialize();
		}
		else
		{
			delete c;
			throw_line("Camera failed to initialize, check error above!");
		}

		this->m_Cameras.push_back(c);
	}

	// Create reconstructor
	Reconstructor reconstructor(this->m_Cameras);
	if (reconstructor.Initialize())
	{
		Processor processor(this->m_Settings, reconstructor, this->m_Cameras);
		processor.Process();
	}
	else
	{
		std::cout << "Failed to initialize voxels!" << std::endl;
	}
}