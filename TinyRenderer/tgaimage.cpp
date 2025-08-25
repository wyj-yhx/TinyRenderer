#include <iostream>
#include <fstream>
#include <string.h>
#include <time.h>
#include <math.h>
#include "tgaimage.h"

TGAImage::TGAImage() : data(NULL), width(0), height(0), bytespp(0) {
}

TGAImage::TGAImage(int w, int h, int bpp) : data(NULL), width(w), height(h), bytespp(bpp) {
	unsigned long nbytes = width * height * bytespp;
	data = new unsigned char[nbytes];
	//将指针data所指向的内存区域的前nbytes个字节设置为0。
	memset(data, 0, nbytes);
}

TGAImage::TGAImage(const TGAImage& img) {
	width = img.width;
	height = img.height;
	bytespp = img.bytespp;
	unsigned long nbytes = width * height * bytespp;
	data = new unsigned char[nbytes];
	//将图像数据img.data复制到目标数组data中。nbytes表示要复制的字节数
	memcpy(data, img.data, nbytes);
}

TGAImage::~TGAImage() {
	if (data) delete[] data;
}

TGAImage& TGAImage::operator =(const TGAImage& img) {
	if (this != &img) {
		if (data) delete[] data;
		width = img.width;
		height = img.height;
		bytespp = img.bytespp;
		unsigned long nbytes = width * height * bytespp;
		data = new unsigned char[nbytes];
		memcpy(data, img.data, nbytes);
	}
	return *this;
}

