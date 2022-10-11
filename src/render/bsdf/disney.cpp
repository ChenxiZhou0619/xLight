#include "core/render-core/bsdf.h"
#include "core/math/math.h"
#include "core/math/warp.h"
#include "spdlog/spdlog.h"
#include "ndf.h"

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
                               float &pdf) const override 
    {
        if (Frame::cosTheta(bRec.wi) <= 0) {
                pdf = .0f;
                return SpectrumRGB {.0f};
        }        
        bRec.wo = Warp::squareToCosineHemisphere(sample);
        pdf = Warp::squareToCosineHemispherePdf(bRec.wo);
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
                               float &pdf) const override
    {
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