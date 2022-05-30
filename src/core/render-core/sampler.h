#pragma once
#include "core/geometry/geometry.h"

class Sampler {
public:
    virtual float next1D() = 0;
    virtual Point2f next2D() = 0;
};