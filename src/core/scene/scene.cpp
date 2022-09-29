#include "scene.h"
#include "core/render-core/sampler.h"
#include "core/render-core/bsdf.h"
#include "core/render-core/medium.h"
#include <stack>

Scene::Scene() {
    device = rtcNewDevice(nullptr);
    scene = rtcNewScene(device);
}

void Scene::addShape(std::shared_ptr<ShapeInterface> shape) {
    shape->initEmbreeGeometry(this->device);
    rtcAttachGeometryByID(scene, shape->embreeGeometry, shapeCount++);
    rtcReleaseGeometry(shape->embreeGeometry);
    shapes.emplace_back(shape);
}

void Scene::postProcess() {
    rtcCommitScene(scene);
    for (auto shape : shapes) {
        if (shape->isEmitter()) {
            areaEmitters.emplace_back(shape);
            emittersDistribution.append(shape->getSurfaceArea());
        }
    }
    emittersSurfaceArea = emittersDistribution.normalize();
}

std::optional<ShapeIntersection> Scene::intersect(const Ray3f &ray) const{
    RTCIntersectContext ictx;
    rtcInitIntersectContext(&ictx);

    RTCRayHit rayhit;
    rayhit.ray = ray.toRTC();
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(scene, &ictx, &rayhit);

    int geomID = rayhit.hit.geomID;
    if (geomID == RTC_INVALID_GEOMETRY_ID) {
        //* No hit
        return std::nullopt;
    }
    std::shared_ptr<ShapeInterface> shape = shapes[geomID];
    ShapeIntersection its;
    //* Fill the intersection
    its.shape = shape;
    int triangleIndex = rayhit.hit.primID;
    its.primID = triangleIndex;
    its.distance = rayhit.ray.tfar;
    
    Point2f uv {
        rayhit.hit.u, rayhit.hit.v
    };        
    its.hitPoint = ray.at(its.distance);
    its.shadingN = shape->getHitNormal(triangleIndex, uv);
    its.geometryN = Normal3f(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z);
//    its.hitPoint = shape->getHitPoint(triangleIndex, uv); 
//    its.geometryN = shape->getHitNormal(triangleIndex);


//    std::cout << "=========================\n";
//    std::cout << its.shadingN << std::endl;
//    std::cout << its.geometryN << std::endl;

    its.geometryF = Frame{its.geometryN};
    its.shadingF = Frame{its.shadingN};
    its.uv = shape->getHitTextureCoordinate(triangleIndex, uv);
    return std::make_optional(its);
}

bool Scene::occlude(const Ray3f &ray) const {
    // TODO, replace this
    auto its = intersect(ray);
    return its.has_value();
}

SpectrumRGB Scene::evaluateEnvironment(const Ray3f &ray) const {
    if (environment == nullptr) {
        return SpectrumRGB{.0f};
    } else {
        return environment->evaluate(ray);
    }
}

void Scene::sampleAreaIllumination(DirectIlluminationRecord *dRec,
                                   Point3f from,
                                   Sampler *sampler) const 
{
    // TODO, just sample area light now
    if (!areaEmitters.empty()) {
        PointQueryRecord pRec;
        int emitterIdx = emittersDistribution.sample(sampler->next1D());
        areaEmitters[emitterIdx]->sampleOnSurface(&pRec, sampler);
        pRec.shape = areaEmitters[emitterIdx];
        pRec.pdf = 1 / emittersSurfaceArea;
        // TODO, replace the raw pointer
        pRec.emitter = pRec.shape->getEmitter().get();

        Ray3f shadowRay {from, pRec.p};
        EmitterQueryRecord eRec{pRec, shadowRay};
        
        dRec->energy = pRec.shape->getEmitter()->evaluate(shadowRay);
        dRec->point_on_emitter = pRec.p;
        dRec->shadow_ray = shadowRay;
        dRec->emitter_type = DirectIlluminationRecord::EmitterType::EArea;
        dRec->pdf = 1 / emittersSurfaceArea;
        dRec->pdf *= shadowRay.tmax * shadowRay.tmax 
            / std::abs(dot(pRec.normal, shadowRay.dir));
    } else {
        std::cerr << "No area light!\n";
        std::exit(1);
    }
}

