#pragma once
#include "core/render-core/medium.h"

class PathSampler {
public:
    virtual std::pair<float, float> 
    sampleDistance(Point3f origin, Vector3f dir,
                   float tBound, const LightSourceInfo *info,
                   Point2f sample) const = 0;
};

//* See 
//* Importance Sampling Techniques for Path Tracing inParticipating Media

class EquiAngular : public PathSampler {
public:
    virtual std::pair<float, float>
    sampleDistance(Point3f origin, Vector3f dir,
                   float tBound, const LightSourceInfo *info,
                   Point2f sample) const override
    {
        assert(info != nullptr);
        Vector3f ori2light = info->lightSourcePoint - origin;
        double a = dot(ori2light, dir),
               b = tBound + a,
               D = std::sqrt(std::max(.0, ori2light.length2() - a * a));
        
        double thetaA = std::atan(a / D),
               thetaB = std::atan(b / D);

        auto [x, y] = sample;
        float sampleT = D * std::tan((1 - x) * thetaA + x * thetaB),
              pdfT = D / (thetaB - thetaA) / (D * D + sampleT * sampleT);
        
        sampleT -= a;

        return {sampleT, pdfT};    
    }
};