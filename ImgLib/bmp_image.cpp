#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <iostream>
#include <string_view>

using namespace std;

namespace img_lib {
    // функция вычисления отступа по ширине
    static int GetBMPStride(int w) {
        const uint8_t PIXEL_SIZE = 3;
        const uint8_t BLOCK_SIZE = 4;
        const uint8_t STRIDE_SIZE = 4;
        return STRIDE_SIZE * ((w * PIXEL_SIZE + PIXEL_SIZE) / BLOCK_SIZE);
    }

    PACKED_STRUCT_BEGIN BitmapFileHeader{
        BitmapFileHeader(int width, int height) {
            bfSize = GetBMPStride(width) * height;
        }
        char bfType[2] = {'B', 'M'};
        uint32_t bfSize = {};
        uint32_t bfReserved = 0;
        uint32_t bfOffBits = 54;
    }
    PACKED_STRUCT_END

    PACKED_STRUCT_BEGIN BitmapInfoHeader {
        // поля заголовка Bitmap Info Header
        BitmapInfoHeader(int width, int height)
            : biWidth(width), biHeight(height) {
            biSizeImage = GetBMPStride(width) * height;
        }
        uint32_t biSize = 40;
        int32_t biWidth = {};
        int32_t biHeight = {};
        uint16_t biPlanes = 1;
        uint16_t biBitCount = 24;
        uint32_t biCompression = 0;
        uint32_t biSizeImage = {};
        int32_t biXPelsPerMeter = 11811;
        int32_t biYPelsPerMeter = 11811;
        int32_t biClrUsed = 0;
        int32_t biClrImportant = 0x1000000;
    }
    PACKED_STRUCT_END

    bool SaveBMP(const Path& file, const Image& image) {
        BitmapFileHeader file_header(image.GetWidth(), image.GetHeight());
        BitmapInfoHeader info_header(image.GetWidth(), image.GetHeight());

        // Добавляет размер заголовков к размеру изображения
        file_header.bfSize += sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);

        ofstream out(file, ios::binary);
        if (!out) {
            return false;
        }

        // Записывает заголовки
        out.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
        out.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));

        const int stride = GetBMPStride(image.GetWidth());
        vector<char> buff(stride, 0);

        // Записывает данные изображения "строка за строкой", снизу вверх
        for (int y = image.GetHeight() - 1; y >= 0; --y) {
            const Color* line = image.GetLine(y);

            for (int x = 0; x < image.GetWidth(); ++x) {
                buff[x * 3 + 0] = static_cast<char>(line[x].b); // B
                buff[x * 3 + 1] = static_cast<char>(line[x].g); // G
                buff[x * 3 + 2] = static_cast<char>(line[x].r); // R
            }

            out.write(buff.data(), stride);
        }

        return out.good();
    }

    Image LoadBMP(const Path& file) {
        std::ifstream in(file, std::ios::binary);
        if (!in) {
            return {};
        }

        // Читает заголовки
        BitmapFileHeader file_header(0, 0);
        BitmapInfoHeader info_header(0, 0);
        if (!in.read(reinterpret_cast<char*>(&file_header), sizeof(file_header))) return {};
        if (in.read(reinterpret_cast<char*>(&info_header), sizeof(info_header))) return {};



        if (file_header.bfType != "BM" or info_header.biBitCount != 24 or info_header.biCompression != 0) {
            return {};
        }

        const int width = info_header.biWidth;
        const int height = info_header.biHeight;
        const int stride = GetBMPStride(width);

        Image result(width, height, Color::Black());
        std::vector<char> buffer(stride);

        // Читает строки изображения
        for (int y = height - 1; y >= 0; --y) {
            Color* line = result.GetLine(y);
            in.read(buffer.data(), stride);
            for (int x = 0; x < width; ++x) {
                line[x].b = static_cast<std::byte>(buffer[x * 3 + 0]);
                line[x].g = static_cast<std::byte>(buffer[x * 3 + 1]);
                line[x].r = static_cast<std::byte>(buffer[x * 3 + 2]);
            }
        }
        if (!in) {
            return {};
        }
        return result;
    }

};  // namespace img_lib