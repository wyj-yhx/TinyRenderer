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

constexpr TGAColor white = TGAColor{255, 255, 255, 255};
constexpr TGAColor red = TGAColor{ 255, 0, 0, 255 };
constexpr TGAColor green = TGAColor{ 0, 255, 0, 255 };
constexpr TGAColor blue = TGAColor{ 0, 0, 255, 255 };
constexpr TGAColor yellow = TGAColor{ 255, 255, 0, 255 };
constexpr TGAColor purple = TGAColor{ 255, 0, 255, 255 };
constexpr TGAColor cyan = TGAColor{ 0, 255, 255, 255 };


Model* model;
TinyRenderer rendererfunc;

static const int ScreenWidth = 1280;
static const int ScreenHeight = 720;


vec3 light_dir{ 0, 0, -0.5 }; // define light_dir
float* zbuffer;

vec3 world2screen(vec4 v, int minSize) {
	return vec3{((v.x / model->GetMaxH() + 1.) * minSize / 2. + .5), (ScreenHeight - (v.y / model->GetMaxH() + 1.) * minSize / 2. + .5), v.z};
}

void ShowTriangle_3D(Model* model, int minSize, SDL_Renderer* renderer, TinyRenderer& rendererfunc)
{
	for (int i = ScreenWidth * ScreenHeight; i--; zbuffer[i] = -std::numeric_limits<float>::max());

	for (int i = 0; i < model->nfaces(); i++) {
		vec3 pts[3];
		vec4 world_coords[3];
		for (int j = 0; j < 3; j++) {
			pts[j] = world2screen(model->vert(i,j), minSize);
			vec4 v = model->vert(i, j);
			world_coords[j] = v;
		}
		vec3 n = rendererfunc.cross((world_coords[2] - world_coords[0]), (world_coords[1] - world_coords[0]));
		n = normalized(n);
		float intensity = n * light_dir;
		if (intensity > 0) {
			rendererfunc.triangle(pts, zbuffer, renderer, TGAColor{ std::uint8_t(intensity * 255),  std::uint8_t(intensity * 255),  std::uint8_t(intensity * 255 ), 255 });
		}
	}
}

vec4 project(vec4 v) { // First of all, (x,y) is an orthogonal projection of the vector (x,y,z).
	return vec4{ (v.x / model->GetMaxH()) * ScreenHeight / 2,       // Second, since the input models are scaled to have fit in the [-1,1]^3 world coordinates,
			 (v.y / model->GetMaxH()) * ScreenHeight / 2,    // we want to shift the vector (x,y) and then scale it to span the entire screen.
			 ((v.z / model->GetMaxH()) + 1.) * 255. };
}

void ShowModel(SDL_Renderer* renderer)
{
	for (int i = ScreenWidth * ScreenHeight; i--; zbuffer[i] = -std::numeric_limits<float>::max());

	for (int i = 0; i < model->nfaces(); i++) { // iterate through all triangles
		vec4 a = project(model->vert(i, 2));
		vec4 b = project(model->vert(i, 1));
		vec4 c = project(model->vert(i, 0));
		TGAColor rnd;
		for (int c = 0; c < 3; c++) rnd[c] = std::rand() % 255;
		/*rendererfunc.triangle(
			ScreenWidth / 2 + a.x, ScreenHeight / 2 - a.y, 
			ScreenWidth / 2 + b.x, ScreenHeight / 2 - b.y, 
			ScreenWidth / 2 + c.x, ScreenHeight / 2 - c.y, renderer, rnd);*/
		rendererfunc.triangle(
			ScreenWidth / 2 + a.x, ScreenHeight / 2 - a.y,a.z ,
			ScreenWidth / 2 + b.x, ScreenHeight / 2 - b.y,b.z ,
			ScreenWidth / 2 + c.x, ScreenHeight / 2 - c.y,c.z ,renderer, zbuffer);
	}

}



/// 初始化设置
void Init()
{
	//model = new Model("../obj/african_head/african_head.obj");
	model = new Model("../obj/diablo3_pose/diablo3_pose.obj");
	zbuffer = new float[ScreenWidth * ScreenHeight];
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
	//// 绘制背景图
	// 绘制一个红色像素点
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // 红色
	SDL_RenderDrawPoint(renderer, 320, 240); // 在屏幕中心绘制

	//SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // 白色
	//for (int i = 0; i < 10; i++) {
	//	// 绘制一系列短直线，形成图案
	//	SDL_RenderDrawLine(renderer, 400, 300, 400 + i * 20, 100 + i * 40);
	//}
	ShowModel(renderer);
	//ShowTriangle_3D(model, 700, renderer, rendererfunc);
}


int main(int argc, char** argv)
{
	
	using namespace std::chrono;

	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
	Mix_Init(MIX_INIT_MP3);
	TTF_Init();

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

	SDL_Window* window = SDL_CreateWindow(u8"<<TinyRenderer>> - By Wyj",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		ScreenWidth, ScreenHeight, SDL_WINDOW_SHOWN);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	//y轴翻转坐标系
	//setupRenderer(renderer, screenWidth, screenHeight);
	// 光标调整为不显示
	//SDL_ShowCursor(SDL_DISABLE);

	// 初始化设置
	Init();
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