void Scene::sampleAttenuatedAreaIllumination(DirectIlluminationRecord *dRec, 
                                             SpectrumRGB *trans, 
                                             Point3f from,
                                             std::shared_ptr<Medium> medium, 
                                             Sampler *sampler) const 
{
    //TODO, just sample area light now
    if (!areaEmitters.empty()) {
        PointQueryRecord pRec;
        int emitterIdx = emittersDistribution.sample(sampler->next1D());
        areaEmitters[emitterIdx]->sampleOnSurface(&pRec, sampler);
        pRec.shape = areaEmitters[emitterIdx];
        pRec.pdf = 1 / emittersSurfaceArea;
        pRec.emitter = pRec.shape->getEmitter().get();

        Ray3f shadowRay {from, pRec.p};
        EmitterQueryRecord eRec{pRec, shadowRay};

        dRec->energy = pRec.emitter->evaluate(shadowRay);
        dRec->point_on_emitter = pRec.p;
        dRec->shadow_ray = shadowRay;
        dRec->emitter_type = DirectIlluminationRecord::EmitterType::EArea;
        dRec->pdf = 1 / emittersSurfaceArea;
        dRec->pdf *= shadowRay.tmax * shadowRay.tmax
            / std::abs(dot(pRec.normal, shadowRay.dir));

        //* Caculate the transmittance, if occulude, trans = SpectrumRGB{.0f}
        bool isOcculude = false;
        auto currentMedium = medium;
        while(true) {
            auto its = this->intersect(shadowRay);
            if (!its.has_value()) {
                //* No occulude
                if (currentMedium) {
                    *trans *= currentMedium->getTrans(shadowRay.ori, shadowRay.at(shadowRay.tmax));
                }
                return;
            } else if (its.has_value() &&
                       (its->shape->getBSDF() == nullptr || 
                        its->shape->getBSDF()->m_type != BSDF::EBSDFType::EEmpty)) 
            {
                //todo, maybe consider the dielectric
                //* Occulude
                *trans = SpectrumRGB{.0f};
                return;    
            } else if (its->shape->getBSDF()->m_type == BSDF::EBSDFType::EEmpty) {
                if (currentMedium)
                    *trans *= currentMedium->getTrans(shadowRay.ori, its->hitPoint);
                currentMedium = getTargetMedium(shadowRay.dir, its.value());
                shadowRay = Ray3f{its->hitPoint , pRec.p};
            }
        }
        
    } else {
        std::cerr << "No area light!\n";
        std::exit(1);
    }

}

SpectrumRGB Scene::evaluateTrans(std::shared_ptr<Medium> medium, 
                                 Point3f from, 
                                 Point3f end) const 
{
    Ray3f shadowRay {from, end};
    SpectrumRGB result {1.f};
    auto currentMedium = medium;

    while(true) {
        auto its = this->intersect(shadowRay);

        if (!its.has_value()) {
            //* No occulude
            if (currentMedium) {
                result *= currentMedium->getTrans(shadowRay.ori, shadowRay.at(shadowRay.tmax));
            }
            return result;
        } else if (its.has_value() &&
                   (its->shape->getBSDF() == nullptr || 
                    its->shape->getBSDF()->m_type != BSDF::EBSDFType::EEmpty)) 
        {
            //todo, maybe consider the dielectric
            //* Occulude
            return SpectrumRGB{.0f};    
        } else if (its->shape->getBSDF()->m_type == BSDF::EBSDFType::EEmpty) {
            if (currentMedium)
                result *= currentMedium->getTrans(shadowRay.ori, its->hitPoint);
            currentMedium = getTargetMedium(shadowRay.dir, its.value());
            shadowRay = Ray3f{its->hitPoint , end};
        }
    }
    return result;
}

float Scene::pdfAreaIllumination(const ShapeIntersection& its, 
                                 const Ray3f &ray) const 
{
    return (1 / emittersSurfaceArea) * 
        its.distance * its.distance
        / std::abs(dot(its.shadingN, ray.dir));
}

std::optional<ShapeIntersection> Scene::intersect(const Ray3f &ray,
                                                  std::shared_ptr<Medium> medium,
                                                  SpectrumRGB *trans) const 
{
    Point3f from = ray.ori;
    Ray3f _ray = ray;
    while (true) {
        auto its = intersect(_ray);
        if (its.has_value()) {
            auto bsdf = its->shape->getBSDF();
            if (!bsdf ||
                bsdf->m_type != BSDF::EBSDFType::EEmpty)
            {
                *trans = evaluateTrans(medium, from, its->hitPoint);
                return its;
            } else {
                //* Continue the ray
                _ray = Ray3f {its->hitPoint, _ray.dir};
            }
        } else {
            //* No intersection
            // *trans = SpectrumRGB{.0f};
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::shared_ptr<Medium> Scene::getTargetMedium(Vector3f wo, 
                                               const ShapeIntersection &its) const 
{
    if (dot(its.geometryN, wo) > 0) {
        return its.shape->getOutsideMedium();
    }
    if (dot(its.geometryN, wo) < 0) {
        return its.shape->getInsideMedium();
    }
    return nullptr;
}