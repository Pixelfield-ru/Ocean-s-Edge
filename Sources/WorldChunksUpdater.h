//
// Created by Ilya on 29.03.2024.
//

#ifndef WORLDCHUNKSUPDATER_H
#define WORLDCHUNKSUPDATER_H

#include <SGCore/Scene/IParallelSystem.h>

namespace OceansEdge
{
    struct WorldChunksUpdater final : public SGCore::IParallelSystem<WorldChunksUpdater>
    {
        void parallelUpdate(const double& dt, const double& fixedDt) noexcept override;
    };
}

#endif //WORLDCHUNKSUPDATER_H
