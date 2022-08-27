#pragma once
#include <memory>
#include <vector>
#include <assimp/scene.h>

#include "core/geometry/geometry.h"
#include "core/math/discretepdf.h"

class BSDF;
class Emitter;
struct PointQueryRecord;
class Sampler;

class Mesh {
public:
    Mesh() = default;

    virtual ~Mesh() = default;

    Mesh(const Mesh&) = delete;

    Mesh& operator=(const Mesh&) = delete;

    virtual Point3ui getFBuf(int triIdx) const = 0;

    virtual Point3f getVtxBuf(int vtxIdx) const = 0;

    virtual Normal3f getNmlBuf(int vtxIdx) const = 0;

    virtual AABB3f getAABB3() const;

    virtual std::string toString() const = 0;

    virtual bool hasNormal() const = 0;

    virtual int getFaceNum() const = 0;

    virtual Point2f getUV(int vtxIdx) const = 0;

    virtual Vector3f getTangent(int vtxIdx) const = 0;

    virtual bool hasTangent() const = 0;

    virtual AABB3f getTriBounds(uint32_t triIdx) const = 0;

    virtual void setBSDF(BSDF *_bsdf) ;

    virtual BSDF* getBSDF() const ;

    virtual void sampleOnSurface(PointQueryRecord &pRec, Sampler *sampler) const = 0;

    virtual float getMeshSurfaceArea() const;

    bool isEmitter() const;

    Emitter *getEmitter() const ;

    void setEmitter(Emitter *_emitter) ;

    Medium *getMedium() const;

    void setMedium(Medium *medium);
    
    std::string mName;

    bool isTwoSide() const {
        return mIsTwoSide;
    }

    void setTwoSide() {
        mIsTwoSide = true;
    }

protected:
    AABB3f mAABB;
    BSDF *mBSDF {nullptr};
    Medium *medium {nullptr};
    Emitter *mEmitter {nullptr};
    float mTotalArea;
    bool mIsTwoSide {false};
};

class TriMesh : public Mesh{
public:

    TriMesh() = default;

    TriMesh(const TriMesh&) = delete;

    TriMesh& operator=(const TriMesh&) = delete;
    
    TriMesh(
        std::vector<Point3f> &&_mVerticesBuf,
        std::vector<Normal3f> &&_mNormalsBuf,
        std::vector<Point3ui> &&_mFacesBuf,
        std::vector<Point2f> &&_mUVsBuf,
        std::vector<Vector3f> &&_mTangentBuf,
        const char *_name
    );

    virtual ~TriMesh() = default;

    virtual Point3ui getFBuf(int triIdx) const;

    virtual Point3f getVtxBuf(int vtxIdx) const;

    virtual Normal3f getNmlBuf(int vtxIdx) const;

    virtual Point2f getUV(int vtxIdx) const;

    virtual Vector3f getTangent(int vtxIdx) const;

    virtual bool hasTangent() const;

    virtual std::string toString() const;

    virtual bool hasNormal() const;

    virtual int getFaceNum() const;

    virtual AABB3f getTriBounds(uint32_t triIdx) const;

    virtual void sampleOnSurface(PointQueryRecord &pRec, Sampler *sampler) const;


protected:
    virtual float getTriArea(uint32_t triIdx) const;

    std::unique_ptr<std::vector<Point3f>>  mVerticesBuf;   // vertices
    std::unique_ptr<std::vector<Normal3f>> mNormalsBuf;    // normals per vertex
    std::unique_ptr<std::vector<Point3ui>> mFacesBuf;      // mesh faces
    std::unique_ptr<std::vector<Point2f>>  mUVsBuf;        // uv coordinates
    std::unique_ptr<std::vector<Vector3f>> mTangentsBuf;   // tangents

    Distribution1D mTriDistribution;


};
