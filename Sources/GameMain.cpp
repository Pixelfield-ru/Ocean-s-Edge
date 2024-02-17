//
// Created by ilya on 16.02.24.
//

#include <entt/entity/entity.hpp>
#include <SGCore/Render/Camera.h>
#include <SGCore/Transformations/Transform.h>
#include <SGCore/Scene/EntityBaseInfo.h>
#include <SGCore/Main/CoreMain.h>
#include <SGCore/Scene/Scene.h>
#include <SGUtils/UniqueName.h>
#include <SGCore/Transformations/Controllable3D.h>
#include <SGCore/Render/RenderingBase.h>
#include <SGCore/Render/Mesh.h>
#include <SGCore/Render/Atmosphere/Atmosphere.h>
#include <SGCore/Memory/Assets/Materials/IMaterial.h>
#include <SGCore/Memory/Assets/ModelAsset.h>
#include <SGUtils/Noise/PerlinNoise.h>
#include <SGCore/ImGuiWrap/ImGuiLayer.h>
#include <SGCore/Render/RenderPipelinesManager.h>
#include <SGCore/Graphics/API/ITexture2D.h>
#include <stb_image_write.h>
#include <SGCore/Render/PBRRP/PBRRenderPipeline.h>

#include "GameMain.h"
#include "Systems/DayNightCycleSystem.h"
#include "BlocksTypes.h"
#include "Atlas.h"

