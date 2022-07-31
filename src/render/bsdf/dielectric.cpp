#include "core/render-core/bsdf.h"
#include "core/math/common.h"
class Dielectric : public BSDF {
    float extIOR, intIOR;
    
    Vector3f refract(const Vector3f &wi, float eta, float cosThetaT) const {
        float scale = -((cosThetaT < 0) ? (1 / eta) : eta);
        return Vector3f {wi.x * scale, cosThetaT, wi.z * scale};
    }

public:
    Dielectric() = default;
    
    Dielectric(const rapidjson::Value &_value) {
        m_type = EBSDFType::EGlossy;

        extIOR = getFloat("extIOR", _value);
        intIOR = getFloat("intIOR", _value);
        // TODO transmittance/reflectance
    }

    ~Dielectric() = default;

    // almost delta distribution, always return 0 
    virtual SpectrumRGB evaluate (const BSDFQueryRecord &bRec) const override {
        return SpectrumRGB {.0f};
    }

    virtual float pdf(const BSDFQueryRecord &bRec) const override {
        // F if reflect, 1 - F if refract
        float cosThetaT;
        float F = fresnelDielectric(
            Frame::cosTheta(bRec.wi), 
            (intIOR / extIOR), 
            cosThetaT
        );
        if (Frame::cosTheta(bRec.wi) * Frame::cosTheta(bRec.wo) >= 0) {
            // same side
            return F;
        } else {
            return 1 - F;
        }
    }

    virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample, float &pdf) const override {
        float cosThetaT;
        float F = fresnelDielectric(
            Frame::cosTheta(bRec.wi),
            (intIOR / extIOR), 
            cosThetaT
        );

        if (sample.x <= F) {
            // reflect
            bRec.wo = reflect(bRec.wi);
            pdf = F;
            // TODO replace with specular reflectance
            return SpectrumRGB {1.f};
        } else {
            // refract
            bRec.wo = refract(bRec.wi, (intIOR / extIOR), cosThetaT);
            pdf = 1 - F;
            // TODO consider the transmittance and factor
            return SpectrumRGB {1.f};
        }
    }

    virtual bool isDiffuse() const override {
        return false;
    }

    
};

REGISTER_CLASS(Dielectric, "dielectric")