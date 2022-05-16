
#pragma once

#include <memory>
#include <eigen3/Eigen/Core>
#include "core/geometry/geometry.h"
#include "core/render-core/spectrum.h"
#include "tinyformat/tinyformat.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

class ImageBlock {
public:
    ImageBlock() = delete;
    ImageBlock& operator=(const ImageBlock&) = delete;
    ImageBlock(const ImageBlock&) = delete;

    ~ImageBlock() {
        for (int i = 0; i < size.x; ++i)
            delete[] pixels[i];
        delete[] pixels;
    }

    ImageBlock(const Vector2i &_offset, const Vector2i &_size) : offset(_offset), size(_size) {
        pixels = new SpectrumRGB*[size.x];
        for (int i = 0; i < size.x; ++i)
            pixels[i] = new SpectrumRGB[size.y];
    }

    void setPixel(const Vector2i &pos, const SpectrumRGB &rgb) {
        pixels[pos.x][pos.y] = rgb;
    }

    /**
     * @brief put a small block into a big one
     * 
     * @param block 
     * @return ImageBlock 
     */
    friend class Image;
    friend std::ostream& operator<<(std::ostream &os, const ImageBlock &block);

private:
    Vector2i offset, size;
    SpectrumRGB **pixels {nullptr};
};

inline std::ostream& operator<<(std::ostream &os, const ImageBlock &block) {
    os << "Image block :\n"
       << "\toffset = " << block.offset << "\n"
       << "\tsize   = " << block.size << "\n";
    for (int i = 0; i < block.size.y; ++i){
        for (int j = 0; j < block.size.x; ++j) {
            os << block.pixels[j][i] << "\t";
        }
        os << std::endl;
    }
    return os;
}


class Image {
public:
    Image() = delete;

    Image(const Image &img) = delete;

    Image& operator=(const Image &img) = delete;

    Image(Vector2i _size, unsigned _channels = 3) : 
        size(_size), channels(_channels), screen(Vector2i(0, 0), _size) { }
    
    void putBlock(const ImageBlock &block) {
        Vector2i rightDown = block.offset + block.size;
        if (rightDown.x > size.x || rightDown.y > size.y) {
            std::cerr << "Fatal : out of image range\n";
            exit(1);
        }
        for (int y = block.offset.y; y < rightDown.y; ++y)
            for (int x = block.offset.x; x < rightDown.x; ++x) {
                screen.pixels[x][y] = block.pixels[x - block.offset.x][y - block.offset.y];
            }
    }

    void savePNG(const char *filename) const {
        uint8_t *data = new uint8_t[size.x * size.y * channels];
        for (int x = 0; x < size.x; ++x)
            for (int y = 0; y < size.y; ++y) {
                data[(x + y * size.x) * channels + 0] = screen.pixels[x][y][0] * 255;
                data[(x + y * size.x) * channels + 1] = screen.pixels[x][y][1] * 255;
                data[(x + y * size.x) * channels + 2] = screen.pixels[x][y][2] * 255;
            }
        stbi_write_png(filename, size.x, size.y, 3, data ,0);
        delete[] data;
    }
    
    friend std::ostream& operator<<(std::ostream &os, const Image &img);



protected:
    Vector2i size;
    unsigned channels;
    ImageBlock screen;
};

inline std::ostream& operator<<(std::ostream &os, const Image &img) {
    os << "Image :\n"
       << "\tsize     = " << img.size << "\n"
       << "\tchannels = " << img.channels << "\n";
    os << img.screen << "\n";
    return os;
}