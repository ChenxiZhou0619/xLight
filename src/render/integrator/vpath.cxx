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
                               Sampler *sampler) const
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
                            &transimattance,
                            medium_stack);
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
                        medium->transmittance(ray.ori, i_rec.p);
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
                        &transimattance,
                        medium_stack);
                    if (!transimattance.isZero()) {
                        BSDFQueryRecord b_rec {i_rec, ray, d_rec.shadow_ray};
                        SpectrumRGB bsdf_value = bsdf->evaluate(b_rec);
                        float bsdf_pdf = bsdf->pdf(b_rec);
                        if (!bsdf_value.isZero()) {
                            direct_illumination += transimattance * throughput *
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


    virtual SpectrumRGB getLi_ (const Scene &scene,
                               const Ray3f &r,
                               Sampler *sampler) const 
    {
        SpectrumRGB Li {.0f}, throughput {1.f};
        int path_length = 0;
        bool is_specular_bounce = false;
        Ray3f ray {r};
        std::stack<Medium *> medium_stack;

        while(true) {
            RayIntersectionRec i_rec;
            bool found_intersection = scene.rayIntersect(ray, i_rec);

            if (path_length == 0 || is_specular_bounce) {
                if (found_intersection) {
                    if (i_rec.meshPtr->isEmitter()) {
                        Li += throughput
                              * i_rec.meshPtr->getEmitter()->evaluate(ray);
                    }
                } else {
                    Li += throughput
                          * scene.evaluateEnvironment(ray);
                }
            }

            if (!found_intersection || path_length >= m_max_depth)
                break;
            
            if (i_rec.meshPtr->isEmitter())
                break;

            Medium *medium = nullptr;
            if (!medium_stack.empty()) 
                medium = medium_stack.top();

            //* If the ray is immuted in medium
            if (medium) {
                //* Sampling Ls first
                SpectrumRGB Ls;
                medium->sampleLs(scene, &Ls);
                Li += throughput * Ls;

                //* Update throughput, evaluate Ld in next loop
                throughput *= medium->transmittance(ray.ori, i_rec.p);
                
                //* Escapte the medium or hit the surface in the medium
                if (i_rec.meshPtr->getBSDF()->m_type == BSDF::EBSDFType::EEmpty) {
                    //* BSDF will not change the ray direction
                    if (dot(ray.dir, i_rec.geoN) > 0) {
                        //* Escape the medium
                        if (!medium_stack.empty())
                            medium_stack.pop();
                    } 
                    if (dot(ray.dir, i_rec.geoN) < 0) {
                        //* Enter a new medium(possible)
                        if (i_rec.meshPtr->getMedium())
                            medium_stack.push(i_rec.meshPtr->getMedium());
                    }
                    ray = Ray3f {i_rec.p, ray.dir};
                    continue;
                }
            } 
            
            //* Evaluate the surface illumination
            const BSDF *bsdf = i_rec.meshPtr->getBSDF();
            
            //* If the surface is empty
            if (bsdf->m_type == BSDF::EBSDFType::EEmpty) {
                if (i_rec.meshPtr->getMedium()) {
                    medium_stack.push (i_rec.meshPtr->getMedium());
                }
                //* Update ray
                ray = Ray3f {i_rec.p, ray.dir};
                continue;
            }

            //* -----------------------------------------------------
            //* --------------- Direct Illumination -----------------
            //* -----------------------------------------------------

            SpectrumRGB direct_illumination {.0f};
            for (int i = 0; i < m_shadowray_nums; ++i) {
                DirectIlluminationRecord d_rec;
                SpectrumRGB trans {1.0f};
                scene.sampleAttenuatedDirectIllumination(
                    &d_rec, 
                    sampler, 
                    i_rec.p, 
                    &trans,
                    medium_stack
                );
                if (!trans.isZero()) {
                    BSDFQueryRecord b_rec {i_rec, ray, d_rec.shadow_ray};
                    SpectrumRGB bsdf_val = bsdf->evaluate(b_rec);
                    float bsdf_pdf = bsdf->pdf(b_rec);
                    if (!bsdf_val.isZero()) {
                        direct_illumination += throughput * trans 
                            * bsdf_val * d_rec.energy / d_rec.pdf
                            *  powerHeuristic(d_rec.pdf, bsdf_pdf);
                    }
                }
            }
            if (m_shadowray_nums != 0)
                Li += direct_illumination / m_shadowray_nums;
        
            //* -----------------------------------------------------
            //* ---------------- BSDF Sampling ----------------------
            //* -----------------------------------------------------
            //TODO, should consider transimattance

            BSDFQueryRecord b_rec {i_rec, ray};
            float bsdf_pdf = .0f;
            SpectrumRGB bsdf_weight = bsdf->sample(b_rec, sampler->next2D(), bsdf_pdf);
            is_specular_bounce = !bsdf->isDiffuse();
            if (bsdf_weight.isZero())
                break;
            ray = Ray3f {i_rec.p, i_rec.toWorld(b_rec.wo)};

            i_rec.clear();
            found_intersection = scene.rayIntersect(ray, i_rec);

            SpectrumRGB lumion_energy {.0f};
            float lumion_pdf = .0f;

            if (!found_intersection) {
                lumion_energy = scene.evaluateEnvironment(ray);
            } else {
                if (i_rec.meshPtr->isEmitter()) {
                    lumion_energy = i_rec.meshPtr->getEmitter()->evaluate(ray);
                    lumion_pdf = scene.pdfArea(i_rec, ray);
                }
            }

            throughput *= bsdf_weight;

            if (!lumion_energy.isZero()) 
                Li += throughput * lumion_energy
                      * powerHeuristic(bsdf_pdf, lumion_pdf);
            
            if (!i_rec.isValid)
                break;

            if (path_length++ > m_rr_threshold) {
                if (sampler->next1D() > 0.95f) break;
                throughput /= 0.95;
            }
        }
        return Li;

    }

    virtual SpectrumRGB getLi__ (const Scene &scene,
                               const Ray3f &r,
                               Sampler *sampler) const
    {
        SpectrumRGB Li {.0f}, throughout {1.f};
        Ray3f ray{r};
        int bounces = 0;

        std::stack<Medium *> medium_stack;
        
        while(true) {
            RayIntersectionRec iRec;
            bool foundIntersection = scene.rayIntersect(ray, iRec);
            
            MediumSampleRecord mRec;
            if (!medium_stack.empty()) {
                Medium *medium = medium_stack.top();
                medium->sample(sampler, &mRec, ray.ori, iRec.p);
                throughout *= mRec.transmittance / mRec.pdf;
            }

            if(mRec.isValid) {
                // TODO
                //* Sample a direction and continue the ray
            } else {
                if (bounces == 0) {
                    if (foundIntersection) {
                        if (iRec.meshPtr->isEmitter())
                            Li += throughout * iRec.meshPtr->getEmitter()->evaluate(ray);
                    } else {
                        Li += throughout * scene.evaluateEnvironment(ray);
                    }
                }

                if (!foundIntersection || bounces >= m_max_depth)
                    break;

                if (iRec.meshPtr->isEmitter())
                    break;
                
                const BSDF *bsdf = iRec.meshPtr->getBSDF();

                if (bsdf->m_type == BSDF::EBSDFType::EEmpty) {
                    //* Just continue the ray
                    ray = Ray3f{iRec.p, ray.dir};
                    //* enter or out the medium
                    Medium *medium = iRec.meshPtr->getMedium();
                    if (dot(iRec.geoN, ray.dir) > 0) {
                        if (medium)
                            medium_stack.emplace(medium);
                    }
                    else if (dot(iRec.geoN, ray.dir) < 0) {
                        if (!medium_stack.empty())
                            medium_stack.pop();
                    }
                    continue;
                }
                //* ---------------------------------------------
                //* --------  Direct Illumination  --------------
                //* ---------------------------------------------
                SpectrumRGB directIllumination {.0f};
                for (int i = 0; i < m_shadowray_nums; ++i) {
                    DirectIlluminationRecord dRec;
                    SpectrumRGB trans {1.f};
                    scene.sampleAttenuatedDirectIllumination(
                        &dRec, 
                        sampler, iRec.p, 
                        &trans, 
                        medium_stack
                    );

                    if (!trans.isZero()) {
                        BSDFQueryRecord bRec{iRec, ray, dRec.shadow_ray};
                        SpectrumRGB bsdfValue = bsdf->evaluate(bRec);
                        float bsdfPdf = bsdf->pdf(bRec);
                        if (!bsdfValue.isZero())
                            directIllumination += 
                                throughout * trans * bsdfValue
                                * dRec.energy / dRec.pdf
                                * powerHeuristic(dRec.pdf, bsdfPdf);   
                    }
                }

                if (m_shadowray_nums != 0) 
                    Li += directIllumination / m_shadowray_nums;
                
                //* ---------------------------------------------
                //* ---------- Sampling BSDF  -------------------
                //* ---------------------------------------------
                BSDFQueryRecord bRec {iRec, ray};
                float bsdfPdf = .0f;
                SpectrumRGB bsdfWeight = bsdf->sample(bRec, sampler->next2D(), bsdfPdf);
                if (bsdfWeight.isZero())
                    break;
                ray = Ray3f {iRec.p, iRec.toWorld(bRec.wo)};
                iRec.clear();
                foundIntersection = scene.rayIntersect(ray, iRec);

                SpectrumRGB lumionEnergy {.0f};
                float lumionPdf;
                if (!foundIntersection) {
                    lumionEnergy = scene.evaluateEnvironment(ray);
                    lumionPdf = .0f;
                } else {
                    if (iRec.meshPtr->isEmitter()) {
                        lumionEnergy = iRec.meshPtr->getEmitter()->evaluate(ray);
                        lumionPdf = scene.pdfArea(iRec, ray);
                    }
                }

                throughout *= bsdfWeight;
                if (!lumionEnergy.isZero()) {
                    Li += throughout * lumionEnergy * powerHeuristic(bsdfPdf, lumionPdf);
                }
                if (!iRec.isValid)
                    break;
            }
            if (bounces++ > m_rr_threshold) {
                if (sampler->next1D() > 0.95f) break;
                throughout /= 0.95f;
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