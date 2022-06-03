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
        // TODO evaluate the hit at emitter
        if (iRec.meshPtr->isEmitter()) {
            return iRec.meshPtr->getEmitter()->evaluate(ray);
        }

        BSDF *bsdf = iRec.meshPtr->getBSDF();
        BSDFQueryRecord bRec {iRec.toLocal(-ray.dir)};

        if (!bsdf->isDiffuse()) {
            //! bsdf * cosTheta
            SpectrumRGB bsdfVal = bsdf->sample(bRec, sampler->next2D());
            Ray3f nextRay {
                iRec.p,
                iRec.toWorld(bRec.wo)
            };
            return bsdfVal * getLi(scene, nextRay, sampler);
        } else {
            // sample a point on emitter surface
            PointQueryRecord pRec;
            scene.sampleEmitterOnSurface(pRec, sampler);
            // evaluate the direct illumination
            Ray3f shadowRay (
                iRec.p,
                pRec.p
            );
            // check whether the shadowRay hit the emitter
            if (scene.rayIntersect(shadowRay)) {
                // no hit
                return SpectrumRGB {.0f};
            } else {
                // evaluate the direct illumination
                EmitterQueryRecord eRec (pRec, shadowRay);
                const auto &emitter = eRec.getEmitter();
                
                BSDFQueryRecord bRec (
                    iRec.toLocal(-ray.dir), iRec.toLocal(shadowRay.dir)
                );
                SpectrumRGB bsdfVal = bsdf->evaluate(bRec);
                return bsdfVal * eRec.getEmitter()->evaluate(eRec)
                    * std::abs(dot(iRec.shdN, shadowRay.dir))
                    * std::abs(dot(pRec.normal, shadowRay.dir))
                    / (shadowRay.dir.length2() * pRec.pdf);
            }
            
        }

    }
};

REGISTER_CLASS(Whitted, "whitted")