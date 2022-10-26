#include "core/render-core/integrator.h"
#include "core/render-core/medium.h"
#include "core/math/common.h"
#include "spdlog/spdlog.h"


class VolPathTracer : public Integrator {
public:
    VolPathTracer() : mMaxDepth(5), mRRThresHold(3), mShadowrayNums(1) { }

    VolPathTracer(const rapidjson::Value &_value) {
        mMaxDepth = getInt("maxDepth", _value);
        mRRThresHold = getInt("rrThreshold", _value);
        mShadowrayNums = getInt("shadowRayNums", _value);
    }

    virtual SpectrumRGB getLi(const Scene &scene,
                              Ray3f ray,
                              Sampler *sampler) const override
    {
        return getLiSingleScattering(scene, ray, sampler);
//        return getLiMultipleScattering(scene, _ray, sampler);
    }

protected:
    SpectrumRGB getLiMultipleScattering(const Scene &scene,
                                        Ray3f ray,
                                        Sampler *sampler) const 
    {
        SpectrumRGB Li{.0f}, beta{1.f};
        int bounces = 0;

        ray.medium = scene.getEnvMedium();

        PathVertex pathVertex = marchRay(scene, ray, sampler);
        beta *= pathVertex.vertexWeight;
        
        while(bounces <= mMaxDepth) {

            std::optional<Intersection> itsOpt = pathVertex.itsOpt;
            bool foundIntersection = itsOpt.has_value();
            
            if (bounces == 0) {
                auto [lumin, luminPdf] 
                    = evaluateDirect(scene, ray, pathVertex.itsOpt);
                if (!lumin.isZero()) {
                    Li += beta * lumin;
                    break;
                }
            }

            if (!foundIntersection)
                break;

            if (auto surfaceIts = std::get_if<ShapeIntersection>(&itsOpt.value()); surfaceIts) {
                auto bsdf = surfaceIts->shape->getBSDF();
                
                //* Sample direct
                for (int i = 0; i < mShadowrayNums; ++i) {
                    auto [lumin, luminPdf, sampleDir, samplePoint, isDelta]
                        = sampleDirect(scene, surfaceIts->hitPoint, sampler); 
                    Ray3f shadowRay {surfaceIts->hitPoint, samplePoint};
                    shadowRay.medium = getTargetMedium(scene, *surfaceIts, shadowRay.dir);

                    //* Evaluate bsdf
                    BSDFQueryRecord bRec{*surfaceIts, -ray.dir, sampleDir};
                    SpectrumRGB bsdfVal = bsdf->evaluate(bRec);
                    float bsdfPdf = bsdf->pdf(bRec);
                    //* Evaluate trans
                    auto [trans, transPdf] = marchRayFromTo(scene, shadowRay);

                    //* The real pdf for constructing the path
                    // add if valid
                    if (!bsdfVal.isZero() && !lumin.isZero() && !trans.isZero()) {
                        float misw = isDelta ? 
                            1 : powerHeuristic(luminPdf, bsdfPdf * transPdf); //! fix
                        Li += beta * bsdfVal * trans * lumin * misw /
                            (mShadowrayNums * luminPdf);
                    }
                }

                //* Sample bsdf * Trans

                //* Sample bsdf first
                BSDFQueryRecord bRec{*surfaceIts, -ray.dir};
                float bsdfPdf = .0f;
                SpectrumRGB bsdfWeight = bsdf->sample(bRec, sampler->next2D(), bsdfPdf);
                beta *= bsdfWeight;

                //* Sample t second
                ray = Ray3f {
                    surfaceIts->hitPoint,
                    surfaceIts->toWorld(bRec.wo)
                };
                ray.medium = getTargetMedium(scene, *surfaceIts, ray.dir);

                pathVertex = marchRay(scene, ray, sampler);
                beta *= pathVertex.vertexWeight;

                if (beta.isZero()) break;

                //* Evaluate, if hit the light source
                auto [lumin, luminPdf] 
                    = evaluateDirect(scene, ray, pathVertex.itsOpt);
                if (!lumin.isZero()) {
                    float misw = bRec.isDelta ? 
                        1 : powerHeuristic(bsdfPdf * pathVertex.vertexPdf, luminPdf);   //! fix
                    Li += beta * lumin * misw;
                }
            }
            else if (auto mediumIts = std::get_if<MediumIntersection>(&itsOpt.value()); mediumIts) {
                auto medium = mediumIts->medium;
                //* Sample direct
                for (int i = 0; i < mShadowrayNums; ++i) {
                    auto [lumin, luminPdf, sampleDir, samplePoint, isDelta]
                        = sampleDirect(scene, mediumIts->scatterPoint, sampler);
                    Ray3f shadowRay {mediumIts->scatterPoint, samplePoint};
                    shadowRay.medium = medium;

                    //* Evaluate phase
                    PhaseQueryRecord pRec{mediumIts->scatterPoint, ray.dir, sampleDir};
                    SpectrumRGB phaseVal = medium->evaluatePhase(pRec);
                    float phasePdf = medium->pdfPhase(pRec);
                    //* Evaluate trans
                    auto [trans, transPdf] = marchRayFromTo(scene, shadowRay);
                    //* The real pdf for constructing the path

                    // add if valid
                    if (!phaseVal.isZero() && !lumin.isZero() && !trans.isZero()) {
                        float misw = isDelta ? 
                            1 : powerHeuristic(luminPdf, phasePdf * transPdf);
                        Li += beta * phaseVal * mediumIts->sigmaS * trans * lumin * misw / 
                            (mShadowrayNums * luminPdf);
                    }
                }

                //* Sample phase * Trans

                //* Sample phase first
                PhaseQueryRecord pRec{mediumIts->scatterPoint, ray.dir};
                SpectrumRGB phaseWeight = medium->samplePhase(&pRec, sampler->next2D());
                beta *= phaseWeight * mediumIts->sigmaS;

                //* Sample t second
                ray = Ray3f {
                    mediumIts->scatterPoint,
                    pRec.localFrame.toWorld(pRec.wo)
                };
                ray.medium = medium;

                pathVertex = marchRay(scene, ray, sampler, false);
                beta *= pathVertex.vertexWeight;

                if (beta.isZero()) break;

                //* Evaluate, if hit the light source
                auto [lumin, luminPdf]
                    = evaluateDirect(scene, ray, pathVertex.itsOpt);
                if (!lumin.isZero()) {
                    float misw = pRec.isDelta ? 
                        1 : powerHeuristic(pRec.pdf * pathVertex.vertexPdf, luminPdf);
                    Li += beta * lumin * misw;
                }
            } else {
                std::cout << "Shouldn't arrive here!\n";
                std::exit(1);
            }
            
            //* rr
            if (bounces++ > mRRThresHold) {
                if (sampler->next1D() > 0.95f) break;
                beta /= 0.95;
            }            
        }
        return Li;
    }

