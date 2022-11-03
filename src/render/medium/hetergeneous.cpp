#include "core/render-core/hetergeneous.h"
#include <openvdb/tools/Interpolation.h>
#include <openvdb/tools/ValueTransformer.h>
#include <core/render-core/info.h>

Hetergeneous::Hetergeneous(openvdb::FloatGrid::Ptr _density) {
    density = _density;
    float tmax = .0f;
    openvdb::tools::foreach(
        density->beginValueOn(), 
        [&](const openvdb::FloatGrid::ValueOnIter &itr) {
            itr.setValue(*itr * scale);
            tmax = tmax > *itr ? tmax : *itr;
        }
    );
    sigmaTMax = SpectrumRGB{tmax};
}


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
        mRec->albedo = SpectrumRGB{1.0f};
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
    /*
    Vector3f dir = end - start; 
    float total = (end - start).length();

    auto constAccessor = density->getConstAccessor();
    openvdb::tools::GridSampler<
        openvdb::FloatGrid::ConstAccessor, 
        openvdb::tools::PointSampler
    > sampler(constAccessor, density->constTransform());



    float thick = .0f;

    for (float len = .0f; len < total; len += step) {
        Point3f p = start + len * dir;
        //TODO fixme
        auto value = sampler.wsSample(openvdb::Vec3R(p.x, p.y, p.z));

        thick += value * step;
    }
    return SpectrumRGB(std::exp(-thick));
*/
    static auto constAccessor = density->getConstAccessor();
    static openvdb::tools::GridSampler<
        openvdb::FloatGrid::ConstAccessor, 
        openvdb::tools::PointSampler
    > sampler(constAccessor, density->constTransform());

    float sigma_t_max = sigmaTMax.average(),
          inv_sigma_t_max = 1.0 / sigma_t_max; 
    float distance = (end - start).length(),
          t = 0;
    Vector3f dir = normalize(end - start);
    SpectrumRGB Tr{1};

    while(true) {
        t -= std::log(1 - Sampler::sample1D()) * inv_sigma_t_max;
        if (t >= distance)
            break;
        Point3f p = start + t * dir;
        float density = sampler.wsSample(openvdb::Vec3R(p.x, p.y, p.z));
        Tr *= SpectrumRGB{1 - std::max(.0f, density * inv_sigma_t_max)};
    }

    return Tr;
}

float Hetergeneous::pdfFromTo(Point3f from, 
                              Point3f end, 
                              bool isExceed) const 
{   

    //todo
    return 1;
}

std::shared_ptr<MediumIntersectionInfo>
Hetergeneous::sampleIntersection(Ray3f ray, float tBounds, Point2f sample) const
{
    auto mIts = std::make_shared<MediumIntersectionInfo>();

    static auto constAccessor = density->getConstAccessor();
    static openvdb::tools::GridSampler<
        openvdb::FloatGrid::ConstAccessor, 
        openvdb::tools::PointSampler
    > mediumQuery(constAccessor, density->constTransform());

    auto [x, y] = sample;
    int channel = std::min(int(y * 3), 2);
    float sigma_t_max = sigmaTMax[channel],
          inv_sigma_t_max = 1.0 / sigma_t_max,
          distance = 0; 

    //* Delta tracking
    while(true) {
        distance += -std::log(1 - Sampler::sample1D()) * inv_sigma_t_max;
        if (distance >= tBounds) {
            mIts->medium = nullptr;
            mIts->position = ray.at(tBounds);
            mIts->distance = tBounds;
            mIts->weight = SpectrumRGB{1};
            break;
        }
        Point3f p = ray.at(distance);
        float sigma_t = mediumQuery.wsSample(openvdb::Vec3R(p.x, p.y, p.z));
        if (Sampler::sample1D() < sigma_t * inv_sigma_t_max) {
            //* Yes, scatter
            mIts->medium = this;
            mIts->position = ray.at(distance);
            mIts->wi = ray.dir;
            mIts->shadingFrame = Frame{mIts->wi};
            mIts->weight = SpectrumRGB{1};
            break;
        }
    }
    return mIts;
}

std::shared_ptr<MediumIntersectionInfo>
Hetergeneous::sampleIntersectionDeterministic(Ray3f ray, float tBounds, 
                                              Point2f sample) const 
{
        auto [x, y] = sample;
        auto mIts = std::make_shared<MediumIntersectionInfo>();
        //* Choose a channel using y
        int channel = std::min(int(y * 3), 2);
        //float sigmaT = mDensity[channel];

        //* Just return out
        mIts->medium = nullptr;
        mIts->pdf = FINF;
        mIts->weight = getTrans(ray.ori, ray.at(tBounds));
        return mIts;
}         