#include "core/render-core/integrator.h"

class Whitted : public Integrator {
public:
    Whitted() = default;
    Whitted(const rapidjson::Value &_value) {
        // do nothing
    }

    virtual SpectrumRGB getLi(const Scene &scene, const Ray3f &ray, Sampler *sampler) const {
        RayIntersectionRec iRec;
        if (!scene.rayIntersect(ray, iRec)) {
            // TODO evaluate the environment light
            return SpectrumRGB {.0f};
        }
        // hit the scene
        SpectrumRGB Li {.0f};
        // TODO evaluate the hit at emitter

        BSDF *bsdf = iRec.meshPtr->getBSDF();
        BSDFQueryRecord bRec {iRec.toLocal(-ray.dir)};

        if (!bsdf->isDiffuse()) {
            //! bsdf * cosTheta / pdf
            SpectrumRGB bsdfWeight = bsdf->sample(bRec, sampler->next2D());
            Ray3f nextRay {
                iRec.p,
                iRec.toWorld(bRec.wo)
            };
            Li += bsdfWeight * getLi(scene, nextRay, sampler);
        } else {
            // just evaluate the direct light
            
        }

    }
};

REGISTER_CLASS(Whitted, "whitted")