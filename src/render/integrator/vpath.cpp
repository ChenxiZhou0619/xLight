#include "core/render-core/integrator.h"
#include "core/render-core/medium.h"
#include "core/math/common.h"
#include <stack>

class VolPathTracer : public Integrator {
public:
    VolPathTracer() : mMaxDepth(5), mRRThresHold(3), mShadowrayNums(1) { }

    VolPathTracer(const rapidjson::Value &_value) {
        mMaxDepth = getInt("maxDepth", _value);
        mRRThresHold = getInt("rrThreshold", _value);
        mShadowrayNums = getInt("shadowRayNums", _value);
    }

    virtual SpectrumRGB getLi (const Scene &scene,
                               const Ray3f &_ray,
                               Sampler *sampler) const override
    {
        SpectrumRGB Li{.0f}, beta{1.f};
        Ray3f ray{_ray};
        int bounces = 0;
        bool isSpecularBounce = false;
        std::stack<std::shared_ptr<Medium>> mediumStack;

        while (bounces <= mMaxDepth) {
            auto its = scene.intersect(ray);
            bool foundIntersection = its.has_value();

            auto medium = mediumStack.empty() ? nullptr : mediumStack.top();
            MediumSampleRecord mRec;
            if (medium && 
                medium->sampleDistance(&mRec, Ray3f{ray.ori, its->hitPoint}, sampler))
            {
                //* Medium interaction
            } else {
                //* Surface interaction
                if (mRec.medium) {
                    beta *= mRec.transmittance / mRec.pdf;
                }

                if (!foundIntersection) {
                    //* No intersection found
                    break;
                }

                if (its->shape->isEmitter()) {
                    break;
                }

                auto shape = its->shape;
                auto bsdf = shape->getBSDF();
                if (bsdf->m_type == BSDF::EBSDFType::EEmpty) {
                    //* Enter or escape a medium, check
                    if (dot(ray.dir, its->geometryN) < 0) {
                        //* Enter
                        if (shape->hasMedium()) {
                            mediumStack.push(shape->getMedium());
                        }
                    } else if (dot(ray.dir, its->geometryN) > 0) {
                        //* Escape
                        if (shape->hasMedium() && !mediumStack.empty()) {
                            mediumStack.pop();
                        }
                    }
                    //* Cause no surface interaction, continue the ray
                    ray = Ray3f{its->hitPoint, ray.dir};
                    continue;
                }

                //*------------------------------------------------------------
                //*------------     Luminaire  Sampling     -------------------
                //*------------------------------------------------------------

                SpectrumRGB luminaire {.0f};
                for (int i = 0; i < mShadowrayNums; ++i) {
                    SpectrumRGB trans {1.f};
                    DirectIlluminationRecord dRec;
                    scene.sampleAttenuatedAreaIllumination(
                        &dRec, 
                        &trans, 
                        its->hitPoint, 
                        mediumStack,
                        sampler
                    );
                    BSDFQueryRecord bRec{its.value(), ray, dRec.shadow_ray};
                    SpectrumRGB bsdfValue = bsdf->evaluate(bRec);
                    float bsdfPdf = bsdf->pdf(bRec);

                    luminaire += trans * beta * bsdfValue * dRec.energy / dRec.pdf
                        * powerHeuristic(dRec.pdf, bsdfPdf);
                }
                if (mShadowrayNums != 0)
                    Li += luminaire / mShadowrayNums;

                //*------------------------------------------------------------
                //*-----------------     BSDF  Sampling     -------------------
                //*------------------------------------------------------------
                
                BSDFQueryRecord bRec{its.value(), ray};
                float bsdfPdf = .0f;
                SpectrumRGB bsdfWeight = bsdf->sample(bRec, sampler->next2D(), bsdfPdf);
                if (bsdfWeight.isZero())
                    break;
                ray = Ray3f{its->hitPoint, its->toWorld(bRec.wo)};
                its = scene.intersect(ray);
                foundIntersection = its.has_value();

                luminaire = SpectrumRGB{.0f};
                float lumionPdf = .0f;

                if (!foundIntersection) {
                    luminaire = scene.evaluateEnvironment(ray);
                    lumionPdf = .0f;
                } else {
                    if (its->shape->isEmitter()) {
                        luminaire = its->shape->getEmitter()->evaluate(ray);
                        lumionPdf = scene.pdfAreaIllumination(its.value(), ray);
                    }
                }

                beta *= bsdfWeight;
                if (!luminaire.isZero())
                    Li += beta * luminaire * powerHeuristic(bsdfPdf, lumionPdf);

                if (!foundIntersection)
                    break;

                if (bounces++ > mRRThresHold) {
                    if (sampler->next1D() > 0.95f) break;
                    beta /= 0.95;
                }
            }
        }
        return Li;
    }

private:
    int mMaxDepth;
    int mRRThresHold;
    int mShadowrayNums;
};

REGISTER_CLASS(VolPathTracer, "vpath")