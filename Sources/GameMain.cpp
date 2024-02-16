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

#include "GameMain.h"
#include "Systems/DayNightCycleSystem.h"

void OceansEdge::GameMain::init()
{
    // todo:
    // SGCore::CoreMain::getWindow().setTitle("Ocean`s Edge");
    
    // CREATING WORLD SCENE --------------------------------------
    
    m_worldScene = SGCore::MakeRef<SGCore::Scene>();
    m_worldScene->createDefaultSystems();
    SGCore::Scene::setCurrentScene(m_worldScene);
    
    auto dayNightCycleSystem = SGCore::MakeRef<DayNightCycleSystem>();
    
    m_worldScene->getAllSystems().insert(dayNightCycleSystem);
    
    // -----------------------------------------------------------
    
    // LOADING MODELS --------------------------------------------
    
    auto cubeModel = SGCore::AssetManager::loadAsset<SGCore::ModelAsset>(
            "../SGResources/models/standard/cube.obj"
    );
    
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
    
    {
        std::vector<entt::entity> skyboxEntities;
        cubeModel->m_nodes[0]->addOnScene(m_worldScene, SG_LAYER_OPAQUE_NAME, [&skyboxEntities](const auto& entity) {
            skyboxEntities.push_back(entity);
        });
        
        SGCore::Mesh& skyboxMesh = m_worldScene->getECSRegistry().get<SGCore::Mesh>(skyboxEntities[2]);
        SGCore::Atmosphere& atmosphereScattering = m_worldScene->getECSRegistry().emplace<SGCore::Atmosphere>(skyboxEntities[2]);
        // atmosphereScattering.m_sunRotation.z = 90.0;
        /*skyboxMesh.m_base.m_meshData->m_material->addTexture2D(SGTextureType::SGTT_SKYBOX,
                                                               standardCubemap
        );*/
        // это топ пж
        skyboxMesh.m_base.m_meshData->m_material->getShader()->removeSubPass("GeometryPass");
        SGCore::MeshesUtils::loadMeshShader(skyboxMesh.m_base, "SkyboxShader");
        skyboxMesh.m_base.m_meshDataRenderInfo.m_enableFacesCulling = false;
        
        SGCore::Transform& skyboxTransform = m_worldScene->getECSRegistry().get<SGCore::Transform>(skyboxEntities[2]);
        // auto transformComponent = skyboxEntities[2]->getComponent<SGCore::Transform>();
        
        skyboxTransform.m_ownTransform.m_scale = { 1150, 1150, 1150 };
    }
    
    // -----------------------------------------------------------
}

void OceansEdge::GameMain::fixedUpdate()
{
    SGCore::Scene::getCurrentScene()->fixedUpdate();
}

void OceansEdge::GameMain::update()
{
    SGCore::Scene::getCurrentScene()->update();
}

int main()
{
    sgSetCoreInitCallback(&OceansEdge::GameMain::init);
    sgSetFixedUpdateCallback(&OceansEdge::GameMain::fixedUpdate);
    sgSetUpdateCallback(&OceansEdge::GameMain::update);
    
    SGCore::CoreMain::start();
    
    return 0;
}
