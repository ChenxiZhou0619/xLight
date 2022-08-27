#include "scene.h"

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
    its.distance = rayhit.ray.tfar;
    
    Point2f uv {
        rayhit.hit.u, rayhit.hit.v
    };        
    its.hitPoint = shape->getHitPoint(triangleIndex, uv); 
    its.shadingN = its.geometryN = shape->getHitNormal(triangleIndex, uv);
    its.uv = shape->getHitTextureCoordinate(triangleIndex, uv);
    return std::make_optional(its);
}