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

    virtual SpectrumRGB getLi(const Scene &scene,
                              const Ray3f &_ray,
                              Sampler *sampler) const override
    {
        SpectrumRGB Li{.0f}, beta{1.f};
        Ray3f ray{_ray};
        int bounces = 0;
        bool isSpecularBounce = false;
        std::shared_ptr<Medium> medium = nullptr;
        float prevBSDFPdf = .0f;

        while(bounces <= mMaxDepth) {
            auto its = scene.intersect(ray);
            bool foundIntersection = its.has_value();

            MediumSampleRecord mRec;
            if (medium &&
                medium->sampleDistance(&mRec, Ray3f{ray.ori, its->hitPoint}, sampler))
            {
                beta *= mRec.transmittance * mRec.sigmaS / mRec.pdf;
                Point3f scatterPoint = ray.at(mRec.pathLength);
                for (int i = 0; i < mShadowrayNums; ++i) {
                    DirectIlluminationRecord dRec;
                    SpectrumRGB trans{1.f};
                    scene.sampleAttenuatedAreaIllumination(
                        &dRec, &trans,
                        scatterPoint , 
                        medium, sampler
                    );
                    PhaseQueryRecord pRec{scatterPoint, ray.dir, dRec.shadow_ray.dir};
                    float phasePdf = medium->pdfPhase(pRec);
                    SpectrumRGB phaseValue = medium->evaluatePhase(pRec);

                    if (!trans.isZero()) 
                        Li += beta * trans * phaseValue * dRec.energy 
                        / dRec.pdf * powerHeuristic(dRec.pdf, phasePdf)
                        / mShadowrayNums ;
                }

                PhaseQueryRecord pRec{scatterPoint, ray.dir};
                medium->samplePhase(&pRec, sampler->next2D());
                beta *= medium->evaluatePhase(pRec) / pRec.pdf;
                ray = Ray3f{scatterPoint, pRec.localFrame.toWorld(pRec.wo)};

                //SpectrumRGB trans {1.f};
                //its = scene.intersect(ray, medium, &trans);
                //foundIntersection = its.has_value();
//
                //SpectrumRGB lumionEnergy {.0f};
                //float lumionPdf = .0f;
//
                //if (!foundIntersection) {
                //    lumionEnergy = scene.evaluateEnvironment(ray);
                //} else {
                //    if (its->shape->isEmitter()) {
                //        lumionEnergy = its->shape->getEmitter()->evaluate(ray);
                //        lumionPdf = scene.pdfAreaIllumination(its.value(), ray);
                //    }
                //}
//
                //if (!lumionEnergy.isZero() && !trans.isZero()) {
                //    //Li += trans * beta * lumionEnergy * powerHeuristic(pRec.pdf, lumionPdf);
                //}


            } else {
                if (medium){
                    beta *= mRec.transmittance / mRec.pdf;
                }
                if (beta.isZero())
                    break;
                if (bounces == 0 || isSpecularBounce) {
                    if (foundIntersection) {
                        if (its->shape->isEmitter()) {
                            Li += beta * its->shape->getEmitter()->evaluate(ray);
                            break;
                        }
                    } else {
                        Li += beta * scene.evaluateEnvironment(ray);
                        break;
                    }
                }

                if (!foundIntersection) {
                    Li += beta * scene.evaluateEnvironment(ray);
                    break;
                }
                    

                if (its->shape->isEmitter()) {
                    float lumionPdf = scene.pdfAreaIllumination(its.value(), ray);
                    Li += beta * its->shape->getEmitter()->evaluate(ray) * powerHeuristic(prevBSDFPdf, lumionPdf);
                    break;
                }

                auto shape = its->shape;
                auto bsdf = shape->getBSDF();
                if (bsdf->m_type == BSDF::EBSDFType::EEmpty) {
                    medium = scene.getTargetMedium(ray.dir, its.value());
                    ray = Ray3f{its->hitPoint , ray.dir};
                    continue;
                }

                for (int i = 0; i < mShadowrayNums; ++i) {
                    DirectIlluminationRecord dRec;
                    SpectrumRGB trans{1.f};
                    scene.sampleAttenuatedAreaIllumination(
                        &dRec, &trans, 
                        its->hitPoint, medium, 
                        sampler
                    );
                    BSDFQueryRecord bRec{its.value(), ray, dRec.shadow_ray};
                    auto bsdfValue = bsdf->evaluate(bRec);
                    auto bsdfPdf = bsdf->pdf(bRec);
                    if (!trans.isZero()) 
                        Li += beta * trans * bsdfValue * dRec.energy / dRec.pdf
                            *powerHeuristic(dRec.pdf, bsdfPdf) 
                            / mShadowrayNums;
                }

                BSDFQueryRecord bRec{its.value(), ray};
                float bsdfPdf = .0f;
                auto bsdfWeight = bsdf->sample(bRec, sampler->next2D(), bsdfPdf);
                prevBSDFPdf = bsdfPdf;
                ray = Ray3f{its->hitPoint, its->toWorld(bRec.wo)};
                beta *= bsdfWeight;

            }
            if (bounces++ > mRRThresHold) {
                if (sampler->next1D() > 0.95) break;
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

REGISTER_CLASS(VolPathTracer, "vpath")