#include "tgaimage.h"
#include "tinyrenderer.h"
#include "model.h"

#include <iostream>
using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);



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



int main(int argc, char** argv) {
	
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


