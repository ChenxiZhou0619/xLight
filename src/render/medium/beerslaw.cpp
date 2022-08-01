#include "core/render-core/medium.h"

//* The medium only consider absorbtion, which means no scattering or emission
class Beerslaw : public Medium {
public:
    Beerslaw() = default;
    Beerslaw(const rapidjson::Value &_value) {
        m_absorbtion = getSpectrumRGB("sigma_a", _value);
    }
    virtual ~Beerslaw() = default;

    //* Sample the path length before next scatter
    virtual void sampleDistance(MediumSampleRecord *m_rec) const override{
        // only scatter, so distance is infinity
        m_rec->pathLength = std::numeric_limits<float>::max();
        m_rec->pdf = 1;
    }

    //* Sample the phase function
    virtual Vector3f sampleDirection(Vector3f wi, Vector3f *wo, float *pdf) const override{
        std::cout << "Beerslaw::sampleDirection not implement!\n";
        std::exit(1);
    }

    //* Evaluate the phase function
    virtual SpectrumRGB evaluatePhase(Vector3f wi, Vector3f wo) const override {
        std::cout << "Beerslaw::evaluateDirection not implement!\n";
        std::exit(1);
    }

    //* Return the pdf
    virtual float pdfPhase(Vector3f wi, Vector3f wo) const override{
        std::cout << "Beerslaw::pdfPhase not implement!\n";
        std::exit(1);
    }

    //* Given two point, return the transmittance between them
    virtual SpectrumRGB transmittance(const Scene& scene, Point3f p0, Point3f p1) const override {
        float length = (p0 - p1).length();
        SpectrumRGB res = SpectrumRGB{
            std::exp(m_absorbtion[0] * -length),
            std::exp(m_absorbtion[1] * -length),
            std::exp(m_absorbtion[2] * -length)
        };
        return res;
    }

private:
    SpectrumRGB m_absorbtion;
};

REGISTER_CLASS(Beerslaw, "beerslaw")