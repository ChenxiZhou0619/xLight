#include "core/render-core/medium.h"

class Homogeneous : public Medium {
public:
    Homogeneous() = default;
    Homogeneous(const rapidjson::Value &_value) {
        mDensity = getSpectrumRGB("density", _value);
        mAlbedo = getSpectrumRGB("albedo", _value);
        mEmission = getSpectrumRGB("emission", _value);
        
    }

    virtual bool sampleDistance(MediumSampleRecord *mRec,
                                const Ray3f &ray, float tmax,
                                Sampler *sampler) const override
    {
        auto [x, y] = sampler->next2D();

        //* Choose a channel using y
        int channel = std::min(int(y * 3), 2);

        float sigmaT = mDensity[channel];


        float distance = -std::log(1 - x) / sigmaT;
        if (distance <= tmax) {
            mRec->pathLength = distance;
            mRec->isValid = true;
            mRec->medium = this;
            mRec->sigmaS = mDensity * mAlbedo;
            mRec->sigmaA = mDensity * (SpectrumRGB{1} - mAlbedo);
            mRec->transmittance = getTrans(ray.ori, ray.at(distance));
            mRec->pdf = .0f;
            for (int i = 0; i < 3; ++i) {
                float pdf = mDensity[i] * mRec->transmittance[i];
                mRec->pdf += pdf / 3;
            }
            mRec->albedo = mAlbedo;
        } else {
            mRec->pathLength = tmax;
            mRec->isValid = false;
            mRec->medium = nullptr;
            mRec->transmittance = getTrans(ray.ori, ray.at(tmax));
            mRec->pdf = .0f;
            for (int i = 0; i < 3; ++i) {
                float pdf = mRec->transmittance[i];
                mRec->pdf += pdf / 3;
            }
        }
        return mRec->isValid;
    }

    virtual SpectrumRGB getTrans(Point3f start,
                                 Point3f end) const override
    {
        float dist = (end - start).length();
        return SpectrumRGB{
            std::exp(-mDensity[0] * dist),
            std::exp(-mDensity[1] * dist),
            std::exp(-mDensity[2] * dist)
        };
    }

    virtual SpectrumRGB Le(const Ray3f &ray) const override {
        return mEmission;
    }

    virtual ~Homogeneous() = default;

    virtual float pdfFromTo(Point3f from,
                            Point3f end,
                            bool isExceed) const override
    {
        float dist = (end - from).length();
        float pdf = .0f;
        
        if (isExceed) {
            for (int i = 0; i < 3; ++i) {
                pdf += std::exp(-mDensity[i] * dist) / 3;
            }
        } else {
            for (int i = 0; i < 3; ++i) {
                pdf += std::exp(-mDensity[i] * dist) * mDensity[i] / 3;
            }
        }
        return pdf;
    }

    
private:
    SpectrumRGB mDensity;
    SpectrumRGB mAlbedo;
    SpectrumRGB mEmission;
};

REGISTER_CLASS(Homogeneous, "homogeneous")
