#pragma once
#include <core/geometry/geometry.h>

#include <embree3/rtcore.h>


class ShapeInterface {
public:
    ShapeInterface() = default;
    
    virtual ~ShapeInterface() = default;

    virtual void initEmbreeGeometry(RTCDevice device) = 0;

    //* Only for mesh like shape
    virtual Point3f getHitPoint(int triIdx, Point2f uv) const = 0;

    //* Only for mesh like shape
    virtual Normal3f getHitNormal(int triIdx, Point2f uv) const = 0;

    //* Only for mesh like shape
    virtual Point2f getHitTextureCoordinate(int triIdx, Point2f) const = 0;

    bool HasUV() const {return hasUV;}

    bool HasTangent() const {return hasTangent;}

    friend class Scene;
protected:
   
    virtual Point3f getVertex(int idx) const = 0;
    
    virtual Point3ui getFace(int idx) const = 0;
    
    virtual Normal3f getNormal(int idx) const = 0;

    virtual Point2f getUV(int idx) const = 0;

    RTCGeometry embreeGeometry;

    bool hasUV;
    
    bool hasTangent;
};

struct ShapeIntersection {
    float distance;

    Point3f hitPoint;

    Normal3f geometryN, shadingN;

    Frame geometryF, shadingF;

    Point2f uv;

    std::shared_ptr<ShapeInterface> shape;

     Vector3f toWorld(const Vector3f &local) const {
        return shadingF.toWorld(local);
    }

    Vector3f toLocal(const Vector3f &world) const {
        return shadingF.toLocal(world);
    }

    void clear() {
        *this = ShapeIntersection();
    }
};