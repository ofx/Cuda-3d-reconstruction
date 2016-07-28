#pragma once

typedef struct
{
	short X, Y, Z;

	unsigned char R, G, B;

	template<class Archive>
	inline void serialize(Archive &ar, const unsigned int fileVersion)
	{
		ar & this->X;
		ar & this->Y;
		ar & this->Z;
		ar & this->R;
		ar & this->G;
		ar & this->B;
	}
} VisibleVoxel;