#include <core/math/common.h>
#include <core/render-core/integrator.h>

class PathTracer : public Integrator {
public:
    PathTracer() : mMaxDepth(5), mRRThreshold(3) {

    }
    
    PathTracer(const rapidjson::Value &_value) {
        mMaxDepth = getInt("maxDepth", _value);
        mRRThreshold = getInt("rrThreshold", _value);
    }

    virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                              Sampler *sampler) const override
    {
        SpectrumRGB Li{.0f}, beta{1.f};
        int bounces = 0;
        PathInfo pathInfo = samplePath(scene, ray);
        const SurfaceIntersectionInfo &itsInfo = *pathInfo.itsInfo.asSurfaceIntersection();
        while(bounces <= mMaxDepth) {
            beta *= pathInfo.weight;
            if (beta.isZero()) break;
            //* Evaluate the Le using mis
            //* <L> = beta * Le * misw(pdfPath, pdfLe)
            {
                //give the emission of the hitpoint and the pdf of sampling this point when sample direct
                SpectrumRGB Le = evaluateLe(scene, ray, itsInfo);
                float pdf = pdfLe(scene, ray, itsInfo);
                float misw = powerHeuristic(pathInfo.pdfDirection, pdf);
                Li += beta * Le * misw;     
            }
            //* Not hit the scene, terminate
            if (!itsInfo) break;            
            auto bsdf = itsInfo.shape->getBSDF();
            //* Sample the direct
            {
                LightSourceInfo lightSourceInfo = scene.sampleLightSource(itsInfo, sampler);
                Ray3f shadowRay = itsInfo.generateShadowRay(scene, lightSourceInfo);
                if (!scene.occlude(shadowRay)) {
                    auto *light = lightSourceInfo.light;
                    SpectrumRGB Le = light->evaluate(lightSourceInfo, itsInfo.position);
                    SpectrumRGB f = bsdf->evaluate(itsInfo, shadowRay.dir);
                    float misw = powerHeuristic(lightSourceInfo.pdf, bsdf->pdf(itsInfo, shadowRay.dir));
                    Li += beta * f * Le / lightSourceInfo.pdf * misw;
                }
            }
            //* Sample the bsdf
            BSDFInfo bsdfInfo = bsdf->sample(itsInfo, sampler->next2D());
            ray = itsInfo.generateRay(scene, bsdfInfo.wo);
            pathInfo = samplePath(scene, ray, &bsdfInfo);
            if (bounces++ > mRRThreshold) {
                if (sampler->next1D() > 0.95f) break;
                beta /= 0.95;
            }
        }
        return Li;
    }

protected:
    PathInfo samplePath(const Scene &scene, Ray3f ray, const BSDFInfo *bsdfInfo = nullptr) const
    {
        PathInfo pathInfo;
        //* In path tracer(with out volumes), length sampling is a dirac delta distribution(given a direction)
        SurfaceIntersectionInfo itsInfo = scene.intersectWithSurface(ray);
        pathInfo.itsInfo = IntersectionInfo{itsInfo};
        pathInfo.length = itsInfo.distance;
        pathInfo.pdfLength = FINF;
        pathInfo.pdfDirection = bsdfInfo ? bsdfInfo->pdf : FINF;
        pathInfo.vertex = itsInfo.position;
        pathInfo.weight = bsdfInfo ? bsdfInfo->weight : SpectrumRGB{1};
        return pathInfo;
    }

    SpectrumRGB evaluateLe(const Scene &scene, const Ray3f &ray, 
                           const SurfaceIntersectionInfo &itsInfo) const
    {   
        SpectrumRGB Le{.0f};
        if (auto light = itsInfo.light; light) {
            Le += light->evaluate(itsInfo, ray);
        }
        return Le;
    }

    float pdfLe(const Scene &scene, const Ray3f &ray,
                const SurfaceIntersectionInfo &itsInfo) const 
    {
        float pdf = .0f;
        if (auto light = itsInfo.light; light) {
            pdf = scene.pdfEmitter(light) * light->pdf(itsInfo, ray);
        }
        return pdf;
    }   

protected:
    int mMaxDepth;
    int mRRThreshold;
};

REGISTER_CLASS(PathTracer, "path-tracer")