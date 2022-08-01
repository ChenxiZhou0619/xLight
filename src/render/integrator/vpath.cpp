#include "core/render-core/integrator.h"
#include "core/render-core/medium.h"
#include "core/math/common.h"
#include <stack>

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
        std::stack<Medium *> medium_stack;

        while(true) {
            //* Intersect 
            RayIntersectionRec i_rec;
            bool found_intersection = scene.rayIntersect(ray, i_rec);

            if (path_length == 0 || is_specular_bounce) {
                if (found_intersection) {
                    if (i_rec.meshPtr->isEmitter()) {
                        Li += throughput * i_rec.meshPtr->getEmitter()->evaluate(ray);
                    }
                } else {
                    Li += throughput * scene.evaluateEnvironment(ray);
                }
            }
            if (!found_intersection || path_length >= m_max_depth)
                break;
            
            if (i_rec.meshPtr->isEmitter())
                break;

            //* Radiative Transfer Equation Sampling
            if (!medium_stack.empty()) {

                //* The ray is in medium
                const Medium *medium = medium_stack.top();

                //* Sample a distance
                MediumSampleRecord m_rec;
                medium->sampleDistance(&m_rec);
                if (m_rec.pathLength < i_rec.t) {
                    //* The ray not escape the medium
                    throughput *= m_rec.sigmaS * m_rec.transmittance / m_rec.pdf;

                    //*----------------------------------------
                    //*------  Evaluate inside medium  --------
                    //*----------------------------------------
                    //! No MIS

                    //*----------------------------------------
                    //*------  Sampling Luminaire  ------------
                    //*----------------------------------------

                    SpectrumRGB direct_illumination {.0f};
                    for (int i = 0; i < m_shadowray_nums; ++i) {
                        DirectIlluminationRecord d_rec;
                        SpectrumRGB transimattance{1.f};
                        scene.sampleAttenuatedDirectIllumination(
                            &d_rec, 
                            sampler, i_rec.p, 
                            &transimattance);
                        //* If transimattance is not zero
                        if (!transimattance.isZero()) {
                            SpectrumRGB phase = medium->evaluatePhase(ray.dir, d_rec.shadow_ray.dir);
                            float phase_pdf = medium->pdfPhase(ray.dir, d_rec.shadow_ray.dir);
                            direct_illumination += 
                                throughput * d_rec.energy * phase * transimattance 
                                / d_rec.pdf;// * powerHeuristic(d_rec.pdf, phase_pdf);                              
                        }
                    }
                    if (m_shadowray_nums != 0)
                        Li += direct_illumination / m_shadowray_nums;

                    //*----------------------------------------
                    //*------  Sampling Phase Function --------
                    //*----------------------------------------
                    
                    Vector3f wo;
                    float phase_pdf;
                    // TODO, the transformation of local wo and world wo
                    wo = medium->sampleDirection(ray.dir, &wo, &phase_pdf);
                    SpectrumRGB phase = medium->evaluatePhase(ray.dir, wo);

                    Point3f scatter_in_medium = ray.at(m_rec.pathLength);
                    ray = Ray3f {scatter_in_medium, wo};                    
                } else {
                    //* The ray escape the medium
                    SpectrumRGB transmattance = 
                        medium->transmittance(scene, ray.ori, i_rec.p);
                    throughput *= transmattance / m_rec.pdf;
                    //* Pop the stack
                    medium_stack.pop();
                    ray = Ray3f{i_rec.p, ray.dir};
                    continue;
                }
            } else {
                //* Current no medium
                const BSDF *bsdf = i_rec.meshPtr->getBSDF();
                if (bsdf->m_type == BSDF::EBSDFType::EEmpty) {
                    // enter the mesh
                    if (i_rec.meshPtr->getMedium())
                        medium_stack.push(i_rec.meshPtr->getMedium());
                    
                    ray = Ray3f{i_rec.p, ray.dir};
                    continue;
                }
                //* ---------------------------------------------
                //* --------  Direct Illumination  --------------
                //* ---------------------------------------------
                SpectrumRGB direct_illumination {.0f};
                for (int i = 0; i < m_shadowray_nums; ++i) {
                    DirectIlluminationRecord d_rec;
                    SpectrumRGB transimattance{1.f};
                    scene.sampleAttenuatedDirectIllumination(
                        &d_rec, 
                        sampler, i_rec.p, 
                        &transimattance);
                    if (!transimattance.isZero()) {
                        BSDFQueryRecord b_rec {i_rec, ray, d_rec.shadow_ray};
                        SpectrumRGB bsdf_value = bsdf->evaluate(b_rec);
                        float bsdf_pdf = bsdf->pdf(b_rec);
                        if (!bsdf_value.isZero()) {
                            direct_illumination += throughput *
                                bsdf_value * d_rec.energy / d_rec.pdf 
                                * powerHeuristic(d_rec.pdf, bsdf_pdf);
                        }
                    }
                }
                if (m_shadowray_nums != 0)
                    Li += direct_illumination / m_shadowray_nums;

                //* ---------------------------------------------
                //* ---------- Sampling BSDF  -------------------
                //* ---------------------------------------------
                BSDFQueryRecord b_rec {i_rec, ray};
                float bsdf_pdf = .0f;
                SpectrumRGB bsdf_weight = bsdf->sample(b_rec, sampler->next2D(), bsdf_pdf);
                is_specular_bounce = !bsdf->isDiffuse();
                if (bsdf_weight.isZero())
                    break;
                ray = Ray3f {i_rec.p, i_rec.toWorld(b_rec.wo)};

                i_rec.clear();
                found_intersection = scene.rayIntersect(ray, i_rec);

                SpectrumRGB lumion_erengy{.0f};
                float lumion_pdf;

                if (!found_intersection) {
                    lumion_erengy = scene.evaluateEnvironment(ray);
                    lumion_pdf = .0f;
                } else {
                    if (i_rec.meshPtr->isEmitter()) {
                        lumion_erengy = i_rec.meshPtr->getEmitter()->evaluate(ray);
                        lumion_pdf = scene.pdfArea(i_rec, ray);
                    }
                }
                throughput *= bsdf_weight;
                if (!lumion_erengy.isZero())
                    Li += throughput * lumion_erengy * powerHeuristic(bsdf_pdf, lumion_pdf);
                
                if (!i_rec.isValid)
                    break;
                
                if (path_length++ > m_rr_threshold) {
                    if (sampler->next1D() > 0.95f) break;
                    throughput /= 0.95;
                }
            }
        }
        return Li;
    }

private:
    int m_max_depth,
        m_rr_threshold,
        m_shadowray_nums;
};

REGISTER_CLASS(VPathTracing, "vpath")