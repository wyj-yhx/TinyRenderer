#include "tinyrenderer.h"

#include <vector>
#include <cmath>
#include <cstdlib>
#include <limits>


/// <summary>
/// 绘制一条直线
/// </summary>
/// <param name="x0"></param>
/// <param name="y0"></param>
/// <param name="x1"></param>
/// <param name="y1"></param>
/// <param name="image"></param>
/// <param name="color"></param>
void TinyRenderer::line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) {
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int dx = x1 - x0;
    int dy = y1 - y0;
    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = y0;
    const int yincr = (y1 > y0 ? 1 : -1);
    for (int x = x0; x <= x1; x++) {
        if (steep) {
            image.set(y, x, color);
        }
        else {
            image.set(x, y, color);
        }
        error2 += derror2;
        if (error2 > dx) {
            y += yincr;
            error2 -= dx * 2;
        }
    }
}

/**
 * 计算带符号的三角形面积（实际是2倍面积，带符号）
 * 利用鞋带公式的变体，返回值为实际面积的2倍，符号表示顶点顺序（顺时针/逆时针）
 */
double TinyRenderer::signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return .5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) + (ay - cy) * (ax + cx));
}

/**
 * 在帧缓冲器中绘制填充三角形
 * 使用重心坐标法进行光栅化，支持背面剔除
 */
void TinyRenderer::triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage& framebuffer, TGAColor color) {
    // 计算三角形的包围盒（最小矩形区域）
    int bbminx = std::min({ax, bx, cx}); // 包围盒左上角x坐标
    int bbminy = std::min({ay, by, cy}); // 包围盒左上角y坐标
    int bbmaxx = std::max({ax, bx, cx}); // 包围盒右下角x坐标
    int bbmaxy = std::max({ay, by, cy}); // 包围盒右下角y坐标

    // 计算整个三角形的有符号面积（实际是2倍面积）
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);

    // 背面剔除：如果面积为负（顺时针顺序）或面积太小（小于1像素），则不绘制
    if (total_area < 1) return;

    // 使用OpenMP并行化加速像素处理
#pragma omp parallel for
// 遍历包围盒内的所有像素
    for (int x = bbminx; x <= bbmaxx; x++) {
        for (int y = bbminy; y <= bbmaxy; y++) {
            // 计算当前像素相对于三角形的三个重心坐标
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;

            // 如果任一重心坐标为负，说明像素在三角形外
            if (alpha < 0 || beta < 0 || gamma < 0) continue;

            // 像素在三角形内，设置帧缓冲器中的颜色
            framebuffer.set(x, y, color);
        }
    }
}

void TinyRenderer::triangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, TGAImage& framebuffer) {
    int bbminx = std::min({ax, bx, cx}); // bounding box for the triangle
    int bbminy = std::min({ay, by, cy}); // defined by its top left and bottom right corners
    int bbmaxx = std::max({ax, bx, cx});
    int bbmaxy = std::max({ay, by, cy});
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
    if (total_area < 1) return; // backface culling + discarding triangles that cover less than a pixel

#pragma omp parallel for
    for (int x = bbminx; x <= bbmaxx; x++) {
        for (int y = bbminy; y <= bbmaxy; y++) {
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            if (alpha < 0 || beta < 0 || gamma < 0) continue; // negative barycentric coordinate => the pixel is outside the triangle
            unsigned char z = static_cast<unsigned char>(alpha * az + beta * bz + gamma * cz);
            framebuffer.set(x, y, { z });
        }
    }
}


//==============================================SDL_Renderer==========================================================


/**
 * 在帧缓冲器中绘制填充三角形
 * 使用重心坐标法进行光栅化，支持背面剔除
 */
void TinyRenderer::triangle(int ax, int ay, int bx, int by, int cx, int cy, SDL_Renderer* renderer, TGAColor color) {
    // 计算三角形的包围盒（最小矩形区域）
    int bbminx = std::min({ ax, bx, cx }); // 包围盒左上角x坐标
    int bbminy = std::min({ ay, by, cy }); // 包围盒左上角y坐标
    int bbmaxx = std::max({ ax, bx, cx }); // 包围盒右下角x坐标
    int bbmaxy = std::max({ ay, by, cy }); // 包围盒右下角y坐标

    // 计算整个三角形的有符号面积（实际是2倍面积）
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);

    // 背面剔除：如果面积为负（顺时针顺序）或面积太小（小于1像素），则不绘制
    if (total_area < 1) return;

    // 使用OpenMP并行化加速像素处理
