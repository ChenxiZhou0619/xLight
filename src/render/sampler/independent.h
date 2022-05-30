#pragma once
#include "core/render-core/sampler.h"
#include <random>

class Independent : public Sampler {
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> dist;
public:
    Independent() : gen(rd()), dist(0.0, 1.0) { }

    virtual float next1D();

    virtual Point2f next2D();
};