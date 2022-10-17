#include "core/render-core/hetergeneous.h"
#include <openvdb/tools/Interpolation.h>

bool Hetergeneous::sampleDistance(MediumSampleRecord *mRec, 
                                  const Ray3f &ray, float tmax,
                                  Sampler *sampler) const 
{
    auto [x, y] = sampler->next2D();

    auto constAccessor = density->getConstAccessor();
    openvdb::tools::GridSampler<
        openvdb::FloatGrid::ConstAccessor, 
        openvdb::tools::PointSampler
    > mediumQuery(constAccessor, density->constTransform());

    float thick = -std::log(1 - x);
    float accumulate = .0f;
    float len = .0f;

    Point3f origin = ray.ori;
    Vector3f dir = ray.dir;

    while(accumulate < thick && len < tmax) {
        Point3f p = origin + dir * len;
        auto value = mediumQuery.wsSample(openvdb::Vec3R(p.x, p.y, p.z)) * scale;
        accumulate += value * step;
        len += step;
    }

    if (len <= tmax) {
        mRec->pathLength = len;
        mRec->isValid = true;
        mRec->medium = this;
        mRec->sigmaA = SpectrumRGB(0);
        Point3f p = origin + dir * len;
        auto sigma_t = mediumQuery.wsSample(openvdb::Vec3R{p.x, p.y, p.z}) * scale; 
        mRec->sigmaS = SpectrumRGB{sigma_t};
        mRec->transmittance = SpectrumRGB(std::exp(-accumulate));
        mRec->pdf = sigma_t * std::exp(-accumulate);
        mRec->albedo = 1.0f;
    } else {
        mRec->pathLength = tmax;
        mRec->isValid = false;
        mRec->medium = nullptr;
        mRec->transmittance = SpectrumRGB(std::exp(-accumulate));
        mRec->pdf = std::exp(-accumulate);
    }
    return mRec->isValid;
}

SpectrumRGB Hetergeneous::getTrans(Point3f start, Point3f end) const 
{

    Vector3f dir = end - start; 
    float total = (end - start).length();

    auto constAccessor = density->getConstAccessor();
    openvdb::tools::GridSampler<
        openvdb::FloatGrid::ConstAccessor, 
        openvdb::tools::PointSampler
    > sampler(constAccessor, density->constTransform());

//    openvdb::tools::GridSampler<
//        openvdb::FloatGrid, openvdb::tools::BoxSampler
//    > sampler(*density);

    float thick = .0f;

//    std::cout << "Total = " << total << std::endl;
    for (float len = .0f; len < total; len += step) {
        Point3f p = start + len * dir;
        //TODO fixme
        auto value = sampler.wsSample(openvdb::Vec3R(p.x, p.y, p.z)) * scale;

        thick += value * step;
    }

    return SpectrumRGB(std::exp(-thick));
}

float Hetergeneous::pdfFromTo(Point3f from, 
                              Point3f end, 
                              bool isExceed) const 
{   

    //todo
    return 1;
}