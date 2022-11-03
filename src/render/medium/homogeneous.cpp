#include "core/render-core/medium.h"
#include <core/render-core/info.h>
#include "pathsampler.h"

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

    virtual bool samplePath(MediumSampleRecord *mRec,
                            const Ray3f &ray, float tmax,
                            const LightSourceInfo *info,
                            Sampler *sampler) const override
    {
        auto[dist, distPdf] = 
            pathSampler->sampleDistance(ray.ori, 
                                        ray.dir, 
                                        tmax, info, 
                                        sampler->next2D());
        if (dist <= tmax) {
            mRec->pathLength = dist;
            mRec->isValid = true;
            mRec->medium = this;
            mRec->sigmaS = mDensity * mAlbedo;
            mRec->sigmaA = mDensity * (SpectrumRGB{1} - mAlbedo);
            mRec->transmittance = getTrans(ray.ori, ray.at(dist));
            mRec->pdf = info->pdf * distPdf;
            mRec->albedo = mAlbedo;
        } else {
            //todo test if equi-angular can archive here
            mRec->pathLength = tmax;
            mRec->isValid = false;
            mRec->medium = nullptr;
            mRec->pdf = info->pdf * distPdf;
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

    virtual std::shared_ptr<MediumIntersectionInfo>
    sampleIntersection(Ray3f ray, float tBounds, Point2f sample) const override
    {
        auto [x, y] = sample;
        auto mIts = std::make_shared<MediumIntersectionInfo>();
        //* Choose a channel using y
        int channel = std::min(int(y * 3), 2);
        float sigmaT = mDensity[channel];
        float distance = -std::log(1 - x) / sigmaT;
        if (distance <= tBounds) {
            //* Sample a medium intersection
            mIts->medium = this;
            mIts->position = ray.at(distance);
            mIts->distance = distance;
            mIts->wi = ray.dir;
            mIts->shadingFrame = Frame{mIts->wi};
            mIts->pdf = .0f;
            SpectrumRGB tr = getTrans(ray.ori, mIts->position);
            for (int i = 0; i < 3; ++i) {
                float pdf = mDensity[i] * tr[i];
                mIts->pdf += pdf / 3;
            }
            //todo ONLY SCATTER KNOW, SO ONLY SIGMAS HERE
            mIts->weight = mDensity * mAlbedo * tr / mIts->pdf;
        } else {
            //* No, escape the medium
            mIts->medium = nullptr;
            mIts->pdf = .0f;
            SpectrumRGB tr = getTrans(ray.ori, ray.at(tBounds));
            for (int i = 0; i < 3; ++i) {
                float pdf =  tr[i];
                mIts->pdf += pdf / 3;
            }
            mIts->weight = tr / mIts->pdf;
        }
        return mIts;
    }

    virtual std::shared_ptr<MediumIntersectionInfo>
    sampleIntersectionDeterministic(Ray3f ray, float tBounds, Point2f sample) const override
    {
        auto [x, y] = sample;
        auto mIts = std::make_shared<MediumIntersectionInfo>();
        //* Choose a channel using y
        int channel = std::min(int(y * 3), 2);
        float sigmaT = mDensity[channel],
              thick = std::exp(-tBounds * sigmaT);
        float distance = -std::log(1 - x * (1 - thick)) / sigmaT;

        if (distance > tBounds) {
            return nullptr;
        }

        mIts->medium = this;
        mIts->position = ray.at(distance);
        mIts->wi = ray.dir;
        mIts->shadingFrame = Frame{mIts->wi};
        SpectrumRGB tr = getTrans(ray.ori, mIts->position);
        mIts->pdf = .0f;
        for (int i = 0; i < 3; ++i) {
            float pdf = mDensity[i] * tr[i] / (1 - thick);
            mIts->pdf += pdf / 3;
        }
        mIts->weight = mDensity * mAlbedo * tr / mIts->pdf;
        mIts->distance = distance;
        float tmp = mIts->weight.average() + thick;
        return mIts;
    }
    
private:
    SpectrumRGB mDensity;
    SpectrumRGB mAlbedo;
    SpectrumRGB mEmission;

    std::shared_ptr<PathSampler> pathSampler = std::make_shared<EquiAngular>();
};

REGISTER_CLASS(Homogeneous, "homogeneous")
