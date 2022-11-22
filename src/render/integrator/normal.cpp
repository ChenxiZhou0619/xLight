#include "core/render-core/integrator.h"
#include "core/scene/scene.h"

class NormalIntegrator : public PixelIntegrator {
 public:
  NormalIntegrator() = default;

  NormalIntegrator(const rapidjson::Value &_value) {}

  virtual ~NormalIntegrator() = default;

  virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                            Sampler *sampler) const override {
    auto its = scene.intersect(ray);
    if (its.has_value()) {
      auto normal = its->geometryN;
      normal = (normal + Vector3f{1.f}) * .5f;
      return SpectrumRGB{normal};
    };
    return SpectrumRGB{.0f};
  }
};

REGISTER_CLASS(NormalIntegrator, "normal")