    SpectrumRGB getLiSingleScattering(const Scene &scene,
                                      Ray3f ray,
                                      Sampler *sampler) const 
    {
        SpectrumRGB Li{0.f}, beta{1.f};
        int bounces = 0;

        ray.medium = scene.getEnvMedium();

        PathVertex pathVertex = marchRay(scene, ray, sampler, true);
        if (bounces == 0) {
            auto [lumin, luminPdf]
                = evaluateDirect(scene, ray, pathVertex.itsOpt);
            if (!lumin.isZero()) {
                Li += beta * lumin;
            }
        }


        pathVertex = marchRay(scene, ray, sampler);
        beta *= pathVertex.vertexWeight;

        while(bounces <= mMaxDepth) {
            std::optional<Intersection> itsOpt = pathVertex.itsOpt;
            bool foundIntersection = itsOpt.has_value();
            if (!foundIntersection) break;

            if (auto surfaceIts = std::get_if<ShapeIntersection>(&itsOpt.value());
                surfaceIts)
            {
                auto bsdf = surfaceIts->shape->getBSDF();

                //*Sample direct
                for (int i = 0; i < mShadowrayNums; ++i) {
                    auto [lumin, luminPdf, sampleDir, samplePoint, isDelta]
                        = sampleDirect(scene, surfaceIts->hitPoint, sampler);
                    Ray3f shadowRay {surfaceIts->hitPoint, samplePoint};
                    shadowRay.medium = getTargetMedium(scene, *surfaceIts, shadowRay.dir);

                    //* Evaluate bsdf
                    BSDFQueryRecord bRec{*surfaceIts, -ray.dir, sampleDir};
                    SpectrumRGB bsdfVal = bsdf->evaluate(bRec);
                    float bsdfPdf = bsdf->pdf(bRec);
                    //* Evaluate trans
                    auto [trans, transPdf] = marchRayFromTo(scene, shadowRay);

                    //add if valid
                    if (!bsdfVal.isZero() && !lumin.isZero() && !trans.isZero()) {
                        float misw = isDelta ? 
                            1 : powerHeuristic(luminPdf, bsdfPdf);
                        Li += beta * bsdfVal * trans * lumin * misw 
                            / (mShadowrayNums * luminPdf);
                    }
                }
                //* Sample bsdf * trans
                BSDFQueryRecord bRec{*surfaceIts, -ray.dir};
                float bsdfPdf = .0f;
                SpectrumRGB bsdfWeight = bsdf->sample(bRec, sampler->next2D(), bsdfPdf);
                beta *= bsdfWeight;

                //* Sample t second
                ray = Ray3f {
                    surfaceIts->hitPoint,
                    surfaceIts->toWorld(bRec.wo)
                };
                ray.medium = getTargetMedium(scene, *surfaceIts, ray.dir);

                pathVertex = marchRay(scene, ray, sampler);
                beta *= pathVertex.vertexWeight;

                if (beta.isZero()) break;

                //* Evaluate, if hit the light source
                auto [lumin, luminPdf]
                    = evaluateDirect(scene, ray, pathVertex.itsOpt);
                if (!lumin.isZero()) {
                    float misw = bRec.isDelta ? 
                        1 : powerHeuristic(bsdfPdf * pathVertex.vertexPdf, luminPdf);
                    Li += beta * lumin * misw;
                }
            } else if (auto mediumIts = std::get_if<MediumIntersection>(&itsOpt.value());
                mediumIts)
            {
                auto medium = mediumIts->medium;
                for (int i = 0; i < mShadowrayNums; ++i) {
                    auto [lumin, luminPdf, sampleDir, samplePoint, isDelta]
                        = sampleDirect(scene, mediumIts->scatterPoint, sampler);
                    Ray3f shadowRay {mediumIts->scatterPoint, samplePoint};
                    shadowRay.medium = medium;

                    //* Evaluate phase
                    PhaseQueryRecord pRec{mediumIts->scatterPoint, ray.dir, sampleDir};
                    SpectrumRGB phaseVal = medium->evaluatePhase(pRec);
                    float phasePdf = medium->pdfPhase(pRec);
                    //* Evaluate trans
                    auto [trans, transPdf] = marchRayFromTo(scene, shadowRay);

                    //add if valid
                    if (!phaseVal.isZero() && !lumin.isZero() && !trans.isZero()) {
                        float misw = isDelta ? 
                            1 : powerHeuristic(luminPdf, phasePdf);
                        Li += beta * phaseVal * mediumIts->sigmaS * trans * lumin * misw 
                            / (mShadowrayNums * luminPdf);
                    }
                } 

                //* Sample phase * trans

                //* Sample phase first
                PhaseQueryRecord pRec{mediumIts->scatterPoint, ray.dir};
                SpectrumRGB phaseWeight = medium->samplePhase(&pRec, sampler->next2D());
                beta *= phaseWeight * mediumIts->sigmaS;

                //* Sample t second
                ray = Ray3f{
                    mediumIts->scatterPoint,
                    pRec.localFrame.toWorld(pRec.wo)
                };
                ray.medium = medium;

                //todo this ray should not scatter in medium
                pathVertex = marchRay(scene, ray, sampler, true);
                beta *= pathVertex.vertexWeight;

                if (beta.isZero()) break;

                //* Evaluate if hit the light
                auto [lumin, luminPdf] 
                    = evaluateDirect(scene, ray, pathVertex.itsOpt);
                if (!lumin.isZero()) {
                    float misw = pRec.isDelta ? 
                        1 : powerHeuristic(pRec.pdf, luminPdf);
                    Li += beta * lumin * misw;
                }
            }

            //* rr
            if (bounces++ > mRRThresHold) {
                if (sampler->next1D() > 0.95f) break;
                beta /= 0.95;
            }            
        }

        return Li;
    }

