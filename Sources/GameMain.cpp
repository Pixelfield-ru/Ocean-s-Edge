//
// Created by ilya on 16.02.24.
//

#include <entt/entity/entity.hpp>
#include <SGCore/Render/PostProcessFrameReceiver.h>
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

#include <BulletCollision/BroadphaseCollision/btDispatcher.h>
#include <SGCore/Render/Camera3D.h>
#include <SGCore/Render/UICamera.h>
#include <SGCore/Memory/Assets/Font.h>
#include <SGCore/Memory/Assets/FontSpecialization.h>
#include <SGCore/UI/Text.h>
#include <SGCore/Main/CoreSettings.h>
#include <SGCore/Render/DisableMeshGeometryPass.h>
#include <SGCore/Render/Batching/Batch.h>
#include <SGCore/Render/ShaderComponent.h>
#include <SGCore/Render/SpacePartitioning/CullableInfo.h>
#include <SGCore/Render/SpacePartitioning/CullableMesh.h>
#include <SGCore/Render/SpacePartitioning/IgnoreOctrees.h>

#include "GameMain.h"
#include "Skybox/DayNightCycleSystem.h"
#include "BlocksTypes.h"
#include "Atlas.h"

void OceansEdge::GameMain::init()
{
    SGCore::RenderPipelinesManager::registerRenderPipeline(SGCore::MakeRef<SGCore::PBRRenderPipeline>());
    SGCore::RenderPipelinesManager::setCurrentRenderPipeline<SGCore::PBRRenderPipeline>();
    
    auto geniusJPG = SGCore::AssetManager::loadAsset<SGCore::ITexture2D>(
            "../SGResources/textures/genius.jpg"
    );
    
    geniusJPG->setRawName("GeniusTexture");
    
    geniusJPG->create();
    
    // todo:
    // SGCore::CoreMain::getWindow().setTitle("Ocean`s Edge");
    
    Atlas::init();
    
    // CREATING WORLD SCENE --------------------------------------
    
    m_worldScene = SGCore::MakeRef<SGCore::Scene>();
    m_worldScene->createDefaultSystems();
    m_worldScene->m_name = "WorldScene";
    SGCore::Scene::addScene(m_worldScene);
    SGCore::Scene::setCurrentScene("WorldScene");
    
    auto dayNightCycleSystem = SGCore::MakeRef<DayNightCycleSystem>();
    dayNightCycleSystem->setScene(m_worldScene);
    m_worldScene->addSystem(dayNightCycleSystem);
    
    // -----------------------------------------------------------
    
    // LOADING MODELS --------------------------------------------
    
    auto skyboxModel = SGCore::AssetManager::loadAssetWithAlias<SGCore::ModelAsset>(
            "OE_SKYBOX",
            "../SGResources/models/standard/cube.obj"
    );
    
    auto mudWithGrassModel = SGCore::AssetManager::loadAssetWithAlias<SGCore::ModelAsset>(
            "OEB_MUD_WITH_GRASS",
            "../SGResources/models/standard/cube.obj"
            //"../SGResources/models/test/vss/scene.gltf"
    );
    
    // mudWithGrassModel->m_nodes[0]->m_children[0]->m_meshesData[0]->setVertexUV();
    
    float aw = (float) Atlas::getAtlas()->m_width;
    float ah = (float) Atlas::getAtlas()->m_height;
    
    /*for(size_t i = 0; i < mudWithGrassModel->m_nodes[0]->m_children[0]->m_children.size(); ++i)
    {
        auto& node = mudWithGrassModel->m_nodes[0]->m_children[0]->m_children[i];
        std::cout << "node " << i << " children cnt " << node->m_children.size() << " meshes cnt " << node->m_meshesData.size() << std::endl;
    }*/
    
    // BlocksTypes::getBlockMeta(BlocksTypes::OEB_MUD_WITH_GRASS) = nullptr;
    auto& mudWithGrassBlockMeta = BlocksTypes::getBlockMeta(BlocksTypes::OEB_MUD_WITH_GRASS);
    // mudWithGrassBlockMeta.m_meshData = mudWithGrassModel->m_nodes[0]->m_children[0]->m_children[0]->m_meshesData[0];
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
    
    auto cameraTransform = m_worldScene->getECSRegistry().emplace<SGCore::Ref<SGCore::Transform>>(testCameraEntity, SGCore::MakeRef<SGCore::Transform>());
    
    auto& cameraEntityCamera = m_worldScene->getECSRegistry().emplace<SGCore::Ref<SGCore::Camera3D>>(testCameraEntity,
            SGCore::MakeRef<SGCore::Camera3D>());
    SGCore::Controllable3D& cameraEntityControllable = m_worldScene->getECSRegistry().emplace<SGCore::Controllable3D>(testCameraEntity);
    auto& cameraRenderingBase = m_worldScene->getECSRegistry().emplace<SGCore::Ref<SGCore::RenderingBase>>(testCameraEntity,
                                                                                                           SGCore::MakeRef<SGCore::RenderingBase>());
    
    // -----------------------------------------------------------
    
    // INITIALIZING SKYBOX ---------------------------------------
    
    {
        std::vector<entt::entity> skyboxEntities;
        skyboxModel->m_nodes[0]->addOnScene(m_worldScene, SG_LAYER_OPAQUE_NAME, [&skyboxEntities](const auto& entity) {
            skyboxEntities.push_back(entity);
        });
        
        SGCore::Mesh& skyboxMesh = m_worldScene->getECSRegistry().get<SGCore::Mesh>(skyboxEntities[2]);
        SGCore::ShaderComponent& shaderComponent = m_worldScene->getECSRegistry().emplace<SGCore::ShaderComponent>(skyboxEntities[2]);
        SGCore::Atmosphere& atmosphereScattering = m_worldScene->getECSRegistry().emplace<SGCore::Atmosphere>(skyboxEntities[2]);
        auto& atmosphereIgnoreOctrees = m_worldScene->getECSRegistry().emplace<SGCore::IgnoreOctrees>(skyboxEntities[2]);
        // atmosphereScattering.m_sunRotation.z = 90.0;
        /*skyboxMesh.m_base.m_meshData->m_material->addTexture2D(SGTextureType::SGTT_SKYBOX,
                                                               standardCubemap
        );*/
        
        //skyboxMesh.m_base.m_meshData->m_material->getShader()->removeSubPass("GeometryPass");
        SGCore::ShadersUtils::loadShader(shaderComponent, "SkyboxShader");
        skyboxMesh.m_base.m_meshDataRenderInfo.m_enableFacesCulling = false;
        
        auto skyboxTransform = m_worldScene->getECSRegistry().get<SGCore::Ref<SGCore::Transform>>(skyboxEntities[2]);
        // auto transformComponent = skyboxEntities[2]->getComponent<SGCore::Transform>();
        
        skyboxTransform->m_ownTransform.m_scale = { 1150, 1150, 1150 };
    }
    
    // -----------------------------------------------------------
    
    {
        entt::entity uiCameraEntity = m_worldScene->getECSRegistry().create();
        SGCore::UICamera& uiCameraEntityCamera = m_worldScene->getECSRegistry().emplace<SGCore::UICamera>(uiCameraEntity);
        auto uiCameraEntityTransform = m_worldScene->getECSRegistry().emplace<SGCore::Ref<SGCore::Transform>>(uiCameraEntity, SGCore::MakeRef<SGCore::Transform>());
        auto& uiCameraEntityRenderingBase = m_worldScene->getECSRegistry().emplace<SGCore::Ref<SGCore::RenderingBase>>(uiCameraEntity,
                SGCore::MakeRef<SGCore::RenderingBase>());
        
        uiCameraEntityRenderingBase->m_left = 0;
        uiCameraEntityRenderingBase->m_right = 2560;
        uiCameraEntityRenderingBase->m_bottom = -1440;
        uiCameraEntityRenderingBase->m_top = 0;
    }
    
    // UI =================================================
    {
        SGCore::Ref<SGCore::Font> timesNewRomanFont = SGCore::AssetManager::loadAssetWithAlias<SGCore::Font>(
                "font_times_new_roman",
                "../SGResources/fonts/arialmt.ttf"
        );
        
        SGCore::Ref<SGCore::FontSpecialization> timesNewRomanFont_height128_rus = timesNewRomanFont->addOrGetSpecialization({ 256, "rus" });
        timesNewRomanFont_height128_rus->parse(u'А', u'Я');
        timesNewRomanFont_height128_rus->parse(u'а', u'я');
        timesNewRomanFont_height128_rus->parse('0', '9');
        timesNewRomanFont_height128_rus->parse({ '.', '!', '?', ')', u'ё', u'Ё'});
        timesNewRomanFont_height128_rus->createAtlas();
        
        SGCore::Ref<SGCore::FontSpecialization> timesNewRomanFont_height128_eng = timesNewRomanFont->addOrGetSpecialization({ 34, "eng" });
        // just example code
        timesNewRomanFont_height128_eng->parse('A', 'Z');
        timesNewRomanFont_height128_eng->parse('a', 'z');
        timesNewRomanFont_height128_eng->parse('0', '9');
        timesNewRomanFont_height128_eng->parse({ '.', '!', '?', ')' });
        timesNewRomanFont_height128_eng->createAtlas();
        
        timesNewRomanFont_height128_rus->saveTextAsTexture("font_spec_text_test_rus.png", u"Здравствуйте.");
        timesNewRomanFont_height128_eng->saveTextAsTexture("font_spec_text_test_eng.png", u"Hi there!!!???))");
        
        timesNewRomanFont_height128_rus->saveAtlasAsTexture("font_spec_test_rus.png");
        timesNewRomanFont_height128_eng->saveAtlasAsTexture("font_spec_test_eng.png");
        
        entt::entity textEntity = m_worldScene->getECSRegistry().create();
        SGCore::Text& helloWorldUIText = m_worldScene->getECSRegistry().emplace<SGCore::Text>(textEntity);
        auto helloWorldUITextTransform = m_worldScene->getECSRegistry().emplace<SGCore::Ref<SGCore::Transform>>(textEntity, SGCore::MakeRef<SGCore::Transform>());
        helloWorldUITextTransform->m_ownTransform.m_scale = { 1.0, 1.0, 1 };
        helloWorldUITextTransform->m_ownTransform.m_position = { 0.0, -50.0, 0 };
        
        std::string formattedVersion = spdlog::fmt_lib::format("{0}.{1}.{2}.{3}", SG_CORE_MAJOR_VERSION, SG_CORE_MINOR_VERSION, SG_CORE_PATCH_VERSION, SG_CORE_BUILD_VERSION);
        helloWorldUIText.m_text = std::u16string(u"Development build. v") +
                                  SGUtils::Utils::fromUTF8<char16_t>(formattedVersion);
        helloWorldUIText.m_usedFont = SGCore::AssetManager::loadAsset<SGCore::Font>("font_times_new_roman");
        helloWorldUIText.m_fontSettings.m_height = 34;
        helloWorldUIText.m_fontSettings.m_name = "eng";
    }
    
    // GENERATING WORLD (TEST) -----------------------------------
    
    {
        SGCore::PerlinNoise perlinNoise;
        // perlinNoise.setSeed(10);
        // perlinNoise.generateMapMultiOctave({ 1000, 1000 }, 1, 1.0f);
        perlinNoise.generate({ 300, 300 }, 6, 0.6f);
        
        auto perlinMapSize = perlinNoise.getCurrentMapSize();
        
        std::cout << "perlin generated" << std::endl;
        
        /*entt::entity blocksInstancingEntity = m_worldScene->getECSRegistry().create();
        SGCore::Instancing& blocksInstancing = m_worldScene->getECSRegistry().emplace<SGCore::Instancing>(blocksInstancingEntity, 200'000);
        blocksInstancing.setExampleMeshData(BlocksTypes::getBlockMeta(BlocksTypes::OEB_MUD_WITH_GRASS).m_meshData);
        std::cout << "ugu" << std::endl;
        blocksInstancing.fillArraysByExample();
        blocksInstancing.updateBuffersEntirely();
        blocksInstancing.m_updateIndices = false;
        blocksInstancing.m_updateUVs = false;
        blocksInstancing.m_updatePositions = false;*/
        
        entt::entity blocksBatchingEntity = m_worldScene->getECSRegistry().create();
        SGCore::Batch& blocksBatch = m_worldScene->getECSRegistry().emplace<SGCore::Batch>(blocksBatchingEntity,
                                                                                           m_worldScene,
                                                                                           1024 * 1024 * 6,
                                                                                           250'000);

        try
        {
            float curZ = 0;
            
            for(int x = 0; x < perlinMapSize.x; ++x)
            {
                for(int y = 0; y < perlinMapSize.y; ++y)
                {
                    float z = perlinNoise.m_map.get(x, y);
                    // curZ += z * 2.0f;
                    
                    // std::cout << z << std::endl;

                    {
                        entt::entity blockEntity = BlocksTypes::getBlockMeta(BlocksTypes::OEB_MUD_WITH_GRASS
                        ).m_meshData->addOnScene(m_worldScene, SG_LAYER_OPAQUE_NAME);
                        m_worldScene->getECSRegistry().emplace<SGCore::DisableMeshGeometryPass>(blockEntity);
                        m_worldScene->getECSRegistry().remove<SGCore::Ref<SGCore::CullableInfo>>(blockEntity);
                        m_worldScene->getECSRegistry().remove<SGCore::Ref<SGCore::CullableMesh>>(blockEntity);

                        blocksBatch.addEntity(blockEntity);
                        // blocksInstancing.m_entitiesToRender.push_back(blockEntity);

                        auto blockTransform = m_worldScene->getECSRegistry().get<SGCore::Ref<SGCore::Transform>>(
                                blockEntity
                        );
                        blockTransform->m_ownTransform.m_position.x = (float) x * 2;
                        blockTransform->m_ownTransform.m_position.y = z * 50.0f;
                        blockTransform->m_ownTransform.m_position.z = (float) y * 2;

                        // m_worldScene->getECSRegistry().patch<SGCore::Transform>(blockEntity);
                    }
                }
            }

            std::cout << "blocks instancing created" << std::endl;

            stbi_write_png("perlin_noise_test.png", perlinMapSize.x, perlinMapSize.y, 4,
                           (perlinNoise.m_map.data()), 4 * perlinMapSize.x
            );
        }
        catch(const std::exception& e)
        {
            spdlog::error("FATAL ERROR: {0}", e.what());
        }
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

void OceansEdge::GameMain::fixedUpdate(const double& dt, const double& fixedDt)
{
    SGCore::Scene::getCurrentScene()->fixedUpdate(dt, fixedDt);
}

void OceansEdge::GameMain::update(const double& dt, const double& fixedDt)
{
    SGCore::CoreMain::getWindow().setTitle("Ocean`s Edge. FPS: " + std::to_string(SGCore::CoreMain::getFPS()));
    
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
    
    SGCore::Scene::getCurrentScene()->update(dt, fixedDt);
    
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
