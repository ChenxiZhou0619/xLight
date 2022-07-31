#include "core/render-core/integrator.h"
#include "core/render-core/medium.h"
#include "core/math/common.h"

class VPathTracing : public Integrator {
public:
    VPathTracing() : m_max_depth(5), m_rr_threshold(3), m_shadowray_nums(1) { }

    VPathTracing(const rapidjson::Value &_value) {
        m_max_depth = getInt("maxDepth", _value);
        m_rr_threshold = getInt("rrThreshold", _value);
        m_shadowray_nums = getInt("shadowRayNums", _value);
    }

    virtual SpectrumRGB getLi (const Scene &scene, 
                               const Ray3f &r,
                               Sampler *sampler) const override
    {
        SpectrumRGB Li{.0f}, throughput{1.f};
        int path_length = 0;
        bool is_specular_bounce = false;
        Ray3f ray{r};

        while(true) {
            //* Intersect 
            RayIntersectionRec i_rec;
            bool found_intersection = scene.rayIntersect(ray, i_rec);
        
            //* Radiative Transfer Equation Sampling
            const Medium *medium = ray.getMedium();
            if (!medium) {
                //* The ray is in medium

                //* Sample a distance
                MediumSampleRecord m_rec;
                medium->sampleDistance(&m_rec);
                if (m_rec.pathLength < i_rec.t) {
                    //* The ray not escape the medium
                    throughput *= m_rec.sigmaS * m_rec.transmittance / m_rec.pdf;

                    //*----------------------------------------
                    //*------  Evaluate inside medium  --------
                    //*----------------------------------------

                    //*----------------------------------------
                    //*------  Sampling Luminaire  ------------
                    //*----------------------------------------

                    SpectrumRGB direct_illumination {.0f};
                    for (int i = 0; i < m_shadowray_nums; ++i) {
                        DirectIlluminationRecord d_rec;
                        scene.sampleDirectIllumination(&d_rec, sampler, i_rec.p);
                    }


                    //*----------------------------------------
                    //*------  Sampling Phase Function --------
                    //*----------------------------------------
                    
                    Vector3f wo;
                    float phase_pdf;
                    wo = medium->sampleDirection(ray.dir, &wo, &phase_pdf);

                } else {
                    //* The ray escape the medium
                    throughput *= m_rec.transmittance / m_rec.pdf;

                }
            }
            
            
        }
    }

private:
    int m_max_depth,
        m_rr_threshold,
        m_shadowray_nums;
};

REGISTER_CLASS(VPathTracing, "vpath")