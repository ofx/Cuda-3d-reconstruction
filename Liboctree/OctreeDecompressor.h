#pragma once

#include "Octree.h"

class OctreeDecompressor
{
private:
	std::ifstream m_CompressedFile;
public:
	OctreeDecompressor(std::string file);
	~OctreeDecompressor(void);

	Octree *Decompress(void);
};