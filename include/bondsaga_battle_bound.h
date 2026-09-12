#ifndef GUARD_BONDSAGA_BATTLE_BOUND_H
#define GUARD_BONDSAGA_BATTLE_BOUND_H

#include "global.h"
#include "pokemon.h"
#include "bondsaga_data.h"

/* Disposable prototype, not a new persistent species or a save schema. */
#define BSG_BB_RUNTIME_PARTY_SLOT 1
#define BSG_BB_PROTOTYPE_LEVEL 20

struct BsgBbStats
{
    u16 hp, attack, defense, speed, spAttack, spDefense;
};

void BsgBbDeriveStats(struct BsgBbStats *out, u16 trainerLevel, const struct Pokemon *partner);
bool32 BsgBbDesignate(struct BsgRoster *roster, u16 count, u16 candidate);
void BsgBbBegin(void);
void BsgBbEnd(void);
bool32 BsgBbActive(void);
bool32 BsgBbIsTrainer(enum BattlerId battler);
bool32 BsgBbIsBroken(enum BattlerId battler);
bool32 BsgBbBreak(enum BattlerId battler);
void BsgBbApplyRuntime(enum BattlerId battler);
bool32 BsgBbLoadGraphics(enum BattlerId battler);
void BsgBbCreateForm(struct Pokemon *runtime, const struct Pokemon *partner, bool32 enemy);
bool32 BsgPrototypeRunning(void);
void CB2_BsgPrototype(void);

#endif
