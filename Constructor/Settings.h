#pragma once

typedef struct Settings
{
	unsigned int NumCameras;

	std::string DataPath;

	std::string CompressedFileName;

	bool UseCalibrationImages;

	bool UseMatteStill;

	bool UseMatteVideo;

	Settings(void)
	{
		this->UseCalibrationImages = false;
		this->UseMatteStill = false;
		this->UseMatteVideo = false;
	}

	void Print(void)
	{
		std::cout << "Settings: " << std::endl;

		std::cout << "Number of cameras: " << this->NumCameras << std::endl;
		std::cout << "Data path: " << this->DataPath << std::endl;
		std::cout << "Compressed file output file: " << this->CompressedFileName << std::endl;
		std::cout << "Calibration images: " << (this->UseCalibrationImages ? "yes" : "no") << std::endl;
		std::cout << "Matte still: " << (this->UseMatteStill ? "yes" : "no") << std::endl;
		std::cout << "Matte video: " << (this->UseMatteVideo ? "yes" : "no") << std::endl;
	}
} Settings;