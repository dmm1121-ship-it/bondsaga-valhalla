#include "global.h"
#include "bondsaga_battle_bound.h"
#include "battle_util.h"
#include "battle_gimmick.h"
#include "event_data.h"
#include "random.h"
#include "string_util.h"
#include "test/battle.h"

// The scripted battle runner uses recorded/link flags, which bypass obedience.
// Exercise the real policy separately with live trainer flags and no badges.
DOUBLE_BATTLE_TEST("Battle-Bound obedience policy ignores level and ownership without consuming RNG")
{
    GIVEN {
        BsgBbBegin();
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_SMEARGLE);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_SMEARGLE);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_TACKLE, target: opponentLeft); }
    } THEN {
        u32 flags = gBattleTypeFlags;
        u32 disobeyed = 0;
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE;
        for (u32 badge = FLAG_BADGE01_GET; badge <= FLAG_BADGE08_GET; badge++)
            FlagClear(badge);
        for (u32 outsider = 0; outsider < 2; outsider++)
        {
            for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
            {
                gBattlerAttacker = battler;
                struct BattlePokemon original = gBattleMons[battler];
                memcpy(&gBattleMons[battler].otId, gSaveBlock2Ptr->playerTrainerId, sizeof(gBattleMons[battler].otId));
                gBattleMons[battler].otId ^= outsider;
                StringCopy(gBattleMons[battler].otName, gSaveBlock2Ptr->playerName);
                gBattleMons[battler].status1 = 0;
                for (u32 level = 20; level <= 100; level += 80)
                {
                    gBattleMons[battler].level = gBattleMons[battler].metLevel = level;
                    for (u32 seed = 0; seed < 256; seed++)
                    {
                        gCurrMovePos = gChosenMovePos = 0;
                        gCurrentMove = gCalledMove = gBattleMons[battler].moves[0];
                        // VBlank normally advances RNG independently of moves.
                        u16 ime = REG_IME;
                        REG_IME = 0;
                        SeedRng(seed);
                        rng_value_t before = gRngValue;
                        enum Obedience result = GetAttackerObedienceForAction();
                        rng_value_t after = gRngValue;
                        REG_IME = ime;
                        EXPECT_EQ(result, OBEYS);
                        EXPECT_EQ(memcmp(&before, &after, sizeof(before)), 0);
                        EXPECT_EQ(gCurrMovePos, 0);
                        EXPECT_EQ(gChosenMovePos, 0);
                        EXPECT_EQ(gCurrentMove, gBattleMons[battler].moves[0]);
                        EXPECT_EQ(gCalledMove, gCurrentMove);
                        EXPECT_EQ(gBattleMons[battler].status1, 0);
                    }
                }
                gBattleMons[battler] = original;
            }
        }
        // Negative control: these exact live flags must reach vanilla obedience
        // once the Bondsaga runtime ends. This prevents a vacuous link-mode test.
        BsgBbEnd();
        gBattlerAttacker = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        gBattleMons[gBattlerAttacker].level = gBattleMons[gBattlerAttacker].metLevel = 20;
        for (u32 seed = 0; seed < 256; seed++)
        {
            SeedRng(seed);
            gCurrMovePos = gChosenMovePos = 0;
            if (GetAttackerObedienceForAction() != OBEYS) disobeyed++;
        }
        EXPECT_GT(disobeyed, 0);
        gBattleTypeFlags = flags;
    }
}

DOUBLE_BATTLE_TEST("Battle-Bound both player roles execute exact selected moves over repeated turns")
{
    GIVEN {
        BsgBbBegin();
        PLAYER(SPECIES_WOBBUFFET) { Level(20); Speed(100); Attack(1); SpAttack(1); Moves(MOVE_TACKLE, MOVE_SWIFT, MOVE_PROTECT, MOVE_CELEBRATE); }
        PLAYER(SPECIES_SMEARGLE) { Level(20); Speed(90); Attack(1); SpAttack(1); Moves(MOVE_SLASH, MOVE_ICY_WIND, MOVE_PROTECT, MOVE_HELPING_HAND); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Speed(10); }
        OPPONENT(SPECIES_SMEARGLE) { MaxHP(1000); HP(1000); Speed(9); }
    } WHEN {
        for (u32 turn = 0; turn < 8; turn++)
            TURN {
                MOVE(playerLeft, turn % 2 ? MOVE_SWIFT : MOVE_TACKLE, target: opponentLeft);
                MOVE(playerRight, turn % 2 ? MOVE_ICY_WIND : MOVE_SLASH, target: opponentRight);
            }
    } SCENE {
        for (u32 turn = 0; turn < 8; turn++)
        {
            ANIMATION(ANIM_TYPE_MOVE, turn % 2 ? MOVE_SWIFT : MOVE_TACKLE, playerLeft);
            ANIMATION(ANIM_TYPE_MOVE, turn % 2 ? MOVE_ICY_WIND : MOVE_SLASH, playerRight);
        }
    } THEN {
        EXPECT_EQ(playerLeft->status1, 0);
        EXPECT_EQ(playerRight->status1, 0);
        EXPECT_EQ(playerLeft->pp[0], GetMovePP(MOVE_TACKLE) - 4);
        EXPECT_EQ(playerLeft->pp[1], GetMovePP(MOVE_SWIFT) - 4);
        EXPECT_EQ(playerRight->pp[0], GetMovePP(MOVE_SLASH) - 4);
        EXPECT_EQ(playerRight->pp[1], GetMovePP(MOVE_ICY_WIND) - 4);
    }
}

