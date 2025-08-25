#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <fstream>

// 告诉编译器取消内存对齐优化，强制结构体成员按1字节对齐（紧密排列）
// 这是为了确保结构体在内存中的布局与磁盘上TGA文件头的原始字节序列完全一致
// 这样可以直接将文件二进制内容读取到该结构体中，无需手动解析每个字段
#pragma pack(push, 1) // 将当前对齐设置压入栈，并设置新的对齐方式为1字节

// 定义TGA文件头结构体
struct TGA_Header {
	char idlength;          // 图像ID字段的长度（字节数）
							// 如果为0，表示没有图像ID字段

	char colormaptype;      // 颜色表类型
							// 0 = 没有颜色表
							// 1 = 有颜色表

	char datatypecode;      // 图像数据类型码（非常重要）
							// 2 = 未压缩的RGB图像
							// 3 = 未压缩的黑白图像
							// 10 = 使用RLE压缩的RGB图像
							// 11 = 使用RLE压缩的黑白图像
							// （还有其他较少用的类型）

	short colormaporigin;   // 颜色表的起始索引/入口（通常为0）
	short colormaplength;   // 颜色表中的颜色数量
							// 如果colormaptype为0，则此项应为0

	char colormapdepth;     // 颜色表中每个颜色的位数（深度）
							// 15, 16, 24, 或32位

	short x_origin;         // 图像左下角的X坐标（通常为0）
	short y_origin;         // 图像左下角的Y坐标（通常为0）

	short width;            // 图像的宽度（以像素为单位）
	short height;           // 图像的高度（以像素为单位）

	char  bitsperpixel;     // 每个像素的位数（色彩深度）
							// 8 = 灰度，16 = ARGB1555，24 = RGB，32 = RGBA

	char  imagedescriptor;  // 图像描述符（位字段）
							// 位0-3: alpha通道的位数（对于某些格式）
							// 位4: 保留（通常为0）
							// 位5: 屏幕原点标志
							//       0 = 原点在左下角
							//       1 = 原点在左上角
							// 位6-7: 交叉存储标志（较少使用）
};

// 恢复编译器之前的内存对齐设置
// pop出之前push的对齐设置，恢复默认的内存对齐方式
#pragma pack(pop)



struct TGAColor {
	// 联合体 取最大的变量的内存大小
	// 如果向val赋值0x01020304，那么raw数组的内容将分别是0x01、0x02、0x03和0x04，同时b、g、r和a的值也将分别是这些值。这种特性常用于底层编程和图像处理等场景。
	union {
		struct {
			unsigned char b, g, r, a;
		};
		unsigned char raw[4];
		unsigned int val;
	};
	int bytespp;

	TGAColor() : val(0), bytespp(1) {
	}

	TGAColor(unsigned char R, unsigned char G, unsigned char B, unsigned char A) : b(B), g(G), r(R), a(A), bytespp(4) {
	}

	TGAColor(int v, int bpp) : val(v), bytespp(bpp) {
	}

	TGAColor(const TGAColor& c) : val(c.val), bytespp(c.bytespp) {
	}

	TGAColor(const unsigned char* p, int bpp) : val(0), bytespp(bpp) {
		for (int i = 0; i < bpp; i++) {
			raw[i] = p[i];
		}
	}

	TGAColor& operator =(const TGAColor& c) {
		if (this != &c) {
			bytespp = c.bytespp;
			val = c.val;
		}
		return *this;
	}
};


class TGAImage {
protected:
	unsigned char* data;
	int width;
	int height;
	int bytespp;

	bool   load_rle_data(std::ifstream& in);
	bool unload_rle_data(std::ofstream& out);
public:
	enum Format {
		GRAYSCALE = 1, RGB = 3, RGBA = 4
	};

	TGAImage();
	TGAImage(int w, int h, int bpp);
	TGAImage(const TGAImage& img);
	bool read_tga_file(const char* filename);
	bool write_tga_file(const char* filename, bool rle = true);
	bool flip_horizontally();
	bool flip_vertically();
	bool scale(int w, int h);
	TGAColor get(int x, int y);
	bool set(int x, int y, TGAColor c);
	~TGAImage();
	TGAImage& operator =(const TGAImage& img);
	int get_width();
	int get_height();
	int get_bytespp();
	unsigned char* buffer();
	void clear();
};

#endif //__IMAGE_H__