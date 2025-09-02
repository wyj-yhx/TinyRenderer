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


Model* model;
TinyRenderer rendererfunc;

static const int ScreenWidth = 1280;
static const int ScreenHeight = 720;


Vec3f light_dir(-0.5, -0.5, -0.5); // define light_dir
float* zbuffer;



void ShowTriangle(Model* model, TGAImage& image, TinyRenderer& rendererfunc)
{

    Vec2i t0[3] = { Vec2i(10, 70),   Vec2i(50, 160),  Vec2i(70, 80) };
    Vec2i t1[3] = { Vec2i(180, 50),  Vec2i(150, 1),   Vec2i(70, 180) };
    Vec2i t2[3] = { Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180) };
    rendererfunc.triangle_scanline(t0[0], t0[1], t0[2], image, red);
    rendererfunc.triangle_scanline(t1[0], t1[1], t1[2], image, red);
    rendererfunc.triangle_scanline(t2[0], t2[1], t2[2], image, red);
}

/// <summary>
/// 扫线的三角形
/// </summary>
/// <param name="model"></param>
/// <param name="minSize"></param>
/// <param name="renderer"></param>
/// <param name="rendererfunc"></param>
void ShowTriangle_line(Model* model, int minSize, SDL_Renderer* renderer, TinyRenderer& rendererfunc)
{
	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i);
		Vec2i screen_coords[3];
		Vec3f world_coords[3];
		for (int j = 0; j < 3; j++) {
			Vec3f v = model->vert(face[j]);
			screen_coords[j].x = (v.x / model->max + 1.) * minSize / 2.;
			screen_coords[j].y = ScreenHeight - (v.y / model->max + 1.) * minSize / 2.;
			world_coords[j] = v;
		}
		Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
		n.normalize();
		float intensity = n * light_dir;
		if (intensity > 0) {
			rendererfunc.triangle_scanline(screen_coords[0], screen_coords[1], screen_coords[2], renderer, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
		}
		//rendererfunc.triangle_scanline(screen_coords[0], screen_coords[1], screen_coords[2], renderer, TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));
	}
}
//绘制三角形
void ShowTriangle(Model* model, int minSize, SDL_Renderer* renderer, TinyRenderer& rendererfunc)
{

	Vec2i t0[3] = { Vec2i(10, 70),   Vec2i(50, 160),  Vec2i(70, 80) };
	//rendererfunc.triangle_scanline(t0[0], t0[1], t0[2], &renderer, red);
	//rendererfunc.triangle_scanline(t1[0], t1[1], t1[2], &renderer, red);
	//rendererfunc.triangle_scanline(t2[0], t2[1], t2[2], &renderer, red);

	rendererfunc.triangle(t0, renderer, red);

	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i);
		Vec2i tp[3];
		for (int j = 0; j < 3; j++) {
			Vec3f v0 = model->vert(face[j]);
			tp[j].x = (v0.x / model->max + 1.) * minSize / 2.;
			tp[j].y = ScreenHeight - (v0.y / model->max + 1.) * minSize / 2.;
		}
		rendererfunc.triangle(tp, renderer, TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));
	}
}


Vec3f world2screen(Vec3f v, int minSize) {
	return Vec3f(int((v.x / model->max + 1.) * minSize / 2. + .5), int(ScreenHeight - (v.y / model->max + 1.) * minSize / 2. + .5), v.z);
}

void ShowTriangle_3D(Model* model, int minSize, SDL_Renderer* renderer, TinyRenderer& rendererfunc)
{
	for (int i = ScreenWidth * ScreenHeight; i--; zbuffer[i] = -std::numeric_limits<float>::max());

	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i);
		Vec3f pts[3];
		Vec3f world_coords[3];
		for (int j = 0; j < 3; j++) {
			pts[j] = world2screen(model->vert(face[j]), minSize);
			Vec3f v = model->vert(face[j]);
			world_coords[j] = v;
		}
		Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
		n.normalize();
		float intensity = n * light_dir;
		if (intensity > 0) {
			rendererfunc.triangle(pts, zbuffer, renderer, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
		}
	}
}


void ShowLine(Model* model, int minSize, TGAImage& image, TinyRenderer& rendererfunc)
{
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        for (int j = 0; j < 3; j++) {
            Vec3f v0 = model->vert(face[j]);
            Vec3f v1 = model->vert(face[(j + 1) % 3]);
            int x0 = (int)((v0.x / model->max + 1.) * minSize / 2.);
            int y0 = (int)((v0.y / model->max + 1.) * minSize / 2.);
            int x1 = (int)((v1.x / model->max + 1.) * minSize / 2.);
            int y1 = (int)((v1.y / model->max + 1.) * minSize / 2.);
			rendererfunc.line(x0, y0, x1, y1, image, white);
        }
    }
}

void ShowLine(Model* model, int minSize, SDL_Renderer* renderer, TinyRenderer& rendererfunc)
{
	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i);
		for (int j = 0; j < 3; j++) {
			Vec3f v0 = model->vert(face[j]);
			Vec3f v1 = model->vert(face[(j + 1) % 3]);
			int x0 = (int)((v0.x / model->max + 1.) * minSize / 2.);
			int y0 = (int)(ScreenHeight - (v0.y / model->max + 1.) * minSize / 2.);
			int x1 = (int)((v1.x / model->max + 1.) * minSize / 2.);
			int y1 = (int)(ScreenHeight - (v1.y / model->max + 1.) * minSize / 2.);
			rendererfunc.line(y0, x0, y1, x1, renderer, white);		//xy被对调
		}
	}
}

void ShowPix(TGAImage image)
{
    image.set(52, 41, red);
}


// 设置渲染器以便原点在左下角
void setupRenderer(SDL_Renderer* renderer, int screenWidth, int screenHeight) {
	// 设置视口并翻转Y轴
	SDL_Rect viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.w = screenWidth;
	viewport.h = screenHeight;

	SDL_RenderSetViewport(renderer, &viewport);
	SDL_RenderSetScale(renderer, 1.0f, -1.0f); // Y轴翻转
	SDL_RenderSetLogicalSize(renderer, screenWidth, screenHeight);
}

/// 初始化设置
void Init()
{
	model = new Model("../obj/african_head/african_head.obj");
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
	//// 绘制一个红色像素点
	//SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // 红色
	//SDL_RenderDrawPoint(renderer, 320, 240); // 在屏幕中心绘制

	//SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // 白色
	//for (int i = 0; i < 10; i++) {
	//	// 绘制一系列短直线，形成图案
	//	SDL_RenderDrawLine(renderer, 400, 300, 400 + i * 20, 100 + i * 40);
	//}

	//ShowLine(model, 700, renderer, rendererfunc);
	ShowTriangle_3D(model, 700, renderer, rendererfunc);
	//ShowTriangle(model, 700, renderer, rendererfunc);
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


int main_1(int argc, char** argv) {
	
    int width = 500;
    int height = 500;

    int minSize = width < height ? width : height;

	TinyRenderer rendererfunc;
	TGAImage image(width, height, TGAImage::RGB);
    Model* model = new Model("../obj/african_head/african_head.obj");


    //Vec2i pts[3] = { Vec2i(10,10), Vec2i(100, 30), Vec2i(190, 160) };
    //renderer.triangle(pts, image, TGAColor(255, 0, 0, 0));

    ShowTriangle(model, image, rendererfunc);
    

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");


	return 0;
}


