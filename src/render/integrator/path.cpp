#include "core/render-core/integrator.h"
#include "core/math/common.h"
#include <spdlog/spdlog.h>

class PathTracer : public Integrator {
public:
    PathTracer() : 
        mMaxDepth(5), mRRThresHold(3), mShadowrayNums(1) { }

    PathTracer(const rapidjson::Value &_value) {
        mMaxDepth = getInt("maxDepth", _value);
        mRRThresHold = getInt("rrThreshold", _value);
        mShadowrayNums = getInt("shadowRayNums", _value);
    }

    virtual SpectrumRGB getLi(const Scene &scene,
                              const Ray3f &_ray,
                              Sampler *sampler) const override
    {
        SpectrumRGB Li{.0f}, beta{1.f};
        Ray3f ray{_ray};
        int bounces = 0;
        bool isSpecularBounce = false;

        while (true) {
            auto its = scene.intersect(ray);
            bool foundIntersection = its.has_value();


            if (bounces == 0 || isSpecularBounce) {
                if (foundIntersection) {
                    if (its->shape->isEmitter()) {
                        Li += beta * its->shape->getEmitter()->evaluate(ray);
                    }
                } else {
                    Li += beta * scene.evaluateEnvironment(ray);
                    break;
                }
            }

            if (!foundIntersection || bounces >= mMaxDepth)
                break;

            if (its->shape->isEmitter())
                break;

            //* Handle the special surface intersection
            auto bsdf = its->shape->getBSDF();
            if (bsdf && bsdf->m_type == BSDF::EBSDFType::EEmpty) {
                ray = Ray3f{its->hitPoint, ray.dir};
                continue;
            }

            //TODO ray differentials and normal map etc.
            bsdf->computeShadingFrame(& its.value());

            SpectrumRGB directIllumination{.0f};
            for (int i = 0; i < mShadowrayNums; ++i) {
                DirectIlluminationRecord dRec;
                scene.sampleAreaIllumination(&dRec,its->hitPoint ,sampler);
                auto shadowRay = dRec.shadow_ray;
                if (!scene.occlude(shadowRay)) {
                    BSDFQueryRecord bRec{its.value(), ray, shadowRay};
                    SpectrumRGB bsdf_value = bsdf->evaluate(bRec);
                    float bsdf_pdf = bsdf->pdf(bRec);
                    directIllumination += beta *
                        bsdf_value * dRec.energy / dRec.pdf * 
                        powerHeuristic(dRec.pdf, bsdf_pdf);
                }
            }
            if (mShadowrayNums != 0) 
                Li += directIllumination / (float)mShadowrayNums;


            BSDFQueryRecord bRec{its.value(), ray};
            float bsdf_pdf = .0f;
            SpectrumRGB bsdf_weight = bsdf->sample(bRec, sampler->next2D(), bsdf_pdf);
            isSpecularBounce = !bsdf->isDiffuse();

            if(bsdf_weight.isZero())
                break;
            if (bsdf_pdf == 0)
                break;

            ray = Ray3f{its->hitPoint, its->toWorld(bRec.wo)};

            its = scene.intersect(ray);
            foundIntersection = its.has_value();

            SpectrumRGB lumion_energy{.0f};
            float lumion_pdf = .0f;

            if (!foundIntersection) {
                lumion_energy = scene.evaluateEnvironment(ray);
            } else {
                if (its->shape->isEmitter()) {
                    lumion_energy = its->shape->getEmitter()->evaluate(ray);
                    lumion_pdf = scene.pdfAreaIllumination(its.value(), ray);
                }
            }

            beta *= bsdf_weight;
            if (!lumion_energy.isZero())
                Li += beta * lumion_energy * powerHeuristic(bsdf_pdf, lumion_pdf);
            
            if (!foundIntersection)
                break;
            
            if (bounces++ > mRRThresHold) {
                if (sampler->next1D() > 0.95f) break;
                beta /= 0.95;
            }

        }
        return Li;
    }

private:
    int mMaxDepth;
    int mRRThresHold;
    int mShadowrayNums;
};

REGISTER_CLASS(PathTracer, "path")
