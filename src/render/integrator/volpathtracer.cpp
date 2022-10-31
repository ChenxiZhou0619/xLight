#include <core/render-core/integrator.h>

class VolPathTracer : public Integrator {
public:
    VolPathTracer() : mMaxDepth(5), mRRThreshold(3) { }

    VolPathTracer(const rapidjson::Value &_value) {
        mMaxDepth = getInt("maxDepth", _value);
        mRRThreshold = getInt("rrThreshold", _value);
    }

    // *Single scattering
    virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                              Sampler *sampler) const override 
    {
        SpectrumRGB Li{.0f}, beta{1.f};
        int bounces = 0;
        PathInfo pathInfo = samplePath(scene, ray, sampler);
        const auto &itsInfo = pathInfo.itsInfo;
        while (bounces <= mMaxDepth) {
            beta *= pathInfo.weight;
            if (beta.isZero()) break;
            //* Evaluate the Le
            {
                SpectrumRGB Le = itsInfo->evaluateLe();
                float pdf = itsInfo->pdfLe();
                float misw = powerHeuristic(pathInfo.pdfDirection, pdf);
                Li += beta * Le * misw;
            }
            if (itsInfo->terminate()) break;

            //* Sample direct
            {
                LightSourceInfo lightSourceInfo = scene.sampleLightSource(*itsInfo, sampler);
                Ray3f shadowRay = itsInfo->scatterRay(scene, lightSourceInfo.position);
                SpectrumRGB Tr = tr(scene, shadowRay);
                auto *light = lightSourceInfo.light;
                SpectrumRGB LeWeight = light->evaluate(lightSourceInfo, itsInfo->position);
                SpectrumRGB f = itsInfo->evaluateScatter(shadowRay.dir);
                float misw = powerHeuristic(lightSourceInfo.pdf, itsInfo->pdfScatter(shadowRay.dir));
                Li += beta * f * LeWeight * Tr * misw;
            }
            //* Sample the scatter
            ScatterInfo scatterInfo = itsInfo->sampleScatter(sampler->next2D());
            ray = itsInfo->scatterRay(scene, scatterInfo.wo);
            pathInfo = samplePath(scene, ray, sampler, &scatterInfo);            
            if (bounces++ > mRRThreshold) {
                if (sampler->next1D() > 0.95f) break;
                beta /= 0.95;
            }
        }
        return Li;
    }
protected:
    int mMaxDepth;
    int mRRThreshold;

    //* This method samples a intersection (surface or medium) along the ray in scene,
    //* The empty surface will be skipped
    //*     1. If the scatter is medium, next scatter event will be surface intersection
    //*     2. If the scatter is surface, next scatter will be surface / medium intersection
    PathInfo samplePath(const Scene &scene, Ray3f ray,
                        Sampler *sampler,
                        const ScatterInfo *info = nullptr) const
    {   
        //* If no scatter info is provided, pdfDirection if FINF
        PathInfo pathInfo;
        pathInfo.pdfDirection = info ? info->pdf : FINF;
        pathInfo.length = 0;
        pathInfo.weight = info ? info->weight : SpectrumRGB{1};
        //* Single scattering, so no medium intersection in this situation
        if (info && info->scatterType == ScatterInfo::ScatterType::Medium) {
            std::shared_ptr<SurfaceIntersectionInfo> sIts;
            do {
                sIts = scene.intersectWithSurface(ray);
                if (auto medium = ray.medium; medium) {
                    pathInfo.weight *= medium->getTrans(ray.ori, sIts->position);
                }
                pathInfo.length += sIts->distance;
                if (sIts->terminate()) break;
                ray = sIts->scatterRay(scene, ray.dir);
            } while(sIts->shape && sIts->shape->getBSDF()->m_type == BSDF::EBSDFType::EEmpty);
            pathInfo.itsInfo = sIts;
            pathInfo.vertex = sIts->position;
        } 
        //* Sample a medium intersection or surface intersection along the ray
        else {
            std::shared_ptr<SurfaceIntersectionInfo> sIts;
            do {
                sIts = scene.intersectWithSurface(ray);
                if (auto medium = ray.medium; medium) {
                    auto mIts = medium->sampleIntersection(ray, sIts->distance, sampler->next2D());           
                    pathInfo.weight *= mIts->weight;
                    if (mIts->medium) {
                        //* Successfully sample a medium intersection
                        pathInfo.itsInfo = mIts;
                        pathInfo.vertex = mIts->position;
                        pathInfo.length += mIts->distance;
                        break;
                    }
                }
                //* No, hit the surface
                pathInfo.itsInfo = sIts;
                pathInfo.vertex = sIts->position;
                pathInfo.length += sIts->distance;
                if (sIts->terminate()) break;
                ray = sIts->scatterRay(scene, ray.dir);
            }while(sIts->shape && sIts->shape->getBSDF()->m_type == BSDF::EBSDFType::EEmpty);
        }
        return pathInfo;
    }

    SpectrumRGB tr(const Scene &scene, Ray3f ray) const
    {
        SpectrumRGB Tr{1.f};
        Point3f destination = ray.at(ray.tmax);
        auto info = scene.intersectWithSurface(ray);
        while(true) {
            if (info->shape && info->shape->getBSDF()->m_type != BSDF::EBSDFType::EEmpty) {
                Tr = SpectrumRGB{.0f};
                break;
            }else if (!info->shape) {
                if (auto medium = ray.medium; medium) {
                    Tr *= medium->getTrans(ray.ori, destination);
                }
                break;
            }
            else if (info->shape->getBSDF()->m_type == BSDF::EBSDFType::EEmpty) {
                if (auto medium = ray.medium; medium) {
                    Tr *= medium->getTrans(ray.ori, info->position);
                }
                ray = info->scatterRay(scene, ray.dir);
                info = scene.intersectWithSurface(ray);
            } 
        }
        return Tr;
    }
};
REGISTER_CLASS(VolPathTracer, "volpath-tracer")