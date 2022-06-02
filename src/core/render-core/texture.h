#pragma once
#include "core/utils/configurable.h"
#include "spectrum.h"

class Texture : public Configurable{

public:
    Texture() = default;

    Texture(const rapidjson::Value &_value);

    virtual SpectrumRGB evaluate(const Point2f &uv) const = 0; 
    
};