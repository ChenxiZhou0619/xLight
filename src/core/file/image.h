
#pragma once

#include <memory>
#include <eigen3/Eigen/Core>
#include "core/geometry/geometry.h"
#include "core/render-core/spectrum.h"
#include "tinyformat/tinyformat.h"

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

    void setAll(SpectrumRGB rgb) {
        for (int i = 0; i < size.x; ++i) {
            for (int j = 0; j < size.y; ++j)
                pixels[i][j] = rgb;
        }
    }

    Vector2i getSize() const {
        return size;
    }

    int getWidth() const {
        return size[0];
    }

    int getHeight() const {
        return size[1];
    }

    Vector2i getOffset() const {
        return offset;
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

    Image(Vector2i _size, const std::string &_filename, 
        uint32_t _spp, unsigned _channels = 3)
        : size(_size), filename(_filename), spp(_spp), channels(_channels), screen(Vector2i(0, 0), _size) { }

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

    void savePNG() const;

    Point2i getSize() const {
        auto [x, y] = size;
        return {x, y};
    }

    int getWidth() const {
        return size[0];
    }

    int getHeight() const {
        return size[1];
    }

    uint32_t getSpp() const {
        return spp;
    }

    SpectrumRGB at(Vector2i pos) const {
        return screen.pixels[pos.x][pos.y];
    }
    
    friend std::ostream& operator<<(std::ostream &os, const Image &img);



protected:
    Vector2i size;
    std::string filename;
    uint32_t spp;

    unsigned channels;
    ImageBlock screen;
};

inline std::ostream& operator<<(std::ostream &os, const Image &img) {
    os << "Image :\n"
       << "\tsize     = " << img.size << "\n"
       << "\tfilename = " << img.filename << "\n"
       << "\tspp      = " << img.spp << "\n"
       << "\tchannels = " << img.channels << "\n";
    return os;
}


constexpr int BLOCKSIZE = 32;
class ImageBlockManager {
    int x, y;
    ImageBlock ***blocks;
public:
    ImageBlockManager() = delete;
    ImageBlockManager(const Vector2i &imgSize) : x(imgSize.x / BLOCKSIZE), y(imgSize.y / BLOCKSIZE) {
        blocks = new ImageBlock**[x];
        for (int _x = 0; _x < x; ++_x) {
            blocks[_x] = new ImageBlock*[y];
        }
        for (int _x = 0; _x < x; ++_x)
            for (int _y = 0; _y < y; ++_y) {
                blocks[_x][_y] = new ImageBlock(
                    Vector2i {_x * BLOCKSIZE, _y * BLOCKSIZE},
                    Vector2i {BLOCKSIZE, BLOCKSIZE}
                );
            }
    }
    ImageBlockManager(const ImageBlockManager &rhs) = delete;
    ImageBlockManager& operator()(const ImageBlockManager &rhs) = delete;
    ~ImageBlockManager() {
        for (size_t _x = 0; _x < x; ++_x)
            for (size_t _y = 0; _y < y; ++_y) {
                delete blocks[_x][_y];
            }
        for (size_t _x = 0; _x < x; ++_x) {
            delete[] blocks[_x];
        }
        delete[] blocks;
    }

    Vector2i getSize() const {return Vector2i {x, y};}

    ImageBlock& at(size_t _x, size_t _y) {return *blocks[_x][_y];}
    
};

class BlockManager {
public:
    BlockManager() = delete;
    BlockManager(Vector2i imgResolution, int _blockSize) {
        blockSize = _blockSize;
        auto [_x, _y] = imgResolution;
        x = _x / blockSize;
        y = _y / blockSize;
    }

    std::shared_ptr<ImageBlock> getBlock(int _x, int _y) const ;

    Vector2i getSize() const {
        return Vector2i{x, y};
    }

private:
    int x, y;
    int blockSize;
};