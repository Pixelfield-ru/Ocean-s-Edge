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
#include <SGCore/Render/PostProcess/StandardFX/SSAO.h>
#include <SGCore/Render/Atmosphere/AtmosphereUpdater.h>
#include <SGCore/Input/InputManager.h>

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
        m_world->load();
        
    }
    
    auto& cameraLayeredFrameReceiver = m_worldScene->getECSRegistry()->get<SGCore::LayeredFrameReceiver>(m_world->getPlayerEntity());
    
    for(const auto& e : cubeEntities)
    {
        std::cout << "has mesh: " << (m_worldScene->getECSRegistry()->try_get<SGCore::Mesh>(e) ? "true" : "false") << std::endl;
    }
    
    m_worldScene->getECSRegistry()->get<SGCore::Mesh>(cubeEntities[2]).m_base.m_meshData->m_material->addTexture2D(SGTextureType::SGTT_DIFFUSE, Resources::getBlocksAtlas());
    
    // ==============================================================================================================
    
    {
        auto chunksPPLayer = cameraLayeredFrameReceiver.addLayer("chunks_layer");
        
        chunksPPLayer->m_frameBuffer->bind();
        chunksPPLayer->m_frameBuffer->addAttachment(
                SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT2, // GBUFFER VERTICES POSITIONS ATTACHMENT
                SGGColorFormat::SGG_RGB,
                SGGColorInternalFormat::SGG_RGB32_FLOAT,
                0,
                0
        );
        chunksPPLayer->m_frameBuffer->addAttachment(
                SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT3, // GBUFFER VERTICES VIEW MODEL NORMALS ATTACHMENT
                SGGColorFormat::SGG_RGB,
                SGGColorInternalFormat::SGG_RGB16_FLOAT,
                0,
                0
        );
        chunksPPLayer->m_frameBuffer->addAttachment(
                SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT4, // SSAO ATTACHMENT
                SGGColorFormat::SGG_RGB,
                SGGColorInternalFormat::SGG_RGB8,
                0,
                0
        );
        chunksPPLayer->m_frameBuffer->addAttachment(
                SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT5, // SSAO BLUR
                SGGColorFormat::SGG_RGB,
                SGGColorInternalFormat::SGG_RGB8,
                0,
                0
        );
        chunksPPLayer->m_frameBuffer->unbind();
        
        SGCore::PostProcessFXSubPass subPass;
        subPass.m_attachmentRenderTo = SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT4;
        chunksPPLayer->m_subPasses.push_back(subPass);
        
        subPass.m_attachmentRenderTo = SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT5;
        subPass.m_enablePostFXDepthPass = true;
        chunksPPLayer->m_subPasses.push_back(subPass);
        
        SGCore::Ref<SGCore::SSAO> chunksSSAO = SGCore::MakeRef<SGCore::SSAO>();
        chunksPPLayer->addEffect<SGCore::SSAO>(chunksSSAO);
        
        chunksPPLayer->m_attachmentsToRenderIn.push_back(SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT2);
        chunksPPLayer->m_attachmentsToRenderIn.push_back(SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT3);
        chunksPPLayer->m_attachmentsToDepthTest.push_back(SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT2);
        chunksPPLayer->m_attachmentsToDepthTest.push_back(SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT3);
        
        chunksPPLayer->m_attachmentsForCombining[SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT0] = SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT5;
        
        SGCore::Ref<SGCore::IShader> ssaoShader = SGCore::MakeRef<SGCore::IShader>();
        ssaoShader->addSubPassShadersAndCompile(SGCore::AssetManager::loadAsset<SGCore::TextFileAsset>(
                "../OEResources/shaders/glsl4/ssao_layer.glsl"));
        chunksPPLayer->setFXSubPassShader(ssaoShader->getSubPassShader("SGLPPLayerFXPass"));
        chunksPPLayer->getFXSubPassShader()->addTextureBinding("chunksGBuf_VerticesPositions",
                                                               chunksPPLayer->m_frameBuffer->getAttachment(
                                                                       SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT2));
        chunksPPLayer->getFXSubPassShader()->addTextureBinding("chunksGBuf_ViewModelNormals",
                                                               chunksPPLayer->m_frameBuffer->getAttachment(
                                                                       SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT3));
        chunksPPLayer->getFXSubPassShader()->addTextureBinding("SG_SSAO_occlusionFormedTexture",
                                                               chunksPPLayer->m_frameBuffer->getAttachment(
                                                                       SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT4));
        chunksPPLayer->getFXSubPassShader()->addTextureBinding("chunksGBuf_Albedo",
                                                               chunksPPLayer->m_frameBuffer->getAttachment(
                                                                       SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT1));
    }
    // ==============================================================================================================
    // ==============================================================================================================
    {
        // создаём новый слой постпроцесса
        auto testPPLayer = cameraLayeredFrameReceiver.addLayer("test_pp_layer");
        
        SGCore::PostProcessFXSubPass subPass;
        
        testPPLayer->m_frameBuffer->bind();
        // добавляем color2 аттачмент для фреймбуфера слоя.
        // он будет использоваться для применения эффекта, на основе данных из аттачмента color1
        testPPLayer->m_frameBuffer->addAttachment(
                SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT2,
                SGGColorFormat::SGG_RGB,
                SGGColorInternalFormat::SGG_RGB8,
                0,
                0
        );
        testPPLayer->m_frameBuffer->unbind();
        
        // добавляем сабпасс, который будет рендериться во второй аттачмент
        subPass.m_attachmentRenderTo = SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT2;
        testPPLayer->m_subPasses.push_back(subPass);
        
        // указываем, что в аттачмент color0 комбинированного фреймбуфера LayeredFrameReceiver пойдёт аттачмент color2
        testPPLayer->m_attachmentsForCombining[SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT0] = SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT2;
        
        // загружаем шейдер с блумом
        SGCore::Ref<SGCore::IShader> bloomShader = SGCore::MakeRef<SGCore::IShader>();
        bloomShader->addSubPassShadersAndCompile(SGCore::AssetManager::loadAsset<SGCore::TextFileAsset>(
                "../OEResources/shaders/glsl4/bloom_layer.glsl"));
        testPPLayer->setFXSubPassShader(bloomShader->getSubPassShader("SGLPPLayerFXPass"));
        
        // добавляем текстурный биндинг, а именно аттачмент color1
        testPPLayer->getFXSubPassShader()->addTextureBinding("test_pp_layer_ColorAttachments[0]",
                                                          testPPLayer->m_frameBuffer->getAttachment(
                                                                  SGFrameBufferAttachmentType::SGG_COLOR_ATTACHMENT1));
        
        // добавляем меш кубика в новый слой
        m_worldScene->getECSRegistry()->get<SGCore::EntityBaseInfo>(cubeEntities[2]).m_postProcessLayers[&cameraLayeredFrameReceiver] = testPPLayer;
    }
    
    // аттачмент color0 - по дефолту - сырой аттачмент, не прошедший проверку на глубину (содержащий данные других слоёв)
    // аттачмент color1 - аттачмент color0, но прошедший проверку на глубину
    // аттачмент color2 - аттачмент с эффектом блума, основанного на данных из color1
    
    // нельзя устанавливать subPass.m_attachmentRenderTo в color1,
    // т.к. мы уже используем color1 как данные для эффекта. будет конфликт
    // ==============================================================================================================
    
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
    SGCore::CoreMain::getWindow().setTitle("Ocean`s Edge. FPS: " + std::to_string(SGCore::CoreMain::getFPS()));

    SGCore::Scene::getCurrentScene()->update(dt, fixedDt);

    m_world->render(m_worldScene);
    
    SGCore::LayeredFrameReceiver& playerCameraReceiver = m_worldScene->getECSRegistry()->get<SGCore::LayeredFrameReceiver>(m_world->getPlayerEntity());
    auto chunksPPLayer = playerCameraReceiver.getLayer("chunks_layer");
    chunksPPLayer->getFXSubPassShader()->bind();
    
    chunksPPLayer->getEffect<SGCore::SSAO>()->setEnabled(Settings::m_enableSSAO);
    
    if(SGCore::InputManager::getMainInputListener()->keyboardKeyReleased(SGCore::KeyboardKey::KEY_P))
    {
        Settings::m_enableSSAO = !Settings::m_enableSSAO;
    }
    
    if(SGCore::InputManager::getMainInputListener()->keyboardKeyDown(SGCore::KeyboardKey::KEY_LEFT_ALT) &&
       SGCore::InputManager::getMainInputListener()->keyboardKeyReleased(SGCore::KeyboardKey::KEY_Z))
    {
        std::lock_guard worldSaveLock(m_world->m_saveWorldMutex);
        
        m_world->save();
    }

    // SGCore::ImGuiWrap::ImGuiLayer::endFrame();
}

SGCore::Ref<OceansEdge::World> OceansEdge::GameMain::getCurrentWorld() noexcept
{
    return m_world;
}

SGCore::Ref<SGCore::Scene> OceansEdge::GameMain::getCurrentWorldScene() noexcept
{
    return m_worldScene;
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
