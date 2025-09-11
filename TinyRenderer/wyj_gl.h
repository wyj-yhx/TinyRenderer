#pragma once
// SDL
#include <SDL.h>

#include "tgaimage.h"
#include "geometry.h"

extern const  int ScreenWidth;
extern const  int ScreenHeight;

void lookat(const vec3 eye, const vec3 center, const vec3 up);
void init_perspective(const double f);
void init_viewport(const int x, const int y, const int w, const int h);
void init_zbuffer(const int width, const int height);

struct IShader {
    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const = 0;
};

typedef vec4 Triangle[3]; // a triangle primitive is made of three ordered points 三角形原语由三个有序的点构成
void rasterize(const Triangle& clip, const IShader& shader, TGAImage& framebuffer);
void rasterize(const Triangle& clip, const IShader& shader, SDL_Renderer& renderer);