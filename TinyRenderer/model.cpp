#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

/**
 * @brief Model类构造函数 - 从Wavefront OBJ文件加载3D模型
 *
 * 该构造函数读取OBJ文件格式的3D模型，解析顶点和面数据，
 * 支持标准的顶点(v)和面(f)定义格式
 *
 * @param filename OBJ模型文件路径
 */
Model::Model(const char* filename) : verts_(), faces_(), max(1.f){
    // 打开文件流
    std::ifstream in;
    in.open(filename, std::ifstream::in);

    // 检查文件是否成功打开
    if (in.fail()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    std::string line;  // 用于存储每行内容

    // 逐行读取文件，直到文件结束
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());  // 将行转换为字符串流便于解析
        char trash;  // 用于丢弃不需要的字符

        // 解析顶点行 (格式: "v x y z")
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;  // 丢弃'v'字符

            Vec3f v;  // 创建三维浮点向量存储顶点坐标

            // 读取x, y, z三个坐标值
            for (int i = 0; i < 3; i++) {
                iss >> v.raw[i];
                if (v.raw[i] > max) max = v.raw[i];
            }

            verts_.push_back(v);  // 将顶点添加到顶点列表
        }
        // 解析面行 (格式: "f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ...")
        else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;  // 存储面包含的顶点索引
            int itrash, idx;     // itrash用于丢弃纹理/法线索引，idx存储顶点索引

            iss >> trash;  // 丢弃'f'字符

            // 循环读取面的每个顶点定义
            // OBJ格式中面定义通常为: 顶点索引/纹理坐标索引/法线索引
            while (iss >> idx >> trash >> itrash >> trash >> itrash) {
                idx--;  // OBJ索引从1开始，转换为从0开始的C++索引
                f.push_back(idx);  // 将顶点索引添加到面中
            }

            faces_.push_back(f);  // 将面添加到面列表
        }
        // 忽略其他类型的行（如纹理坐标vt、法线vn、材质mtl等）
    }

    // 输出加载统计信息
    std::cerr << "# V: " << verts_.size()
        << " F: " << faces_.size() << std::endl;

    // 文件流会在作用域结束时自动关闭
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
    return faces_[idx];
}

Vec3f Model::vert(int i) {
    return verts_[i];
}
