#include "core/render-core/integrator.h"


class Whitted : public Integrator {
public:
    Whitted() = default;
    Whitted(const rapidjson::Value &_value) {
        // do nothing
    }

    virtual SpectrumRGB getLi(const Scene &scene, 
                              const Ray3f &ray,
                              Sampler *sampler) const override
    {
        auto its = scene.intersect(ray);
        bool foundIntersection = its.has_value();

        if (!foundIntersection)
            return scene.evaluateEnvironment(ray);
        if (its->shape->isEmitter())
            return its->shape->getEmitter()->evaluate(ray);

        auto bsdf = its->shape->getBSDF();

        if (!bsdf->isDiffuse()) {
            if (sampler->next1D() < 0.95) {
                BSDFQueryRecord bRec{its.value(), ray};
                float pdf;
                SpectrumRGB bsdf_weight = bsdf->sample(bRec, sampler->next2D(), pdf);
                Ray3f nextRay{
                    its->hitPoint,
                    its->toWorld(bRec.wo)
                };
                return bsdf_weight * 1.057 * getLi(scene, nextRay, sampler);
            } else {
                return SpectrumRGB{.0f};
            }
        } else {
            DirectIlluminationRecord dRec;
            scene.sampleAreaIllumination(&dRec, its->hitPoint, sampler);
            if (!scene.occlude(dRec.shadow_ray)) {
                BSDFQueryRecord bRec{
                    its.value(),
                    ray,
                    dRec.shadow_ray
                };
                SpectrumRGB bsdf_value = bsdf->evaluate(bRec);
                return dRec.energy * bsdf_value / dRec.pdf;
                //return SpectrumRGB{5.f} * bsdf_value / dRec.pdf;
            } else {
                return SpectrumRGB{.0f};
            }

        }
    }
};

REGISTER_CLASS(Whitted, "whitted")