    //todo to handle the infinity volume    
    /**
     * @brief March the ray in scene, return the intersection(optionally), rayWeight and pdf of selecting the path vertex
     * 
     * @param scene 
     * @param ray 
     * @param sampler 
     * @return PathVertex 
     */
    PathVertex marchRay(const Scene &scene,
                        Ray3f ray,
                        Sampler *sampler,
                        bool ignoreMedium = false) const
    {   
        auto itsOpt = scene.intersect(ray);

        if (ignoreMedium) {
            if (auto medium = ray.medium; medium) {
                if (!itsOpt.has_value()) {
                    return {
                        std::nullopt, SpectrumRGB{.0f}, 1.f
                    };
                } else {
                    Intersection its{itsOpt.value()};
                    return {
                        std::make_optional(its), medium->getTrans(ray.ori, itsOpt->hitPoint), 1
                    };
                }
            } else {
                if (!itsOpt.has_value()) {
                    return {std::nullopt, SpectrumRGB{1}, 1};
                } else {
                    Intersection its{itsOpt.value()};
                    return {
                        std::make_optional(its), SpectrumRGB{1}, 1
                    };
                }
            }
        }

        if (auto medium = ray.medium; medium) {
            if (!itsOpt.has_value()) { 
                //* Infinity volume not allowed now, due to some round errors
                if (auto envMedium = scene.getEnvMedium(); 
                    !envMedium) {
                    return {std::nullopt, SpectrumRGB{1.f}, 1.f};
                } else {
                    medium = envMedium;
                }
            }

            float tBounds = itsOpt.has_value() ? itsOpt->distance : FLOATMAX;

            //* Sample the trans
            MediumSampleRecord mRec;
            //todo here should be careful
            auto [p, pdf] = scene.sampleLightPoint(sampler->next1D(), sampler->next3D());
            LightSourceInfo info{p, pdf};

            if (medium->samplePath(&mRec, ray, tBounds,&info ,sampler)) {
                //* Return a mediumIntersection
                MediumIntersection mediumIts;
                mediumIts.medium = medium;
                mediumIts.distance = mRec.pathLength;
                mediumIts.forwardDirection = ray.dir;
                mediumIts.forwardFrame = Frame{ray.dir};
                mediumIts.scatterPoint = ray.at(mRec.pathLength);
                mediumIts.sigmaS = mRec.sigmaS;
                mediumIts.sigmaA = mRec.sigmaA;
                mediumIts.albedo = mRec.albedo;

                SpectrumRGB weight = mRec.transmittance / mRec.pdf;
                //todo when add the emission, pdf here should change
                return {
                    std::make_optional(Intersection{mediumIts}),
                    weight,
                    mRec.pdf * mRec.albedo.average()
                };

            } else {

                //todo
                if (!itsOpt.has_value()) {
                    //* Maybe the medium's density is zero
                    //* Handle it specially
                    return {std::nullopt, SpectrumRGB{1}, 1.f};
                }
                if (itsOpt->shape->getBSDF()->m_type != BSDF::EBSDFType::EEmpty) {
                    if (!scene.getEnvMedium()) {
                        return { std::nullopt, SpectrumRGB{1.f}, 1.f };
                    } else {
                        Intersection surfaceIts{itsOpt.value()};
                        return {std::make_optional(surfaceIts), mRec.transmittance / mRec.pdf, mRec.pdf};
                    }
                }

                if (itsOpt->shape->getBSDF()->m_type == BSDF::EBSDFType::EEmpty) {
                    SpectrumRGB weight = mRec.transmittance / mRec.pdf;
                    
                    ray = Ray3f {
                        itsOpt->hitPoint + ray.dir * mEpsilon,
                        ray.dir 
                    };
                    ray.medium = getTargetMedium(scene, itsOpt.value(), ray.dir);
                    PathVertex pv = marchRay(scene, ray, sampler);

                    return {
                        pv.itsOpt,
                        pv.vertexWeight * weight,
                        pv.vertexPdf * mRec.pdf
                    };

                }
            }

        } else {
            if (!itsOpt.has_value()) {
                return {
                    std::nullopt,
                    SpectrumRGB{1},
                    1
                };
            }
            //* March the ray along its direction, until hit a surface
            //* skip the empty surface
            if (itsOpt->shape->getBSDF()->m_type != BSDF::EBSDFType::EEmpty) {
                //* return the surface intersection
                return {
                    std::make_optional(Intersection{itsOpt.value()}),
                    SpectrumRGB{1},
                    1
                };
            } else {
                //* Hit the empty, skip it
                ray = Ray3f {
                    itsOpt->hitPoint + ray.dir * mEpsilon,
                    ray.dir
                };
                ray.medium = getTargetMedium(scene, itsOpt.value(), ray.dir);
                return marchRay(scene, ray, sampler);
            }
        }

        return PathVertex{};
    }