bool TGAImage::read_tga_file(const char* filename) {
	// 如果 data 指针不为空（即指向已分配的内存），则释放这块内存，防止内存泄漏
	if (data) delete[] data;
	// 将 data 指针设置为 NULL（或 nullptr，现代 C++ 中更推荐），避免成为悬空指针
	data = NULL;

	// 创建一个输入文件流对象 'in'，用于读取文件
	std::ifstream in;
	// 以二进制模式打开指定文件。std::ios::binary 模式确保文件被原样读取，不进行任何转换（如换行符转换）
	in.open(filename, std::ios::binary);
	// 检查文件是否成功打开
	if (!in.is_open()) {
		// 如果打开失败，向标准错误输出错误信息
		std::cerr << "can't open file " << filename << "\n";
		// 确保文件流被关闭（尽管析构函数通常会做，但显式关闭是好习惯）
		in.close();
		// 返回 false 表示加载失败
		return false;
	}

	// 声明一个 TGA_Header 结构体的变量 'header'，用于存储从文件读取的头部信息
	TGA_Header header;
	// 从文件流中读取 sizeof(header) 字节的数据到 header 变量的内存地址中
	in.read((char*)&header, sizeof(header));
	// 检查上一步的读取操作是否成功（流状态是否良好）
	if (!in.good()) {
		in.close();
		std::cerr << "an error occured while reading the header\n";
		return false;
	}
	
	// 将从头部读取的宽度、高度和每像素字节数赋值给类的成员变量
	width = header.width;
	height = header.height;
	// header.bitsperpixel 是每像素的位数（bit），右移 3 位（除以 8）得到字节数（bytes）
	bytespp = header.bitsperpixel >> 3;

	// 验证读取到的图像参数是否有效：宽度和高度必须为正数，bytespp 必须是预期值（1=灰度, 3=RGB, 4=RGBA）
	if (width <= 0 || height <= 0 || (bytespp != GRAYSCALE && bytespp != RGB && bytespp != RGBA)) {
		in.close();
		std::cerr << "bad bpp (or width/height) value\n";
		return false;
	}


	// 计算图像像素数据部分的总字节数
	unsigned long nbytes = bytespp * width * height;
	// 根据计算出的总字节数，为类的 data 成员动态分配内存
	data = new unsigned char[nbytes];

	// 根据 TGA 文件的数据类型码（datatypecode）来处理图像数据
	// 数据类型码 2 和 3 表示未压缩的 True-color 或灰度图像
	if (3 == header.datatypecode || 2 == header.datatypecode) {
		// 对于未压缩图像，直接从文件流中读取 nbytes 字节到刚分配的 data 内存中,从新的位置（第一次读取结束后的位置）开始读取
		in.read((char*)data, nbytes);
		if (!in.good()) {
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	}
	// 数据类型码 10 和 11 表示使用 Run-Length Encoding (RLE) 压缩的 True-color 或灰度图像
	else if (10 == header.datatypecode || 11 == header.datatypecode) {
		// 调用专门的函数 load_rle_data 来解压缩并读取数据
		if (!load_rle_data(in)) {
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	}
	// 如果数据类型码是其他值，则表示是不支持的格式
	else {
		in.close();
		std::cerr << "unknown file format " << (int)header.datatypecode << "\n";
		return false;
	}

	// 检查图像描述符的第 5 位（0x20）。如果该位为 0，表示图像原点在左下角。
	// 而许多图形系统期望原点在左上角，因此需要垂直翻转图像数据。
	if (!(header.imagedescriptor & 0x20)) {
		flip_vertically(); // 调用成员函数执行垂直翻转
	}

	// 检查图像描述符的第 4 位（0x10）。如果该位为 1，表示需要水平翻转图像。
	if (header.imagedescriptor & 0x10) {
		flip_horizontally(); // 调用成员函数执行水平翻转
	}

	// 打印成功加载的图像信息：宽度 x 高度 / 色彩深度（bit）
	std::cerr << width << "x" << height << "/" << bytespp * 8 << "\n";

	// 关闭文件流
	in.close();

	return true;
}

bool TGAImage::load_rle_data(std::ifstream& in) {
	unsigned long pixelcount = width * height;
	unsigned long currentpixel = 0;
	unsigned long currentbyte = 0;
	TGAColor colorbuffer;
	do {
		unsigned char chunkheader = 0;
		chunkheader = in.get();
		if (!in.good()) {
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
		if (chunkheader < 128) {
			chunkheader++;
			for (int i = 0; i < chunkheader; i++) {
				in.read((char*)colorbuffer.raw, bytespp);
				if (!in.good()) {
					std::cerr << "an error occured while reading the header\n";
					return false;
				}
				for (int t = 0; t < bytespp; t++)
					data[currentbyte++] = colorbuffer.raw[t];
				currentpixel++;
				if (currentpixel > pixelcount) {
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
		else {
			chunkheader -= 127;
			in.read((char*)colorbuffer.raw, bytespp);
			if (!in.good()) {
				std::cerr << "an error occured while reading the header\n";
				return false;
			}
			for (int i = 0; i < chunkheader; i++) {
				for (int t = 0; t < bytespp; t++)
					data[currentbyte++] = colorbuffer.raw[t];
				currentpixel++;
				if (currentpixel > pixelcount) {
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
	} while (currentpixel < pixelcount);
	return true;
}

bool TGAImage::write_tga_file(const char* filename, bool rle) {
	unsigned char developer_area_ref[4] = { 0, 0, 0, 0 };
	unsigned char extension_area_ref[4] = { 0, 0, 0, 0 };
	unsigned char footer[18] = { 'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0' };
	std::ofstream out;
	out.open(filename, std::ios::binary);
	if (!out.is_open()) {
		std::cerr << "can't open file " << filename << "\n";
		out.close();
		return false;
	}
	TGA_Header header;
	memset((void*)&header, 0, sizeof(header));
	header.bitsperpixel = bytespp << 3;
	header.width = width;
	header.height = height;
	header.datatypecode = (bytespp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));
	header.imagedescriptor = 0x20; // top-left origin
	out.write((char*)&header, sizeof(header));
	if (!out.good()) {
		out.close();
		std::cerr << "can't dump the tga file\n";
		return false;
	}
	if (!rle) {
		out.write((char*)data, width * height * bytespp);
		if (!out.good()) {
			std::cerr << "can't unload raw data\n";
			out.close();
			return false;
		}
	}
	else {
		if (!unload_rle_data(out)) {
			out.close();
			std::cerr << "can't unload rle data\n";
			return false;
		}
	}
	out.write((char*)developer_area_ref, sizeof(developer_area_ref));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write((char*)extension_area_ref, sizeof(extension_area_ref));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write((char*)footer, sizeof(footer));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.close();
	return true;
}

// TODO: it is not necessary to break a raw chunk for two equal pixels (for the matter of the resulting size)
bool TGAImage::unload_rle_data(std::ofstream& out) {
	const unsigned char max_chunk_length = 128;
	unsigned long npixels = width * height;
	unsigned long curpix = 0;
	while (curpix < npixels) {
		unsigned long chunkstart = curpix * bytespp;
		unsigned long curbyte = curpix * bytespp;
		unsigned char run_length = 1;
		bool raw = true;
		while (curpix + run_length < npixels && run_length < max_chunk_length) {
			bool succ_eq = true;
			for (int t = 0; succ_eq && t < bytespp; t++) {
				succ_eq = (data[curbyte + t] == data[curbyte + t + bytespp]);
			}
			curbyte += bytespp;
			if (1 == run_length) {
				raw = !succ_eq;
			}
			if (raw && succ_eq) {
				run_length--;
				break;
			}
			if (!raw && !succ_eq) {
				break;
			}
			run_length++;
		}
		curpix += run_length;
		out.put(raw ? run_length - 1 : run_length + 127);
		if (!out.good()) {
			std::cerr << "can't dump the tga file\n";
			return false;
		}
		out.write((char*)(data + chunkstart), (raw ? run_length * bytespp : bytespp));
		if (!out.good()) {
			std::cerr << "can't dump the tga file\n";
			return false;
		}
	}
	return true;
}

TGAColor TGAImage::get(int x, int y) {
	if (!data || x < 0 || y < 0 || x >= width || y >= height) {
		return TGAColor();
	}
	return TGAColor(data + (x + y * width) * bytespp, bytespp);
}

bool TGAImage::set(int x, int y, TGAColor c) {
	if (!data || x < 0 || y < 0 || x >= width || y >= height) {
		return false;
	}
	memcpy(data + (x + y * width) * bytespp, c.raw, bytespp);
	return true;
}

int TGAImage::get_bytespp() {
	return bytespp;
}

int TGAImage::get_width() {
	return width;
}

int TGAImage::get_height() {
	return height;
}

bool TGAImage::flip_horizontally() {
	if (!data) return false;
	int half = width >> 1;
	for (int i = 0; i < half; i++) {
		for (int j = 0; j < height; j++) {
			TGAColor c1 = get(i, j);
			TGAColor c2 = get(width - 1 - i, j);
			set(i, j, c2);
			set(width - 1 - i, j, c1);
		}
	}
	return true;
}

bool TGAImage::flip_vertically() {
	if (!data) return false;
	unsigned long bytes_per_line = width * bytespp;
	unsigned char* line = new unsigned char[bytes_per_line];
	int half = height >> 1;
	for (int j = 0; j < half; j++) {
		unsigned long l1 = j * bytes_per_line;
		unsigned long l2 = (height - 1 - j) * bytes_per_line;
		memmove((void*)line, (void*)(data + l1), bytes_per_line);
		memmove((void*)(data + l1), (void*)(data + l2), bytes_per_line);
		memmove((void*)(data + l2), (void*)line, bytes_per_line);
	}
	delete[] line;
	return true;
}

unsigned char* TGAImage::buffer() {
	return data;
}

void TGAImage::clear() {
	memset((void*)data, 0, width * height * bytespp);
}

bool TGAImage::scale(int w, int h) {
	if (w <= 0 || h <= 0 || !data) return false;
	unsigned char* tdata = new unsigned char[w * h * bytespp];
	int nscanline = 0;
	int oscanline = 0;
	int erry = 0;
	unsigned long nlinebytes = w * bytespp;
	unsigned long olinebytes = width * bytespp;
	for (int j = 0; j < height; j++) {
		int errx = width - w;
		int nx = -bytespp;
		int ox = -bytespp;
		for (int i = 0; i < width; i++) {
			ox += bytespp;
			errx += w;
			while (errx >= (int)width) {
				errx -= width;
				nx += bytespp;
				memcpy(tdata + nscanline + nx, data + oscanline + ox, bytespp);
			}
		}
		erry += h;
		oscanline += olinebytes;
		while (erry >= (int)height) {
			if (erry >= (int)height << 1) // it means we jump over a scanline
				memcpy(tdata + nscanline + nlinebytes, tdata + nscanline, nlinebytes);
			erry -= height;
			nscanline += nlinebytes;
		}
	}
	delete[] data;
	data = tdata;
	width = w;
	height = h;
	return true;
}
