#include "core/render-core/bsdf.h"
#include "core/math/distribution.h"
#include "core/math/common.h"

class RoughDielectric : public BSDF {

    float intIOR, extIOR;

    float alpha;

    float m_eta;

    float m_inv_eta;


    Vector3f reflect(Vector3f i, Vector3f m) const {
        Vector3f o = 2 * dot(i, m) * m - i;
        return normalize(o);
    }

    Vector3f refract(Vector3f i, Vector3f m) const {
        float eta = extIOR / intIOR;
        if (dot(i, m) < 0) {
            eta = 1 / eta;
        }
        float c = dot(i, m);
        Vector3f o = (
            eta * c 
            - sign(Frame::cosTheta(i)) * std::sqrt(1 + eta * (c * c - 1))
        ) * m - eta * i;
        return normalize(o);
    }

public:

    RoughDielectric() = default;

    RoughDielectric(const rapidjson::Value &_value) {
        intIOR = getFloat("intIOR", _value);
        extIOR = getFloat("extIOR", _value);
        alpha = getFloat("alpha", _value);
        m_eta = intIOR / extIOR;
        m_inv_eta = 1 / m_eta;
    }
    
    ~RoughDielectric() = default;

    virtual bool isDiffuse() const override {
        return false;
    }

    /**
     * @brief 
        When  
            1. reflect
                bsdf = Fresnel * G * D / (4 * dot(n, i) * dot(n, o))
            2. refract
                bsdf = ...
     * 
     * @param bRec 
     * @return SpectrumRGB 
     */

    virtual SpectrumRGB evaluate (const BSDFQueryRecord &bRec) const override {
        if (Frame::cosTheta(bRec.wi) == 0) {
            return SpectrumRGB(.0f);
        }

        bool isReflect = Frame::cosTheta(bRec.wi) * Frame::cosTheta(bRec.wo) > 0;
        //! eta = eta_o / eta_i
        float eta = Frame::cosTheta(bRec.wi) > 0 
            ? (intIOR / extIOR) : (extIOR / intIOR);
        
        //! The normal of microfacet
        Vector3f H;
        if (isReflect) {
            H = normalize(bRec.wi + bRec.wo);
        } else {
            H = normalize(bRec.wi + bRec.wo * eta); 
        }
        H *= sign(Frame::cosTheta(H));

        float D = BeckmannDistribution::D(alpha, H);
        
        if (D == 0) {
            return SpectrumRGB(.0f);
        }

        float F = FresnelDielectricAccurate(dot(bRec.wi, H), extIOR, intIOR);

        float G = BeckmannDistribution::G(bRec.wi, bRec.wo, alpha);

        if (isReflect) {
            return SpectrumRGB {
                F * D * G / (4.f * std::abs(Frame::cosTheta(bRec.wi)))
            };
        } else {
            float sqrtDenom = dot(bRec.wi, H) * eta * dot(bRec.wo, H);
            float value = ((1 - F) * D * G * eta * eta)
                * dot(bRec.wi, H) * dot(bRec.wo, H) / 
                (Frame::cosTheta(bRec.wi) * sqrtDenom * sqrtDenom);
            return SpectrumRGB(value);
        }

    }

    virtual float pdf (const BSDFQueryRecord &bRec) const override {
        Vector3f H;
        float dwh_dwo;

        bool isReflect = Frame::cosTheta(bRec.wi) * Frame::cosTheta(bRec.wo) > 0;
        if (isReflect) {
            H = normalize(bRec.wi + bRec.wo);
            float F = FresnelDielectricAccurate(dot(H, bRec.wi), extIOR, intIOR);
            dwh_dwo = 1.f / (4.f * dot(bRec.wo, H)) * F;
        } else {
            float eta = Frame::cosTheta(bRec.wi) > 0 
                ? (intIOR / extIOR) : (extIOR / intIOR);
            
            H = normalize(bRec.wi + bRec.wo * eta);
            //float F = FresnelDielectricAccurate(dot(H, bRec.wi), extIOR, intIOR);
            float F = FresnelDielectric(dot(H, bRec.wi), m_eta);
            float sqrtDenom = dot(bRec.wi, H) + eta * dot(bRec.wo, H);
            dwh_dwo = eta * eta * dot(bRec.wo, H) / (sqrtDenom * sqrtDenom) * (1 - F);
        }
        H *= sign(Frame::cosTheta(H));

        float pdf = BeckmannDistribution::D(alpha, H) * std::abs(Frame::cosTheta(H) * dwh_dwo);
        return pdf;
    }


    virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample, float &pdf) const override {
        SpectrumRGB weight {1.0f};

        // sample normal first
        float microfacet_pdf;
        Vector3f m = BeckmannDistribution::sampleWh(alpha, sample, microfacet_pdf);

        // compute the fresnel term
        float F = FresnelDielectricAccurate(dot(m, bRec.wi), extIOR, intIOR);
        //float F = FresnelDielectric(dot(m, bRec.wi), m_eta);

        float reflect_prob = .5f * (sample[0] + sample[1]);

        float dwh_dwo;
        if (reflect_prob < F) {
            // reflect
            bRec.wo = reflect(bRec.wi, m);
            microfacet_pdf *= F;

            dwh_dwo = 1 / (4 * dot(bRec.wo, m));
        } else {
            // refract
            //std::cout << "Refract \n";
            bRec.wo = refract(bRec.wi, m);
            //std::cout << "wi = " << bRec.wi << std::endl;
            //std::cout << "wo = " << bRec.wo << std::endl;
            microfacet_pdf *= (1 - F);

            float eta = dot(bRec.wi, m) > 0 ? m_eta : m_inv_eta;
            float sqrtDenom = dot(bRec.wi, m) + eta * dot(bRec.wo, m);
            dwh_dwo = (eta*eta * dot(bRec.wo, m)) / (sqrtDenom*sqrtDenom);
        }

        pdf *= std::abs(dwh_dwo);

        weight = weight * std::abs(
            BeckmannDistribution::D(alpha, m) * BeckmannDistribution::G(bRec.wi, bRec.wo, alpha)
            * dot(bRec.wi, m) / (microfacet_pdf * Frame::cosTheta(bRec.wi))
        );

        return weight;
    }
};

REGISTER_CLASS(RoughDielectric, "rough-dielectric")