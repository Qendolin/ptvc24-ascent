#pragma once

#include "Objects/BarrierObstacle.h"
#include "Objects/BoostRing.h"
#include "Objects/Checkpoint.h"
#include "Objects/Crate.h"
#include "Objects/GoalFirework.h"
#include "Objects/MillObstacle.h"

namespace scene {
void registerEntityTypes(scene::NodeEntityFactory &factory) {
    factory.registerEntity<CheckpointEntity>("CheckpointEntity");
    factory.registerEntity<BarrierObstacleEntity>("BarrierObstacleEntity");
    factory.registerEntity<BarrierObstacleEntity>("TestObstacleEntity");  // Deprecated
    factory.registerEntity<MillObstacleEntity>("MillObstacleEntity");
    factory.registerEntity<CrateEntity>("CrateEntity");
    factory.registerEntity<BoostRingEntity>("BoostRingEntity");
    factory.registerEntity<GoalFireworkEntity>("GoalFireworkEntity");
}
}  // namespace scene