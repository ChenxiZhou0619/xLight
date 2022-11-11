#include "core/render-core/hetergeneous.h"
#include <openvdb/tools/Interpolation.h>
#include <openvdb/tools/ValueTransformer.h>
#include <core/render-core/info.h>

Hetergeneous::Hetergeneous(openvdb::FloatGrid::Ptr _density, float scale) {
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

SpectrumRGB Hetergeneous::evaluateTr(Point3f start, Point3f end) const 
{
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
        mIts->weight = evaluateTr(ray.ori, ray.at(tBounds));
        return mIts;
}         