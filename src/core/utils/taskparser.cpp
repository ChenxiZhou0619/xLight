#include "taskparser.h"
#include "rapidjson/document.h"
#include "core/file/image.h"
#include "core/utils/configurable.h"
#include <fstream>
#include <iostream>

Vector2i RenderTask::getImgSize() const {
    return image->getSize();
}
uint32_t RenderTask::getSpp() const {
    return image->getSpp();
}

Texture* RenderTask::getTexture(const std::string &textureName) const {
    const auto &itr = textures.find(textureName);
    if (itr == textures.end()) {
        std::cout << "Error! : Undefined texture name \"" <<textureName << "\"" 
                  << std::endl;
        exit(1);
    }
    return itr->second.get();
}

std::unique_ptr<RenderTask> RenderTaskParser::createTask(const std::string &jsonFilePath) {
    RenderTask *task = new RenderTask();

    /*=========== read the file and parse ==============*/
    std::ifstream jsonFile;
    jsonFile.open(jsonFilePath, std::ios::in);
    std::streampos begin, end;
    begin = jsonFile.tellg();
    jsonFile.seekg(0, std::ios::end);
    end = jsonFile.tellg();
    jsonFile.seekg(0, std::ios::beg);
    char *json = new char[end - begin];
    jsonFile.read(json, end-begin);
    jsonFile.close();
    rapidjson::Document document;
    document.Parse(json);
    delete[] json;

    /*============= traverse the document object =========*/
    for (auto itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) {
        if (strcmp(itr->name.GetString(), "output") == 0) {
            // configure the img object
            const std::string &filename 
                = (*itr).value["filename"].GetString();
            const auto &size 
                = (*itr).value["size"].GetArray();
            int spp 
                = (*itr).value["spp"].GetInt();
            task->image = std::make_unique<Image>(
                Vector2i {size[0].GetInt(), size[1].GetInt()},
                filename,
                spp
            );
            std::cout << "========== Output setting ==========\n";
            std::cout << *task->image << std::endl;
            std::cout << "====================================\n";
        } 
        else if (strcmp(itr->name.GetString(), "scene") == 0) {
            // configure textures
            const auto &textures = (*itr).value["textures"].GetArray();
            for (int i = 0; i < textures.Size(); ++i) {
                const auto &texture = textures[i].GetObject();
                const std::string &textureName
                    = texture["name"].GetString();
                const std::string &textureType
                    = texture["type"].GetString();
                task->textures[textureName].reset(
                    static_cast<Texture *> (ObjectFactory::createInstance(
                        textureType, texture))
                );  
            }
            // configure bsdf
            const auto &bsdfs = (*itr).value["bsdfs"].GetArray();
            for (int i = 0; i < bsdfs.Size(); ++i) {
                const auto &bsdf = bsdfs[i].GetObject();
                const std::string &bsdfName
                    = bsdf["name"].GetString();
                const std::string &bsdfType
                    = bsdf["type"].GetString();
                BSDF *newBsdf
                        = static_cast<BSDF*>(
                            ObjectFactory::createInstance(bsdfType, bsdf)
                        );
                if (bsdf.HasMember("textureRef")) {
                    const std::string &textureRef
                        = bsdf["textureRef"].GetString();
                    newBsdf->setTexture(
                        task->getTexture(textureRef)
                    );
                } else {
                    newBsdf->setTexture(nullptr);
                }

                if (bsdf.HasMember("bumpmapRef")) {
                    const std::string &bumpmapRef
                        = bsdf["bumpmapRef"].GetString();
                    newBsdf->setBumpmap(
                        task->getTexture(bumpmapRef)
                    );
                } else {
                    newBsdf->setBumpmap(nullptr);
                }

                newBsdf->initialize();                
                task->bsdfs[bsdfName].reset(newBsdf);
            }
            // configure emitters
            const auto emitters = (*itr).value["emitters"].GetArray();
            Emitter *env_emitter = nullptr;
            for (int i = 0; i < emitters.Size(); ++i) {
                const auto &emitter = emitters[i].GetObject();
                const std::string &emitterType
                    = emitter["type"].GetString();
                if (strcmp(emitterType.c_str(), "envemitter") == 0) {
                    // set the texture to the envmap
                    const std::string &textureRef
                        = emitter["textureRef"].GetString();
                    // TODO scene has not been construct
                    Texture *env_texture = task->getTexture(textureRef);
                    env_emitter
                        = static_cast<Emitter *>(
                            ObjectFactory::createInstance("envemitter", emitter)
                        );
                    env_emitter->setTexture(env_texture);
                    env_emitter->initialize();
                    
                } else {
                    const std::string &emitterName
                        = emitter["name"].GetString();
                    Emitter *newEmitter
                        = static_cast<Emitter *>(
                            ObjectFactory::createInstance("area", emitter)
                        );
                    task->emitters[emitterName].reset(newEmitter);
                }            
            }

            const auto mediums = (*itr).value["mediums"].GetArray();
            for (int i = 0; i < mediums.Size(); ++i) {
                const auto &medium = mediums[i].GetObject();
                const std::string &mediumType
                    = medium["type"].GetString();
                const std::string &mediumName 
                    = medium["name"].GetString();
                Medium *newMedium 
                    = static_cast<Medium *>(
                        ObjectFactory::createInstance(mediumType, medium)
                    );
                task->mediums[mediumName].reset(newMedium);
            }
            
            // configure the scene object
            MeshLoader meshLoader;
            const auto &entities = (*itr).value["entities"].GetArray();
            MeshSet *meshSet = new MeshSet();
            for (int i = 0; i < entities.Size(); ++i) {
                const auto &entity = entities[i].GetObject();
                const std::string &filepath = entity["filepath"].GetString();
                std::unique_ptr<MeshSet> tmp;
                tmp.reset(
                    meshLoader.loadFromFile(filepath)
                );
                // configure the bsdf 
                const auto &meshProperties = entity["meshProperties"].GetArray();
                for (int i = 0; i < meshProperties.Size(); ++i) {
                    const auto &property = meshProperties[i].GetObject();
                    const std::string &meshName 
                        = property["meshName"].GetString();
                    Mesh *targetMesh = tmp->getByName(meshName);
                    if (!targetMesh) {
                        std::cout << "No such a mesh : \"" << meshName << "\"" << std::endl;
                        exit(1);
                    }
                    if (property.HasMember("BSDFRef")) {
                        const std::string &bsdfRef
                            = property["BSDFRef"].GetString();
                        const auto &itr = task->bsdfs.find(bsdfRef);
                        if (itr == task->bsdfs.end()) {
                            std::cout << "No such a BSDF : \"" << bsdfRef << "\"" << std::endl;
                            exit(1);
                        }
                        targetMesh->setBSDF(itr->second.get());
                    }
                    if (property.HasMember("emitterRef")) {
                        const std::string &emitterRef
                            = property["emitterRef"].GetString();
                        const auto &itr = task->emitters.find(emitterRef);
                        if (itr == task->emitters.end()) {
                            std::cout << "No such an Emitter : \"" << emitterRef << "\"" << std::endl; 
                            exit(1);
                        }
                        targetMesh->setEmitter(itr->second.get());
                    }

                    if (property.HasMember("mediumRef")) {
                        const std::string &mediumRef
                            = property["mediumRef"].GetString();
                        const auto &itr = task->mediums.find(mediumRef);
                        if (itr == task->mediums.end()) {
                            std::cout << "No such an medium : \"" << mediumRef << "\"" << std::endl; 
                            exit(1);
                        }
                        targetMesh->setMedium(itr->second.get());
                    }

                    if (property.HasMember("isTwoSide")) {
                        if (property["isTwoSide"].GetBool())
                            targetMesh->setTwoSide();
                    }
                }

                meshSet->mergeMeshSet(std::move(tmp));
            }
            task->scene = std::make_unique<Scene>(meshSet);
            task->scene->setEnvMap(env_emitter);
            task->scene->preprocess();

        } else if (strcmp(itr->name.GetString(), "renderer") == 0) {
            // configure the renderer settings
            const std::string 
                &samplerType = (*itr).value["sampler"]["type"].GetString();
            task->sampler.reset(static_cast<Sampler *> (ObjectFactory::createInstance(samplerType, (*itr).value["sampler"]))); 
            std::cout << "Configure the sampler : " << samplerType << std::endl;        

            const std::string
                &cameraType = (*itr).value["camera"]["type"].GetString();
            task->camera.reset(static_cast<Camera *> (ObjectFactory::createInstance(cameraType, (*itr).value["camera"])));
            std::cout << "Configure the camera : " << cameraType << std::endl;

            const std::string 
                &integratorType = (*itr).value["integrator"]["type"].GetString();
            task->integrator.reset(static_cast<Integrator *> (ObjectFactory::createInstance(integratorType, (*itr).value["integrator"])));
            std::cout << "Configure the integrator : " << integratorType << std::endl;
        }
    }

    std::unique_ptr<RenderTask> res {nullptr}; res.reset(task);
    return res;
}