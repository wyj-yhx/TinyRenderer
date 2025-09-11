#pragma once
// SDL
#include <SDL.h>

#include "tgaimage.h"
#include "geometry.h"

extern const int ScreenWidth;
extern const int ScreenHeight;

extern mat<4, 4> ModelView, Viewport, Perspective;

class TinyRenderer
{
public:
	void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color);
	double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy);
	void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage& framebuffer, TGAColor color);
	void triangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, TGAImage& framebuffer);
	void rasterize(const vec4 clip[3], std::vector<double>& zbuffer, TGAImage& framebuffer, const TGAColor color);

	vec3 cross(const vec4& a, const vec4& b);
	vec3 barycentric(vec3* pts, vec3 P);
	void triangle(int ax, int ay, int bx, int by, int cx, int cy, SDL_Renderer* renderer, TGAColor color);
	void triangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, SDL_Renderer* renderer, float* zbuffer);
	void triangle(vec3* pts, float* zbuffer, SDL_Renderer* renderer, TGAColor color);
	void rasterize(const vec4 clip[3], std::vector<double>& zbuffer, SDL_Renderer* renderer, const TGAColor color);
};