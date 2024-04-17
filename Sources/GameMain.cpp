#include "WorldChunksUpdater.h"
#include "SGUtils/CrashHandler/Platform.h"

#ifdef PLATFORM_OS_WINDOWS
#ifdef __cplusplus
extern "C" {
#endif
#include <windows.h>
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#ifdef __cplusplus
}
#endif
#endif

#include <SGUtils/CrashHandler/Platform.h>

#include <entt/entity/entity.hpp>
#include <SGCore/Render/LayeredFrameReceiver.h>
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
#include <SGCore/Render/SpacePartitioning/OctreeCullable.h>
#include <SGCore/Render/SpacePartitioning/IgnoreOctrees.h>
#include <SGCore/Threading/ThreadsManager.h>

#include "GameMain.h"
#include "Skybox/DayNightCycleSystem.h"
#include "BlocksTypes.h"
#include "World.h"
#include "Player/LocalPlayer.h"
#include "OEPhysicalEntity.h"
#include "Defines.h"
#include "Resources.h"
#include "PlayerController.h"
#include "AmbientPlayer.h"

void OceansEdge::GameMain::init()
{
    Settings::init();
    
    SGCore::RenderPipelinesManager::registerRenderPipeline(SGCore::MakeRef<SGCore::PBRRenderPipeline>());
    SGCore::RenderPipelinesManager::setCurrentRenderPipeline<SGCore::PBRRenderPipeline>();

    Resources::init();
    
    // CREATING WORLD SCENE --------------------------------------
    
    m_worldScene = SGCore::MakeRef<SGCore::Scene>();
    m_worldScene->createDefaultSystems();
    m_worldScene->m_name = "WorldScene";
    SGCore::Scene::addScene(m_worldScene);
    SGCore::Scene::setCurrentScene("WorldScene");
    
    auto dayNightCycleSystem = SGCore::MakeRef<DayNightCycleSystem>();
    auto worldChunksUpdater = SGCore::MakeRef<WorldChunksUpdater>();
    auto playerControllerSystem = SGCore::MakeRef<PlayerController>();
    auto ambientPlayer = SGCore::MakeRef<AmbientPlayer>();
    m_worldScene->addSystem(dayNightCycleSystem);
    m_worldScene->addSystem(worldChunksUpdater);
    m_worldScene->addSystem(playerControllerSystem);
    m_worldScene->addSystem(ambientPlayer);
    
    m_worldScene->createLayer("test_pp");
    
    // -----------------------------------------------------------
    
    // LOADING MODELS --------------------------------------------
    
    auto skyboxModel = SGCore::AssetManager::loadAssetWithAlias<SGCore::ModelAsset>(
            "OE_SKYBOX",
            "../SGResources/models/standard/cube.obj"
    );
    
    auto cubeModel = SGCore::AssetManager::loadAssetWithAlias<SGCore::ModelAsset>(
            "model_cube",
            "../SGResources/models/standard/cube.obj"
    );
    
    std::vector<SGCore::entity_t> cubeEntities;
    cubeModel->m_nodes[0]->addOnScene(m_worldScene, "test_pp", [&cubeEntities](const SGCore::entity_t& entity) {
        cubeEntities.push_back(entity);
    });
    
    auto cubeTransform = m_worldScene->getECSRegistry()->get<SGCore::Ref<SGCore::Transform>>(cubeEntities[0]);
    cubeTransform->m_ownTransform.m_position.y = 600.0f;
    
    // INITIALIZING SKYBOX ---------------------------------------
    
    {
        std::vector<SGCore::entity_t> skyboxEntities;
        skyboxModel->m_nodes[0]->addOnScene(m_worldScene, SG_LAYER_OPAQUE_NAME, [&skyboxEntities](const auto& entity) {
            skyboxEntities.push_back(entity);
        });
        
        SGCore::Mesh& skyboxMesh = m_worldScene->getECSRegistry()->get<SGCore::Mesh>(skyboxEntities[2]);
        SGCore::ShaderComponent& shaderComponent = m_worldScene->getECSRegistry()->emplace<SGCore::ShaderComponent>(skyboxEntities[2]);
        SGCore::Atmosphere& atmosphereScattering = m_worldScene->getECSRegistry()->emplace<SGCore::Atmosphere>(skyboxEntities[2]);
        auto& atmosphereIgnoreOctrees = m_worldScene->getECSRegistry()->emplace<SGCore::IgnoreOctrees>(skyboxEntities[2]);
        // atmosphereScattering.m_sunRotation.z = 90.0;
        /*skyboxMesh.m_base.m_meshData->m_material->addTexture2D(SGTextureType::SGTT_SKYBOX,
                                                               standardCubemap
        );*/



        //skyboxMesh.m_base.m_meshData->m_material->getShader()->removeSubPass("GeometryPass");
        shaderComponent.m_shader->addSubPassShadersAndCompile(
            SGCore::AssetManager::loadAsset<SGCore::TextFileAsset>(
                Settings::getShadersPaths()["FoggedSkyboxShader"].getCurrentRealization()));
        shaderComponent.m_isCustomShader = true;
        // SGCore::ShadersUtils::loadShader(shaderComponent, "SkyboxShader");
        skyboxMesh.m_base.m_meshDataRenderInfo.m_enableFacesCulling = false;
        
        auto skyboxTransform = m_worldScene->getECSRegistry()->get<SGCore::Ref<SGCore::Transform>>(skyboxEntities[2]);
        // auto transformComponent = skyboxEntities[2]->getComponent<SGCore::Transform>();
        
        skyboxTransform->m_ownTransform.m_scale = { 1150, 1150, 1150 };
    }
    
    // -----------------------------------------------------------
    
    {
        auto uiCameraEntity = m_worldScene->getECSRegistry()->create();
        SGCore::UICamera& uiCameraEntityCamera = m_worldScene->getECSRegistry()->emplace<SGCore::UICamera>(uiCameraEntity);
        auto uiCameraEntityTransform = m_worldScene->getECSRegistry()->emplace<SGCore::Ref<SGCore::Transform>>(uiCameraEntity, SGCore::MakeRef<SGCore::Transform>());
        auto& uiCameraEntityRenderingBase = m_worldScene->getECSRegistry()->emplace<SGCore::Ref<SGCore::RenderingBase>>(uiCameraEntity,
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
        
        auto textEntity = m_worldScene->getECSRegistry()->create();
        SGCore::Text& helloWorldUIText = m_worldScene->getECSRegistry()->emplace<SGCore::Text>(textEntity);
        auto helloWorldUITextTransform = m_worldScene->getECSRegistry()->emplace<SGCore::Ref<SGCore::Transform>>(textEntity, SGCore::MakeRef<SGCore::Transform>());
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
        
        m_world = SGCore::MakeRef<World>();
        
        // chunk0Transform = m_worldScene->getECSRegistry().get<SGCore::Ref<SGCore::Transform>>(ChunksManager::getChunks()[0]);
        m_world->prepareGrid(m_worldScene);
        
    }
    
    auto& cameraLayeredFrameReceiver = m_worldScene->getECSRegistry()->get<SGCore::LayeredFrameReceiver>(m_world->getPlayerEntity());
    
    for(const auto& e : cubeEntities)
    {
        std::cout << "has mesh: " << (m_worldScene->getECSRegistry()->try_get<SGCore::Mesh>(e) ? "true" : "false") << std::endl;
    }
    
    auto testPPLayer = cameraLayeredFrameReceiver.addLayer("test_pp_layer");
    SGCore::PostProcessFXSubPass subPass;
    subPass.m_attachmentRenderTo = SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT0;
    testPPLayer->m_subPasses.push_back(subPass);
    subPass.m_attachmentRenderTo = SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT1;
    testPPLayer->m_subPasses.push_back(subPass);
    m_worldScene->getECSRegistry()->get<SGCore::EntityBaseInfo>(cubeEntities[2]).m_postProcessLayers[&cameraLayeredFrameReceiver] = testPPLayer;
    m_worldScene->getECSRegistry()->get<SGCore::Ref<SGCore::Transform>>(cubeEntities[0])->m_ownTransform.m_scale = { 3.0, 3.0, 3.0 };
    
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

    worldChunksUpdater->getThread()->start();
}

void OceansEdge::GameMain::fixedUpdate(const double& dt, const double& fixedDt)
{
    // chunk0Transform->m_ownTransform.m_rotation.y += 10.0f * dt;
    
    SGCore::Scene::getCurrentScene()->fixedUpdate(dt, fixedDt);
}

void OceansEdge::GameMain::update(const double& dt, const double& fixedDt)
{
    // std::cout << playerTransform->m_ownTransform.m_position.x << std::endl;
    
    SGCore::CoreMain::getWindow().setTitle("Ocean`s Edge. FPS: " + std::to_string(SGCore::CoreMain::getFPS()));
    
    // SGCore::ImGuiWrap::ImGuiLayer::beginFrame();
    
    /*ImGui::Begin("ECS Systems Stats");
    {
        if(ImGui::BeginTable("SystemsStats", 5))
        {
            // ImGui::Columns(5, "Columns", true);
            
            ImGui::TableNextColumn();
            ImGui::Text("System");
            ImGui::TableNextColumn();
            ImGui::Text("update");
            ImGui::TableNextColumn();
            ImGui::Text("fixedUpdate");
            ImGui::TableNextColumn();
            ImGui::Text("parallelUpdate");
            ImGui::TableNextColumn();
            ImGui::Text("Thread ID");
            
            for(const auto& system : SGCore::Scene::getCurrentScene()->getAllSystems())
            {
                ImGui::TableNextColumn();
                std::string systemName = std::string(typeid(*(system)).name());
                ImGui::Text(systemName.c_str());
                
                ImGui::TableNextColumn();
                
                ImGui::Text((std::to_string(system->m_executionTimes["update"]) + " ms").c_str());
                
                ImGui::TableNextColumn();
                
                ImGui::Text((std::to_string(system->m_executionTimes["fixedUpdate"]) + " ms").c_str());
                
                ImGui::TableNextColumn();
                
                ImGui::Text((std::to_string(system->m_executionTimes["parallelUpdate"]) + " ms").c_str());
                
                ImGui::TableNextColumn();
                
                ImGui::Text(std::to_string(system->getThreadID()).c_str());
                
                //SGCore::Ref<SGCore::IP
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
    ImGui::End();*/

    SGCore::Scene::getCurrentScene()->update(dt, fixedDt);

    m_world->render(m_worldScene);

    // SGCore::ImGuiWrap::ImGuiLayer::endFrame();
}

SGCore::Ref<OceansEdge::World> OceansEdge::GameMain::getCurrentWorld() noexcept
{
    return m_world;
}

int main()
{
    std::cout << "main thread: " << SGCore::Threading::ThreadsManager::currentThread() << std::endl;
    
    auto sampleWorker = SGCore::Threading::ThreadsManager::currentThread()->createWorker();
    
    sampleWorker->setOnExecutedCallback([]() {
        std::cout << "sample worker done" << std::endl;
    });
    
    SGCore::Threading::ThreadsManager::currentThread()->addWorker(sampleWorker);
    
    SGCore::Threading::ThreadsManager::currentThread()->processWorkers();
    
    SGCore::CoreMain::onInit.connect<&OceansEdge::GameMain::init>();
    SGCore::CoreMain::getRenderTimer().onUpdate.connect<&OceansEdge::GameMain::update>();
    SGCore::CoreMain::getFixedTimer().onUpdate.connect<&OceansEdge::GameMain::fixedUpdate>();
    
    SGCore::CoreMain::start();
    
    return 0;
}
