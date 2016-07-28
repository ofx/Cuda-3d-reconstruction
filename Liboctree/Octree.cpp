#include "Stdafx.h"

#include "Octree.h"

Octree::Octree(glm::vec3 center, glm::vec3 halfsize, const unsigned int maxPerCell) : m_MaxPerCell(maxPerCell)
{
	this->m_Root = new OctreeNode(center, halfsize);
	this->m_Root->IsRoot = true;

	this->m_NumNodes = 1;
}

Octree::~Octree(void)
{
	delete this->m_Root;
}

void Octree::Traverse(OctreeNode *node, VisibleVoxel *object)
{
	if (node->IsSplit)
	{
		const unsigned int idx = node->BestIdx(glm::vec3(object->X, object->Y, object->Z));
		OctreeNode *n = node->IdxToNodePointer(idx);

		#pragma omp task
		{
			this->Traverse(n, object);
		}
	}
	else
	{
		node->Add(object);

		if (node->Objects.size() > this->m_MaxPerCell)
		{
			node->Subdivide();
			node->Rebalance();

			this->m_NumNodes += 8;
		}
	}
}

void Octree::Build(void)
{
	omp_set_num_threads(omp_get_max_threads());

	#pragma omp parallel
	for (int i = 0 ; i < this->m_NumVoxels ; ++i)
	{
		if (this->m_Root->Contains(glm::vec3(this->m_Voxels[i].X, this->m_Voxels[i].Y, this->m_Voxels[i].Z)))
		{
			#pragma omp single nowait
			{
				this->Traverse(this->m_Root, &this->m_Voxels[i]);
			}
		}
	}
}