    //todo handle the medium emission
    /**
     * @brief Evaluate the direct lighting (Le) of intersection, and correspond pdf
     * 
     * @param scene 
     * @param ray 
     * @param itsOpt 
     * @return std::pair<SpectrumRGB, float> 
     */
    std::pair<SpectrumRGB, float>
    evaluateDirect(const Scene &scene,
                   Ray3f ray,
                   std::optional<Intersection> &itsOpt) const
    {
        std::shared_ptr<Emitter> emitter = nullptr;
        SpectrumRGB Le{.0f};
        
        if (!itsOpt.has_value()) {
            emitter = scene.getEnvEmitter();
            Le = scene.evaluateEnvironment(ray);
        } else {
            if (auto surfaceIts = std::get_if<ShapeIntersection>(&itsOpt.value());
                surfaceIts) 
            {
                emitter = surfaceIts->shape->getEmitter();
                Le = emitter ? emitter->evaluate(ray) : SpectrumRGB{.0f};
            }
        }

        float pdfEmitter = scene.pdfEmitter(emitter),
              pdfSample = emitter ? 
                emitter->pdf(toEmitterHitInfo(itsOpt, ray)) : 0;
        return {Le, pdfEmitter * pdfSample};
    }

    /**
     * @brief Sample the lightsource
     * 
     * @param scene 
     * @param from 
     * @param sampler 
     * @return LuminRecord 
     */
    LuminRecord sampleDirect(const Scene &scene,
                             Point3f from,
                             Sampler *sampler) const
    {
        DirectIlluminationRecord dRec;
        scene.sampleDirectLumin(&dRec, from, sampler);
        Ray3f shadowRay = dRec.shadow_ray;
        return {dRec.energy, dRec.pdf, shadowRay.dir, shadowRay.at(shadowRay.tmax), dRec.isDelta};
    }


