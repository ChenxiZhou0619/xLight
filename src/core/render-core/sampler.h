#pragma once
#include "core/utils/configurable.h"
#include "core/geometry/geometry.h"

class Sampler : public Configurable {
public:
    Sampler() = default;

    Sampler(const rapidjson::Value &_value);

    ~Sampler() = default;

    virtual float next1D() = 0;
    virtual Point2f next2D() = 0;
};