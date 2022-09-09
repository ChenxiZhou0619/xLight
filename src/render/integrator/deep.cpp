#include "core/render-core/integrator.h"
#include "core/scene/scene.h"

class DeepIntegrator : public Integrator {
public:
    DeepIntegrator() = default;

    DeepIntegrator(const rapidjson::Value &_value) { }

    virtual ~DeepIntegrator() = default;

    virtual SpectrumRGB getLi(const Scene &scene,
                              const Ray3f &ray,
                              Sampler *sampler) const override 
    {
        auto its = scene.intersect(ray);
        if (its.has_value()) {
            return SpectrumRGB{ 1 / its->distance};
        };
        return SpectrumRGB {.0f};
    }
};

REGISTER_CLASS(DeepIntegrator, "deep")