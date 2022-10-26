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
                              Ray3f ray,
                              Sampler *sampler) const override
    {
        SpectrumRGB Li{.0f}, beta{1.f};
        int bounces = 0;

        std::optional<ShapeIntersection> itsOpt = scene.intersect(ray);
        bool foundIntersection = itsOpt.has_value();

        if (foundIntersection) {
            if (itsOpt->shape->isEmitter()) {
                return itsOpt->shape->getEmitter()->evaluate(ray);
            }
        } else {
            return scene.evaluateEnvironment(ray);
        }

        while (bounces < mMaxDepth) {
 
            if (!foundIntersection || bounces >= mMaxDepth)
                break;

            //todo fixme
            if (itsOpt->shape->isEmitter())
                break;

            //* Handle the special surface intersection
            auto its = itsOpt.value();
            auto bsdf = its.shape->getBSDF();
            if (bsdf && bsdf->m_type == BSDF::EBSDFType::EEmpty) {
                ray = Ray3f{its.hitPoint + ray.dir * 1e-4, ray.dir};
                itsOpt = scene.intersect(ray);
                foundIntersection = itsOpt.has_value();
                continue;
            }

            //TODO ray differentials and normal map etc.
            bsdf->computeShadingFrame(&its);

            for (int i = 0; i < mShadowrayNums; ++i) {              
                //* sampleDirect : This should consider occlude and transmittance
                auto [lumin, 
                      luminPdf, 
                      sampleDir, 
                      luminPoint,
                      isDelta] 
                    = sampleDirect(scene, its.hitPoint, sampler);
                // add if valid
                if (!lumin.isZero()) {
                    BSDFQueryRecord bRec {its, -ray.dir, sampleDir};
                    SpectrumRGB bsdfVal = bsdf->evaluate(bRec);
                    // mis
                    float misw = isDelta ? 
                        1 : powerHeuristic(luminPdf, bsdf->pdf(bRec));
                    Li += beta * bsdfVal * lumin / luminPdf * misw / mShadowrayNums;
                }
                
            }

            BSDFQueryRecord bRec{its, ray};
            float bsdfPdf = .0f;
            SpectrumRGB bsdfWeight = bsdf->sample(bRec, sampler->next2D(), bsdfPdf);

            if(bsdfWeight.isZero() || bsdfPdf == 0)
                break;

            //* Update the ray, beta and itsOpt
            beta *= bsdfWeight;
            ray = Ray3f{its.hitPoint + its.toWorld(bRec.wo) * 1e-4, its.toWorld(bRec.wo), 0, 0.001};
            itsOpt = scene.intersect(ray);
            foundIntersection = itsOpt.has_value();

            //* evaluateDirect : This should consider occulude and transmittance
            auto [lumin, luminPdf] = evaluateDirect(scene, ray, itsOpt);
            
            
            // add if valid            
            if (!lumin.isZero()) {
                float misw = bRec.isDelta ? 
                    1 : powerHeuristic(bsdfPdf, luminPdf);
                Li += beta * lumin * misw;
            }
            
            if (bounces++ > mRRThresHold) {
                if (sampler->next1D() > 0.95f) break;
                beta /= 0.95;
            }

        }
        return Li;
    }


protected:

    /**
     * @brief Randomly choose a light source, and sample it.
     * This should take occlusion and transmittance into consideration
     * Sample scene only if exists, or area
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
        if (scene.hasEnvironment()) {
            DirectIlluminationRecord dRec;
            auto sample = sampler->next2D();
            scene.sampleEnvironment(&dRec, from, sample);
            if (scene.occlude(dRec.shadow_ray)) {
                return {SpectrumRGB{.0f}, .0f, Vector3f{}, Point3f{}, false};
            }

            return {dRec.energy, dRec.pdf, dRec.shadow_ray.dir, Point3f{}, false};
        } else {
            DirectIlluminationRecord dRec;
            scene.sampleAreaIllumination(&dRec, from, sampler);

            // test visibility
            if (scene.occlude(dRec.shadow_ray)) {
                return {SpectrumRGB{.0f}, .0f, Vector3f{}, Point3f{}, false};
            }

            return {dRec.energy, dRec.pdf, dRec.shadow_ray.dir, Point3f{}, false};
        }
    }

    std::pair<SpectrumRGB, float> 
    evaluateDirect(const Scene &scene,
                   Ray3f ray,
                   std::optional<ShapeIntersection> &itsOpt) const 
    {
        if (!itsOpt.has_value()) {
            return {scene.evaluateEnvironment(ray), scene.pdfEnvironment(ray)};
        }
        else if (itsOpt->shape->isEmitter())
            return {
                itsOpt->shape->getEmitter()->evaluate(ray),
                scene.hasEnvironment() ? 0 : scene.pdfAreaIllumination(itsOpt.value(), ray)
            };
        return {SpectrumRGB{.0f}, .0f};
    }



private:
    int mMaxDepth;
    int mRRThresHold;
    int mShadowrayNums;
};

REGISTER_CLASS(PathTracer, "path")
