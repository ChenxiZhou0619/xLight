#include "texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

SpectrumRGB** TextureLoader::toRGB(
    const std::string &filepath, int &width, int &height, int &channels ) {
    
    unsigned char *tmp 
        = stbi_load(filepath.c_str(), &width, &height, &channels, 3);
    
    SpectrumRGB **data
        = new SpectrumRGB* [width];
    for (int i = 0; i < width; ++i) {
        data[i] = new SpectrumRGB[height]();
    }

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            int offset = x * 3 + y * width * 3;
            data[x][y] = SpectrumRGB {
                (float)tmp[offset] / 255.f,
                (float)tmp[offset + 1] / 255.f,
                (float)tmp[offset + 2] / 255.f
            };
        }
    }
    delete [] tmp;
    return data;
}