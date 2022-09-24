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
/*
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
                beta *= mRec.transmittance / mRec.pdf;
                Point3f scatterPoint = ray.at(mRec.pathLength);
                //* Medium interaction
                auto xi = sampler->next1D();
                if (xi < mRec.albedo) {
                    //* Scatter
                    //*------------------------------------------------------------
                    //*----------------   Luminaire  Sampling     -----------------
                    //*------------------------------------------------------------
                    SpectrumRGB luminaire {.0f};
                    for (int i = 0; i < mShadowrayNums; ++i) {
                        DirectIlluminationRecord dRec;
                        SpectrumRGB trans{1.f};
                        scene.sampleAttenuatedAreaIllumination(
                            &dRec, &trans, 
                            scatterPoint, 
                            mediumStack, sampler
                        );
                        PhaseQueryRecord pRec{scatterPoint, ray.dir, dRec.shadow_ray.dir};
                        SpectrumRGB phaseValue = medium->evaluatePhase(pRec); 
                        float phasePdf = medium->pdfPhase(pRec);

                        luminaire += trans * beta * mRec.sigmaA * phaseValue * dRec.energy / dRec.pdf
                            * powerHeuristic(dRec.pdf, phasePdf);                                       
                    }
                    if (mShadowrayNums != 0) 
                        Li += luminaire / mShadowrayNums / mRec.albedo;

                    //*------------------------------------------------------------
                    //*----------------     Phase  Sampling     -------------------
                    //*------------------------------------------------------------
                    PhaseQueryRecord pRec{scatterPoint, ray.dir};
                    float phasePdf = .0f;
                    SpectrumRGB phaseWeight = medium->samplePhase(&pRec, sampler->next2D());
                    if (phaseWeight.isZero())
                        break;
                    ray = Ray3f{scatterPoint, pRec.wo};
                    auto its_ = scene.intersect(ray);
                    foundIntersection = its_.has_value();

                    luminaire = SpectrumRGB{.0f};
                    float lumionPdf = .0f;

                    if(!foundIntersection) {
                        //! infinity volume, shouldn't come
                        std::cerr << "Infinity volume, Error!\n";
                        //std::exit(1);
                        break;
                    } else {
                        if (its->shape->isEmitter()) {
                            // TODO, count the trans
                            luminaire = its->shape->getEmitter()->evaluate(ray);
                            lumionPdf = scene.pdfAreaIllumination(its.value(), ray);
                        }
                    }
                    beta *= phaseWeight;
                    Li += beta * luminaire * powerHeuristic(phasePdf, lumionPdf) / mRec.albedo;
                } else {
                    //* Absorbtion
                    Li += beta * mRec.sigmaA * medium->Le(ray) / (1 - mRec.albedo);
                    break;
                }
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
                    auto flag = dot(ray.dir, its->geometryN);
                    if (dot(ray.dir, its->geometryN) < -EPSILON) {
                        //* Enter
                        if (shape->hasMedium()) {
                            mediumStack.push(shape->getMedium());
                        }
                    } else if (dot(ray.dir, its->geometryN) > EPSILON) {
                        //* Escape
                        if (shape->hasMedium() && !mediumStack.empty()) {
                            mediumStack.pop();
                        }
                    }
                    //* Cause no surface interaction, continue the ray
                    ray = Ray3f{its->hitPoint + ray.dir * 0.001, ray.dir};
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
            }
            if (bounces++ > mRRThresHold) {
                if (sampler->next1D() > 0.95f) break;
                beta /= 0.95;
            }
        }
        return Li;
    }
*/
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

                SpectrumRGB trans {1.f};
                its = scene.intersect(ray, medium, &trans);
                foundIntersection = its.has_value();

                SpectrumRGB lumionEnergy {.0f};
                float lumionPdf = .0f;

                if (!foundIntersection) {
                    lumionEnergy = scene.evaluateEnvironment(ray);
                } else {
                    if (its->shape->isEmitter()) {
                        lumionEnergy = its->shape->getEmitter()->evaluate(ray);
                        lumionPdf = scene.pdfAreaIllumination(its.value(), ray);
                    }
                }

                if (!lumionEnergy.isZero() && !trans.isZero()) {
                    Li += trans * beta * lumionEnergy * powerHeuristic(pRec.pdf, lumionPdf);
                    break;
                }
                if (!foundIntersection)
                    break;

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

                    ray = Ray3f{its->hitPoint, ray.dir};
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