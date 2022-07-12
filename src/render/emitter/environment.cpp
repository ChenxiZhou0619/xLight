#include "core/render-core/emitter.h"
#include "core/render-core/texture.h"
#include "core/math/math.h"
#include <type_traits>
#include "stb/stb_image_write.h"
#include "core/render-core/sampler.h"

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
        int width = resolution.x,
            height = resolution.y;
        m_env_distribution = std::make_unique<Distribution2D>(height, width);
        for (int v = 0; v < height; ++v) {
            float vp = (float) v / height;
            float sin_theta = std::sin(M_PI * float(v + .5f) / float(height));
            for (int u = 0; u < width; ++u) {
                float up = (float) u / width;
                SpectrumRGB energy = m_envmap->evaluate(Point2f(up, vp));
                float value = (energy[0] + energy[1] + energy[2]) * sin_theta;
                m_env_distribution->appendAtX(v, value);
            }
        }
        m_env_distribution->normalize();

        std::cout << "Finish envmap distribution construction\n";
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
/*        
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
*/      
        float pdf = .0f;
        Vector2i resolution = m_envmap->getResolution();
        int width = resolution.x,
            height = resolution.y;
        Vector2i vu = m_env_distribution->sample(sample, &pdf);
        float u = (float)vu[1] / width,
              v = (float)vu[0] / height;
        
        float theta = v * M_PI,
              phi   = u * 2 * M_PI;

        pRec->p = Point3f{
            std::sin(phi) * std::cos(theta),
            std::cos(phi),
            std::sin(phi) * std::sin(theta)
        } * m_envshpere_radius;

        pRec->normal = Point3f(.0f) - pRec->p;

        pRec->emitter = this;

        pRec->pdf = pdf / (2 * M_PI * M_PI * std::sin(theta));
    }

    virtual void setTexture(Texture *texture) override {
        m_envmap = texture;
    }

    virtual void sample(DirectIlluminationRecord *d_rec, Point2f sample) const override {
        float pdf = .0f;
        Vector2i resolution = m_envmap->getResolution();
        int width = resolution.x,
            height = resolution.y;
        Vector2i vu = m_env_distribution->sample(sample, &pdf);
        float u = (float)vu[1] / width,
              v = (float)vu[0] / height;
        
        float theta = v * M_PI,
              phi   = u * 2 * M_PI;

        d_rec->point_on_emitter = Point3f {
            std::sin(theta) * std::cos(phi),
            std::cos(theta),
            std::sin(theta) * std::sin(phi)
        };
        d_rec->energy = m_envmap->evaluate(Point2f {u, v});
        d_rec->emitter_type = DirectIlluminationRecord::EmitterType::EEnvironment;
        d_rec->pdf = pdf / (2 * M_PI * M_PI * std::sin(theta));
    }

    virtual float pdf(const Ray3f &ray) const override {
        assert(m_envmap != nullptr);
        float cosTheta = ray.dir.y,
              tanPhi = ray.dir.z / ray.dir.x;
        float theta = std::acos(cosTheta),
              phi = std::atan(tanPhi);
        if (phi < 0) 
            phi += ray.dir.x > 0 ? 2 * M_PI : M_PI;
        else {
            phi += ray.dir.x > 0 ? .0f : M_PI;
        }
        float u = phi / (2 * M_PI),
              v = theta / M_PI;
        int x = u * m_envmap->getResolution().x,
            y = v * m_envmap->getResolution().y;
        return m_env_distribution->pdf(Vector2i{y, x}) / (2 * M_PI * M_PI * std::sin(theta));
        
    }


private:
    Texture *m_envmap = nullptr;

    std::unique_ptr<Distribution2D> m_env_distribution;

    // TODO, Hack, replace it
    float m_envshpere_radius = 100000.f;

    float m_energy_scale = 1.f;

};

REGISTER_CLASS(EnvironmentEmitter, "envemitter")