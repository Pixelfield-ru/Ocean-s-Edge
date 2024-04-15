//
// Created by ilya on 02.04.24.
//

#ifndef OCEANSEDGE_PHYSICALCHUNK_H
#define OCEANSEDGE_PHYSICALCHUNK_H

#include <vector>
#include <SGCore/Main/CoreGlobals.h>
#include <SGCore/Physics/Rigidbody3D.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

#include "Chunk.h"

namespace OceansEdge
{
    struct PhysicalChunk
    {
        SGCore::entity_t m_entity = entt::null;
        std::vector<float> m_colliderVertices;
        SGCore::Ref<SGCore::Rigidbody3D> m_rigidbody3D;
        SGCore::Ref<btTriangleMesh> m_physicalTriangleMesh;
        Chunk* m_chunk = nullptr;
        bool m_isColliderFormed = false;
    };
}

#endif //OCEANSEDGE_PHYSICALCHUNK_H
