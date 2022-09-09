#include "image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

void Image::savePNG() const {
    uint8_t *data = new uint8_t[size.x * size.y * channels];
    for (int x = 0; x < size.x; ++x)
        for (int y = 0; y < size.y; ++y) {
            data[(x + y * size.x) * channels + 0] = std::min(screen.pixels[x][y][0], 1.f) * 255;
            data[(x + y * size.x) * channels + 1] = std::min(screen.pixels[x][y][1], 1.f) * 255;
            data[(x + y * size.x) * channels + 2] = std::min(screen.pixels[x][y][2], 1.f) * 255;
        }
    stbi_write_png(filename.c_str(), size.x, size.y, 3, data ,0);
    delete[] data;
}

std::shared_ptr<ImageBlock>
BlockManager::getBlock(int _x, int _y) const {
    return std::make_shared<ImageBlock>(
        Vector2i(_x * blockSize, _y * blockSize),
        Vector2i(blockSize)
    );
}