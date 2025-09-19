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
constexpr int shadoww = 800;    // shadow map buffer size
constexpr int shadowh = 800;

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

struct BlankShader : IShader {
	const Model& model;

	BlankShader(const Model& m) : model(m) {}

	virtual vec4 vertex(const int face, const int vert) {
		vec4 gl_Position = ModelView * model.vert(face, vert);
		return Perspective * gl_Position;
	}

	virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
		return { false, {255, 255, 255, 255} };
	}
};

struct PhongShader : IShader {
	const Model& model;
	//vec3 l;          // light direction in eye coordinates
	//vec3 tri[3];     // triangle in eye coordinates
	//vec3 varying_nrm[3]; // normal per vertex to be interpolated by the fragment 每个顶点的法线将被片段插值
	/*****法线贴图******/
	vec4 l;              // light direction in eye coordinates
	vec2 varying_uv[3];  // triangle uv coordinates, written by the vertex shader, read
	/******切线空间法线贴图******/
	vec4 varying_nrm[3]; // normal per vertex to be interpolated by the fragment shader
	vec4 tri[3];         // triangle in view coordinates

	PhongShader(const vec3 light, const Model& m) : model(m) {
		//l = normalized((ModelView * vec4{ light.x, light.y, light.z, 0. }).xyz()); // transform the light vector to view coordinates 将光向量转换为视图坐标
		/*****法线贴图******/
		l = normalized((ModelView * vec4{ light.x, light.y, light.z, 0. })); // transform the light vector to view coordinates
	}

	virtual vec4 vertex(const int face, const int vert) {
		//vec4 v = model.vert(face, vert);                          // current vertex in object coordinates
		/******平滑处理********/
		//vec4 n = model.normal(face, vert);
		//varying_nrm[vert] = (ModelView.invert_transpose() * vec4 { n.x, n.y, n.z, 0. }).xyz();
		//vec4 gl_Position = ModelView * vec4{ v.x, -v.y, v.z, 1. };
		//tri[vert] = gl_Position.xyz();                            // in eye coordinates
		/*****法线贴图******/
		varying_uv[vert] = model.uv(face, vert);
		/*****切线空间法线贴图******/
		varying_nrm[vert] = ModelView.invert_transpose() * model.normal(face, vert);
		vec4 gl_Position = ModelView * model.vert(face, vert);
		gl_Position.y = -gl_Position.y;

		/*****切线空间法线贴图******/
		tri[vert] = gl_Position;
		return Perspective * gl_Position;                         // in clip coordinates
	}

	virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
		//TGAColor gl_FragColor = { 255, 255, 255, 255 };             // output color of the fragment
		//vec3 n = normalized(cross(tri[2] - tri[0], tri[1] - tri[0]));// per-vertex normal 添加光的反射
		/******平滑处理********/
		//vec3 n = normalized(varying_nrm[0] * bar[0] + varying_nrm[1] * bar[1] + varying_nrm[2] * bar[2]);// per-vertex normal 
		// 将2，0反转，
		//vec3 n = normalized(varying_nrm[2] * bar[0] + varying_nrm[1] * bar[1] + varying_nrm[0] * bar[2]);// per-vertex normal	
		//vec3 r = normalized(n * (n * l) * 2 - l);                   // reflected light direction
		/*****法线贴图******/ 
		//vec2 uv = varying_uv[0] * bar[0] + varying_uv[1] * bar[1] + varying_uv[2] * bar[2];
		// 将2，0反转，
		/*****切线空间法线贴图******/
		mat<2, 4> E = { tri[1] - tri[0], tri[2] - tri[0] };
		mat<2, 2> U = { varying_uv[1] - varying_uv[0], varying_uv[2] - varying_uv[0] };
		mat<2, 4> T = U.invert() * E;
		mat<4, 4> D = { normalized(T[0]),  // tangent vector
					  normalized(T[1]),  // bitangent vector
					  normalized(varying_nrm[0] * bar[2] + varying_nrm[1] * bar[1] + varying_nrm[2] * bar[0]), // interpolated normal
					  {0,0,0,1} }; // Darboux frame

