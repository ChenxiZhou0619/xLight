#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <memory>
// forward decleration
class Mesh;
class MeshSet;

class MeshLoader {
public:
    MeshLoader() = default;

    MeshSet* loadFromFile (const std::string &filePath) const ;

    std::unique_ptr<Mesh> convertFromAIMesh (const aiMesh &_aiMesh) const ;
};