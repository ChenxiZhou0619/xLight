#pragma once
#include <memory>
#include <vector>

#include <assimp/scene.h>

#include "core/geometry/geometry.h"

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
    
protected:
    AABB3f mAABB;
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
        std::vector<Point2f> &&_mUVsBuf
    );

    virtual ~TriMesh() = default;

    virtual Point3ui getFBuf(int triIdx) const;

    virtual Point3f getVtxBuf(int vtxIdx) const;

    virtual Normal3f getNmlBuf(int vtxIdx) const;

    virtual std::string toString() const;

    virtual bool hasNormal() const;

    virtual int getFaceNum() const;

protected:

    std::unique_ptr<std::vector<Point3f>>  mVerticesBuf;   // vertices
    std::unique_ptr<std::vector<Normal3f>> mNormalsBuf;    // normals per vertex
    std::unique_ptr<std::vector<Point3ui>> mFacesBuf;      // mesh faces
    std::unique_ptr<std::vector<Point2f>>  mUVsBuf;        // uv coordinates

};