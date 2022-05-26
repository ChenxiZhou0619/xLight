#include "simple.h"
#include <iostream>

SpectrumRGB SimpleIntegrator::getLi(const Scene &scene, const Ray3f &ray) const {
    RayIntersectionRec iRec;
    if (!scene.rayIntersect(ray, iRec))
        return SpectrumRGB{.0f};

    Vector3f dir = m_lightPosition - iRec.p;
    Ray3f shadowRay {
        iRec.p, dir, .0f, EPSILON, dir.length()
    };
    //std::cout << shadowRay << std::endl;

    if (!scene.rayIntersect(shadowRay)) {
        SpectrumRGB Li = m_lightEnergy * (.25f * INV_PI * INV_PI)
            * std::max(.0f, dot(iRec.shdN, shadowRay.dir)) / dir.length2();
        //std::cout << Li << std::endl;
        return Li;
    }
    return SpectrumRGB {.0f};
}