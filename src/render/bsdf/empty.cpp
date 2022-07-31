#include "core/render-core/bsdf.h"

class EmptyBSDF : public BSDF {
public:
    EmptyBSDF() = default;
    EmptyBSDF(const rapidjson::Value &_value) {
        m_type = EBSDFType::EEmpty;
    }
    virtual ~EmptyBSDF() = default;

    virtual bool isDiffuse() const override {
        std::cout << "EmptyBSDF::isDiffuse not implement!\n";
        std::exit(1);
    }

    virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const override {
        std::cout << "EmptyBSDF::evaluate not implement!\n";
        std::exit(1);    
    }

    virtual float pdf(const BSDFQueryRecord &bRec) const override {
        std::cout << "EmptyBSDF::pdf not implement!\n";
        std::exit(1);    
    }

    virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample, float &pdf) const override {
        std::cout << "EmptyBSDF::sample not implement!\n";
        std::exit(1);
    }

};

REGISTER_CLASS(EmptyBSDF, "empty")