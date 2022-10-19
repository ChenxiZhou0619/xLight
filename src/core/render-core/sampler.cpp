#include "sampler.h"

float PixelSampler::next1D() {
    if (currentDimension1D < nDimensions)
        return samples1D[currentDimension1D++][currentSamplePixelIndex];
    else return dist(rng);
}

Point2f PixelSampler::next2D() {
    if (currentDimension2D < nDimensions) {
        return samples2D[currentDimension2D++][currentSamplePixelIndex];
    }
    else return Point2f(dist(rng), dist(rng));
}

Point3f PixelSampler::next3D() {
    float x = next1D();
    auto [y, z] = next2D();
    return {x, y, z};
}


