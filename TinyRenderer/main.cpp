#define SDL_MAIN_HANDLED

#include "tgaimage.h"
#include "tinyrenderer.h"
#include "model.h"

// SDL
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#include <chrono>
#include <thread>

#include <iostream>
using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);
const TGAColor yellow = TGAColor(255, 255, 0, 255);
const TGAColor purple = TGAColor(255, 0, 255, 255);
const TGAColor cyan = TGAColor(0, 255, 255, 255);



void ShowTriangle(Model* model, TGAImage& image, TinyRenderer& renderer)
{

    Vec2i t0[3] = { Vec2i(10, 70),   Vec2i(50, 160),  Vec2i(70, 80) };
    Vec2i t1[3] = { Vec2i(180, 50),  Vec2i(150, 1),   Vec2i(70, 180) };
    Vec2i t2[3] = { Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180) };
    renderer.triangle_scanline(t0[0], t0[1], t0[2], image, red);
    renderer.triangle_scanline(t1[0], t1[1], t1[2], image, red);
    renderer.triangle_scanline(t2[0], t2[1], t2[2], image, red);
}


void ShowLine(Model* model, int minSize, TGAImage& image, TinyRenderer& renderer)
{
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        for (int j = 0; j < 3; j++) {
            Vec3f v0 = model->vert(face[j]);
            Vec3f v1 = model->vert(face[(j + 1) % 3]);
            int x0 = (v0.x / model->max + 1.) * minSize / 2.;
            int y0 = (v0.y / model->max + 1.) * minSize / 2.;
            int x1 = (v1.x / model->max + 1.) * minSize / 2.;
            int y1 = (v1.y / model->max + 1.) * minSize / 2.;
            renderer.line(x0, y0, x1, y1, image, white);
        }
    }
}

void ShowPix(TGAImage image)
{
    image.set(52, 41, red);
}



/// <summary>
/// 更新
/// </summary>
/// <param name="delta"></param>
void OnUpdate(float delta) {

}

/// <summary>
/// 绘图
/// </summary>
/// <param name="renderer"></param>
void OnRender(SDL_Renderer* renderer)
{
	// 绘制背景图
	// 绘制一个红色像素点
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // 红色
	SDL_RenderDrawPoint(renderer, 320, 240); // 在屏幕中心绘制

	// 绘制多条不同颜色和方向的直线
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // 红色
	SDL_RenderDrawLine(renderer, 100, 100, 700, 100); // 水平线

	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // 绿色
	SDL_RenderDrawLine(renderer, 100, 500, 700, 500); // 水平线

	SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // 蓝色
	SDL_RenderDrawLine(renderer, 100, 100, 100, 500); // 垂直线

	SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // 黄色
	SDL_RenderDrawLine(renderer, 700, 100, 700, 500); // 垂直线

	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); // 紫色
	SDL_RenderDrawLine(renderer, 100, 100, 700, 500); // 对角线

	SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); // 青色
	SDL_RenderDrawLine(renderer, 700, 100, 100, 500); // 对角线

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // 白色
	for (int i = 0; i < 10; i++) {
		// 绘制一系列短直线，形成图案
		SDL_RenderDrawLine(renderer, 400, 300, 400 + i * 20, 100 + i * 40);
	}
}



int main(int argc, char** argv)
{
	using namespace std::chrono;

	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
	Mix_Init(MIX_INIT_MP3);
	TTF_Init();

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

	SDL_Window* window = SDL_CreateWindow(u8"《拼好饭传奇》 - By Wyj",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1280, 720, SDL_WINDOW_SHOWN);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	// 光标调整为不显示
	//SDL_ShowCursor(SDL_DISABLE);

	//ResMgr::Instance()->Load(renderer);

	//可交互区域
	//InitRegions();

	//Mix_PlayChannel(-1, ResMgr::Instance()->FindAudio("bgm"), -1);

	SDL_Event event;
	bool is_quit = false;

	const nanoseconds frame_duration(1000000000 / 144);
	steady_clock::time_point last_tick = steady_clock::now();

	while (!is_quit)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type) {
			case SDL_QUIT:
				is_quit = true;
				break;
			}

			/*CursorMgr::Instance()->OnInput(event);
			RegionMgr::Instance()->OnInput(event);*/
		}

		steady_clock::time_point frame_start = steady_clock::now();
		duration<float> delta = duration<float>(frame_start - last_tick);

		OnUpdate(delta.count());

		// 清除屏幕
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // 黑色背景
		SDL_RenderClear(renderer);
		OnRender(renderer);
		SDL_RenderPresent(renderer);

		last_tick = frame_start;

		nanoseconds sleep_duration = frame_duration - (steady_clock::now() - frame_start);
		if (sleep_duration > nanoseconds(0))
			std::this_thread::sleep_for(sleep_duration);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	TTF_Quit();
	Mix_Quit();
	IMG_Quit();
	SDL_Quit();

	return 0;
}


int main_1(int argc, char** argv) {
	
    int width = 500;
    int height = 500;

    int minSize = width < height ? width : height;

	TinyRenderer renderer;
	TGAImage image(width, height, TGAImage::RGB);
    Model* model = new Model("../obj/african_head/african_head.obj");


    //Vec2i pts[3] = { Vec2i(10,10), Vec2i(100, 30), Vec2i(190, 160) };
    //renderer.triangle(pts, image, TGAColor(255, 0, 0, 0));

    ShowTriangle(model, image, renderer);
    

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");


	return 0;
}


