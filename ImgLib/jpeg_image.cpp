#include "ppm_image.h"

#include <array>
#include <fstream>
#include <setjmp.h>
#include <stdio.h>

#include <jpeglib.h>

using namespace std;

namespace img_lib {

// структура из примера LibJPEG
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr* my_error_ptr;

// функция из примера LibJPEG
METHODDEF(void)
my_error_exit (j_common_ptr cinfo) {
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}


bool SaveJPEG(const Path& file, const Image& image) {
    // Проверяет, что изображение корректное
    if (!image || image.GetWidth() == 0 || image.GetHeight() == 0) {
        return false;
    }

    // Открывает файл
    FILE* outfile = nullptr;
#if defined(_MSC_VER)
    outfile = _wfopen(file.c_str(), L"wb");
#else
    outfile = fopen(file.c_str(), "wb");
#endif
    if (!outfile) {
        return false;
    }

    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;

    // Устанавливает обработчик ошибок
    cinfo.err = jpeg_std_error(&jerr);

    // Инициализирует объект компрессии JPEG
    jpeg_create_compress(&cinfo);

    // Указывает выходной файл
    jpeg_stdio_dest(&cinfo, outfile);

    // Устанавливает параметры изображения
    cinfo.image_width = image.GetWidth();
    cinfo.image_height = image.GetHeight();
    cinfo.input_components = 3;  // Компоненты R, G, B
    cinfo.in_color_space = JCS_RGB; // Цветовое пространство

    // Использует параметры по умолчанию
    jpeg_set_defaults(&cinfo);

    // Начинает компрессию
    jpeg_start_compress(&cinfo, TRUE);

    // Создаёт временный буфер для строки изображения
    std::vector<JSAMPLE> row_buffer(image.GetWidth() * 3);

    // Получает доступ к данным изображения
    for (int y = 0; y < image.GetHeight(); ++y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < image.GetWidth(); ++x) {
            const Color& pixel = line[x];
            row_buffer[x * 3] = static_cast<JSAMPLE>(pixel.r);
            row_buffer[x * 3 + 1] = static_cast<JSAMPLE>(pixel.g);
            row_buffer[x * 3 + 2] = static_cast<JSAMPLE>(pixel.b);
        }

        // Указатель на строку для записи
        JSAMPLE* row_pointer = row_buffer.data();
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    // Завершает компрессию
    jpeg_finish_compress(&cinfo);

    // Освобождает ресурсы
    jpeg_destroy_compress(&cinfo);
    fclose(outfile);

    return true;
}


// тип JSAMPLE фактически псевдоним для unsigned char
void SaveSсanlineToImage(const JSAMPLE* row, int y, Image& out_image) {
    Color* line = out_image.GetLine(y);
    for (int x = 0; x < out_image.GetWidth(); ++x) {
        const JSAMPLE* pixel = row + x * 3;
        line[x] = Color{byte{pixel[0]}, byte{pixel[1]}, byte{pixel[2]}, byte{255}};
    }
}

Image LoadJPEG(const Path& file) {
    jpeg_decompress_struct cinfo;
    my_error_mgr jerr;
    
    FILE* infile;
    JSAMPARRAY buffer;
    int row_stride;

    // Тут не избежать функции открытия файла из языка C,
    // поэтому приходится использовать конвертацию пути к string.
    // Под Visual Studio это может быть опасно, и нужно применить
    // нестандартную функцию _wfopen
#ifdef _MSC_VER
    if ((infile = _wfopen(file.wstring().c_str(), "rb")) == NULL) {
#else
    if ((infile = fopen(file.string().c_str(), "rb")) == NULL) {
#endif
        return {};
    }

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return {};
    }

    jpeg_create_decompress(&cinfo);

    jpeg_stdio_src(&cinfo, infile);

    (void) jpeg_read_header(&cinfo, TRUE);

    cinfo.out_color_space = JCS_RGB;
    cinfo.output_components = 3;

    (void) jpeg_start_decompress(&cinfo);
    
    row_stride = cinfo.output_width * cinfo.output_components;
    
    buffer = (*cinfo.mem->alloc_sarray)
                ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    Image result(cinfo.output_width, cinfo.output_height, Color::Black());

    while (cinfo.output_scanline < cinfo.output_height) {
        int y = cinfo.output_scanline;
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);

        SaveSсanlineToImage(buffer[0], y, result);
    }

    (void) jpeg_finish_decompress(&cinfo);

    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return result;
}

} // of namespace img_lib