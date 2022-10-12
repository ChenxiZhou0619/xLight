#pragma once
#include "core/utils/configurable.h"
#include "bsdf.h"
#include "sampler.h"
#include "emitter.h"
#include "core/scene/scene.h"
class Scene;

class Integrator : public Configurable {
public:
    Integrator() = default;
    Integrator(const rapidjson::Value &_value) {}
    
    virtual ~Integrator() {}
    virtual SpectrumRGB getLi(const Scene &scene, const Ray3f &ray, Sampler *sampler) const = 0;

};


struct LiSampleRecord {
    SpectrumRGB Li {.0f};
    float pdf;
};