#include "core/render-core/bsdf.h"
#include "core/math/math.h"
#include "core/math/warp.h"
#include "spdlog/spdlog.h"
#include "ndf.h"
#include <core/render-core/info.h>

class DisneyDiffuse : public BSDF {
public:
    DisneyDiffuse() = default;

    DisneyDiffuse(const rapidjson::Value &_value) {
        mRoughness = getFloat("roughness", _value);
        mSubsurface = getFloat("subsurface", _value);
        m_type = EBSDFType::EGlossy;
    }

    virtual ~DisneyDiffuse() = default;

    virtual bool isDiffuse() const override{
        return true;;
    }    

    virtual SpectrumRGB evaluate (const BSDFQueryRecord &bRec) const override{
        if (Frame::cosTheta(bRec.wi) <= 0 || Frame::cosTheta(bRec.wo) <= 0)
            return SpectrumRGB {.0f};

        Vector3f half = normalize(bRec.wi + bRec.wo);

        //* Compute diffuse        
        float fd90 = 0.5f + 2 * mRoughness * std::pow(dot(half, bRec.wo), 2); 
        float fdi = FresnelDiffuse(fd90, Frame::cosTheta(bRec.wi)),
              fdo = FresnelDiffuse(fd90, Frame::cosTheta(bRec.wo));

        SpectrumRGB diffuse 
            = m_texture->evaluate(bRec.uv) * INV_PI * fdi * fdo * Frame::cosTheta(bRec.wo);
        
        //* Compute subsurface
        float fss90 = mRoughness * std::pow(dot(half, bRec.wo), 2);
        float fssi = FresnelSubserface(fss90, Frame::cosTheta(bRec.wi)),
              fsso = FresnelSubserface(fss90, Frame::cosTheta(bRec.wo));
        SpectrumRGB subsurface 
            = m_texture->evaluate(bRec.uv) * INV_PI * 1.25 * 
              (fssi * fsso * (1 / (Frame::cosTheta(bRec.wi) + Frame::cosTheta(bRec.wo)) - 0.5) + 0.5) * 
              Frame::cosTheta(bRec.wo);

        return diffuse * (1 - mSubsurface) + subsurface * mSubsurface;
    }
    
    virtual SpectrumRGB sample(BSDFQueryRecord &bRec, 
                               const Point2f &sample,
                               float &pdf,
                               ScatterSampleType *type) const override 
    {
        if (Frame::cosTheta(bRec.wi) <= 0) {
                pdf = .0f;
                return SpectrumRGB {.0f};
        }        
        bRec.wo = Warp::squareToCosineHemisphere(sample);
        pdf = Warp::squareToCosineHemispherePdf(bRec.wo);
        *type = ScatterSampleType::SurfaceReflection;
        return evaluate(bRec) / pdf;
    }

    virtual float pdf(const BSDFQueryRecord &bRec) const override {
        if (Frame::cosTheta(bRec.wi) <= 0 || Frame::cosTheta(bRec.wo) <= 0)
            return .0f;
        return Warp::squareToCosineHemispherePdf(bRec.wo);
    }

protected:
    float mRoughness;
    float mSubsurface;

    float FresnelDiffuse(float fd90, float cosTheta) const {
        return (
            1 + 
            (fd90 - 1) * std::pow(1 - cosTheta, 5)
        );
    }

    float FresnelSubserface(float fss90, float cosTheta) const {
        return (
            1 + 
            (fss90 - 1) * std::pow(1 - cosTheta, 5)
        );
    }

};
REGISTER_CLASS(DisneyDiffuse, "disney-diffuse");


class DisneyMetal : public BSDF {
public:
    DisneyMetal() = default;

    DisneyMetal(const rapidjson::Value &_value) {
        mRoughness = getFloat("roughness", _value);
        mAnistropic = getFloat("anistropic", _value);
        ndf = std::make_shared<GGX>(mRoughness, mAnistropic);
        m_type = EBSDFType::EGlossy;
    }

    virtual ~DisneyMetal() = default;

    //todo fixme
    virtual bool isDiffuse() const override {
        return true;
    }

    virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const override
    {
        if (Frame::cosTheta(bRec.wi) <= 0 || Frame::cosTheta(bRec.wo) <= 0)
        {
            return SpectrumRGB{.0f};
        }

        Vector3f half = normalize(bRec.wi + bRec.wo);
        SpectrumRGB baseColor = m_texture->evaluate(bRec.uv);
        SpectrumRGB Fresnel = 
            baseColor + 
            SpectrumRGB(1 - baseColor.max()) * std::pow(1 - dot(half, bRec.wo), 5); 

        float D = ndf->eval(half);
        float G = ndf->G(bRec.wi, bRec.wo);

        return Fresnel * D * G * 0.25f / Frame::cosTheta(bRec.wi);
        
    }

