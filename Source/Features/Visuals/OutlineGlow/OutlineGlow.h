#pragma once

#include <utility>
#include "OutlineGlowContext.h"

template <typename HookContext, typename Context = OutlineGlowContext<HookContext>>
class OutlineGlow {
public:
    template <typename... Args>
    OutlineGlow(Args&&... args) noexcept
        : context{std::forward<Args>(args)...}
    {
    }

    void applyGlowToEntity(EntityTypeInfo entityTypeInfo, auto&& modelEntity) noexcept
    {
        if (!context.state().masterSwitch)
            return;

        if (modelEntity.glowProperty().isGlowing().valueOr(false))
            return;

        if (entityTypeInfo.is<cs2::C_CSPlayerPawn>())
            context.applyGlowToPlayer(modelEntity.template as<PlayerPawn>());
        else if (entityTypeInfo.is<cs2::CBaseAnimGraph>())
            context.applyGlowToDefuseKit(modelEntity.baseEntity());
        else if (entityTypeInfo.is<cs2::CPlantedC4>())
            context.applyGlowToPlantedBomb(modelEntity.template as<PlantedC4>());
        else if (entityTypeInfo.is<cs2::C_C4>())
            context.applyGlowToBomb(modelEntity.baseEntity());
        else if (entityTypeInfo.is<cs2::C_Hostage>())
            context.applyGlowToHostage(modelEntity.baseEntity());
        else if (entityTypeInfo.isGrenadeProjectile())
            context.applyGlowToGrenadeProjectile(entityTypeInfo, modelEntity.baseEntity());
        else if (entityTypeInfo.isWeapon())
            context.applyGlowToWeapon(entityTypeInfo, modelEntity.baseEntity());
    }

private:
    Context context;
};
