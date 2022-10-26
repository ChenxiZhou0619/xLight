#include <core/render-core/info.h>
#include <core/scene/scene.h>

Ray3f SurfaceIntersectionInfo::generateShadowRay(const Scene &scene, const LightSourceInfo &info) const {
    Ray3f shadowRay;

    if (info.lightType == LightSourceInfo::LightType::Environment) {
        shadowRay = Ray3f{position, info.direction};
    } else {
        shadowRay = Ray3f{position, info.position};
    }
    if (dot(shadowRay.dir, geometryNormal) > 0) shadowRay.medium = scene.getEnvMedium();
    else shadowRay.medium = shape->getInsideMedium();
    return shadowRay;
}

Ray3f SurfaceIntersectionInfo::generateRay(const Scene &scene, Vector3f dir) const {
    Ray3f ray;
    ray.ori = position;
    ray.dir = dir;
    if (dot(ray.dir, geometryNormal) > 0) ray.medium = scene.getEnvMedium();
    else ray.medium = shape->getInsideMedium();
    return ray;
}

Vector3f SurfaceIntersectionInfo::toLocal(Vector3f v) const 
{
    return shadingFrame.toLocal(v);
}

Vector3f SurfaceIntersectionInfo::toWorld(Vector3f v) const 
{
    return shadingFrame.toWorld(v);
}

IntersectionInfo::IntersectionInfo(const SurfaceIntersectionInfo &sIts){
    data = Intersection{sIts};
}

IntersectionInfo::IntersectionInfo(const MediumIntersectionInfo &mIts){
    data = Intersection{mIts};
}

const SurfaceIntersectionInfo* IntersectionInfo::asSurfaceIntersection() const
{
    return std::get_if<SurfaceIntersectionInfo>(&data);
}

const MediumIntersectionInfo* IntersectionInfo::asMediumIntersection() const
{
    return std::get_if<MediumIntersectionInfo>(&data);
}
