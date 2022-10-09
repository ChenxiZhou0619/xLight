#include "core/render-core/bsdf.h"
#include "core/math/math.h"
#include "core/math/warp.h"
#include "spdlog/spdlog.h"

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