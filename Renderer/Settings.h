#pragma once

typedef struct Settings
{
	std::string InputFile;

	Settings(void)
	{
	}

	void Print(void)
	{
		std::cout << "Settings: " << std::endl;

		std::cout << "Input file: " << this->InputFile << std::endl;
	}
} Settings;