#pragma once
#include "core/utils/configurable.h"
#include "core/core.h"
#include "bsdf.h"
#include "sampler.h"
#include "emitter.h"

class Integrator : public Configurable {
public:
    Integrator() = default;
    Integrator(const rapidjson::Value &_value) {}
    
    virtual ~Integrator() {}
    virtual SpectrumRGB getLi(const Scene &scene, const Ray3f &ray, Sampler *sampler) const = 0;

};