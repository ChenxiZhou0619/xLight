#pragma once

#include "core/core.h"

class Integrator {
public:
    virtual SpectrumRGB getLi(const Scene &scene, const Ray3f &ray) const = 0;

};