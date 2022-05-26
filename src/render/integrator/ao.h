#include "integrator.h"

class AoIntegrator : public Integrator {

public:
    virtual SpectrumRGB getLi(const Scene &scene, const Ray3f &ray) const;

};