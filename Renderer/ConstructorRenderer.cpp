#include "Stdafx.h"

#include "ConstructorRenderer.h"

#include "Settings.h"
#include "Exception.h"
#include "Getopt.h"
#include "OctreeDecompressor.h"
#include "OctreeRenderer.h"

ConstructorRenderer::ConstructorRenderer(int argc, char **argv) : m_Argc(argc), m_Argv(argv)
{
	this->ParseArguments();

	this->m_Decompressor = new OctreeDecompressor(this->m_Settings.InputFile);
}

ConstructorRenderer::~ConstructorRenderer()
{
	delete this->m_Decompressor;
}

void ConstructorRenderer::Run(void)
{
	Octree *octree = this->m_Decompressor->Decompress();
	OctreeRenderer renderer(octree);
}

void ConstructorRenderer::ParseArguments(void)
{
	bool hasInputFile = false;

	int opt;
	while ((opt = getopt(this->m_Argc, this->m_Argv, "hi:")) != -1)
	{
		switch (opt)
		{
		// Input file
		case 'i':
			hasInputFile = true;
			this->m_Settings.InputFile = std::string(optarg);
			break;
		// Help
		case 'h':
		default:
			goto usage;
		}
	}

	// Check requirements
	if (!hasInputFile)
	{
		std::cout << "Missing input file!" << std::endl << std::endl;

		goto usage;
	}

	return;

usage:
	std::cout << "Available parameters:" << std::endl;

	throw_line_graceful();
}

int main(int argc, char **argv)
{
	try
	{
		ConstructorRenderer r(argc, argv);
		r.Run();
	}
	catch (ConstructorException &e)
	{
		if (e.isGraceful())
		{
			return EXIT_SUCCESS;
		}
		else
		{
			std::cout << "Uncaught exception: " << std::endl << e.what() << std::endl;

			return EXIT_FAILURE;
		}
	}
}