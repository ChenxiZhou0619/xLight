#include "core/render-core/integrator.h"

class AoIntegrator : public Integrator {

public:
    virtual SpectrumRGB getLi(const Scene &scene, const Ray3f &ray) const {
        RayIntersectionRec iRec;
        if (scene.rayIntersect(ray, iRec)) {



        } else {
            return SpectrumRGB {.0f};
        }
    }
};
