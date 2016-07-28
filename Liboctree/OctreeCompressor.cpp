#include "Stdafx.h"

#include "OctreeCompressor.h"
#include "Exception.h"

OctreeCompressor::OctreeCompressor(std::string file)
{
	// Create output stream for compressed pointcloud file
	this->m_CompressedFile.open(file.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	if (!this->m_CompressedFile.good())
	{
		throw_line("Could not open compressed file for writing");
	}
}

OctreeCompressor::~OctreeCompressor()
{
	this->m_CompressedFile.close();
}

void OctreeCompressor::Compress(Octree *octree)
{
	boost::archive::binary_oarchive archive(this->m_CompressedFile);
	archive << octree;
}