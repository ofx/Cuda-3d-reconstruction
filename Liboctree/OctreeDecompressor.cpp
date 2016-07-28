#include "Stdafx.h"

#include "OctreeDecompressor.h"
#include "Exception.h"

OctreeDecompressor::OctreeDecompressor(std::string file)
{
	// Create output stream for compressed pointcloud file
	this->m_CompressedFile.open(file.c_str(), std::ios::in | std::ios::binary);
	if (!this->m_CompressedFile.good())
	{
		throw_line("Could not open compressed file for writing");
	}
}

OctreeDecompressor::~OctreeDecompressor()
{
	this->m_CompressedFile.close();
}

Octree *OctreeDecompressor::Decompress(void)
{
	Octree *octree;

	boost::archive::binary_iarchive archive(this->m_CompressedFile);
	archive >> octree;

	return octree;
}