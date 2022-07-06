#include "core/render-core/bsdf.h"
#include "core/math/common.h"
#include "core/math/warp.h"

//! lot of bugs
class Plastic : public BSDF {

    float intIOR, extIOR;

    float specularReflectance;

    float specularSamplingWeight;

public:

    Plastic() = default;

    Plastic(const rapidjson::Value &_value) {
        intIOR = getFloat("intIOR", _value);
        extIOR = getFloat("extIOR", _value);
        
        // TODO, replace it with a texture
        specularReflectance = 1.0f;

        // TODO, change it
        specularSamplingWeight = specularReflectance 
                                / (specularReflectance + 0.6f);
    }

    virtual SpectrumRGB evaluate (const BSDFQueryRecord &bRec) const override {
        float cosThetaT;
        float eta = (intIOR/extIOR);
        float Fi = fresnelDielectric(
            Frame::cosTheta(bRec.wi), 
            eta, 
            cosThetaT
        );
        if (std::abs(dot(reflect(bRec.wi), bRec.wo) - 1) < EPSILON) {
            return SpectrumRGB{Fi * specularReflectance};
        } else {
            float Fo = fresnelDielectric(Frame::cosTheta(bRec.wo), eta, cosThetaT);
            return
                m_texture->evaluate(bRec.uv)
                * Warp::squareToCosineHemispherePdf(bRec.wo)
                * (1 - Fi)
                / eta / eta;  
        }
    }

    virtual float pdf(const BSDFQueryRecord &bRec) const override {
        float cosThetaT;
        float eta = (intIOR / extIOR);
        float Fi = fresnelDielectric(Frame::cosTheta(bRec.wi), eta, cosThetaT);
        float probSpecular = 
            Fi * specularSamplingWeight
            / (Fi * specularSamplingWeight + (1 - Fi) * (1 - specularSamplingWeight));
    
        if (std::abs(dot(reflect(bRec.wi), bRec.wo) - 1) < EPSILON) {
            return probSpecular;
        } else {
            return Warp::squareToCosineHemispherePdf(bRec.wo) * (1 - probSpecular);
        }
    }

    virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample, float &pdf) const override {
        float eta = (intIOR / extIOR);
        float cosThetaT;
        float F = fresnelDielectric(Frame::cosTheta(bRec.wi), eta, cosThetaT);

        float probSpecular = (F * specularSamplingWeight)
                             / (F * specularSamplingWeight + (1 - F) * (1 - specularSamplingWeight));
        if (sample[0] < probSpecular) {
            bRec.wo = reflect(bRec.wi);
            return SpectrumRGB {
                specularReflectance * F / probSpecular
            };
        } else {
            bRec.wo = Warp::squareToCosineHemisphere(
                Point2f {
                    (sample.x - probSpecular)/(1 - probSpecular),
                    sample.y
                }
            );
            float cosThetaT;
            float Fo = fresnelDielectric(
                Frame::cosTheta(bRec.wo),
                eta,
                cosThetaT
            );
            SpectrumRGB diff = m_texture->evaluate(bRec.uv);
            pdf = (1 - probSpecular) * Warp::squareToCosineHemispherePdf(bRec.wo);
            return diff 
                    * ((1 / eta) * (1 / eta))
                    *((1 - F) * (1 - Fo))
                    / (1 - probSpecular);
        }
    }

    virtual bool isDiffuse() const override {
        return true;
    }


};

REGISTER_CLASS(Plastic, "plastic")