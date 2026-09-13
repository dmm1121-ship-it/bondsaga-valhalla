#include "global.h"
#include "bondsaga_battle_bound.h"
#include "battle.h"
#include "battle_util.h"
#include "data.h"
#include "decompress.h"
#include "palette.h"
#include "string_util.h"
#include "constants/abilities.h"
#include "constants/moves.h"
#include "constants/trainers.h"

static EWRAM_DATA bool8 sActive = FALSE;
static EWRAM_DATA u8 sBroken = 0;

/* Replaceable 0.1 formula: Trainer level plus half the partner's normal
 * calculated stat. No canonical growth/trait/equipment meaning is assigned. */
void BsgBbDeriveStats(struct BsgBbStats *out, u16 trainerLevel, const struct Pokemon *partner)
{
    out->hp = 30 + trainerLevel + partner->maxHP / 2;
    out->attack = trainerLevel + partner->attack / 2;
    out->defense = trainerLevel + partner->defense / 2;
    out->speed = trainerLevel + partner->speed / 2;
    out->spAttack = trainerLevel + partner->spAttack / 2;
    out->spDefense = trainerLevel + partner->spDefense / 2;
}

bool32 BsgBbDesignate(struct BsgRoster *roster, u16 count, u16 candidate)
{
    u32 i;
    if (sActive || BsgRosterValidate(roster, count) != BSG_DATA_OK || candidate >= count)
        return FALSE;
    for (i = 0; i < BSG_EXPEDITION_CAPACITY; i++)
        if (roster->expedition[i] == candidate)
            break;
    if (i == BSG_EXPEDITION_CAPACITY)
        return FALSE;
    roster->bound = candidate;
    for (i = 0; i < BSG_DEPLOYMENT_REFERENCE_CAPACITY; i++)
        roster->deployed[i] = BSG_RECORD_NONE;
    /* The disposable roster has no authored deployment preferences. */
    for (i = 0; i < BSG_EXPEDITION_CAPACITY; i++)
        if (roster->expedition[i] != candidate && roster->expedition[i] != BSG_RECORD_NONE)
        {
            roster->deployed[0] = roster->expedition[i];
            break;
        }
    return TRUE;
}

void BsgBbBegin(void) { sActive = TRUE; sBroken = 0; }
void BsgBbEnd(void) { sActive = FALSE; sBroken = 0; }
bool32 BsgBbActive(void) { return sActive; }

bool32 BsgBbIsTrainer(enum BattlerId battler)
{
    /* Roles belong to battlefield positions, never to species identity. */
    return sActive && battler < MAX_BATTLERS_COUNT
        && (GetBattlerPosition(battler) == B_POSITION_PLAYER_RIGHT
         || GetBattlerPosition(battler) == B_POSITION_OPPONENT_RIGHT);
}

bool32 BsgBbIsBroken(enum BattlerId battler)
{
    return BsgBbIsTrainer(battler) && (sBroken & (1u << battler));
}

bool32 BsgBbBreak(enum BattlerId battler)
{
    if (!BsgBbIsTrainer(battler) || gBattleMons[battler].hp != 0)
        return FALSE;
    sBroken |= 1u << battler; // Exhaustion lives only for this battle.
    gHitMarker |= HITMARKER_FAINTED(battler);
    gBattlerFainted = battler;
    gBattleStruct->eventState.faintedAction = 0;
    return TRUE;
}

void BsgBbCreateForm(struct Pokemon *runtime, const struct Pokemon *partner, bool32 enemy)
{
    struct BsgBbStats stats;
    /* Species supplies only a safe engine compatibility shell. Trainer role,
     * stats, graphics and techniques are explicit, and this is never saved. */
    CreateMonWithIVs(runtime, SPECIES_SMEARGLE, BSG_BB_PROTOTYPE_LEVEL, 0, OTID_STRUCT_PLAYER_ID, 0);
    BsgBbDeriveStats(&stats, BSG_BB_PROTOTYPE_LEVEL, partner);
    SetMonData(runtime, MON_DATA_NICKNAME, enemy ? COMPOUND_STRING("RIVAL-ARM") : COMPOUND_STRING("TRAINER"));
    SetMonData(runtime, MON_DATA_MAX_HP, &stats.hp);
    SetMonData(runtime, MON_DATA_HP, &stats.hp);
    SetMonData(runtime, MON_DATA_ATK, &stats.attack);
    SetMonData(runtime, MON_DATA_DEF, &stats.defense);
    SetMonData(runtime, MON_DATA_SPEED, &stats.speed);
    SetMonData(runtime, MON_DATA_SPATK, &stats.spAttack);
    SetMonData(runtime, MON_DATA_SPDEF, &stats.spDefense);
    /* Existing move effects are deliberately disposable technique stand-ins:
     * offense, guard, ally support, and speed control. No ordinary partner
     * move is copied into the Armament set. */
    SetMonMoveSlot(runtime, MOVE_SLASH, 0);
    SetMonMoveSlot(runtime, MOVE_PROTECT, 1);
    SetMonMoveSlot(runtime, MOVE_HELPING_HAND, 2);
    SetMonMoveSlot(runtime, MOVE_ICY_WIND, 3);
}

void BsgBbApplyRuntime(enum BattlerId battler)
{
    if (BsgBbIsTrainer(battler))
    {
        gBattleMons[battler].ability = ABILITY_NONE;
        gBattleMons[battler].types[0] = TYPE_NORMAL;
        gBattleMons[battler].types[1] = TYPE_NORMAL;
        gBattleMons[battler].types[2] = TYPE_MYSTERY;
    }
}

bool32 BsgBbLoadGraphics(enum BattlerId battler)
{
    u8 *gfx;
    u32 i;
    const u16 *pal;
    if (!BsgBbIsTrainer(battler))
        return FALSE;
    gfx = gMonSpritesGfxPtr->spritesGfx[GetBattlerPosition(battler)];
    /* A human Trainer silhouette plus a plain bar-shaped placeholder Armament.
     * Repeat the frame so inherited creature animation cannot reveal a mon. */
    DecompressDataWithHeaderWram(GetTrainerFrontPicData(TRAINER_PIC_YOUNGSTER), gfx);
    for (u32 y = 24; y < 58; y++)
        for (u32 x = 51; x < 55; x++)
        {
            u32 pos = (y / 8 * 8 + x / 8) * 32 + y % 8 * 4 + x % 8 / 2;
            gfx[pos] = (gfx[pos] & (x & 1 ? 0x0f : 0xf0)) | (x & 1 ? 0x10 : 0x01);
        }
    for (i = 1; i < MAX_MON_PIC_FRAMES; i++)
        memcpy(gfx + i * 0x800, gfx, 0x800);
    pal = GetTrainerFrontPicPalette(TRAINER_PIC_YOUNGSTER);
    LoadPalette(pal, OBJ_PLTT_ID(battler), PLTT_SIZE_4BPP);
    LoadPalette(pal, BG_PLTT_ID(8 + battler), PLTT_SIZE_4BPP);
    return TRUE;
}
