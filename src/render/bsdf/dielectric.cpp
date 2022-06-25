#include "core/render-core/bsdf.h"
#include "core/math/common.h"
class Dielectric : public BSDF {
    float intIOR, extIOR;
    float eta, invEta;

    // cosThetaT = cos<wo, normal>
    Vector3f refract(const Vector3f &wi, float cosThetaT) const {
        float scale = -(cosThetaT < 0 ? invEta : eta);
        return Vector3f(scale * wi.x, cosThetaT, scale * wi.z);
    }
public:
    Dielectric() = default;
    
    Dielectric(const rapidjson::Value &_value) {
        intIOR = getFloat("eta", _value);
        // outside default air
        extIOR = 1.f;

        eta = intIOR / extIOR;
        invEta = 1 / eta; 

        //TODO add reflectance/transmittance
    }

    ~Dielectric() = default;

    // ! always zero if evaluate
    virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const override{
        return SpectrumRGB {.0f};
    }

    virtual float pdf(const BSDFQueryRecord &bRec) const override{
        float cosThetaT;
        float F = fresnelDielectric(Frame::cosTheta(bRec.wi), extIOR, intIOR, cosThetaT);
        if (Frame::cosTheta(bRec.wi) * Frame::cosTheta(bRec.wo) >= 0) {
            // same side, reflect
            return F;
        } else {
            // different side, refract
            return 1 - F;
        }
    }

    virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample, float &pdf) const override{
        float cosThetaT;
        float F = fresnelDielectric(Frame::cosTheta(bRec.wi), extIOR, intIOR, cosThetaT);

        if (sample.x <= F) {
            // if sample < F, reflect
            bRec.wo = reflect(bRec.wi);
            pdf = F;

            // TODO replace with specular reflectance
            return SpectrumRGB {1.f};
        } else {
            // if sample > F, refract
            bRec.wo = refract(bRec.wi, cosThetaT);
            pdf = 1 - F;
            float factor = cosThetaT < 0 ? invEta : eta;
            // TODO replace with specular transmittance
            return SpectrumRGB{factor * factor};
        }
    }

    virtual bool isDiffuse() const{
        return false;
    }
};

REGISTER_CLASS(Dielectric, "dielectric")