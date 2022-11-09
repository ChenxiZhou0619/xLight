#pragma once
#include <core/render-core/texture.h>
#include <core/math/common.h>
#include <core/file/figure.h>
#include <fmt/format.h>

class MipMap : public Texture {
public:
    MipMap() = delete;

    MipMap(const rapidjson::Value &_value) {
        std::string filepath = _value["filepath"].GetString();
        auto originMap = std::make_shared<Figure>(_value["filepath"].GetString());
        std::cout << "Constructing mipmap\n";

        int levels = std::min(
            std::log2(originMap->width),
            std::log2(originMap->height)
        );

        maps.reserve(levels);
        maps.emplace_back(originMap);
        for (int i = 1; i < levels; ++i) {
            maps.emplace_back(maps[i-1]->shrinkHalfBox());
        }
    }
    
    //* Delete this, replace with json
    MipMap(std::shared_ptr<Figure> originMap) {
        std::cout << "Constructing mipmap\n";
        int levels = std::min(
            std::log2(originMap->width),
            std::log2(originMap->height)
        );

        maps.reserve(levels);
        maps.emplace_back(originMap);
        for (int i = 1; i < levels; ++i) {
            maps.emplace_back(maps[i-1]->shrinkHalfBox());
        }
    }

    virtual ~MipMap() = default;

    virtual SpectrumRGB evaluate(const Point2f &uv, float du = 0, float dv = 0) const override;

    virtual SpectrumRGB average() const override{

    }

    virtual Vector2i getResolution() const override{

    }
    
    virtual SpectrumRGB dfdu(Point2f uv, float du = 0, float dv = 0) const override{

    }

    virtual SpectrumRGB dfdv(Point2f uv, float du = 0, float dv = 0) const override{

    }

private:
    std::vector<std::shared_ptr<Figure>> maps;
};
