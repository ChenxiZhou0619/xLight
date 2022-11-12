#include "medium.h"
#include <core/render-core/info.h>

std::shared_ptr<MediumIntersectionInfo>
Medium::sampleIntersectionEquiAngular(Ray3f ray, float tBounds, Point2f sample,
                                      const LightSourceInfo &info) const 
{
    if (info.lightType == LightSourceInfo::LightType::Area ||
        info.lightType == LightSourceInfo::LightType::Spot ) {
        //* Only these two situations do equi-angular sampling
        Vector3f ori2light = info.position - ray.ori;
        double a = -dot(ori2light, ray.dir) / ray.dir.length(),
               b = tBounds + a,
               D = std::sqrt(std::max(.0, ori2light.length2() - a * a));
        
        double thetaA = std::atan(a / D),
               thetaB = std::atan(b / D);
        
        auto [x, y] = sample;
        float sample_t = D * std::tan((1 - x) * thetaA + x * thetaB),
              pdf_t = D / (thetaB - thetaA) / (D * D + sample_t * sample_t);

        sample_t -= a;

        if (sample_t > tBounds) return nullptr;

        auto res = std::make_shared<MediumIntersectionInfo>();
        res->medium = this;
        res->position = ray.at(sample_t);
        res->wi = ray.dir;
        res->shadingFrame = Frame(ray.dir);
        res->pdf = pdf_t;
        res->weight = sigmaS(res->position) * evaluateTr(ray.ori, res->position) / pdf_t;
        res->distance = sample_t;
        return res;
    } else {
        //* Just sample propotional to tr
        return sampleIntersectionDeterministic(ray, tBounds, sample);
    }
}    