#pragma once

#include "Octree.h"

class OctreeCompressor
{
private:
	std::ofstream m_CompressedFile;
public:
	OctreeCompressor(std::string file);
	~OctreeCompressor(void);

	void Compress(Octree *octree);
};