		vec2 uv = varying_uv[0] * bar[2] + varying_uv[1] * bar[1] + varying_uv[2] * bar[0];

		//vec4 n = normalized(ModelView.invert_transpose() * model.normal(uv));

		/*****切线空间法线贴图******/
		vec4 n = normalized(D.transpose() * model.normal(uv));
		vec4 r = normalized(n * (n * l) * 2 - l);                   // reflected light direction

		double ambient = .4;                                      // ambient light intensity
		/*******法线*******/
		//double diff = std::max(0., n * l);                        // diffuse light intensity
		//double spec = std::pow(std::max(r.z, 0.), 35);            // specular intensity, note that the camera lies on the z-axis (in eye coordinates), therefore simple r.z, since (0,0,1)*(r.x, r.y, r.z) = r.z
		//for (int channel : {0, 1, 2}){
		//	gl_FragColor[channel] *= std::min(1., ambient + .4 * diff + .9 * spec);
		//	//cout << ambient << " | " << diff << " | " << l << " | " << endl;
		//}

		/*****贴图******/
		double diffuse = 1. * std::max(0., n * l);                 // diffuse light intensity
		double specular = (1. + 3. * sample2D(model.specular(), uv)[0] / 255.) * std::pow(std::max(r.z, 0.), 35);  // specular intensity, note that the camera lies on the z-axis (in eye coordinates), therefore simple r.z, since (0,0,1)*(r.x, r.y, r.z) = r.z
		TGAColor gl_FragColor = sample2D(model.diffuse(), uv);
		//TGAColor gl_FragColor = {255, 255, 255, 255};
		for (int channel : {0, 1, 2})
			gl_FragColor[channel] = std::min<int>(255, gl_FragColor[channel] * (ambient + diffuse + specular));

		return { false, gl_FragColor };                             // do not discard the pixel
	}
};

void drop_zbuffer(std::string filename, std::vector<double>& zbuffer, int width, int height) {
	TGAImage zimg(width, height, TGAImage::GRAYSCALE, { 0,0,0,0 });
	double minz = +1000;
	double maxz = -1000;
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			double z = zbuffer[x + y * width];
			if (z < -100) continue;
			minz = std::min(z, minz);
			maxz = std::max(z, maxz);
		}
	}
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			double z = zbuffer[x + y * width];
			if (z < -100) continue;
			z = (z - minz) / (maxz - minz) * 255;
			zimg.set(x, y, { (std::uint8_t)z, 255, 255, 255 });
		}
	}
	zimg.write_tga_file(filename);
}

Model* model;
RandomShader* randomshader;
PhongShader* phongshader;
BlankShader* shader;

Model* model_2;
PhongShader* phongshader_2;


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
	//model_2 = new Model("../obj/african_head/african_head.obj");
	model = new Model("../obj/diablo3_pose/diablo3_pose.obj");
	//model = new Model("../obj/boggie/body.obj");

	model_2 = new Model("../obj/floor.obj");

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

	shader = new BlankShader{ *model };
	phongshader = new PhongShader(light, *model);

	phongshader_2 = new PhongShader(light, *model_2);
}


void Destory() {
	delete model;
	delete randomshader;
	delete phongshader;
	delete shader;

	delete model_2;
	delete phongshader_2;
}


