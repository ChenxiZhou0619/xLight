#include "scene.h"
#include "core/render-core/sampler.h"
#include "core/render-core/bsdf.h"
#include "core/render-core/medium.h"
#include <stack>
#include <spdlog/spdlog.h>


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

void Scene::addEmitter(std::shared_ptr<Emitter> emitter) {
    emitters.emplace_back(emitter);
}

void Scene::postProcess() {
    rtcCommitScene(scene);  
    //* Construct the light distribution using some measure
    //* Here distribution is refer to a single emitter
    //! Just uniform distribution now
    for (auto emitter : emitters) {
        lightDistrib.append(emitter, 1);
    }
    lightDistrib.postProcess();

    if (environment)
        environment->initialize();
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
    its.geometryN = Normal3f(
        rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z
    );
    its.geometryF = Frame{its.geometryN};
    its.shadingN = its.geometryN;    
    its.shadingF = Frame{its.shadingN};

    //* override the shading frame if tangent exists
    if (shape->HasTangent()) {
        its.shadingN = shape->getHitNormal(triangleIndex, uv);

        //* Compute dpdu first
        //Vector3f dpdu = shape->dpdu(triangleIndex);
        //todo fixme
        auto [dpdu, dpdv] = shape->positionDifferential(triangleIndex);
        its.dpdu = dpdu;
        its.dpdv = dpdv;
        Vector3f tangent = shape->getHitTangent(triangleIndex, uv);
        Vector3f bitangent = normalize(cross(its.shadingN, tangent));
        tangent = normalize(cross(bitangent, its.shadingN));
        its.shadingF = Frame{its.shadingN, tangent, bitangent};

        its.geometryN = its.shadingN;
        its.geometryF = its.shadingF;
    }

    its.uv = shape->getHitTextureCoordinate(triangleIndex, uv);
    return std::make_optional(its);
}

bool Scene::occlude(const Ray3f &ray) const {
    // TODO, replace this
    auto its = intersect(ray);
    return its.has_value();
}

float Scene::pdfEmitter(std::shared_ptr<Emitter> emitter) const
{
    return lightDistrib.pdf(emitter);
}


std::shared_ptr<SurfaceIntersectionInfo> 
Scene::intersectWithSurface(const Ray3f &ray) const
{
    auto info = std::make_shared<SurfaceIntersectionInfo>();
    //todo replace the old interface
    auto itsOpt = intersect(ray);
    
    if (!itsOpt) {
        info->shape = nullptr;
        info->light = getEnvEmitter();
        info->distance = 100000.f;
        info->position = ray.at(info->distance);
        info->wi = ray.dir;

        return info;
    }

    info->shape = itsOpt->shape.get();
    info->light = info->shape->getEmitter();
    info->position = itsOpt->hitPoint;
    info->distance = itsOpt->distance;
    info->wi = -ray.dir;
    info->geometryNormal = itsOpt->geometryN;
    info->shadingFrame = itsOpt->shadingF;
    info->uv = itsOpt->uv;
    info->dpdu = itsOpt->dpdu;
    info->dpdv = itsOpt->dpdv;
    info->computeDifferential(ray);
    return info;
}

LightSourceInfo Scene::sampleLightSource(const SurfaceIntersectionInfo &itsInfo, 
                                         Sampler *sampler) const 
{
    auto [light, pdfLight] = lightDistrib.sample(sampler->next1D()); 
    LightSourceInfo info = light->sampleLightSource(itsInfo, sampler->next3D());
    info.pdf *= pdfLight;
    return info;    
}    

LightSourceInfo Scene::sampleLightSource(const IntersectionInfo &itsInfo, 
                                         Sampler *sampler) const 
{
    auto [light, pdfLight] = lightDistrib.sample(sampler->next1D()); 
    LightSourceInfo info = light->sampleLightSource(itsInfo, sampler->next3D());
    info.pdf *= pdfLight;
    return info;    
}