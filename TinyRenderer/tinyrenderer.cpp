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


/// <summary>
/// 计算三角形得重心
/// </summary>
/// <param name="pts"></param>
/// <param name="P"></param>
/// <returns></returns>
Vec3f TinyRenderer::barycentric(Vec2i* pts, Vec2i P) {

    Vec3f u = Vec3f(pts[2].raw[0] - pts[0].raw[0], pts[1].raw[0] - pts[0].raw[0], pts[0].raw[0] - P.raw[0]) ^ Vec3f(pts[2].raw[1] - pts[0].raw[1], pts[1].raw[1] - pts[0].raw[1], pts[0].raw[1] - P.raw[1]);
    /* `pts` and `P` has integer value as coordinates
       so `abs(u[2])` < 1 means `u[2]` is 0, that means
       triangle is degenerate, in this case return something with negative coordinates */
    if (std::abs(u.z) < 1) return Vec3f(-1, 1, 1);
    return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}


/// <summary>
/// 绘制填充三角形，重心
/// </summary>
/// <param name="pts"></param>
/// <param name="image"></param>
/// <param name="color"></param>
void TinyRenderer::triangle(Vec2i* pts, TGAImage& image, TGAColor color) {
    Vec2i bboxmin(image.get_width() - 1, image.get_height() - 1);
    Vec2i bboxmax(0, 0);
    Vec2i clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) {
        bboxmin.x = std::max(0, std::min(bboxmin.x, pts[i].x));
        bboxmin.y = std::max(0, std::min(bboxmin.y, pts[i].y));

        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i].x));
        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i].y));
    }
    Vec2i P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(pts, P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
            image.set(P.x, P.y, color);
        }
    }
}

/// <summary>
/// 绘制填充三角形， 扫线方法
/// </summary>
/// <param name="t0"></param>
/// <param name="t1"></param>
/// <param name="t2"></param>
/// <param name="image"></param>
/// <param name="color"></param>
void TinyRenderer::triangle_scanline(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color) {
    if (t0.y == t1.y && t0.y == t2.y) return; // I dont care about degenerate triangles 
    // sort the vertices, t0, t1, t2 lower−to−upper (bubblesort yay!) 
    if (t0.y > t1.y) std::swap(t0, t1);
    if (t0.y > t2.y) std::swap(t0, t2);
    if (t1.y > t2.y) std::swap(t1, t2);
    int total_height = t2.y - t0.y;
    for (int i = 0; i < total_height; i++) {
        bool second_half = i > t1.y - t0.y || t1.y == t0.y;
        int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
        float alpha = (float)i / total_height;
        float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height; // be careful: with above conditions no division by zero here 
        Vec2i A = t0 + (t2 - t0) * alpha;
        Vec2i B = second_half ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;
        if (A.x > B.x) std::swap(A, B);
        for (int j = A.x; j <= B.x; j++) {
            image.set(j, t0.y + i, color); // attention, due to int casts t0.y+i != A.y 
        }
    }
}

/// <summary>
/// 三维空间三角形的重心
/// </summary>
/// <param name="A"></param>
/// <param name="B"></param>
/// <param name="C"></param>
/// <param name="P"></param>
/// <returns></returns>
Vec3f TinyRenderer::barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P) {
    Vec3f s[2];
    for (int i = 1; i >= 0; i--) {
        s[i].x = C.raw[i] - A.raw[i];
        s[i].y = B.raw[i] - A.raw[i];
        s[i].z = A.raw[i] - P.raw[i];
    }
    Vec3f u = s[0] ^ s[1];
    if (std::abs(u.raw[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate,不要忘记u[2]是一个整数。如果它是零，那么三角形ABC是简并的
        return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator,在这种情况下生成负坐标，它将被栅格化器丢弃
}

/// <summary>
/// 绘制三维空间的三角形
/// </summary>
/// <param name="pts"></param>
/// <param name="zbuffer"></param>
/// <param name="image"></param>
/// <param name="color"></param>
void TinyRenderer::triangle(Vec3f* pts, float* zbuffer, TGAImage& image, TGAColor color) {
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin.raw[j] = std::max(0.f, std::min(bboxmin.raw[j], pts[i].raw[j]));
            bboxmax.raw[j] = std::min(clamp.raw[j], std::max(bboxmax.raw[j], pts[i].raw[j]));
        }
    }
    Vec3f P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(pts[0], pts[1], pts[2], P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
            P.z = 0;
            for (int i = 0; i < 3; i++) P.z += pts[i].raw[2] * bc_screen.raw[i];
            if (zbuffer[int(P.x + P.y * ScreenWidth)] < P.z) {
                zbuffer[int(P.x + P.y * ScreenWidth)] = P.z;
                image.set(P.x, P.y, color);
            }
        }
    }
}