void OceansEdge::GameMain::init()
{
    SGCore::RenderPipelinesManager::registerRenderPipeline(SGCore::MakeRef<SGCore::PBRRenderPipeline>());
    SGCore::RenderPipelinesManager::setCurrentRenderPipeline<SGCore::PBRRenderPipeline>();
    
    // todo:
    // SGCore::CoreMain::getWindow().setTitle("Ocean`s Edge");
    
    Atlas::init();
    
    // CREATING WORLD SCENE --------------------------------------
    
    m_worldScene = SGCore::MakeRef<SGCore::Scene>();
    m_worldScene->createDefaultSystems();
    SGCore::Scene::setCurrentScene(m_worldScene);
    
    auto dayNightCycleSystem = SGCore::MakeRef<DayNightCycleSystem>();
    
    m_worldScene->getAllSystems().insert(dayNightCycleSystem);
    
    // -----------------------------------------------------------
    
    // LOADING MODELS --------------------------------------------
    
    auto skyboxModel = SGCore::AssetManager::loadAssetWithAlias<SGCore::ModelAsset>(
            "OE_SKYBOX",
            "../SGResources/models/standard/cube.obj"
    );
    
    auto mudWithGrassModel = SGCore::AssetManager::loadAssetWithAlias<SGCore::ModelAsset>(
            "OEB_MUD_WITH_GRASS",
            "../SGResources/models/standard/cube.obj"
    );
    
    // mudWithGrassModel->m_nodes[0]->m_children[0]->m_meshesData[0]->setVertexUV();
    
    float aw = (float) Atlas::getAtlas()->m_width;
    float ah = (float) Atlas::getAtlas()->m_height;
    
    // BlocksTypes::getBlockMeta(BlocksTypes::OEB_MUD_WITH_GRASS) = nullptr;
    auto& mudWithGrassBlockMeta = BlocksTypes::getBlockMeta(BlocksTypes::OEB_MUD_WITH_GRASS);
    mudWithGrassBlockMeta.m_meshData = mudWithGrassModel->m_nodes[0]->m_children[0]->m_meshesData[0];
    mudWithGrassBlockMeta.m_meshData->m_material->addTexture2D(SGTextureType::SGTT_DIFFUSE, Atlas::getAtlas());
    
    mudWithGrassBlockMeta.m_meshData->setVertexUV(8, 0, 0, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(9, 160.0f / aw, 0, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(10, 160.0f / aw, 160 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(11, 0, 160 / ah, 0);
    
    mudWithGrassBlockMeta.m_meshData->setVertexUV(1, 0, 160 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(2, 160.0f / aw, 160 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(3, 160.0f / aw, 320 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(0, 0, 320 / ah, 0);
    
    mudWithGrassBlockMeta.m_meshData->setVertexUV(6, 0, 160 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(7, 160.0f / aw, 160 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(4, 160.0f / aw, 320 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(5, 0, 320 / ah, 0);
    
    mudWithGrassBlockMeta.m_meshData->setVertexUV(15, 0, 160 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(12, 160.0f / aw, 160 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(13, 160.0f / aw, 320 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(14, 0, 320 / ah, 0);
    
    mudWithGrassBlockMeta.m_meshData->setVertexUV(16, 3 * 160 / aw, 160 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(17, 4 * 160.0f / aw, 160 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(18, 4 * 160.0f / aw, 320 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(19, 3 * 160 / aw, 320 / ah, 0);
    
    mudWithGrassBlockMeta.m_meshData->setVertexUV(20, 0, 160 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(21, 160.0f / aw, 160 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(22, 160.0f / aw, 320 / ah, 0);
    mudWithGrassBlockMeta.m_meshData->setVertexUV(23, 0, 320 / ah, 0);
    
    mudWithGrassBlockMeta.m_meshData->prepare();
    // mudWithGrassBlockMeta.m_
    
    // cubeModel->m_nodes[0]->m_meshesData[0]
    
    // -----------------------------------------------------------
    
    // INITIALIZING PLAYER ---------------------------------------
    
    entt::entity testCameraEntity = m_worldScene->getECSRegistry().create();
    SGCore::EntityBaseInfo& cameraBaseInfo = m_worldScene->getECSRegistry().emplace<SGCore::EntityBaseInfo>(testCameraEntity);
    cameraBaseInfo.setRawName("SGMainCamera");
    
    SGCore::Transform& cameraTransform = m_worldScene->getECSRegistry().emplace<SGCore::Transform>(testCameraEntity);
    
    SGCore::Camera& cameraEntityCamera = m_worldScene->getECSRegistry().emplace<SGCore::Camera>(testCameraEntity);
    SGCore::Controllable3D& cameraEntityControllable = m_worldScene->getECSRegistry().emplace<SGCore::Controllable3D>(testCameraEntity);
    SGCore::RenderingBase& cameraRenderingBase = m_worldScene->getECSRegistry().emplace<SGCore::RenderingBase>(testCameraEntity);
    
    // -----------------------------------------------------------
    
    // INITIALIZING SKYBOX ---------------------------------------
    
    /*{
        std::vector<entt::entity> skyboxEntities;
        skyboxModel->m_nodes[0]->addOnScene(m_worldScene, SG_LAYER_OPAQUE_NAME, [&skyboxEntities](const auto& entity) {
            skyboxEntities.push_back(entity);
        });
        
        SGCore::Mesh& skyboxMesh = m_worldScene->getECSRegistry().get<SGCore::Mesh>(skyboxEntities[2]);
        SGCore::Atmosphere& atmosphereScattering = m_worldScene->getECSRegistry().emplace<SGCore::Atmosphere>(skyboxEntities[2]);
        // atmosphereScattering.m_sunRotation.z = 90.0;
        *//*skyboxMesh.m_base.m_meshData->m_material->addTexture2D(SGTextureType::SGTT_SKYBOX,
                                                               standardCubemap
        );*//*
        
       *//* skyboxMesh.m_base.m_meshData->m_material->getShader()->removeSubPass("GeometryPass");
        SGCore::MeshesUtils::loadMeshShader(skyboxMesh.m_base, "SkyboxShader");*//*
        skyboxMesh.m_base.m_meshDataRenderInfo.m_enableFacesCulling = false;
        
        SGCore::Transform& skyboxTransform = m_worldScene->getECSRegistry().get<SGCore::Transform>(skyboxEntities[2]);
        // auto transformComponent = skyboxEntities[2]->getComponent<SGCore::Transform>();
        
        skyboxTransform.m_ownTransform.m_scale = { 1150, 1150, 1150 };
    }*/
    
    // -----------------------------------------------------------
    
    // GENERATING WORLD (TEST) -----------------------------------
    
    {
        SGCore::PerlinNoise perlinNoise;
        perlinNoise.setSeed(10);
        perlinNoise.generateMap({ 32, 32 });
        
        auto perlinMapSize = perlinNoise.getCurrentMapSize();
        
        for(int x = 0; x < perlinMapSize.x; ++x)
        {
            for(int y = 0; y < perlinMapSize.y; ++y)
            {
                float z = perlinNoise.m_map.get(x, y);
                
                {
                    entt::entity blockEntity = BlocksTypes::getBlockMeta(BlocksTypes::OEB_MUD_WITH_GRASS).m_meshData->addOnScene(m_worldScene, SG_LAYER_OPAQUE_NAME);
                    
                    SGCore::Transform* blockTransform = m_worldScene->getECSRegistry().try_get<SGCore::Transform>(
                            blockEntity);
                    blockTransform->m_ownTransform.m_position.x = (float) x * 2;
                    blockTransform->m_ownTransform.m_position.y = (float) z * 2;
                    blockTransform->m_ownTransform.m_position.z = (float) y * 2;
                }
            }
        }
        
        stbi_write_png("perlin_noise_test.png", perlinMapSize.x, perlinMapSize.y, 4,
                       (perlinNoise.m_map.data()), 4 * perlinMapSize.x);
    }
    
    // 0.94 килобайта для Transform + EntityBaseInfo + Mesh
    std::cout <<
    "sizeof Transform: " << sizeof(SGCore::Transform) <<
    ", EntityBaseInfo: " << sizeof(SGCore::EntityBaseInfo) <<
    ", Mesh: " << sizeof(SGCore::Mesh) <<
    ", IMeshData: " << sizeof(SGCore::IMeshData) <<
    ", IMaterial: " << sizeof(SGCore::IMaterial) <<
    ", IShader: " << sizeof(SGCore::IShader) <<
    ", ISubPassShader: " << sizeof(SGCore::ISubPassShader) <<
    std::endl;
    
    // -----------------------------------------------------------
    
    SGCore::ImGuiWrap::ImGuiLayer::initImGui();
}

void OceansEdge::GameMain::fixedUpdate()
{
    SGCore::Scene::getCurrentScene()->fixedUpdate();
}

void OceansEdge::GameMain::update()
{
    SGCore::ImGuiWrap::ImGuiLayer::beginFrame();
    
    ImGui::Begin("ECS Systems Stats");
    {
        if(ImGui::BeginTable("SystemsStats", 3))
        {
            ImGui::TableNextColumn();
            ImGui::Text("System");
            ImGui::TableNextColumn();
            ImGui::Text("update");
            ImGui::TableNextColumn();
            ImGui::Text("fixedUpdate");
            ImGui::TableNextColumn();
            
            for(const auto& system : SGCore::Scene::getCurrentScene()->getAllSystems())
            {
                std::string systemName = std::string(typeid(*(system)).name());
                ImGui::Text(systemName.c_str());
                
                ImGui::TableNextColumn();
                
                ImGui::Text((std::to_string(system->getUpdateFunctionExecutionTime()) + " ms").c_str());
                
                ImGui::TableNextColumn();
                
                ImGui::Text((std::to_string(system->getFixedUpdateFunctionExecutionTime()) + " ms").c_str());
                
                ImGui::TableNextColumn();
            }
            
            ImGui::EndTable();
        }
        
        double t0 = SGCore::Scene::getCurrentScene()->getUpdateFunctionExecutionTime();
        double t1 = SGCore::Scene::getCurrentScene()->getFixedUpdateFunctionExecutionTime();
        double t2 = SGCore::RenderPipelinesManager::getCurrentRenderPipeline()->getRenderPassesExecutionTime();
        double t3 = SGCore::CoreMain::getWindow().getSwapBuffersExecutionTime();
        
        ImGui::Text("Scene update execution: %f", t0);
        ImGui::Text("Scene fixed update execution: %f", t1);
        ImGui::Text("Scene total execution: %f", t0 + t1);
        ImGui::Text("Render pipeline execution: %f", t2);
        ImGui::Text("Swap buffers execution: %f", t3);
        ImGui::Text("Render total execution: %f", t2 + t3);
        ImGui::Text("Total frame time: %f", t0 + t1 + t2 + t3);
    }
    ImGui::End();
    
    SGCore::Scene::getCurrentScene()->update();
    
    SGCore::ImGuiWrap::ImGuiLayer::endFrame();
}

int main()
{
    sgSetCoreInitCallback(&OceansEdge::GameMain::init);
    sgSetFixedUpdateCallback(&OceansEdge::GameMain::fixedUpdate);
    sgSetUpdateCallback(&OceansEdge::GameMain::update);
    
    SGCore::CoreMain::start();
    
    return 0;
}
