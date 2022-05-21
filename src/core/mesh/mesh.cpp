#include "mesh.h"
#include "tinyformat/tinyformat.h"

TriMesh::TriMesh( std::vector<Point3f> &&_mVerticesBuf, std::vector<Normal3f> &&_mNormalsBuf,
    std::vector<Point3ui> &&_mFacesBuf, std::vector<Point2f> &&_mUVsBuf
): mVerticesBuf(std::make_unique<std::vector<Point3f>>(_mVerticesBuf)),
   mNormalsBuf(std::make_unique<std::vector<Normal3f>>(_mNormalsBuf)),
   mFacesBuf(std::make_unique<std::vector<Point3ui>>(_mFacesBuf)),
   mUVsBuf(std::make_unique<std::vector<Point2f>>(_mUVsBuf)) {

    // initialize for aabb
    for (const Point3f &p : *mVerticesBuf) {
        mAABB.expands(p);
    }    
}

Point3ui TriMesh::getFBuf(int triIdx) const {
    return (*mFacesBuf)[triIdx];
}

Point3f TriMesh::getVtxBuf(int vtxIdx) const {
    return (*mVerticesBuf)[vtxIdx];
}

Normal3f TriMesh::getNmlBuf(int vtxIdx) const {
    if (mNormalsBuf->size() == 0) {
        // caculate the Normal
        std::cout << "Not implement getNmlBuf when need to compute the normal\n";
        exit(1);
    }
    return (*mNormalsBuf)[vtxIdx];
}

AABB3f Mesh::getAABB3() const {
    return mAABB;
}

AABB3f Mesh::getTriBounds(uint32_t triIdx) const {
    Point3ui fBuf = getFBuf(triIdx);
    Point3f p0 = getVtxBuf(fBuf[0]),
            p1 = getVtxBuf(fBuf[1]),
            p2 = getVtxBuf(fBuf[2]);
    AABB3f bounds;
    bounds.expands(p0);bounds.expands(p1),bounds.expands(p2);
    return bounds;
}

bool TriMesh::hasNormal() const {
    return !mNormalsBuf->empty();
}

int TriMesh::getFaceNum() const {
    return mFacesBuf->size();
}

std::string TriMesh::toString() const {
    return tfm::format("numVertices = %d, numFaces = %d, numNormals = %d, numUVs = %d", 
        mVerticesBuf->size(), mFacesBuf->size(), mNormalsBuf->size(), mUVsBuf->size());
}