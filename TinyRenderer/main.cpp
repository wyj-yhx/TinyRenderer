#include "tgaimage.h"
#include "tinyrenderer.h"
#include "model.h"

#include <iostream>
using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);

int main(int argc, char** argv) {
	
    int width = 500;
    int height = 800;

	TinyRenderer renderer;
	TGAImage image(width, height, TGAImage::RGB);
    Model* model = new Model("../obj/african_head/african_head.obj");

	image.set(52, 41, red);

    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        for (int j = 0; j < 3; j++) {
            Vec3f v0 = model->vert(face[j]);
            Vec3f v1 = model->vert(face[(j + 1) % 3]);
            int x0 = (v0.x / model->max + 1.) * width / 2.;
            int y0 = (v0.y / model->max + 1.) * height / 2.;
            int x1 = (v1.x / model->max + 1.) * width / 2.;
            int y1 = (v1.y / model->max + 1.) * height / 2.;
            renderer.line(x0, y0, x1, y1, image, white);
        }
    }

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");


	return 0;
}
