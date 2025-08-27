#pragma once

#include "tgaimage.h"

class TinyRenderer
{
public:
	void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color);
};