    std::shared_ptr<Medium> getTargetMedium(const Scene &scene,
                                            const ShapeIntersection &surfaceIts,
                                            Vector3f dir) const
    {
        if (dot(surfaceIts.geometryN, dir) < 0) {
            return surfaceIts.shape->getInsideMedium();
        } else {
            return scene.getEnvMedium();
        }
    }


    /**
     * @brief If occlude, return {Spectrum{.0f}, .0f}, else return required information
     * 
     * @param scene 
     * @param ray 
     * @return std::pair<SpectrumRGB, float> 
     */
    std::pair<SpectrumRGB, float>
    marchRayFromTo(const Scene &scene,
                   Ray3f ray) const
    {
        Point3f origin = ray.ori,
                dest = ray.at(ray.tmax);
        SpectrumRGB trans{1.f};
        float pdf = 1;

        auto itsOpt = scene.intersect(ray);
        while (itsOpt.has_value()) {
            if (itsOpt->shape->getBSDF()->m_type != BSDF::EBSDFType::EEmpty) {
                //* Occlude
                return {SpectrumRGB{.0f}, .0f};
            }
            if (auto medium = ray.medium; medium) {
                Point3f from = ray.ori,
                        end = itsOpt->hitPoint;
                trans *= medium->getTrans(from, end);
                pdf *= medium->pdfFromTo(from, end, true);
            }
            ray = Ray3f {
                itsOpt->hitPoint + ray.dir * mEpsilon,
                dest
            };
            ray.medium = getTargetMedium(scene, itsOpt.value(), ray.dir);
            itsOpt = scene.intersect(ray);
        }
        return {trans, pdf};
    }


    EmitterHitInfo toEmitterHitInfo(std::optional<Intersection> &itsOpt,
                                    const Ray3f &ray) const 
    {
        EmitterHitInfo info;
        if (itsOpt.has_value()) {
            if (auto surfaceIts = std::get_if<ShapeIntersection>(&itsOpt.value());
                surfaceIts)
            {
                info.dist = surfaceIts->distance;
                info.dir = ray.dir,
                info.hitpoint = surfaceIts->hitPoint;
                info.normal = surfaceIts->geometryN;
            }
        }
        return info;
    }


private:
    int mMaxDepth;
    int mRRThresHold;
    int mShadowrayNums;
    const float mEpsilon = 1e-4;
};

REGISTER_CLASS(VolPathTracer, "vpath")

class SingleVolPathTracer : public Integrator {
public:
    SingleVolPathTracer() = default;