    virtual SpectrumRGB sample(BSDFQueryRecord &bRec,
                               const Point2f &sample,
                               float &pdf,
                               ScatterSampleType *type) const override
    {
        *type = ScatterSampleType::SurfaceReflection;
        auto [half, halfPdf] = ndf->sample(bRec.wi, sample);
        bRec.wo = normalize(2 * dot(half, bRec.wi) * half - bRec.wi);
        pdf = halfPdf / (4 * dot(bRec.wo, half));
        if (pdf == 0) 
            return SpectrumRGB{.0f};

        SpectrumRGB baseColor = m_texture->evaluate(bRec.uv);
        SpectrumRGB Fresnel = 
            baseColor + 
            SpectrumRGB(1 - baseColor.max()) * std::pow(1 - dot(half, bRec.wo), 5); 

        auto res = Fresnel / (1 + ndf->Lambda(bRec.wo))
            * dot(half, bRec.wo) / dot(half, bRec.wi);
        
        return res;
    }

    virtual float pdf(const BSDFQueryRecord &bRec) const override
    {
        Vector3f half = normalize(bRec.wi + bRec.wo);
        return ndf->pdf(bRec.wi, half) / (4 * dot(bRec.wo, half));
    }


protected:
    float mRoughness;
    float mAnistropic;
    const float mAlphaMin = 0.0001f;
    std::shared_ptr<NDF> ndf = nullptr;
};
REGISTER_CLASS(DisneyMetal, "disney-metal")

class DisneyClearcoat : public BSDF {
public:
    DisneyClearcoat() = default;

    DisneyClearcoat(const rapidjson::Value &_value) {
        mClearcoatGloss = getFloat("glossy", _value);
        mAlphaG = (1 - mClearcoatGloss) * 0.1 + mClearcoatGloss * 0.001;
        m_type = EBSDFType::EGlossy;
    }

    virtual ~DisneyClearcoat() = default;

    virtual bool isDiffuse() const override{
        return false;
    }

    virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const override
    {
        Vector3f half = normalize(bRec.wi + bRec.wo);
        SpectrumRGB Fresnel = SpectrumRGB(R0 + (1 - R0) * std::pow(1 - dot(half, bRec.wo), 5));
        auto D = (mAlphaG * mAlphaG - 1) * INV_PI /
            std::log(mAlphaG * mAlphaG) /
            (1 + (mAlphaG * mAlphaG - 1) * half.y * half.y);
        auto g = G(bRec.wi) * G(bRec.wo);
        return Fresnel * D * g * 0.25 / Frame::cosTheta(bRec.wi);
    }

    virtual SpectrumRGB sample(BSDFQueryRecord &bRec,
                               const Point2f &sample,
                               float &pdf,
                               ScatterSampleType *type) const override
    {
        *type = ScatterSampleType::SurfaceReflection;
        float cosElevation = std::sqrt((1 - std::pow(mAlphaG * mAlphaG, 1 - sample.x)) / (1 - mAlphaG * mAlphaG)),
              sinElevation = std::sqrt(1 - cosElevation * cosElevation);
        float azimuth = 2 * M_PI * sample.y;

        Normal3f half {sinElevation * std::cos(azimuth), cosElevation, sinElevation * std::sin(azimuth)};
        bRec.wo = normalize(2 * dot(bRec.wi, half) * half - bRec.wi);

        if (Frame::cosTheta(bRec.wo) < 0) {
            pdf = 0;
            return SpectrumRGB{.0f};
        }

        float pdfHalf = (mAlphaG * mAlphaG - 1) * cosElevation * sinElevation / 
            (mAlphaG * mAlphaG * cosElevation * cosElevation + sinElevation * sinElevation) /
            std::log(mAlphaG);
        pdf = pdfHalf * 0.25 / dot(half, bRec.wo);

        SpectrumRGB Fresnel = SpectrumRGB(R0 + (1 - R0) * std::pow(1 - dot(half, bRec.wo), 5));
        auto g = G(bRec.wi) * G(bRec.wo);

        auto res = Fresnel * 2 * g * Frame::cosTheta(bRec.wo) /
            (M_PI * Frame::cosTheta(bRec.wi) * cosElevation * sinElevation); 
        return res;        
    }

    virtual float pdf(const BSDFQueryRecord &bRec) const override
    {   

        Normal3f half = normalize(bRec.wi + bRec.wo);
        float cosElevation = half.y,
              sinElevation = std::sqrt(1 - cosElevation * cosElevation);
        float pdfHalf = (mAlphaG * mAlphaG - 1) * cosElevation * sinElevation / 
            (mAlphaG * mAlphaG * cosElevation * cosElevation + sinElevation * sinElevation) /
            std::log(mAlphaG);
        return pdfHalf * 0.25 / dot(half, bRec.wo);
    }
protected:
    float mClearcoatGloss;
    float mAlphaG;
    const float R0 = 0.04;

    float G(Vector3f w) const {
        float Sigma = std::sqrt((1 + (std::pow(w.x * 0.25, 2) + std::pow(w.z * 0.25, 2)) / (w.y * w.y))) + 1.f;
        Sigma *= 0.5;
        return 1 / Sigma;
    }
};

REGISTER_CLASS(DisneyClearcoat, "disney-clearcoat")