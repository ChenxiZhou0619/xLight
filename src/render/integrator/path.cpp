#include "core/render-core/integrator.h"
#include "core/math/common.h"

class PathTracing : public Integrator {
    int maxDepth, rrThreshold;
public:
    PathTracing() : maxDepth(5), rrThreshold(3) { };

    PathTracing(const rapidjson::Value &_value) {
        // do nothing
        maxDepth = getInt("maxDepth", _value);
        rrThreshold = getInt("rrThreshold", _value);
    }

    virtual SpectrumRGB getLi(const Scene &scene, const Ray3f &r, Sampler *sampler) const {
        SpectrumRGB Li {.0f}, beta {1.f};
        int bounces;
        bool specularBounce = false;
        Ray3f ray(r);
        // TODO too change it
        float emitterPdf = 0;
        
        for (bounces = 0;; ++bounces) {
            RayIntersectionRec iRec;
            // test for intersection
            bool foundIntersection = scene.rayIntersect(ray, iRec);

            if (bounces == 0 || specularBounce) {
                if (foundIntersection) {
                    if (iRec.meshPtr->isEmitter()) {
                        Li += beta * iRec.meshPtr->getEmitter()->evaluate(ray);
                    }
                } else {
                    Li += beta * scene.evaluateEnvironment(ray);
                }
            }

            // terminate if 
            if (!foundIntersection || bounces >= maxDepth ) {
                break;
            }
            if (iRec.meshPtr->isEmitter())
                break;
            const BSDF *bsdf = iRec.meshPtr->getBSDF();
            specularBounce = !bsdf->isDiffuse();
            // sample the direct illumination at this point
            PointQueryRecord pRec;
            scene.sampleEmitterOnSurface(pRec, sampler);
            emitterPdf = pRec.pdf;
            Ray3f shadowRay (
                iRec.p,
                pRec.p
            );
            if (!scene.rayIntersect(shadowRay)) {
                // hit the emitter
                EmitterQueryRecord eRec (pRec, shadowRay);
                const auto &emitter = eRec.getEmitter();

                // evaluate the direct illumination
                BSDFQueryRecord bRec (
                    iRec.toLocal(-ray.dir), iRec.toLocal(shadowRay.dir), iRec.UV
                );
                SpectrumRGB bsdfVal = bsdf->evaluate(bRec);
                float pdf = (shadowRay.tmax * shadowRay.tmax) / std::abs(dot(pRec.normal, shadowRay.dir)) * pRec.pdf;
                float bsdfPdf = bsdf->pdf(bRec);
                Li += beta * powerHeuristic(pdf, bsdfPdf)
                      * bsdfVal 
                      * eRec.getEmitter()->evaluate(eRec) 
                      / pdf;
            }

            // sample the bsdf
            float pdf;
            BSDFQueryRecord bRec(iRec.toLocal(-ray.dir), iRec.UV);
            SpectrumRGB bsdfVal = bsdf->sample(bRec, sampler->next2D(), pdf);
            if (bsdfVal.isZero()) {
                break;
            }
            ray = Ray3f(
                iRec.p,
                iRec.toWorld(bRec.wo)
            );
            iRec = RayIntersectionRec();
            foundIntersection = scene.rayIntersect(ray, iRec);
            SpectrumRGB value(.0f);
            float lumPdf = .0f;
            if (!foundIntersection) {
                value = scene.evaluateEnvironment(ray);
                if (value.isZero())
                    break;
                lumPdf = INV_PI;
            } else {
                if (iRec.meshPtr->isEmitter()) {
                    value = iRec.meshPtr->getEmitter()->evaluate(ray);
                    lumPdf = specularBounce ? .0f : (iRec.t * iRec.t) / std::abs(dot(iRec.geoN, ray.dir)) * emitterPdf;
                }
            }
            beta *= bsdfVal;
            // TODO compute the lumPdf when env light matters
            //float lumPdf = specularBounce ? .0f : (iRec.t * iRec.t) / std::abs(dot(iRec.geoN, ray.dir)) * emitterPdf;
            //float lumPdf = .0f;
            if (!value.isZero()) {
                Li += beta * value * powerHeuristic(pdf, lumPdf);
            }

            // continue the next ray
            if (!iRec.isValid) break;
            if (bounces++ > rrThreshold) {
                if (sampler->next1D() > 0.95f) break;
                beta /= 0.95f;
            }
        }
        return Li;
    }

};

REGISTER_CLASS(PathTracing, "path")