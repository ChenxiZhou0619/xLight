#include "core/render-core/hetergeneous.h"
#include <openvdb/tools/Interpolation.h>

bool Hetergeneous::sampleDistance(MediumSampleRecord *mRec, 
                                  const Ray3f &ray, 
                                  Sampler *sampler) const 
{
    //TODO
    return false;
}

SpectrumRGB Hetergeneous::getTrans(Point3f start, Point3f end) const 
{
    float step = .01;
    Vector3f dir = end - start; 
    float total = (end - start).length();

    openvdb::tools::GridSampler<
        openvdb::FloatGrid, openvdb::tools::BoxSampler
    > sampler(*density);

    float thick = .0f;

    for (float len = .0f; len < total; len += step) {
        Point3f p = start + len * dir;
        auto value = sampler.wsSample(openvdb::Vec3R(p.x, p.y, p.z)) * 10;

        thick += value * step;
    }

    return SpectrumRGB(std::exp(-thick));
}