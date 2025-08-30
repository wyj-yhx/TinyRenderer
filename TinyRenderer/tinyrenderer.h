#pragma once

#include "tgaimage.h"
#include "geometry.h"

class TinyRenderer
{
public:
	void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color);
	Vec3f barycentric(Vec2i* pts, Vec2i P);
	void triangle(Vec2i* pts, TGAImage& image, TGAColor color);
	void triangle_scanline(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color);
};