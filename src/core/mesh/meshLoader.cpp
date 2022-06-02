#include "meshLoader.h"
#include "mesh.h"
#include "meshSet.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <queue>
#include <stack>

MeshSet* MeshLoader::loadFromFile(const std::string &filePath) const {
    
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile( filePath, 
//        aiProcess_CalcTangentSpace      |
//        aiProcess_Triangulate           |
//        aiProcess_JoinIdenticalVertices |
//        aiProcess_SortByPType
          aiProcess_Triangulate           |
          aiProcess_ConvertToLeftHanded
    );
    if (nullptr == scene) {
        std::cerr << "Error when loading the file " << filePath << std::endl;
        exit(0);
    }

    std::cout << "Convert the format\n";
    
    MeshSet* meshSetPtr = new MeshSet();
    for (int i = 0; i < scene->mNumMeshes; ++i) {
        auto meshPtr = convertFromAIMesh(*(scene->mMeshes[i]));
        meshSetPtr->addMesh(std::move(meshPtr));
    }
    
    //traverse the aiNode, apply the transformation to all the point
    //const auto &root = scene->mRootNode;
    // a aiNode queue and matrix stack
    //std::queue<aiNode*> nodeQueue;
    //std::stack<aiMatrix4x4> transMat;

    //nodeQueue.push(root); 
    //transMat.push(root->mTransformation);

    //while (!nodeQueue.empty()) {
    //    const auto &node = nodeQueue.front(); nodeQueue.pop();
    //    const auto &trans= transMat.top(); transMat.pop();
    //    if (node->mNumMeshes != 0) {
    //        for (int i = 0; i < node->mNumMeshes; ++i) {
    //            unsigned int index = node->mMeshes[i];
    //            auto meshPtr = convertFromAIMesh(*(scene->mMeshes[index]), trans);
    //            meshSetPtr->addMesh(std::move(meshPtr));
    //        }
    //    } else {
    //        if (node->mNumChildren != 0) {
    //            for (int i = 0; i < node->mNumChildren; ++i) {
    //                const auto &childNodePtr = node->mChildren[i];
    //                nodeQueue.push(childNodePtr);
    //                transMat.push(childNodePtr->mTransformation * trans);
    //            }
    //        }
    //    }
    //}


    std::cout << filePath << " contains " << meshSetPtr->getMeshNum() << " meshes\n";

    return meshSetPtr;
}

std::unique_ptr<Mesh> MeshLoader::convertFromAIMesh (const aiMesh &_aiMesh) const {
    // convert the assimp data structure to specified format
    
    std::cout << "Current meshName is : " << _aiMesh.mName.C_Str() << std::endl;

    std::vector<Point3f>  vertices;
    std::vector<Normal3f> normals;
    std::vector<Point3ui> faces;
    std::vector<Point2f>  UVs;

    // the two optional data
    bool hasNormals = true, hasUVs = true;
    
    // load the vertices
    if (!_aiMesh.HasPositions()) {
        std::cerr << "Fatal : Meshes with no vertices\n";
        exit(1);
    }
    // reserve the size
    vertices.reserve(_aiMesh.mNumVertices);
    for (int i = 0; i < _aiMesh.mNumVertices; ++i) {
        auto vertex = _aiMesh.mVertices[i];
        vertices.emplace_back(Point3f {vertex[0], vertex[1], vertex[2]});
    }

    // load the normals
    if (!_aiMesh.HasNormals())
        hasNormals = false;
    else {
        // reserve the size
        normals.reserve(_aiMesh.mNumVertices);
        // The array is mNumVertices in size (According to the assimp-doc)
        for (int i = 0; i < _aiMesh.mNumVertices; ++i) {
            auto normal = _aiMesh.mNormals[i];
            normals.emplace_back(Normal3f {normal[0], normal[1], normal[2]});
        }
    }
    
    // load the faces
    if (!_aiMesh.HasFaces()) {
        std::cerr << "Fatal : Meshes with no faces\n";
        exit(1);
    }
    // reserve the size
    faces.reserve(_aiMesh.mNumFaces);
    for (int i = 0; i < _aiMesh.mNumFaces; ++i) {
        const auto &face = _aiMesh.mFaces[i];
        if (face.mNumIndices != 3) {
            std::cerr << "Fatal : Current mesh only support face channels = 3!\n";
            exit(1);
        }
        const auto &indexBuf = face.mIndices;

        faces.emplace_back(Point3ui {indexBuf[0], indexBuf[1], indexBuf[2]});
    }

    // load the UV
    if (_aiMesh.GetNumUVChannels() > 1) {
        std::cerr << "Fatal : Currently not support for uv channels > 1!\n";
        exit(1);
    } else if (_aiMesh.GetNumUVChannels() == 0) {
        hasUVs = false;
    } else if (_aiMesh.GetNumUVChannels() == 1) {
        if (_aiMesh.mNumUVComponents[0] !=2 ) {
            std::cerr << "Fatal : Current only support for uv channels = 2!\n";
            exit(1);
        }
        // reserve the size
        UVs.reserve(_aiMesh.mNumVertices);
        for (int i = 0; i < _aiMesh.mNumVertices; ++i) {
            const auto &uv = _aiMesh.mTextureCoords[0];
            UVs.emplace_back(Point2f {uv[i][0], uv[i][1]});
        }
    } else {
        std::cerr << "Fatal : Undefined behavior when loading the UV!\n";
        exit(1);
    }

    std::unique_ptr<TriMesh> meshPtr = std::make_unique<TriMesh>(
        std::move(vertices),
        std::move(normals),
        std::move(faces),
        std::move(UVs),
        _aiMesh.mName.C_Str()        
    );

    return meshPtr;
}