#pragma omp parallel for
// 遍历包围盒内的所有像素
    for (int x = bbminx; x <= bbmaxx; x++) {
        for (int y = bbminy; y <= bbmaxy; y++) {
            // 计算当前像素相对于三角形的三个重心坐标
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;

            // 如果任一重心坐标为负，说明像素在三角形外
            if (alpha < 0 || beta < 0 || gamma < 0) continue;

            SDL_SetRenderDrawColor(renderer, color[0], color[1],color[2],color[3]);
            // 像素在三角形内，设置帧缓冲器中的颜色
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }
}


void TinyRenderer::triangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, SDL_Renderer* renderer, float* zbuffer) {
    int bbminx = std::min({ax, bx, cx}); // bounding box for the triangle
    int bbminy = std::min({ay, by, cy}); // defined by its top left and bottom right corners
    int bbmaxx = std::max({ax, bx, cx});
    int bbmaxy = std::max({ay, by, cy});
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
    if (total_area < 1) return; // backface culling + discarding triangles that cover less than a pixel

    //SDL_SetRenderDrawColor(renderer, std::rand() % 255, std::rand() % 255, std::rand() % 255, 255); // 设置颜色
#pragma omp parallel for
    for (int x = bbminx; x <= bbmaxx; x++) {
        for (int y = bbminy; y <= bbmaxy; y++) {
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            if (alpha < 0 || beta < 0 || gamma < 0) continue; // negative barycentric coordinate => the pixel is outside the triangle
            unsigned char z = static_cast<unsigned char>(alpha * az + beta * bz + gamma * cz);
            //framebuffer.set(x, y, { z });
            //TGAColor color = { alpha * az, beta * bz, gamma * cz, 255};

            if (zbuffer[int(x + y * ScreenWidth)] < z) {
                zbuffer[int(x + y * ScreenWidth)] = z;

                SDL_SetRenderDrawColor(renderer, z, z, z, 255); // 设置颜色
                SDL_RenderDrawPoint(renderer, x, y);     //绘制点
            }

        }
    }
}










// 叉乘公式：a × b = (ay*bz - az*by, az*bx - ax*bz, ax*by - ay*bx)
vec3 TinyRenderer::cross(const vec4& a, const vec4& b) {
    return vec3{
        a.y * b.z - a.z * b.y,  // x分量
        a.z * b.x - a.x * b.z,  // y分量  
        a.x * b.y - a.y * b.x   // z分量
    };
}
// 重心坐标
vec3 TinyRenderer::barycentric(vec3* pts, vec3 P) {
    vec3 u = cross(vec4{ pts[2][0] - pts[0][0], pts[1][0] - pts[0][0], pts[0][0] - P[0] }, vec4{ pts[2][1] - pts[0][1], pts[1][1] - pts[0][1], pts[0][1] - P[1] });
    if (std::abs(u.z) < 1) return vec3{ -1., 1., 1. };
    return vec3{ 1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z };
}


/// <summary>
/// 绘制三维空间的三角形
/// </summary>
/// <param name="pts"></param>
/// <param name="zbuffer"></param>
/// <param name="image"></param>
/// <param name="color"></param>
void TinyRenderer::triangle(vec3* pts, float* zbuffer, SDL_Renderer* renderer, TGAColor color) {
    vec2 bboxmin{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    vec2 bboxmax{ -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };
    vec2 clamp{ ScreenWidth - 1, ScreenHeight - 1 };
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::max(static_cast < double>(0.f), std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }
    vec3 P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            vec3 bc_screen = barycentric(pts, P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
            P.z = 0;
            for (int i = 0; i < 3; i++) P.z += pts[i][2] * bc_screen[i];
            if (zbuffer[int(P.x + P.y * ScreenWidth)] < P.z) {
                zbuffer[int(P.x + P.y * ScreenWidth)] = P.z;

                SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], color[3]); // 设置颜色
                SDL_RenderDrawPoint(renderer, P.x, P.y);     //绘制点
            }
        }
    }
}