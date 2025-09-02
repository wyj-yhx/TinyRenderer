#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model {
private:
	std::vector<Vec3f> verts_;
	std::vector<Vec3f> vterts_;
	std::vector<std::vector<int> > faces_;
	std::vector<std::vector<int> > texture_;
public:
	Model(const char* filename);
	~Model();
	int nverts();
	int nvterts();
	int nfaces();
	int ntexture();

	Vec3f vert(int i);
	Vec3f vtert(int i);
	std::vector<int> face(int idx);
	std::vector<int> texture(int idx);

	float max;
};

#endif //__MODEL_H__