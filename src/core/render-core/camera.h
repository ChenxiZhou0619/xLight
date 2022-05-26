#pragma once

#include "core/geometry/geometry.h"
#include "core/math/math.h"

class Camera {
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

    /**
     * @brief Construct a new Camera object
     * Refer the pbrt v3
     * @param _pos 
     * @param lookAt 
     * @param up 
     */

    Camera(const Point3f& _pos, const Point3f lookAt, const Vector3f &up) 
        : pos(_pos), aspectRatio(1.7778f), vertFov(30.f), distToFilm(1.f) {
        // initialize the translation part
        cameraToWorld(0, 3) = _pos.x;
        cameraToWorld(1, 3) = _pos.y;
        cameraToWorld(2, 3) = _pos.z;
        cameraToWorld(3, 3) = 1;

        // initial the rotation part
        Vector3f dir = normalize(lookAt - _pos);
        Vector3f left = normalize(cross(normalize(up), dir));
        Vector3f newUp = cross(dir, left);

        cameraToWorld(0, 0) = left.x;
        cameraToWorld(1, 0) = left.y;
        cameraToWorld(2, 0) = left.z;
        cameraToWorld(3, 0) = .0f;
        cameraToWorld(0, 1) = newUp.x;
        cameraToWorld(1, 1) = newUp.y;
        cameraToWorld(2, 1) = newUp.z;
        cameraToWorld(3, 1) = .0f;
        cameraToWorld(0, 2) = dir.x;
        cameraToWorld(1, 2) = dir.y;
        cameraToWorld(2, 2) = dir.z;
        cameraToWorld(3, 2) = .0f;
    }

    Camera(const Mat4f& _cameraToWorld):
        cameraToWorld(_cameraToWorld),aspectRatio(1.7778f), vertFov(30.f), distToFilm(1.f){
        pos = Point3f {_cameraToWorld(0, 3), _cameraToWorld(1, 3),_cameraToWorld(2, 3)};
    }

    virtual Ray3f sampleRay (const Vector2f &offset) const = 0;

protected:
    Point3f pos;
    
    float aspectRatio, vertFov, distToFilm;

    Mat4f cameraToWorld;
};

class PerspectiveCamera : public Camera {
public:
    PerspectiveCamera() = delete;

    PerspectiveCamera(const Point3f& pos, const Point3f lookAt, const Vector3f &up) : Camera(pos, lookAt, up) { }

    PerspectiveCamera(const Mat4f &_cameraToWorld) : Camera(_cameraToWorld) { }

    friend std::ostream& operator<<(std::ostream &os, const PerspectiveCamera &camera);

    /**
     * @brief e.g. f = (.5f, .5f) the center 
     * 
     * @param offset 
     * @return Ray3f 
     */
    virtual Ray3f sampleRay (const Vector2f &offset) const {
        // generate in the camera coordinate first
        float halfH = std::tan(vertFov * pi / 360.f) * distToFilm,
              halfW = halfH * aspectRatio;
    
        Point3f filmTopLeft = 
            Point3f(0, 0, 0) 
            + Vector3f(0, 0, 1) * distToFilm 
            + Vector3f(-halfW, halfH, 0);

        Point3f pointOnFilm = 
            filmTopLeft
            + Vector3f(2 * halfW * offset.x, - 2 * halfH * offset.y, 0);

        Vector3f rayDirLocal = normalize(pointOnFilm - Point3f(0, 0, 0));

        // turn to the world coordinate
        Vector3f rayDirWorld = cameraToWorld.rotate(rayDirLocal);
        
        return Ray3f {
            pos,
            rayDirWorld
        };
    }
};

inline std::ostream& operator<<(std::ostream &os, const PerspectiveCamera &camera) {
    os << "Perspective Camera:\n"
       << "cameraToWorld matrix:\n" << camera.cameraToWorld << std::endl
       << "position: " << camera.pos << std::endl;
    return os;
}