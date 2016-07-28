#pragma once

#include <list>

#include "VisibleVoxel.h"

#define iLoc(x, y, z) x << 2 + y << 1 + z << 0

class Vec3 : public glm::vec3
{
public:
	Vec3() {}
	Vec3(glm::vec3 vec)
	{
		this->x = vec.x;
		this->y = vec.y;
		this->z = vec.z;
	}

	template<class Archive>
	inline void serialize(Archive &ar, const unsigned int fileVersion)
	{
		ar & this->x;
		ar & this->y;
		ar & this->z;
	}
};

typedef struct OctreeNode
{
	Vec3 Center;
	Vec3 Halfsize;

	OctreeNode *NextSibling;
	OctreeNode *Parent;
	OctreeNode *FirstChild;

	std::list<VisibleVoxel*> Objects;

	unsigned int Loc;

	bool IsRoot;
	bool IsSplit;

	template<class Archive>
	inline void serialize(Archive &ar, const unsigned int fileVersion)
	{
		ar & this->Center;
		ar & this->Halfsize;
		ar & this->NextSibling;
		ar & this->Parent;
		ar & this->FirstChild;
		ar & this->Objects;
		ar & this->Loc;
		ar & this->IsRoot;
		ar & this->IsSplit;
	}

	OctreeNode(void) {}

	OctreeNode(glm::vec3 center, glm::vec3 halfsize, OctreeNode *parent = 0, const unsigned int loc = 0) : Loc(loc)
	{
		this->Center = center;
		this->Halfsize = halfsize;

		this->NextSibling = 0;
		this->Parent = parent;
		this->FirstChild = 0;

		this->IsRoot = false;
		this->IsSplit = false;
	}

	~OctreeNode(void)
	{
		delete this->NextSibling;
		delete this->FirstChild;
	}

	void Add(VisibleVoxel *object)
	{
		this->Objects.push_back(object);
	}

	void Subdivide(void)
	{
		glm::vec3 qsize = this->Halfsize * 0.5f;

		glm::vec3 llnc(this->Center.x - qsize.x, this->Center.y - qsize.y, this->Center.z - qsize.z);
		glm::vec3 llfc(this->Center.x - qsize.x, this->Center.y - qsize.y, this->Center.z + qsize.z);
		glm::vec3 lunc(this->Center.x - qsize.x, this->Center.y + qsize.y, this->Center.z - qsize.z);
		glm::vec3 lufc(this->Center.x - qsize.x, this->Center.y + qsize.y, this->Center.z + qsize.z);

		glm::vec3 rlnc(this->Center.x + qsize.x, this->Center.y - qsize.y, this->Center.z - qsize.z);
		glm::vec3 rlfc(this->Center.x + qsize.x, this->Center.y - qsize.y, this->Center.z + qsize.z);
		glm::vec3 runc(this->Center.x + qsize.x, this->Center.y + qsize.y, this->Center.z - qsize.z);
		glm::vec3 rufc(this->Center.x + qsize.x, this->Center.y + qsize.y, this->Center.z + qsize.z);

		OctreeNode *lln = new OctreeNode(llnc, qsize, this, iLoc(0, 0, 0));
		OctreeNode *llf = new OctreeNode(llfc, qsize, this, iLoc(0, 0, 1));
		OctreeNode *lun = new OctreeNode(lunc, qsize, this, iLoc(0, 1, 0));
		OctreeNode *luf = new OctreeNode(lufc, qsize, this, iLoc(0, 1, 1));

		OctreeNode *rln = new OctreeNode(rlnc, qsize, this, iLoc(1, 0, 0));
		OctreeNode *rlf = new OctreeNode(rlfc, qsize, this, iLoc(1, 0, 1));
		OctreeNode *run = new OctreeNode(runc, qsize, this, iLoc(1, 1, 0));
		OctreeNode *ruf = new OctreeNode(rufc, qsize, this, iLoc(1, 1, 1));

		this->FirstChild = lln;

		lln->NextSibling = llf;
		llf->NextSibling = lun;
		lun->NextSibling = luf;
		luf->NextSibling = rln;
		rln->NextSibling = rlf;
		rlf->NextSibling = run;
		run->NextSibling = ruf;

		this->IsSplit = true;
	}

	inline const unsigned int BestIdx(glm::vec3 v)
	{
		return ((v.x > this->Center.x) << 2) + ((v.y > this->Center.y) << 1) + ((v.z > this->Center.z) << 0);
	}

	OctreeNode *IdxToNodePointer(const unsigned int idx)
	{
		if (idx == 0)
		{
			return this->FirstChild;
		}
		else
		{
			OctreeNode *node = this->FirstChild;
			for (int i = 0 ; i < idx ; ++i)
			{
				node = node->NextSibling;
			}
			return node;
		}
	}

	void Rebalance(void)
	{
		std::list<VisibleVoxel*>::const_iterator it;
		for (it = this->Objects.begin() ; it != this->Objects.end() ; ++it)
		{
			this->IdxToNodePointer(this->BestIdx(glm::vec3((*it)->X, (*it)->Y, (*it)->Z)))->Add(*it);
		}

		this->Objects.clear();
	}

	inline bool Contains(glm::vec3 point)
	{
		if (abs(this->Center.x - point.x) > this->Halfsize.x || abs(this->Center.y - point.y) > this->Halfsize.y || abs(this->Center.z - point.z) > this->Halfsize.z)
		{
			return false;
		}

		return true;
	}
} OctreeNode;

class Octree
{
private:
	unsigned int m_MaxPerCell;

	VisibleVoxel *m_Voxels;

	unsigned int m_NumVoxels;

	OctreeNode *m_Root;

	unsigned int m_NumNodes;
public:
	Octree(void) {}
	Octree(glm::vec3 center, glm::vec3 halfsize, const unsigned int maxPerCell = 1000000);
	~Octree(void);

	OctreeNode *GetRoot(void) { return this->m_Root; }

	unsigned int GetNumVoxels(void) { return this->m_NumVoxels; }
	unsigned int GetNumNodes(void) { return this->m_NumNodes; }

	VisibleVoxel *GetVoxels(void) { return this->m_Voxels; }

	void SetVoxels(VisibleVoxel *voxels) { this->m_Voxels = voxels; }
	void SetNumVoxels(unsigned int numVoxels) { this->m_NumVoxels = numVoxels; }

	template<class Archive>
	inline void serialize(Archive &ar, const unsigned int fileVersion)
	{
		ar & this->m_Voxels;
		ar & this->m_Root;
		ar & this->m_MaxPerCell;
		ar & this->m_NumVoxels;
		ar & this->m_NumNodes;
	}

	void Traverse(OctreeNode *node, VisibleVoxel *object);
	void Build(void);
};