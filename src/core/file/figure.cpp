#include <memory>
#include <iostream>
#include "figure.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Figure::Figure(int _width, int _height)
    : width(_width), height(_height), channels(3)
{   
    // allocate the space
    data = new PixelValue[width * height];

}

Figure::Figure(const std::string &filename) {
    bool isHdr = stbi_is_hdr(filename.c_str());
    stbi_info(filename.c_str(), &width, &height, &channels);
    assert(channels == 3);
    
    // allocate the space
    data = new PixelValue[width * height];
    
    // load the image
    if (isHdr) {
        float *tmp = stbi_loadf(filename.c_str(), &width, &height, &channels, 0);
        assert(tmp);
        memcpy(data, tmp, sizeof(float) * 3 * width * height);
        stbi_image_free(tmp);
    } else {
        u_int8_t *tmp = stbi_load(filename.c_str(), &width, &height, &channels, 0);
        assert(tmp);
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                int offset = x + y * width;
                for (int channel = 0; channel < 3; ++channel) {
                    data[offset][channel] = (float)tmp[offset * 3 + channel] / 255;
                }
            }
        }
        stbi_image_free(tmp);
    }
}

Figure::~Figure() {
    // free the space
    delete [] data;   
}

void Figure::saveAsPng(const std::string &filename) const {
    char *raw = new char[width * height * 3]();

    auto clamp = [](float f) {
        return std::min(int(f * 255), 255);
    };

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            int offset = x + y * width;
            for (int channel = 0; channel < 3; ++channel) {
                raw[3 * offset + channel] = clamp(data[offset][channel]);
            }
        }
    }

    stbi_write_png(filename.c_str(), width, height, 3, raw, 0);
    delete [] raw;
    return;
}

void Figure::saveAsHdr(const std::string &filename) const {
    stbi_write_hdr(filename.c_str(), width, height, 3, (float *)data);
}

void Figure::setPixel(float rgb[3], int x, int y)
{
    int offset = x + y * width;
    for (int channel = 0; channel < 3; ++channel) {
        data[offset][channel] = rgb[channel];
    }
}

std::shared_ptr<Figure> Figure::shrinkHalfNearest() const {
    std::shared_ptr<Figure> res = std::make_shared<Figure>(width / 2, height / 2);
    for (int i = 0; i < width - 1; i+=2) {
        for (int j = 0; j < height -1; j+=2) {
            int offset = i + j * width;
            auto rgb = data[offset];
            res->setPixel(rgb, i/2, j/2);
        }
    }
    return res;
}

std::shared_ptr<Figure> Figure::shrinkHalfBox() const {
    std::shared_ptr<Figure> res = std::make_shared<Figure>(width / 2, height / 2);
    for (int i = 0; i < width - 1; i+=2) {
        for (int j = 0; j < height - 1; j+=2) {
            int offset = i + j * width;
            auto rgb1 = data[offset],
                 rgb2 = (i != width - 1) ? data[offset + 1] : rgb1,
                 rgb3 = (j != height - 1) ? data[offset + width] : rgb1,
                 rgb4 = (i != width - 1 && j != height - 1) ? data[offset + width + 1] : rgb1;

            float rgb[3]{0, 0, 0};

            for (int i = 0; i < 3; ++i) {
                rgb[i] = 0.25 * (rgb1[i] + rgb2[i] + rgb3[i] + rgb4[i]);
            }

            res->setPixel(rgb, i/2, j/2);
        }
    }
    return res;
}