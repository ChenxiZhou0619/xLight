#include "core/render-core/bsdf.h"
#include "core/math/distribution.h"
#include "core/math/common.h"

class RoughDielectric : public BSDF {
public:
    RoughDielectric() = default;

    RoughDielectric(const rapidjson::Value &_value) {
        m_type = EBSDFType::EDiffuse;
        m_alpha = getFloat("alpha", _value);
        m_eta_i = getFloat("extIOR", _value);
        m_eta_t = getFloat("intIOR", _value);
    }

    virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const override {
        
        //! m_eta = eta_t / eta_i
        float eta = Frame::cosTheta(bRec.wi) > 0 ? (m_eta_t / m_eta_i) : (m_eta_i / m_eta_t);
        bool is_reflect = Frame::cosTheta(bRec.wi) * Frame::cosTheta(bRec.wo) >= 0;
        Vector3f m;
        if (is_reflect) {
            m = normalize(bRec.wi + bRec.wo);
            if (Frame::cosTheta(m) < 0)
                m = -m;
        }
        else 
            m = -normalize(bRec.wi * eta + bRec.wo);
        
        //! compute F, D, G
        float F = FresnelDielectricAccurate(dot(bRec.wi, m), eta);
        float D = BeckmannDistribution::D(m_alpha, m);
        float G = BeckmannDistribution::G(bRec.wi, bRec.wo, m_alpha);

        if (is_reflect) {
            return SpectrumRGB{0.25f * F * G * D / std::abs(Frame::cosTheta(bRec.wi))};
        } else {
            float top = std::abs(dot(m, bRec.wi) * dot(m, bRec.wo)) * eta * eta * (1 - F) * G * D;
            float denominator = std::abs(Frame::cosTheta(bRec.wi)) 
                                * (dot(m, bRec.wi) + eta * dot(m, bRec.wo)) 
                                * (dot(m, bRec.wi) + eta * dot(m, bRec.wo));
            return SpectrumRGB{top / denominator};
        }
    } 

    virtual float pdf(const BSDFQueryRecord &bRec) const override {
        //! m_eta = eta_t / eta_i
        float eta = Frame::cosTheta(bRec.wi) > 0 ? (m_eta_t / m_eta_i) : (m_eta_i / m_eta_t);
        bool is_reflect = Frame::cosTheta(bRec.wi) * Frame::cosTheta(bRec.wo) >= 0;
        Vector3f m;
        if (is_reflect) {
            m = normalize(bRec.wi + bRec.wo);
            if (Frame::cosTheta(m) < 0)
                m = -m;
        }
        else 
            m = -normalize(bRec.wi * eta + bRec.wo);
        
        //! compute F, D, G
        float F = FresnelDielectricAccurate(dot(bRec.wi, m), eta);
        float D = BeckmannDistribution::D(m_alpha, m);
        
        //TODO, if m is always positive semisphere??
        float m_pdf = D * std::abs(Frame::cosTheta(m));
        float dwh_dho;
        if (is_reflect) {
            dwh_dho = 0.25f / std::abs(dot(m, bRec.wo));
            m_pdf *= F;
        } else {
            dwh_dho = std::abs(eta * eta * dot(m, bRec.wo))  
                      / (dot(m, bRec.wi) + eta * dot(m, bRec.wo))
                      / (dot(m, bRec.wi) + eta * dot(m, bRec.wo));
            m_pdf *= (1 - F);
        }
        return m_pdf * dwh_dho;
    }

    virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample, float &pdf) const override {
        //! m_eta = eta_t / eta_i
        float eta = Frame::cosTheta(bRec.wi) > 0 ? (m_eta_t / m_eta_i) : (m_eta_i / m_eta_t);
        float microfacet_pdf;
        Vector3f m = BeckmannDistribution::sampleWh(m_alpha, sample, microfacet_pdf);

        float D = BeckmannDistribution::D(m_alpha, m);
        float F = FresnelDielectricAccurate(dot(m, bRec.wi), eta);

        bool is_reflect = 0.5f * (sample[0] + sample[1]) < F;

        if (is_reflect) {
            bRec.wo = 2.0f * dot(m, bRec.wi) * m - bRec.wi;
            microfacet_pdf *= F;
        } else {
            float inv_eta = 1 / eta;
            float cos_theta_i = Frame::cosTheta(bRec.wi);
            float cos_theta_t = 
                std::sqrt(
                    std::max(
                        .0f, 
                        1 - 
                        (1 - cos_theta_i * cos_theta_i) * inv_eta * inv_eta
                    )
                );
            if (cos_theta_i > 0)
                cos_theta_t = - cos_theta_t;
            bRec.wo = normalize(
                -inv_eta * bRec.wi 
                + (cos_theta_t + inv_eta * cos_theta_i) * m 
            );
            microfacet_pdf *= (1 - F);
        }
        if (is_reflect) {
            pdf = microfacet_pdf * 0.25f / std::abs(dot(m, bRec.wo));
        } else {
            pdf = microfacet_pdf *
                     std::abs(eta * eta * dot(m, bRec.wo))  
                      / (dot(m, bRec.wi) + eta * dot(m, bRec.wo))
                      / (dot(m, bRec.wi) + eta * dot(m, bRec.wo));
        }
        float G = BeckmannDistribution::G(bRec.wi, bRec.wo, m_alpha);
        return SpectrumRGB{
            std::abs(
                dot(m, bRec.wi) 
                / (Frame::cosTheta(bRec.wi) * Frame::cosTheta(m))
            ) * G
        };
    }

    virtual bool isDiffuse() const override {
        return true;
    }

private:
    float m_alpha;      // indicate the roughness

    float m_eta_i;      // default the extern eta

    float m_eta_t;      // default the internal eta
};

REGISTER_CLASS(RoughDielectric, "rough-dielectric")