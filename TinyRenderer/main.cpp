#define SDL_MAIN_HANDLED

#include "tgaimage.h"
#include "tinyrenderer.h"
#include "model.h"
#include "wyj_gl.h"

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


TinyRenderer rendererfunc;


static const int ScreenWidth = 800;
static const int ScreenHeight = 800;

// 定义4x4的矩阵
//mat<4, 4> ModelView, Viewport, Perspective;

vec3 light_dir{ 0, 0, -0.5 }; // define light_dir

extern mat<4, 4> ModelView, Perspective; // "OpenGL" state matrices and
extern std::vector<double> zbuffer;     // the depth buffer



struct RandomShader : IShader {
	const Model& model;
	TGAColor color = {};
	vec3 tri[3];  // triangle in eye coordinates

	RandomShader(const Model& m) : model(m) {
	}

	virtual vec4 vertex(const int face, const int vert) {
		vec4 v = model.vert(face, vert);                          // current vertex in object coordinates
		vec4 gl_Position = ModelView * vec4{ v.x, -v.y, v.z, 1. };
		tri[vert] = gl_Position.xyz();                            // in eye coordinates
		return Perspective * gl_Position;                         // in clip coordinates
	}

	virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
		return { false, color };                                    // do not discard the pixel
	}
};

struct PhongShader : IShader {
	const Model& model;
	vec3 l;          // light direction in eye coordinates
	vec3 tri[3];     // triangle in eye coordinates
	//vec3 varying_nrm[3]; // normal per vertex to be interpolated by the fragment

	PhongShader(const vec3 light, const Model& m) : model(m) {
		l = normalized((ModelView * vec4{ light.x, light.y, light.z, 0. }).xyz()); // transform the light vector to view coordinates
	}

	virtual vec4 vertex(const int face, const int vert) {
		vec4 v = model.vert(face, vert);                          // current vertex in object coordinates
		//vec4 n = model.normal(face, vert);
		//varying_nrm[vert] = (ModelView.invert_transpose() * vec4 { n.x, n.y, n.z, 0. }).xyz();
		vec4 gl_Position = ModelView * vec4{ v.x, -v.y, v.z, 1. };
		tri[vert] = gl_Position.xyz();                            // in eye coordinates
		return Perspective * gl_Position;                         // in clip coordinates
	}

	virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
		TGAColor gl_FragColor = { 255, 255, 255, 255 };             // output color of the fragment
		vec3 n = normalized(cross(tri[2] - tri[0], tri[1] - tri[0]));// per-vertex normal 
		//vec3 n = normalized(varying_nrm[0] * bar[0] + varying_nrm[1] * bar[1] + varying_nrm[2] * bar[2]);// per-vertex normal 
		vec3 r = normalized(n * (n * l) * 2 - l);                   // reflected light direction
		double ambient = .3;                                      // ambient light intensity
		double diff = std::max(0., n * l);                        // diffuse light intensity
		double spec = std::pow(std::max(r.z, 0.), 35);            // specular intensity, note that the camera lies on the z-axis (in eye coordinates), therefore simple r.z, since (0,0,1)*(r.x, r.y, r.z) = r.z
		for (int channel : {0, 1, 2}){
			gl_FragColor[channel] *= std::min(1., ambient + .4 * diff + .9 * spec);
			//cout << ambient << " | " << diff << " | " << l << " | " << endl;
		}
		return { false, gl_FragColor };                             // do not discard the pixel
	}
};

Model* model;
RandomShader* randomshader;
PhongShader* phongshader;


vec3 world2screen(vec4 v, int minSize) {
	return vec3{((v.x / model->GetMaxH() + 1.) * minSize / 2. + .5), (ScreenHeight - (v.y / model->GetMaxH() + 1.) * minSize / 2. + .5), v.z};
}


vec4 rot(vec4 v) {
	/*constexpr */
	double a = M_PI / 6;
	mat<4, 4> Ry = { {{std::cos(a), 0, std::sin(a),0}, {0,1,0,0}, {-std::sin(a), 0, std::cos(a),0}, {0,0,0,0} } };
	return Ry * v;
}

vec4 persp(vec4 v) {
	constexpr double c = 4.;
	return v / (1 - v.z / c);
}

vec4 project(vec4 v) { // First of all, (x,y) is an orthogonal projection of the vector (x,y,z).
	return vec4{ (v.x / model->GetMaxH()) * ScreenHeight / 2,       // Second, since the input models are scaled to have fit in the [-1,1]^3 world coordinates,
			 (v.y / model->GetMaxH()) * ScreenHeight / 2,    // we want to shift the vector (x,y) and then scale it to span the entire screen.
			 ((v.z / model->GetMaxH()) + 1.) * 255. / 2};
}

void ShowModel_1(SDL_Renderer* renderer)
{
	for (int i = ScreenWidth * ScreenHeight; i--; zbuffer[i] = -std::numeric_limits<float>::max());

	for (int i = 0; i < model->nfaces(); i++) { // iterate through all triangles
		vec4 clip[3];
		for (int d : {0, 1, 2}) {            // assemble the primitive
			vec4 v = model->vert(i, d);
			clip[d] = Perspective * ModelView * vec4{ v.x, -v.y, v.z, 1. };
		}
		TGAColor rnd;
		for (int c = 0; c < 3; c++) rnd[c] = std::rand() % 255;
		rendererfunc.rasterize(clip, zbuffer, renderer, rnd); // rasterize the primitive
	}

}


/// 初始化设置
void Init()
{
	//model = new Model("../obj/african_head/african_head.obj");
	model = new Model("../obj/diablo3_pose/diablo3_pose.obj");

	zbuffer = std::vector<double>(ScreenWidth * ScreenHeight, -std::numeric_limits<double>::max());
	
	constexpr vec3  light{ 1, 1, 1 }; // light source
	constexpr vec3    eye{ -1,0,2 }; // camera position 相机的位置
	constexpr vec3 center{ 0,0,0 };  // camera direction 相机的方向
	constexpr vec3     up{ 0,1,0 };  // camera up vector 相机向上矢量

	//初始化矩阵
	lookat(eye, center, up);                                   // build the ModelView   matrix
	init_perspective(norm(eye - center));                        // build the Perspective matrix
	init_viewport(ScreenWidth / 16, ScreenHeight / 16, ScreenWidth * 7 / 8, ScreenHeight * 7 / 8); // build the Viewport    matrix
	init_zbuffer(ScreenWidth, ScreenHeight);
	//TGAImage framebuffer(ScreenWidth, ScreenHeight, TGAImage::RGB, { 177, 195, 209, 255 });

	randomshader = new RandomShader(*model);
	phongshader = new PhongShader(light, *model);
}


void Destory() {
	delete model;
	delete randomshader;
	delete phongshader;
}


void ShowModel(SDL_Renderer* renderer)
{
	for (int i = ScreenWidth * ScreenHeight; i--; zbuffer[i] = -std::numeric_limits<float>::max());

	for (int f = 0; f < model->nfaces(); f++) {      // iterate through all facets
		//randomshader->color = { (uint8_t)(std::rand() % 255), (uint8_t)(std::rand() % 255), (uint8_t)(std::rand() % 255), 255 };
		Triangle clip = { phongshader->vertex(f, 0),  // assemble the primitive
						  phongshader->vertex(f, 1),
						  phongshader->vertex(f, 2) };
		rasterize(clip, *phongshader, *renderer);   // rasterize the primitive
	}
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

	Destory();

	return 0;
}

