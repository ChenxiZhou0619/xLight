#include "core/render-core/integrator.h"
#include "core/math/common.h"

class PathTracing : public Integrator {
public:
    PathTracing() : m_max_depth(5), m_rr_threshold(3), m_shadowray_nums(1) { };

    PathTracing(const rapidjson::Value &_value) {
        // do nothing
        m_max_depth = getInt("maxDepth", _value);
        m_rr_threshold = getInt("rrThreshold", _value);
        m_shadowray_nums = getInt("shadowRayNums", _value);
    }
/*
    virtual SpectrumRGB getLi(const Scene &scene, const Ray3f &r, Sampler *sampler) const {
        SpectrumRGB Li {.0f}, beta {1.f};
        int bounces;
        bool specularBounce = false;
        Ray3f ray(r);
        // TODO too change it
        float emitterPdf = 0;
        
        for (bounces = 0;; ++bounces) {
            RayIntersectionRec iRec;
            // test for intersection
            bool foundIntersection = scene.rayIntersect(ray, iRec);

            if (bounces == 0 || specularBounce) {
                if (foundIntersection) {
                    if (iRec.meshPtr->isEmitter()) {
                        Li += beta * iRec.meshPtr->getEmitter()->evaluate(ray);
                    }
                } else {
                    Li += beta * scene.evaluateEnvironment(ray);
                }
            }

            // terminate if 
            if (!foundIntersection || bounces >= maxDepth ) {
                break;
            }
            if (iRec.meshPtr->isEmitter())
                break;
            const BSDF *bsdf = iRec.meshPtr->getBSDF();
            specularBounce = !bsdf->isDiffuse();
            // sample the direct illumination at this point
            PointQueryRecord pRec;
            scene.sampleEmitterOnSurface(pRec, sampler);
            emitterPdf = pRec.pdf;
            Ray3f shadowRay (
                iRec.p,
                pRec.p
            );
            if (!scene.rayIntersect(shadowRay)) {
                // hit the emitter
                EmitterQueryRecord eRec (pRec, shadowRay);
                const auto &emitter = eRec.getEmitter();

                // evaluate the direct illumination
                BSDFQueryRecord bRec (
                    iRec.toLocal(-ray.dir), iRec.toLocal(shadowRay.dir), iRec.UV
                );
                SpectrumRGB bsdfVal = bsdf->evaluate(bRec);
                //float pdf = (shadowRay.tmax * shadowRay.tmax) / std::abs(dot(pRec.normal, shadowRay.dir)) * pRec.pdf;
                float pdf = pRec.pdf;
                float bsdfPdf = bsdf->pdf(bRec);
                Li += beta * powerHeuristic(pdf, bsdfPdf)
                      * bsdfVal 
                      * emitter->evaluate(eRec) 
                      / pdf;
            }

            // sample the bsdf
            float pdf;
            BSDFQueryRecord bRec(iRec.toLocal(-ray.dir), iRec.UV);
            SpectrumRGB bsdfVal = bsdf->sample(bRec, sampler->next2D(), pdf);
            if (bsdfVal.isZero()) {
                break;
            }
            ray = Ray3f(
                iRec.p,
                iRec.toWorld(bRec.wo)
            );
            iRec = RayIntersectionRec();
            SpectrumRGB value(.0f);
            float lumPdf = .0f;
            if (!foundIntersection) {
                value = scene.evaluateEnvironment(ray);
                if (value.isZero())
                    break;
                lumPdf = emitterPdf;
            } else {
                if (iRec.meshPtr->isEmitter()) {
                    value = iRec.meshPtr->getEmitter()->evaluate(ray);
                    lumPdf = specularBounce ? .0f : (iRec.t * iRec.t) / std::abs(dot(iRec.geoN, ray.dir)) * emitterPdf;
                }
            }
            beta *= bsdfVal;
            // TODO compute the lumPdf when env light matters
            //float lumPdf = specularBounce ? .0f : (iRec.t * iRec.t) / std::abs(dot(iRec.geoN, ray.dir)) * emitterPdf;
            //float lumPdf = .0f;
            if (!value.isZero()) {
                Li += beta * value * powerHeuristic(pdf, lumPdf);
            }

            // continue the next ray
            if (!iRec.isValid) break;
            if (bounces++ > rrThreshold) {
                if (sampler->next1D() > 0.95f) break;
                beta /= 0.95f;
            }
        }
        return Li;
    }
*/
    virtual SpectrumRGB getLi(const Scene &scene, 
                              const Ray3f &r, 
                              Sampler *sampler) const override {
        SpectrumRGB Li {.0f}, throughput {1.0f};
        int path_length = 0;
        bool is_specular_bounce = false;
        Ray3f ray{r};

        while(true) {
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
            
            //*------------------------------------------------------
            //*------------     Direct Illumination     -------------
            //*------------------------------------------------------
            const BSDF *bsdf = i_rec.meshPtr->getBSDF();
            Vector3f wi = i_rec.toLocal(-ray.dir);
            SpectrumRGB direct_illumination {.0f};
            for (int i = 0; i < m_shadowray_nums; ++i) {
                DirectIlluminationRecord d_rec;
                scene.sampleDirectIllumination(&d_rec, sampler, i_rec.p);
                Ray3f shadow_ray = d_rec.shadow_ray;
                if (!scene.rayIntersect(shadow_ray)) {
                    // hit the emitter
                    Vector3f wo = i_rec.toLocal(shadow_ray.dir);
                    BSDFQueryRecord b_rec {wi, wo, i_rec.UV};
                    //* Evaluate the direct illumination
                    SpectrumRGB bsdf_value = bsdf->evaluate(b_rec);
                    float bsdf_pdf = bsdf->pdf(b_rec);
                    direct_illumination += 
                        throughput * bsdf_value * d_rec.energy
                        / d_rec.pdf * powerHeuristic(d_rec.pdf, bsdf_pdf);
                }
            }
            if (m_shadowray_nums != 0)
                Li += (direct_illumination / m_shadowray_nums);

            //*------------------------------------------------------
            //*------------    Sampling BSDF       ------------------
            //*------------------------------------------------------
            BSDFQueryRecord b_rec {wi, i_rec.UV};
            float bsdf_pdf = .0f;
            SpectrumRGB bsdf_weight = bsdf->sample(b_rec, sampler->next2D(), bsdf_pdf);

            if (bsdf_weight.isZero())
                break;
            //! Update the ray
            ray = Ray3f {i_rec.p, i_rec.toWorld(b_rec.wo)};
            i_rec.clear();
            found_intersection = scene.rayIntersect(ray, i_rec);
            
            SpectrumRGB lumion_energy {.0f};
            float lumion_pdf = .0f;

            if (!found_intersection) {
                //* hit the environment, evaluate the Li and terminate
                //* Cause not sample the environment, the pdf should be 0
                lumion_energy = scene.evaluateEnvironment(ray);
                //lumion_pdf = scene.pdfEnvironment(ray);
                lumion_pdf = .0f;
            } else {
                //* hit the scene
                if (i_rec.meshPtr->isEmitter()) {
                    //* hit emitter, evaluate the Li and terminate
                    lumion_energy = i_rec.meshPtr->getEmitter()->evaluate(ray);
                    lumion_pdf = scene.pdfArea(i_rec, ray);
                }
            }
            throughput *= bsdf_weight;
            if (!lumion_energy.isZero())
                Li += throughput * lumion_energy * powerHeuristic(bsdf_pdf, lumion_pdf);
            
            if (!i_rec.isValid)
                break;
            if (path_length++ > m_rr_threshold) {
                if (sampler->next1D() > 0.95f) break;
                throughput /= 0.95f;
            }
        }
        return Li;
    }
private:
    int m_max_depth, m_rr_threshold, m_shadowray_nums;

};

REGISTER_CLASS(PathTracing, "path")