#pragma once
#include "core/utils/configurable.h"
#include "core/core.h"

class Integrator : public Configurable {
public:
    Integrator() = default;
    Integrator(const rapidjson::Value &_value) {}
    
    virtual ~Integrator() {}
    virtual SpectrumRGB getLi(const Scene &scene, const Ray3f &ray) const = 0;

};