DOUBLE_BATTLE_TEST("Battle-Bound actual Spore still puts both player roles to Sleep")
{
    GIVEN {
        BsgBbBegin();
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_SMEARGLE) { Speed(90); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); }
        OPPONENT(SPECIES_SMEARGLE) { Speed(9); }
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_SPORE, target: playerLeft); MOVE(opponentRight, MOVE_SPORE, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPORE, opponentLeft);
        MESSAGE("Wobbuffet fell asleep!");
        STATUS_ICON(playerLeft, sleep: TRUE);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPORE, opponentRight);
        MESSAGE("Smeargle fell asleep!");
        STATUS_ICON(playerRight, sleep: TRUE);
    } THEN {
        EXPECT(playerLeft->status1 & STATUS1_SLEEP);
        EXPECT(playerRight->status1 & STATUS1_SLEEP);
    }
}

TEST("Battle-Bound designation validates canonical expedition references")
{
    struct BsgRoster roster;
    BsgBbEnd();
    BsgRosterInitialize(&roster);
    for (u32 i = 0; i < 3; i++) roster.placement[i] = roster.expedition[i] = i;
    EXPECT(BsgBbDesignate(&roster, 3, 0));
    EXPECT_EQ(roster.bound, 0);
    EXPECT_NE(roster.deployed[0], roster.bound);
    EXPECT(BsgBbDesignate(&roster, 3, 2));
    EXPECT_EQ(roster.bound, 2);
    EXPECT_EQ(BsgRosterValidate(&roster, 3), BSG_DATA_OK);
    EXPECT(!BsgBbDesignate(&roster, 3, 3));
    BsgBbBegin();
    bool32 changed = BsgBbDesignate(&roster, 3, 1);
    BsgBbEnd();
    EXPECT(!changed);
    EXPECT_EQ(roster.bound, 2);
    EXPECT_EQ(BSG_CREATURE_WIRE_SIZE, 112);
}

TEST("Battle-Bound derivation changes with partner without mutating the partner")
{
    struct Pokemon partner, original, form;
    struct BsgBbStats a, b;
    CreateMonWithIVs(&partner, SPECIES_COMBUSKEN, 20, 1, OTID_STRUCT_PLAYER_ID, 15);
    original = partner;
    BsgBbDeriveStats(&a, 20, &partner);
    BsgBbCreateForm(&form, &partner, FALSE);
    EXPECT_EQ(memcmp(&partner, &original, sizeof(partner)), 0);
    EXPECT_EQ(GetMonData(&form, MON_DATA_HP), a.hp);
    EXPECT_EQ(GetMonData(&form, MON_DATA_ATK), a.attack);
    EXPECT_EQ(GetMonData(&form, MON_DATA_MOVE1), MOVE_SLASH);
    EXPECT_EQ(GetMonData(&form, MON_DATA_MOVE2), MOVE_PROTECT);
    EXPECT_EQ(GetMonData(&form, MON_DATA_MOVE3), MOVE_HELPING_HAND);
    EXPECT_EQ(GetMonData(&form, MON_DATA_MOVE4), MOVE_ICY_WIND);
    CreateMonWithIVs(&partner, SPECIES_MARSHTOMP, 20, 2, OTID_STRUCT_PLAYER_ID, 15);
    BsgBbDeriveStats(&b, 20, &partner);
    EXPECT_NE(memcmp(&a, &b, sizeof(a)), 0);
}

DOUBLE_BATTLE_TEST("Battle-Bound has independent actions and independently targetable positions")
{
    GIVEN {
        BsgBbBegin();
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_SMEARGLE);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_SMEARGLE);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_TACKLE, target: opponentLeft); MOVE(playerRight, MOVE_SLASH, target: opponentRight); }
        TURN { MOVE(opponentLeft, MOVE_TACKLE, target: playerRight); MOVE(opponentRight, MOVE_SLASH, target: playerLeft); }
    } THEN {
        EXPECT_LT(playerLeft->hp, playerLeft->maxHP);
        EXPECT_LT(playerRight->hp, playerRight->maxHP);
        EXPECT_LT(opponentLeft->hp, opponentLeft->maxHP);
        EXPECT_LT(opponentRight->hp, opponentRight->maxHP);
        EXPECT(BsgBbIsTrainer(GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT)));
        EXPECT(BsgBbIsTrainer(GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT)));
        EXPECT(!BsgBbIsTrainer(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)));
        EXPECT_EQ(playerRight->ability, ABILITY_NONE);
        EXPECT(!CanBattlerSwitch(GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT)));
        for (enum BattlerId battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
            for (enum Gimmick gimmick = 0; gimmick < GIMMICKS_COUNT; gimmick++)
                EXPECT(!CanActivateGimmick(battler, gimmick));
    }
}

