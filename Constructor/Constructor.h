#pragma once

#include "Common.h"
#include "Camera.h"
#include "Reconstructor.h"
#include "Processor.h"
#include "Settings.h"

class Constructor
{
	Settings m_Settings;

	std::vector<Camera*> m_Cameras;
public:
	Constructor(void);
	virtual ~Constructor(void);

	static void PrintUsage(void);

	bool ParseArguments(int argc, char **argv);

	void Run(int argc, char **argv);
};