    SingleVolPathTracer(const rapidjson::Value &_value) {
        mMaxDepth = getInt("maxDepth", _value);
        mRRThreshold = getInt("rrThreshold", _value);
    }

    virtual SpectrumRGB getLi(const Scene &scene,
                              Ray3f ray,
                              Sampler *sampler) const override
    {
        SpectrumRGB Li{0.f}, beta{1.f}; 
        int bounces = 0;

        auto itsOpt_weight = marchRay(scene, ray);

        {
            auto [lumin, luminPdf] = evaluateDirect(scene, ray, itsOpt_weight.first);
            if (!lumin.isZero()) Li += lumin * itsOpt_weight.second;
        }
        
        while (bounces <= mMaxDepth) {
            const auto &itsOpt = itsOpt_weight.first;
            const auto &weight = itsOpt_weight.second;

            if (auto medium = ray.medium; medium) {
                //* There are many ways to sample a mediumIntersection, such as
                //*     1. Sample the transmittance
                //*     2. Equi-angular sampling
                //*     3. Joint-importance samping
                //*   So, the sampling routinue of medium should know the information of emitter, 
                //* which is required by some samplimg methods

                //* Sample a light source first
                //todo fixme 
                LightSourceInfo info{Point3f{1, 1, 1}, 1, SpectrumRGB{10}};
                
                float tBound = itsOpt ? itsOpt->distance : ray.tmax;
                MediumSampleRecord mRec;

                //* Then, sample a scatterPoint
                bool inMedium = medium->samplePath(&mRec, ray, tBound, &info, sampler);

                if (inMedium) {
                    //todo do the light sample and no requirement for transPdf
                    //* Successfully sample a medium intersection
                    //* < L > = phase * sigmaS * Le * Tr / pdf

                    Point3f scatterPoint = ray.at(mRec.pathLength);
                    Vector3f directLuminDir = info.lightSourcePoint - scatterPoint;
                    float luminDistSquare = directLuminDir.length2();
                    Ray3f shadowRay {scatterPoint, info.lightSourcePoint};
                    shadowRay.medium = medium;
                    directLuminDir = normalize(directLuminDir);

                    PhaseQueryRecord pRec{ray.at(mRec.pathLength), ray.dir, directLuminDir};
                    SpectrumRGB phaseValue = medium->evaluatePhase(pRec),
                                lumin = info.lightSourceEnergy / luminDistSquare;
                    SpectrumRGB tr = evaluateTr(scene, medium, scatterPoint, info.lightSourcePoint);

                    Li += beta * phaseValue * mRec.sigmaS * lumin * tr;//pdf is 1
                } else {
                    //todo don't know do what
                }
            }

            //* Handle the surface situation
            if (!itsOpt.has_value()) {
                //todo handle infinite light
                break;
            }

            auto its = itsOpt.value();
            auto bsdf = its.shape->getBSDF();
            //* Handle special surface
            if (bsdf->m_type == BSDF::EBSDFType::EEmpty) {
                //* Just skip the surface
                ray = Ray3f{
                    its.hitPoint + ray.dir * mEpsilon,
                    ray.dir
                };
                ray.medium = getTargetMedium(scene, its, ray.dir);
                continue;
            }

            //* < L > = bsdf * cos * lumin * Tr / pdf
            
            //* Sample direct
            {
                DirectIlluminationRecord dRec;
                scene.sampleDirectLumin(&dRec, its.hitPoint, sampler);

                Ray3f shadowRay = dRec.shadow_ray;
                auto medium = getTargetMedium(scene, its, shadowRay.dir); 
                
                SpectrumRGB tr = evaluateTr(scene, medium, its.hitPoint, shadowRay.at(shadowRay.tmax));

                BSDFQueryRecord bRec{its, -ray.dir, shadowRay.dir};
                SpectrumRGB bsdfValue = bsdf->evaluate(bRec);
                float bsdfPdf = bsdf->pdf(bRec);

                if (!tr.isZero() && !bsdfValue.isZero()) {
                    Li += bsdfValue * dRec.energy * tr / dRec.pdf;
                }
            }
            //* Sample BSDF
            {
                BSDFQueryRecord bRec{its, -ray.dir};
                float bsdfPdf = .0f;
                SpectrumRGB bsdfWeight = bsdf->sample(bRec, sampler->next2D(), bsdfPdf);
                ray = Ray3f{
                    its.hitPoint,
                    its.toWorld(bRec.wo)
                };
                ray.ori = ray.ori + ray.dir * mEpsilon;
                ray.medium = getTargetMedium(scene, its, ray.dir);
                
                itsOpt_weight = marchRay(scene, ray);
                
                beta *= bsdfWeight * weight;

                if (beta.isZero()) break;

                if (itsOpt.has_value()) {
                    auto [lumin, luminPdf] = evaluateDirect(scene, ray, itsOpt);
                    if (!lumin.isZero())
                        Li += beta * lumin * powerHeuristic(bsdfPdf, luminPdf);
                }
            }
            //* rr
            if (bounces++ > mRRThreshold) {
                if (sampler->next1D() > 0.95f) break;
                beta /= 0.95;
            } 

        }

        return Li;
    }
protected:

