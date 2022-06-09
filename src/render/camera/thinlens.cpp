#include "core/render-core/camera.h"
#include "core/math/warp.h"

class ThinLens : public Camera {
    float mApertureRadius, mFocalLen, mFocalDistance;
public:
    ThinLens() = delete;

    ThinLens(const rapidjson::Value &_value) : Camera(_value) {
        mApertureRadius = 
            getFloat("apertureRadius", _value);
        mFocalLen =    
            getFloat("focalLen", _value);
        mFocalDistance = 
            getFloat("focalDistance", _value);
        
        distToFilm = (mFocalLen * mFocalDistance) / (mFocalDistance - mFocalLen);

        sampleToFilm = Mat4f::Perspective(
            vertFov, 
            aspectRatio, 
            distToFilm, 
            std::numeric_limits<float>::max()
        );
        sampleToFilm = 
            Mat4f::Scale(Vector3f {.5f, -.5f, 1.f})
            * Mat4f::Translate(Vector3f {1.f, -1.f, .0f})
            * sampleToFilm;

        sampleToFilm = sampleToFilm.inverse();
    }

    virtual Ray3f sampleRay (const Vector2i &offset, const Vector2i &resolution, const Point2f &sample) const {
        Point3f pointOnFilm = sampleToFilm * Point3f {
            ((float)offset.x + sample.x) / (float)resolution.x,
            ((float)offset.y + sample.y) / (float)resolution.y,
            .0f
        };
        Point3f pointOnFocalPlane
            = pointOnFilm * (mFocalDistance / pointOnFilm.z);
        Point2f tmp
            = Warp::squareToUniformDisk(sample);
        Point3f pointOnAperture
            = Point3f {tmp.x, tmp.y, .0f} * mApertureRadius;
        Vector3f rayDirLocal = normalize(pointOnFocalPlane - pointOnAperture);
        Vector3f rayDirWorld = cameraToWorld.rotate(rayDirLocal);
        return Ray3f {
            cameraToWorld * pointOnAperture,
            rayDirWorld
        };
    }

    



};

REGISTER_CLASS(ThinLens, "thinlens")