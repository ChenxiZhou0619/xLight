#include "core/render-core/medium.h"

class Homogeneous : public Medium {
public:
    Homogeneous() = default;
    Homogeneous(const rapidjson::Value &_value) {
        mDensity = getFloat("density", _value);
        mAlbedo = getFloat("albedo", _value);
        mEmission = getSpectrumRGB("emission", _value);
        mPhase = std::make_shared<IsotropicPhase>();
    }

    virtual bool sampleDistance(MediumSampleRecord *mRec,
                                const Ray3f &ray,
                                Sampler *sampler) const override
    {
        auto [x, y] = sampler->next2D();
        float distance = -std::log(1 - x) / mDensity;
        if (distance <= ray.tmax) {
            mRec->pathLength = distance;
            mRec->isValid = true;
            mRec->medium = this;
            mRec->sigmaS = SpectrumRGB{mDensity * mAlbedo};
            mRec->sigmaA = SpectrumRGB{mDensity * (1 - mAlbedo)};
            mRec->transmittance = getTrans(ray.ori, ray.at(distance));
            mRec->pdf = mDensity * std::exp(-mDensity * distance);
            mRec->albedo = mAlbedo;
        } else {
            mRec->pathLength = ray.tmax;
            mRec->isValid = false;
            mRec->medium = nullptr;
            mRec->transmittance = getTrans(ray.ori, ray.at(ray.tmax));
            mRec->pdf = std::exp(-mDensity * ray.tmax);
        }
        return mRec->isValid;
    }

    virtual SpectrumRGB getTrans(Point3f start,
                                 Point3f end) const override
    {
        float dist = (end - start).length();
        float thick = std::exp(-mDensity * dist);
        return SpectrumRGB{thick};
    }

    virtual SpectrumRGB Le(const Ray3f &ray) const override {
        return mEmission;
    }

    virtual ~Homogeneous() = default;

    
private:
    float mDensity;
    float mAlbedo;
    SpectrumRGB mEmission;
};

REGISTER_CLASS(Homogeneous, "homogeneous")
