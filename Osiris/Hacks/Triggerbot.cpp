#include <Windows.h>

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/UserCmd.h"
#include "../SDK/WeaponData.h"
#include "../SDK/WeaponId.h"
#include "Triggerbot.h"

void Triggerbot::run(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive() || localPlayer->nextAttack() > memory->globalVars->serverTime())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip() || activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return;

    auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return;

    if (!config->triggerbot[weaponIndex].enabled)
        weaponIndex = getWeaponClass(activeWeapon->itemDefinitionIndex2());

    if (!config->triggerbot[weaponIndex].enabled)
        weaponIndex = 0;

    if (!config->triggerbot[weaponIndex].enabled)
        return;

    static auto lastTime = 0.0f;
    static auto lastContact = 0.0f;

    const auto now = memory->globalVars->realtime;

    if (now - lastContact < config->triggerbot[weaponIndex].burstTime) {
        cmd->buttons |= UserCmd::IN_ATTACK;
        return;
    }
    lastContact = 0.0f;

    if (config->triggerbot[weaponIndex].onKey && !GetAsyncKeyState(config->triggerbot[weaponIndex].key))
        return;

    if (now - lastTime < config->triggerbot[weaponIndex].shotDelay / 1000.0f)
        return;

    if (!config->triggerbot[weaponIndex].ignoreFlash && localPlayer->flashDuration() > 75.0f)
        return;

    if (config->triggerbot[weaponIndex].scopedOnly && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
        return;

    const auto weaponData = activeWeapon->getWeaponData();
    if (!weaponData)
        return;

    const auto aimPunch = localPlayer->getAimPunch();

    const Vector viewAngles{ std::cos(degreesToRadians(cmd->viewangles.x + aimPunch.x)) * std::cos(degreesToRadians(cmd->viewangles.y + aimPunch.y)) * weaponData->range,
                             std::cos(degreesToRadians(cmd->viewangles.x + aimPunch.x)) * std::sin(degreesToRadians(cmd->viewangles.y + aimPunch.y)) * weaponData->range,
                            -std::sin(degreesToRadians(cmd->viewangles.x + aimPunch.x)) * weaponData->range };

    const auto startPos = localPlayer->getEyePosition();
    const auto endPos = startPos + viewAngles;

    Trace trace;
    interfaces->engineTrace->traceRay({ startPos, endPos }, 0x46004009, localPlayer.get(), trace);

    lastTime = now;

    if (!trace.entity || !trace.entity->isPlayer())
        return;

    if (!config->triggerbot[weaponIndex].friendlyFire && !localPlayer->isOtherEnemy(trace.entity))
        return;

    if (trace.entity->gunGameImmunity())
        return;

    if (config->triggerbot[weaponIndex].hitgroup && trace.hitgroup != config->triggerbot[weaponIndex].hitgroup)
        return;

    if (!config->triggerbot[weaponIndex].ignoreSmoke && memory->lineGoesThroughSmoke(startPos, endPos, 1))
        return;

    float damage = (activeWeapon->itemDefinitionIndex2() != WeaponId::Taser ? HitGroup::getDamageMultiplier(trace.hitgroup) : 1.0f) * weaponData->damage * std::pow(weaponData->rangeModifier, trace.fraction * weaponData->range / 500.0f);

    if (float armorRatio{ weaponData->armorRatio / 2.0f }; activeWeapon->itemDefinitionIndex2() != WeaponId::Taser && HitGroup::isArmored(trace.hitgroup, trace.entity->hasHelmet()))
        damage -= (trace.entity->armor() < damage * armorRatio / 2.0f ? trace.entity->armor() * 4.0f : damage) * (1.0f - armorRatio);

    if (damage >= (config->triggerbot[weaponIndex].killshot ? trace.entity->health() : config->triggerbot[weaponIndex].minDamage)) {
        cmd->buttons |= UserCmd::IN_ATTACK;
        lastTime = 0.0f;
        lastContact = now;
    }
}
