#include "core/render-core/medium.h"
#include <core/render-core/info.h>

//* The medium only consider absorbtion, which means no scattering or emission
class Beerslaw : public Medium {
public:
    Beerslaw() = default;
    Beerslaw(const rapidjson::Value &_value) {
        m_absorbtion = getSpectrumRGB("sigma_a", _value);
    }
    virtual ~Beerslaw() = default;

    //* Sample the path length before next scatter
    virtual bool sampleDistance(MediumSampleRecord *m_rec,
                                const Ray3f &ray, float tmax,
                                Sampler *sampler) const override
    {
        // only scatter, so distance is infinity
        m_rec->pathLength = std::numeric_limits<float>::max();
        m_rec->pdf = 1;
        m_rec->transmittance = getTrans(ray.ori, ray.at(tmax));
        m_rec->medium = this;
        return false;
    }

    virtual bool samplePath(MediumSampleRecord *mRec,
                            const Ray3f &ray, float tmax,
                            const LightSourceInfo *info,
                            Sampler *sampler) const override
    {
        //todo 
    }

    virtual SpectrumRGB getTrans(Point3f start,
                                 Point3f end) const override
    {
        auto r = m_absorbtion.r(),
             g = m_absorbtion.g(),
             b = m_absorbtion.b();
        
        auto dist = (end - start).length();

        return SpectrumRGB{
            std::exp(-dist * r),
            std::exp(-dist * g),
            std::exp(-dist * b)
        };

    }

    virtual SpectrumRGB Le(const Ray3f &ray) const override {
        return SpectrumRGB{0};
    }

    virtual float pdfFromTo(Point3f from,
                            Point3f end,
                            bool isExceed) const override
    {
        if (isExceed) return 1;
        else return 0;
    }

    virtual std::shared_ptr<MediumIntersectionInfo>
    sampleIntersection(Ray3f ray, float tBounds, Point2f sample) const override
    {
        auto mIts = std::make_shared<MediumIntersectionInfo>();
        mIts->medium = nullptr; //* Cuz no possibility for inside intersection
        mIts->weight = getTrans(ray.ori, ray.at(tBounds));
        //* This is a delta distribution
        mIts->pdf = FINF;
        return mIts;
    }

    virtual std::shared_ptr<MediumIntersectionInfo>
    sampleIntersectionDeterministic(Ray3f ray, float tBounds, Point2f sample) const override
    {
        return nullptr;
    }


private:
    SpectrumRGB m_absorbtion;
};

REGISTER_CLASS(Beerslaw, "beerslaw")