void ShowModel(SDL_Renderer* renderer)
{
	for (int i = ScreenWidth * ScreenHeight; i--; zbuffer[i] = -std::numeric_limits<float>::max());

	for (int f = 0; f < model->nfaces(); f++) {      // iterate through all facets
		Triangle clip = { phongshader->vertex(f, 0),  // assemble the primitive
						  phongshader->vertex(f, 1),
						  phongshader->vertex(f, 2) };
		rasterize(clip, *phongshader, *renderer);   // rasterize the primitive
	}


	for (int f = 0; f < model_2->nfaces(); f++) {      // iterate through all facets
		//randomshader->color = { (uint8_t)(std::rand() % 255), (uint8_t)(std::rand() % 255), (uint8_t)(std::rand() % 255), 255 };
		Triangle clip = { phongshader_2->vertex(f, 0),  // assemble the primitive
						  phongshader_2->vertex(f, 1),
						  phongshader_2->vertex(f, 2) };
		rasterize(clip, *phongshader_2, *renderer);   // rasterize the primitive
	}

	{ // shadow rendering pass
		constexpr vec3  light{ 1, 1, 1 }; // light source
		constexpr vec3    eye{ -1, 0, 2 }; // camera position
		constexpr vec3 center{ 0, 0, 0 }; // camera direction
		constexpr vec3     up{ 0, 1, 0 }; // camera up vector
		lookat(light, center, up);
		init_perspective(norm(eye - center));
		init_viewport(shadoww / 16, shadowh / 16, shadoww * 7 / 8, shadowh * 7 / 8);
		init_zbuffer(shadoww, shadowh);
		TGAImage trash(shadoww, shadowh, TGAImage::RGB, { 177, 195, 209, 255 });

		for (int f = 0; f < model->nfaces(); f++) {      // iterate through all facets
			Triangle clip = { shader->vertex(f, 0),  // assemble the primitive
							  shader->vertex(f, 1),
							  shader->vertex(f, 2) };
			rasterize(clip, *shader, trash);         // rasterize the primitive
		}

		trash.write_tga_file("shadowmap.tga");
	}

	drop_zbuffer("zbuffer2.tga", zbuffer, shadoww, shadoww);


	std::vector<bool> mask(ScreenWidth * ScreenHeight, false);
	std::vector<double> zbuffer_copy = zbuffer;
	mat<4, 4> M = (Viewport * Perspective * ModelView).invert();
	TGAImage framebuffer(ScreenWidth, ScreenHeight, TGAImage::RGB, { 177, 195, 209, 255 });
	mat<4, 4> N = Viewport * Perspective * ModelView;
	// post-processing
	for (int x = 0; x < ScreenWidth; x++) {
		for (int y = 0; y < ScreenHeight; y++) {
			vec4 fragment = M * vec4{ (double)x, (double)y, zbuffer_copy[x + y * ScreenWidth], 1. };
			vec4 q = N * fragment;
			vec3 p = q.xyz() / q.w;
			bool lit = (fragment.z < -100 ||                                   // it's the background or
				(p.x < 0 || p.x >= shadoww || p.y < 0 || p.y >= shadowh) ||   // it is out of bounds of the shadow buffer
				(p.z > zbuffer[int(p.x) + int(p.y) * shadoww] - .03));  // it is visible
			mask[x + y * ScreenWidth] = lit;
		}
	}

	TGAImage maskimg(ScreenWidth, ScreenHeight, TGAImage::GRAYSCALE);
	for (int x = 0; x < ScreenWidth; x++) {
		for (int y = 0; y < ScreenHeight; y++) {
			if (mask[x + y * ScreenWidth]) continue;
			maskimg.set(x, y, { 255, 255, 255, 255 });
		}
	}
	maskimg.write_tga_file("mask.tga");

	for (int x = 0; x < ScreenWidth; x++) {
		for (int y = 0; y < ScreenHeight; y++) {
			if (mask[x + y * ScreenWidth]) continue;
			TGAColor c = framebuffer.get(x, y);
			vec3 a = { c[0], c[1], c[2] };
			if (norm(a) < 80) continue;
			a = normalized(a) * 80;
			framebuffer.set(x, y, { (std::uint8_t)a[0], (std::uint8_t)a[1], (std::uint8_t)a[2], 255 });
		}
	}
	framebuffer.write_tga_file("shadow.tga");

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
	//SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // 红色
	//SDL_RenderDrawPoint(renderer, 320, 240); // 在屏幕中心绘制

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