//////////////////////////////////////////////////////////使用SDL绘制图像///////////////////////////////////////////////////////////

/// <summary>
/// 绘制一条直线
/// </summary>
/// <param name="x0"></param>
/// <param name="y0"></param>
/// <param name="x1"></param>
/// <param name="y1"></param>
/// <param name="image"></param>
/// <param name="color"></param>
void TinyRenderer::line(int x0, int y0, int x1, int y1, SDL_Renderer* renderer, TGAColor color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a); // 设置颜色
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
            SDL_RenderDrawPoint(renderer, x, y);     //绘制点
            //image.set(y, x, color);
        }
        else {
            SDL_RenderDrawPoint(renderer, y, x);     //绘制点
            /*image.set(x, y, color);*/
        }
        error2 += derror2;
        if (error2 > dx) {
            y += yincr;
            error2 -= dx * 2;
        }
    }
}


/// <summary>
/// 绘制填充三角形，重心
/// </summary>
/// <param name="pts"></param>
/// <param name="image"></param>
/// <param name="color"></param>
void TinyRenderer::triangle(Vec2i* pts, SDL_Renderer* renderer, TGAColor color) {

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a); // 设置颜色
    Vec2i bboxmin(ScreenWidth - 1, ScreenHeight - 1);
    Vec2i bboxmax(0, 0);
    Vec2i clamp(ScreenWidth - 1, ScreenHeight - 1);
    for (int i = 0; i < 3; i++) {
        bboxmin.x = std::max(0, std::min(bboxmin.x, pts[i].x));
        bboxmin.y = std::max(0, std::min(bboxmin.y, pts[i].y));

        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i].x));
        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i].y));
    }
    Vec2i P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(pts, P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
            SDL_RenderDrawPoint(renderer, P.x, P.y);     //绘制点，以屏幕左下角为基点
            //image.set(P.x, P.y, color);
        }
    }

}


/// <summary>
/// 绘制填充三角形， 扫线方法
/// </summary>
/// <param name="t0"></param>
/// <param name="t1"></param>
/// <param name="t2"></param>
/// <param name="image"></param>
/// <param name="color"></param>
void TinyRenderer::triangle_scanline(Vec2i t0, Vec2i t1, Vec2i t2,SDL_Renderer* renderer, TGAColor color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a); // 设置颜色
    if (t0.y == t1.y && t0.y == t2.y) return; // I dont care about degenerate triangles 
    // sort the vertices, t0, t1, t2 lower−to−upper (bubblesort yay!) 
    if (t0.y > t1.y) std::swap(t0, t1);
    if (t0.y > t2.y) std::swap(t0, t2);
    if (t1.y > t2.y) std::swap(t1, t2);
    int total_height = t2.y - t0.y;
    for (int i = 0; i < total_height; i++) {
        bool second_half = i > t1.y - t0.y || t1.y == t0.y;
        int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
        float alpha = (float)i / total_height;
        float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height; // be careful: with above conditions no division by zero here 
        Vec2i A = t0 + (t2 - t0) * alpha;
        Vec2i B = second_half ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;
        if (A.x > B.x) std::swap(A, B);
        for (int j = A.x; j <= B.x; j++) {
            SDL_RenderDrawPoint(renderer, j, t0.y + i);     //绘制点
            //image.set(j, t0.y + i, color); // attention, due to int casts t0.y+i != A.y 
        }
    }
}



/// <summary>
/// 绘制三维空间的三角形
/// </summary>
/// <param name="pts"></param>
/// <param name="zbuffer"></param>
/// <param name="image"></param>
/// <param name="color"></param>
void TinyRenderer::triangle(Vec3f* pts, float* zbuffer, SDL_Renderer* renderer, TGAColor color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a); // 设置颜色
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(ScreenWidth - 1, ScreenHeight - 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin.raw[j] = std::max(0.f, std::min(bboxmin.raw[j], pts[i].raw[j]));
            bboxmax.raw[j] = std::min(clamp.raw[j], std::max(bboxmax.raw[j], pts[i].raw[j]));
        }
    }
    Vec3f P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(pts[0], pts[1], pts[2], P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
            P.z = 0;
            for (int i = 0; i < 3; i++) P.z += pts[i].raw[2] * bc_screen.raw[i];
            if (zbuffer[int(P.x + P.y * ScreenWidth)] < P.z) {
                zbuffer[int(P.x + P.y * ScreenWidth)] = P.z;
                SDL_RenderDrawPoint(renderer, P.x, P.y);     //绘制点
                //image.set(P.x, P.y, color);
            }
        }
    }
}