#include <img_lib.h>
#include <bmp_image.h>
#include <jpeg_image.h>
#include <ppm_image.h>
#include <filesystem>
#include <string_view>
#include <iostream>

using namespace std;

// Пространство имён для интерфейсов форматов
namespace FormatInterfaces {

class ImageFormatInterface {
public:
    virtual ~ImageFormatInterface() = default;
    virtual bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const = 0;
    virtual img_lib::Image LoadImage(const img_lib::Path& file) const = 0;
};

class PPMFormat : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SavePPM(file, image);
    }

    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadPPM(file);
    }
};

class JPEGFormat : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveJPEG(file, image);
    }

    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadJPEG(file);
    }
};
    
class BMPFormat : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveBMP(file, image);
    }

    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadBMP(file);
    }
};

} // namespace FormatInterfaces

// Определение формата файла по расширению
enum class Format {
    JPEG,
    PPM,
    BMP,
    UNKNOWN
};

Format GetFormatByExtension(const img_lib::Path& input_file) {
    const string ext = input_file.extension().string();
    if (ext == ".jpg"sv || ext == ".jpeg"sv) {
        return Format::JPEG;
    }
    if (ext == ".ppm"sv) {
        return Format::PPM;
    }
    if (ext == ".bmp"sv) {
        return Format::BMP;
    }
    
    return Format::UNKNOWN;
}

// Возвращает интерфейс для обработки форматов изображений
const FormatInterfaces::ImageFormatInterface* GetFormatInterface(const img_lib::Path& path) {

    switch (GetFormatByExtension(path)) {
        case Format::JPEG:
            static const FormatInterfaces::JPEGFormat jpegInterface;
            return &jpegInterface;
        case Format::PPM:
            static const FormatInterfaces::PPMFormat ppmInterface;
            return &ppmInterface;
        case Format::BMP:
            static const FormatInterfaces::BMPFormat bmpInterface;
            return &bmpInterface;
        default:
            return nullptr;
    }
}

int main(int argc, const char** argv) {
    if (argc != 3) {
        cerr << "Usage: "sv << argv[0] << " <in_file> <out_file>"sv << endl;
        return 1;
    }

    img_lib::Path in_path = argv[1];
    img_lib::Path out_path = argv[2];

    // Определяем форматы входного и выходного файлов
    const FormatInterfaces::ImageFormatInterface* inputFormat = GetFormatInterface(in_path);
    if (!inputFormat) {
        cerr << "Unknown format of the input file"sv << endl;
        return 2;
    }

    const FormatInterfaces::ImageFormatInterface* outputFormat = GetFormatInterface(out_path);
    if (!outputFormat) {
        cerr << "Unknown format of the output file"sv << endl;
        return 3;
    }

    // Загружаем изображение
    img_lib::Image image = inputFormat->LoadImage(in_path);
    if (!image) {
        cerr << "Loading failed"sv << endl;
        return 4;
    }

    // Сохраняем изображение
    if (!outputFormat->SaveImage(out_path, image)) {
        cerr << "Saving failed"sv << endl;
        return 5;
    }

    cout << "Successfully converted"sv << endl;
    return 0;
}