DOUBLE_BATTLE_TEST("Battle-Bound guard and ally support use the doubles action economy", s16 damage)
{
    bool32 support;
    PARAMETRIZE { support = FALSE; }
    PARAMETRIZE { support = TRUE; }
    GIVEN {
        BsgBbBegin();
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_SMEARGLE);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_SMEARGLE);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_PROTECT); MOVE(opponentLeft, MOVE_TACKLE, target: playerRight); }
        TURN {
            if (support) MOVE(playerRight, MOVE_HELPING_HAND, target: playerLeft);
            MOVE(playerLeft, MOVE_TACKLE, target: opponentLeft);
        }
    } SCENE {
        MESSAGE("Smeargle protected itself!");
        NOT HP_BAR(playerRight);
        HP_BAR(opponentLeft, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.5), results[1].damage);
    }
}

DOUBLE_BATTLE_TEST("Battle-Bound player Bond Break leaves reserves for the creature slot")
{
    GIVEN {
        BsgBbBegin();
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_SMEARGLE) { HP(1); }
        PLAYER(SPECIES_WYNAUT);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_SMEARGLE);
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_TACKLE, target: playerRight); }
        TURN { SWITCH(playerLeft, 2); }
        TURN { MOVE(playerLeft, MOVE_TACKLE, target: opponentLeft); }
    } SCENE {
        MESSAGE("Bond Break! Smeargle's Armament ends!");
        HP_BAR(opponentLeft);
    } THEN {
        EXPECT_EQ(BsgBbActive(), TRUE);
        EXPECT_EQ(BsgBbIsTrainer(GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT)), TRUE);
        EXPECT_EQ(BsgBbIsBroken(GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT)), TRUE);
        EXPECT(gAbsentBattlerFlags & (1u << GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT)));
        EXPECT_EQ(gBattlerPartyIndexes[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)], 2);
        // The test runner ends the specified turns with a synthetic escape.
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_PLAYER_TELEPORTED);
    }
}

DOUBLE_BATTLE_TEST("Battle-Bound enemy Bond Break continues without filling the Trainer position")
{
    GIVEN {
        BsgBbBegin();
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_SMEARGLE);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_SMEARGLE) { HP(1); }
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_TACKLE, target: opponentRight); }
        TURN { MOVE(opponentLeft, MOVE_TACKLE, target: playerLeft); }
    } SCENE {
        MESSAGE("Bond Break! The opposing Smeargle's Armament ends!");
        HP_BAR(playerLeft);
    } THEN {
        EXPECT_EQ(BsgBbActive(), TRUE);
        EXPECT_EQ(BsgBbIsTrainer(GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT)), TRUE);
        EXPECT_EQ(BsgBbIsBroken(GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT)), TRUE);
        EXPECT(gAbsentBattlerFlags & (1u << GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT)));
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_PLAYER_TELEPORTED);
    }
}

DOUBLE_BATTLE_TEST("Battle-Bound victory resolves after Bond Break and the final normal creature")
{
    GIVEN {
        BsgBbBegin();
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_SMEARGLE);
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); }
        OPPONENT(SPECIES_SMEARGLE) { HP(1); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_TACKLE, target: opponentRight); }
        TURN { MOVE(playerLeft, MOVE_TACKLE, target: opponentLeft); }
    } THEN {
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_WON);
    }
}

DOUBLE_BATTLE_TEST("Battle-Bound Trainer can continue after the last normal creature falls")
{
    GIVEN {
        BsgBbBegin();
        PLAYER(SPECIES_WOBBUFFET) { HP(1); }
        PLAYER(SPECIES_SMEARGLE);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_SMEARGLE);
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_TACKLE, target: playerLeft); }
        TURN { MOVE(playerRight, MOVE_SLASH, target: opponentRight); }
    } SCENE {
        MESSAGE("Wobbuffet fainted!");
        HP_BAR(opponentRight);
    } THEN {
        EXPECT(gAbsentBattlerFlags & (1u << GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)));
        EXPECT(!BsgBbIsBroken(GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT)));
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_PLAYER_TELEPORTED);
    }
}

DOUBLE_BATTLE_TEST("Battle-Bound defeat resolves only after the remaining normal creature falls")
{
    GIVEN {
        BsgBbBegin();
        PLAYER(SPECIES_WOBBUFFET) { HP(1); }
        PLAYER(SPECIES_SMEARGLE) { HP(1); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_SMEARGLE);
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_TACKLE, target: playerRight); }
        TURN { MOVE(opponentLeft, MOVE_TACKLE, target: playerLeft); }
    } THEN {
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_LOST);
    }
}
