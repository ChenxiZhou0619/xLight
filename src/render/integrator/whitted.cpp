#include "core/render-core/integrator.h"


class Whitted : public Integrator {
public:
    Whitted() = default;
    Whitted(const rapidjson::Value &_value) {
        // do nothing
    }
/*
    virtual SpectrumRGB getLi_(const Scene &scene, const Ray3f &ray, Sampler *sampler) const {
        RayIntersectionRec iRec;
        if (!scene.rayIntersect(ray, iRec)) {
            return scene.evaluateEnvironment(ray);
        }
        // hit the scene
        if (iRec.meshPtr->isEmitter()) {
            return iRec.meshPtr->getEmitter()->evaluate(ray);
        }

        BSDF *bsdf = iRec.meshPtr->getBSDF();
        BSDFQueryRecord bRec {iRec.toLocal(-ray.dir)};

        if (!bsdf->isDiffuse()) {
            //! bsdf * cosTheta
            if (sampler->next1D() < 0.95f) {
                float pdf;
                SpectrumRGB bsdfVal = bsdf->sample(bRec, sampler->next2D(), pdf);
                Ray3f nextRay {
                    iRec.p,
                    iRec.toWorld(bRec.wo)
                };
                return bsdfVal * 1.057f * getLi(scene, nextRay, sampler);
            } else {
                return SpectrumRGB{.0f};
            }
            
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
                float pdf = (shadowRay.tmax * shadowRay.tmax) / std::abs(dot(pRec.normal, shadowRay.dir)) * pRec.pdf;
                return bsdfVal * eRec.getEmitter()->evaluate(eRec) / pdf;
            }
            
        }

    }
*/
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