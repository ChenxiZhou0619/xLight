/**
 * @file img.h
 * @author Chenxi Zhou
 * @brief The object handle the images in rendering (output, image-texture etc.)
 * @version 0.1
 * @date 2022-11-04
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once
#include <string>
#include <memory>
class SpectrumRGB;
#include <core/geometry/geometry.h>

//* Only 3 channels are allowed now
class Figure {
public:
    int width, height, channels;
    
    Figure() = delete;

    Figure(int _width, int _height);

    Figure(const std::string &filename);

    ~Figure();

    void saveAsPng(const std::string &filename) const;

    void saveAsHdr(const std::string &filename) const;

    void saveAsExr(const std::string &filename) const;

    SpectrumRGB evaluate(Point2f uv, bool biFilter = false) const;

    void setPixel(float rgb[3], Point2i pixel);

    std::shared_ptr<Figure> shrinkHalfNearest() const;

    std::shared_ptr<Figure> shrinkHalfBox() const;

private:

    using PixelValue = float[3];
    PixelValue *data = nullptr;
};