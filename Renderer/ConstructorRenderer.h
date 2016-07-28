#pragma once

#include "Settings.h"
#include "OctreeDecompressor.h"

class ConstructorRenderer
{
	int m_Argc;
	char **m_Argv;

	OctreeDecompressor *m_Decompressor;

	Settings m_Settings;
public:
	ConstructorRenderer(int argc, char **argv);
	~ConstructorRenderer(void);

	void Run(void);

	void ParseArguments(void);
};