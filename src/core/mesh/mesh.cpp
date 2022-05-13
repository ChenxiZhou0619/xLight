#include "mesh.h"
#include "tinyformat/tinyformat.h"

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

std::string TriMesh::toString() const {
    return tfm::format("numVertices = %d, numFaces = %d, numNormals = %d, numUVs = %d", 
        mVerticesBuf->size(), mFacesBuf->size(), mNormalsBuf->size(), mUVsBuf->size());
}