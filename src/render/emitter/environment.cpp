#include "core/render-core/emitter.h"
#include "core/render-core/texture.h"
#include "core/math/math.h"
#include <type_traits>


class EnvironmentEmitter : public Emitter {
public:
    EnvironmentEmitter() = default;

    EnvironmentEmitter(const rapidjson::Value &_value) {
        // do nothing currently
        m_energy_scale = getFloat("energyScale", _value);
    }

    virtual ~EnvironmentEmitter() = default;

    virtual void initialize() override{
        std::cout << "Start to initialize EnvironmentEmitter\n";

        // Construct a pixel-wise importance sampling distribution
        Vector2i resolution = m_envmap->getResolution();
        
    }

    virtual SpectrumRGB evaluate(const EmitterQueryRecord &bRec) const override{
        assert(m_envmap != nullptr);
        const Ray3f ray = bRec.ray;
        float cosTheta = ray.dir.y,
              tanPhi = ray.dir.z / ray.dir.x;
        float theta = std::acos(cosTheta),
              phi = std::atan(tanPhi);
        if (phi < 0) 
            phi += ray.dir.x > 0 ? 2 * M_PI : M_PI;
        else {
            phi += ray.dir.x > 0 ? .0f : M_PI;
        }
        return m_envmap->evaluate(Point2f(
            phi / (2 * M_PI),
            theta / M_PI
        )) * m_energy_scale;
    }

    virtual SpectrumRGB evaluate(const Ray3f &ray) const override {
        float cosTheta = ray.dir.y,
              tanPhi = ray.dir.z / ray.dir.x;
        float theta = std::acos(cosTheta),
              phi = std::atan(tanPhi);
        if (phi < 0) 
            phi += ray.dir.x > 0 ? 2 * M_PI : M_PI;
        else {
            phi += ray.dir.x > 0 ? .0f : M_PI;
        }
        return m_envmap->evaluate(Point2f(
            phi / (2 * M_PI),
            theta / M_PI
        )) * m_energy_scale;
    }

    //! Consider the envmap on a infinite large sphere
    //! Uniformly sample it
    virtual void sample(PointQueryRecord *pRec, Point2f sample) const override {
        pRec->mesh = nullptr;
        
        float theta = 2 * M_PI * sample[0],
              phi   = std::acos(2 * sample[1] - 1);
        
        pRec->p = Point3f{
            std::sin(phi) * std::cos(theta),
            std::cos(phi),
            std::sin(phi) * std::sin(theta)
        } * m_envshpere_radius;

        pRec->normal = Point3f(.0f) - pRec->p;

        //pRec->pdf = 1 / (m_envshpere_radius * m_envshpere_radius * M_PI);

        pRec->pdf = 0.25f * INV_PI;

        pRec->emitter = this; 
    }

    virtual void setTexture(Texture *texture) override {
        m_envmap = texture;
    }


private:
    Texture *m_envmap = nullptr;

    // TODO, Hack, replace it
    float m_envshpere_radius = 100000.f;

    float m_energy_scale = 1.f;

};

REGISTER_CLASS(EnvironmentEmitter, "envemitter")