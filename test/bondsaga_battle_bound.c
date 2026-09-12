#include "global.h"
#include "bondsaga_battle_bound.h"
#include "battle_util.h"
#include "battle_gimmick.h"
#include "test/battle.h"

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
