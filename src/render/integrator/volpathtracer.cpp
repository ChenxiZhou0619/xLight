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
        PathInfo pathInfo = samplePath(scene, ray);
        const IntersectionInfo &itsInfo = pathInfo.itsInfo;
        while (bounces <= mMaxDepth) {
            beta *= pathInfo.weight;
            if (beta.isZero()) break;
            //* Evaluate the Le
            {
                SpectrumRGB Le = evaluateLe(scene, ray, itsInfo);
                float pdf = pdfLe(scene, ray, itsInfo);
                float misw = powerHeuristic(pathInfo.pdfDirection, pdf);
                Li += beta * Le * misw;
            }
            if (!itsInfo) break;
            if (itsInfo.skipIts()) {
                ray = itsInfo.generateRay(scene, ray.dir);
                pathInfo = samplePath(scene, ray, &pathInfo);
            }

            //* Sample direct
            if (bsdf->m_type != BSDF::EBSDFType::EEmpty){
                LightSourceInfo lightSourceInfo = scene.sampleLightSource(itsInfo, sampler);
                Ray3f shadowRay = itsInfo.generateShadowRay(scene, lightSourceInfo);
                SpectrumRGB Tr = tr(scene, shadowRay);
                auto *light = lightSourceInfo.light;
                SpectrumRGB Le = light->evaluate(lightSourceInfo, itsInfo.position);
                SpectrumRGB f = bsdf->evaluate(itsInfo, shadowRay.dir);
                float misw = powerHeuristic(lightSourceInfo.pdf, bsdf->pdf(itsInfo, shadowRay.dir));
                Li += beta * f * Le * Tr / lightSourceInfo.pdf * misw;
            }
            //* Sample the bsdf
            Point2f sample = bsdf->m_type == BSDF::EBSDFType::EEmpty ? 
                Point2f{.0f} : sampler->next2D();
            BSDFInfo bsdfInfo = bsdf->sample(itsInfo, sample);
            ray = itsInfo.generateRay(scene, bsdfInfo.wo);
            pathInfo = samplePath(scene, ray, pathInfo.pdfDirection, &bsdfInfo);
            if (bsdfInfo.type == BSDF::EBSDFType::EEmpty) bounces--;
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

    //* This methods always sample the cloest surface intersection of the ray
    //* Cuz the empty-bsdf should not change the pdfDirection of pathInfo,
    //* so, we need know the pdfDirection of previous pathInfo when the bsdfInfo is 
    //* generate by empty bsdf
    PathInfo samplePath(const Scene &scene, Ray3f ray,
                        const PathInfo *prevPath = nullptr,
                        const BSDFInfo *info = nullptr) const
    {
        PathInfo pathInfo;
        SurfaceIntersectionInfo itsInfo = scene.intersectWithSurface(ray);
        pathInfo.itsInfo = IntersectionInfo(itsInfo);
        pathInfo.length = itsInfo.distance;
        pathInfo.vertex = itsInfo.position;
        pathInfo.pdfLength = FINF;
        if (info) {
            pathInfo.pdfDirection = info->type == BSDF::EBSDFType::EEmpty ? 
                prevPdfDirection : info->pdf;
            pathInfo.weight = info->weight;
        } else {
            pathInfo.pdfDirection = FINF;
            pathInfo.weight = SpectrumRGB{1};
        }
        //todo pathInfo.weight
        if (auto medium = ray.medium; medium) {
            pathInfo.weight *= medium->getTrans(ray.ori, itsInfo.position);
        }
        return pathInfo;
    }

    //* This method samples a medium intersection in given medium and 
    //* ray segment
    PathInfo sampleMedium(Ray3f ray, float tBound, Sampler *sampler) const {

    } 

    SpectrumRGB evaluateLe(const Scene &scene, Ray3f ray, 
                           const IntersectionInfo &info) const
    {
        SpectrumRGB Le{.0f};
        if (auto light = info.light; light) {
            Le += light->evaluate(info, ray);
        }
        return Le;
    }

    float pdfLe(const Scene &scene, Ray3f ray, 
                const IntersectionInfo &info) const
    {
        float pdf = .0f;
        if (auto light = info.light; light) {
            pdf = scene.pdfEmitter(light) * light->pdf(info, ray);
        }
        return pdf;
    }

    SpectrumRGB tr(const Scene &scene, Ray3f ray) const
    {
        SpectrumRGB Tr{1.f};
        Point3f destination = ray.at(ray.tmax);
        SurfaceIntersectionInfo info = scene.intersectWithSurface(ray);
        while(info) {
            if (auto medium = ray.medium; medium) {
                Tr *= medium->getTrans(ray.ori, info.position);
            }
            if (info.shape->getBSDF()->m_type != BSDF::EBSDFType::EEmpty) {
                Tr = SpectrumRGB{.0f};
                break;
            } else {
                ray = info.generateShadowRay(scene, destination);
                info = scene.intersectWithSurface(ray);
            }
        }
        return Tr;
    }
};
REGISTER_CLASS(VolPathTracer, "volpathtracer")