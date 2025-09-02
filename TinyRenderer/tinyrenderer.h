#pragma once
// SDL
#include <SDL.h>

#include "tgaimage.h"
#include "geometry.h"

extern const int ScreenWidth;
extern const int ScreenHeight;

class TinyRenderer
{
public:
	void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color);
	Vec3f barycentric(Vec2i* pts, Vec2i P);
	void triangle(Vec2i* pts, TGAImage& image, TGAColor color);
	void triangle_scanline(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color); 
	Vec3f barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P);
	void triangle(Vec3f* pts, float* zbuffer, TGAImage& image, TGAColor color);

	void line(int x0, int y0, int x1, int y1, SDL_Renderer* renderer, TGAColor color);
	void triangle(Vec2i* pts, SDL_Renderer* renderer, TGAColor color);
	void triangle_scanline(Vec2i t0, Vec2i t1, Vec2i t2, SDL_Renderer* renderer, TGAColor color);
	void triangle(Vec3f* pts, float* zbuffer, SDL_Renderer* renderer, TGAColor color);
};