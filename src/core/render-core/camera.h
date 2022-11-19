#pragma once
#include "core/utils/configurable.h"
#include "core/geometry/geometry.h"
#include "core/math/math.h"
#include "sampler.h"

class Camera : public Configurable {
public:
    /**
     * @brief 
     * Generate ray in local coordinate (lefthand)
     * pos = (0, 0, 0), lookAt = (0, 0, 1), up = (0, 1, 0)
     * 
     * @param offset 
     * @return Ray3f 
     */
    Camera() = delete;

    Camera(const rapidjson::Value &_value):
        Camera(
            getPoint3f("position", _value),
            getPoint3f("lookAt", _value),
            getVector3f("up", _value)
        ){
        aspectRatio = getFloat("aspectRatio", _value);
        vertFov = getFloat("vertFov", _value);
    }

    ~Camera() = default;


    /**
     * @brief Construct a new Camera object
     * Refer the pbrt v3
     * @param _pos 
     * @param lookAt 
     * @param up 
     */

    Camera(const Point3f& _pos, const Point3f lookAt, const Vector3f &up) 
        : pos(_pos), aspectRatio(1.7778f), vertFov(39.f), distToFilm(1.f) {
        // initialize the translation part
        cameraToWorld(0, 3) = _pos.x;
        cameraToWorld(1, 3) = _pos.y;
        cameraToWorld(2, 3) = _pos.z;
        cameraToWorld(3, 3) = 1;

        // initial the rotation part
        Vector3f dir = normalize(lookAt - _pos);
        Vector3f right = normalize(cross(normalize(up), dir));
        Vector3f newUp = cross(dir, right);

        cameraToWorld(0, 0) = right.x;
        cameraToWorld(1, 0) = right.y;
        cameraToWorld(2, 0) = right.z;
        cameraToWorld(3, 0) = .0f;
        cameraToWorld(0, 1) = newUp.x;
        cameraToWorld(1, 1) = newUp.y;
        cameraToWorld(2, 1) = newUp.z;
        cameraToWorld(3, 1) = .0f;
        cameraToWorld(0, 2) = dir.x;
        cameraToWorld(1, 2) = dir.y;
        cameraToWorld(2, 2) = dir.z;
        cameraToWorld(3, 2) = .0f;

//        cameraToWorld = cameraToWorld.inverse();
    }

    Camera(const Mat4f& _cameraToWorld):
        cameraToWorld(_cameraToWorld),aspectRatio(1.7778f), vertFov(30.f), distToFilm(1.f){
        pos = Point3f {_cameraToWorld(0, 3), _cameraToWorld(1, 3),_cameraToWorld(2, 3)};
    }

    virtual Ray3f sampleRay (const Vector2i &offset, 
                             const Vector2i &resolution, 
                             const CameraSample &sample) const = 0;

    virtual Ray3f sampleRayDifferential (const Point2i &offset,
                                         const Point2i &resolution,
                                         const CameraSample &sample) const = 0;

protected:
    Point3f pos;
    
    float aspectRatio, vertFov, distToFilm;

    Mat4f cameraToWorld;

    Mat4f sampleToFilm;
};