    /**
     * @brief Return the optional surface intersection and Tr
     * 
     * @param scene 
     * @param ray 
     * @return std::pair<std::optional<ShapeIntersection>, float> 
     */
    std::pair<std::optional<ShapeIntersection>, SpectrumRGB> 
    marchRay(const Scene &scene, Ray3f ray) const
    {
        auto itsOpt = scene.intersect(ray);
        SpectrumRGB Tr{1.f};

        if (auto medium = ray.medium; medium) {
            //* Compute the Tr
            if (itsOpt.has_value()) {
                //* Hit the surface
                Tr *= medium->getTrans(ray.ori, itsOpt->hitPoint);
            } else {
                //* Hit environment
                if (scene.hasEnvironment()) {
                    Tr = SpectrumRGB{.0f};
                }
            }
        }
        return {itsOpt, Tr};
    }

    EmitterHitInfo toEmitterHitInfo(const std::optional<ShapeIntersection> &itsOpt,
                                    const Ray3f &ray) const 
    {
        EmitterHitInfo info;
        if (itsOpt.has_value()) {
            info.dist = itsOpt->distance;
            info.dir = ray.dir,
            info.hitpoint = itsOpt->hitPoint;
            info.normal = itsOpt->geometryN;
        }
        return info;
    }


    std::pair<SpectrumRGB, float>
    evaluateDirect(const Scene &scene,
                   Ray3f ray,
                   const std::optional<ShapeIntersection> &itsOpt) const
    {
        std::shared_ptr<Emitter> emitter = nullptr;
        SpectrumRGB Le{.0f};
        
        if (!itsOpt.has_value()) {
            emitter = scene.getEnvEmitter();
            Le = scene.evaluateEnvironment(ray);
        } else {    
            emitter = itsOpt->shape->getEmitter();
            Le = emitter ? emitter->evaluate(ray) : SpectrumRGB{.0f};
        }

        float pdfEmitter = scene.pdfEmitter(emitter),
              pdfSample = emitter ? 
                emitter->pdf(toEmitterHitInfo(itsOpt, ray)) : 0;
        return {Le, pdfEmitter * pdfSample};
    }

//todo fix interface
    SpectrumRGB evaluateTr(const Scene &scene, std::shared_ptr<Medium> medium,
                           Point3f from, Point3f dest) const
    {
        SpectrumRGB trans{1.f};
        Ray3f ray {from, dest};
        
        auto itsOpt = scene.intersect(ray);
        do {
            if (!itsOpt.has_value()) {
                if (medium) {
                    trans *= medium->getTrans(ray.ori, dest);
                } 
                break;
            }

            if (itsOpt->shape->getBSDF()->m_type != BSDF::EBSDFType::EEmpty) {
                //* Occlude
                return SpectrumRGB{.0f};
            }
            if (medium) {
                trans *= medium->getTrans(ray.ori, itsOpt->hitPoint);
            }
            ray = Ray3f {
                itsOpt->hitPoint + ray.dir * mEpsilon,
                dest
            };
            medium = getTargetMedium(scene, itsOpt.value(), ray.dir);
            itsOpt = scene.intersect(ray);
        } while (itsOpt.has_value());


        return trans;
    }

    std::shared_ptr<Medium> getTargetMedium(const Scene &scene,
                                            const ShapeIntersection &surfaceIts,
                                            Vector3f dir) const
    {
        if (dot(surfaceIts.geometryN, dir) < 0) {
            return surfaceIts.shape->getInsideMedium();
        } else {
            return scene.getEnvMedium();
        }
    }


    int mMaxDepth;
    int mRRThreshold;
    const float mEpsilon = 1e-4;
};

REGISTER_CLASS(SingleVolPathTracer, "singlevpath")