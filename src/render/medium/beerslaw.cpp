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
    virtual bool sampleDistance(MediumSampleRecord *m_rec,
                                const Ray3f &ray,
                                Sampler *sampler) const override
    {
        // only scatter, so distance is infinity
        m_rec->pathLength = std::numeric_limits<float>::max();
        m_rec->pdf = 1;
        m_rec->transmittance = getTrans(ray.ori, ray.at(ray.tmax));
        m_rec->medium = this;
        return false;
    }

    virtual SpectrumRGB getTrans(Point3f start,
                                 Point3f end) const override
    {
        auto r = m_absorbtion.r(),
             g = m_absorbtion.g(),
             b = m_absorbtion.b();
        
        auto dist = (end - start).length();

        return SpectrumRGB{
            std::exp(-dist * r),
            std::exp(-dist * g),
            std::exp(-dist * b)
        };

    }


private:
    SpectrumRGB m_absorbtion;
};

REGISTER_CLASS(Beerslaw, "beerslaw")