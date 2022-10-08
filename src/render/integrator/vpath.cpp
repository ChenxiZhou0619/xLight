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
        std::shared_ptr<Medium> medium = nullptr;
        const float epsilon = 0.01f;
        
        auto its = scene.intersect(ray);
        if (bounces == 0 && !its.has_value()) {
            if (!its.has_value())
                return scene.evaluateEnvironment(ray);
            if (its->shape->isEmitter())
                return its->shape->getEmitter()->evaluate(ray);
        }


        while(true) {
            if (!its.has_value() || its->shape->isEmitter())
                break;

            MediumSampleRecord mRec;
            if (medium) medium->sampleDistance(&mRec, Ray3f{ray.ori, its->hitPoint}, sampler);
            beta *= mRec.transmittance / mRec.pdf;
            
            if (mRec.isValid) {
                //* Medium Interaction
                beta *= mRec.sigmaS;
                Point3f scatterPoint = ray.at(mRec.pathLength);

                for (int i = 0; i < mShadowrayNums; ++i) {
                    DirectIlluminationRecord dRec;
                    SpectrumRGB trans {1.f};
                    scene.sampleAttenuatedAreaIllumination(&dRec, &trans, scatterPoint, medium, sampler);
                    PhaseQueryRecord pRec {scatterPoint, -ray.dir, dRec.shadow_ray.dir};
                    SpectrumRGB phaseVal = medium->evaluatePhase(pRec);
                    float phasePdf = medium->pdfPhase(pRec);
                    if (!trans.isZero())
                        Li += beta * dRec.energy * trans * phaseVal
                            / dRec.pdf * powerHeuristic(dRec.pdf, phasePdf)
                            / mShadowrayNums;
                }

                PhaseQueryRecord pRec {scatterPoint, -ray.dir};
                beta *= medium->samplePhase(&pRec, sampler->next2D());
                float phasePdf = medium->pdfPhase(pRec);
                ray = Ray3f{scatterPoint, pRec.localFrame.toWorld(pRec.wo)};
                
                SpectrumRGB trans{1.f};
                auto lumionIts = scene.intersect(ray, medium, &trans);
                float lumionPdf = .0f;
                SpectrumRGB lumion {.0f};
                if (!lumionIts.has_value()) {
                    lumion = scene.evaluateEnvironment(ray);
                } else if (lumionIts->shape->isEmitter()) {
                    lumion = lumionIts->shape->getEmitter()->evaluate(ray);
                    lumionPdf = scene.pdfAreaIllumination(lumionIts.value(), ray);
                }

                if (!lumion.isZero())
                    Li += beta * lumion * trans * powerHeuristic(phasePdf, lumionPdf);
                
                
                its = scene.intersect(ray);

            } else {
                //* Surface Interaction
                
                //* Handle special case
                auto shape = its->shape;
                auto bsdf = shape->getBSDF();

                if (bsdf->m_type == BSDF::EBSDFType::EEmpty) {
                    medium = scene.getTargetMedium(ray.dir, its.value());
                    ray = Ray3f {its->hitPoint + epsilon * ray.dir, ray.dir};
                    its = scene.intersect(ray); 
                    continue;
                }

                for (int i = 0; i < mShadowrayNums; ++i) {
                    DirectIlluminationRecord dRec;
                    SpectrumRGB trans {1.f};
                    scene.sampleAttenuatedAreaIllumination(&dRec, &trans, its->hitPoint, medium, sampler);
                    
                    BSDFQueryRecord bRec {its.value(), ray, dRec.shadow_ray};
                    SpectrumRGB bsdfVal = bsdf->evaluate(bRec);
                    float bsdfPdf = bsdf->pdf(bRec);

                    if (!trans.isZero())
                        Li += beta * dRec.energy * trans * bsdfVal
                            / dRec.pdf * powerHeuristic(dRec.pdf, bsdfPdf)
                            / mShadowrayNums;

                }

                BSDFQueryRecord bRec {its.value(), ray};
                float bsdfPdf = .0f;
                bsdf->sample(bRec, sampler->next2D(), bsdfPdf);
                SpectrumRGB bsdfVal = bsdf->evaluate(bRec);

                beta *= (bsdfVal / bsdfPdf);
            
                if (bsdfPdf == 0) {
                    break;
                }
                ray = Ray3f{its->hitPoint, its->toWorld(bRec.wo)};
                ray = Ray3f{ray.ori + epsilon * ray.dir, ray.dir};
                medium = scene.getTargetMedium(ray.dir, its.value());

                SpectrumRGB trans{1.f};
                auto lumionIts = scene.intersect(ray, medium, &trans);

                if (!lumionIts.has_value()) {
                    Li += beta * scene.evaluateEnvironment(ray) * trans;
                } else if(lumionIts->shape->isEmitter()){
                    float lumionPdf = scene.pdfAreaIllumination(lumionIts.value(), ray);
                    SpectrumRGB lumion = lumionIts->shape->getEmitter()->evaluate(ray);
                    Li += beta * lumion * trans * powerHeuristic(bsdfPdf, lumionPdf);
                }

                its = scene.intersect(ray);
            }

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

REGISTER_CLASS(VolPathTracer, "vpath")