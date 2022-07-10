#pragma once
#include "core/utils/configurable.h"
#include "spectrum.h"


class Texture : public Configurable{

public:
    Texture() = default;

    Texture(const rapidjson::Value &_value);

    virtual ~Texture() = default;

    virtual SpectrumRGB evaluate(const Point2f &uv) const = 0; 

    virtual SpectrumRGB average() const = 0;

    virtual Vector2i getResolution() const = 0;
    
};

class TextureLoader {
public:
    TextureLoader() = delete;

    static SpectrumRGB** toRGB(
        const std::string &filepath,
        int &width,
        int &height,
        int &channels
    );
};