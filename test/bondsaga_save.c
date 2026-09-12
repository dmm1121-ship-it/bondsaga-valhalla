#include "global.h"
#include "bondsaga_foundation.h"
#include "test/test.h"

TEST("Bondsaga CRC32C matches the standard check vector")
{
    static const uint8_t vector[] = "123456789";
    EXPECT_EQ(BsgCrc32c(vector, 9), 0xe3069283u);
}

TEST("Bondsaga wire retains sequel progression and combined exceptional states")
{
    struct BsgCreature source = {0};
    struct BsgCreature decoded = {0};
    uint8_t wire[BSG_CREATURE_WIRE_SIZE];
    source.serial = 0x12345678;
    source.totalExp = 0xfedcba98;
    source.earnedLevel = 200;
    source.acquisitionLevel = 150;
    source.exceptionalFlags = BSG_CREATURE_ALPHA | BSG_CREATURE_APEX | BSG_CREATURE_VARIANT;
    source.nicknameLength = 12;
    for (unsigned i = 0; i < 12; i++)
        source.nickname[i] = i + 1;
    EXPECT_EQ(BsgCreatureEncode(wire, sizeof(wire), &source, NULL), BSG_DATA_OK);
    EXPECT_EQ(wire[0], 0x78);
    EXPECT_EQ(wire[3], 0x12);
    EXPECT_EQ(BsgCreatureDecode(&decoded, wire, sizeof(wire), NULL), BSG_DATA_OK);
    EXPECT_EQ(decoded.serial, source.serial);
    EXPECT_EQ(decoded.totalExp, source.totalExp);
    EXPECT_EQ(decoded.earnedLevel, 200);
    EXPECT_EQ(decoded.acquisitionLevel, 150);
    EXPECT_EQ(decoded.exceptionalFlags, 7);
    EXPECT_EQ(decoded.nickname[11], 12);
}

TEST("Bondsaga admitted save keeps the complete future-system reserve")
{
    EXPECT_EQ(BSG_OWNED_CAPACITY, 384);
    EXPECT_EQ(BSG_CREATURE_WIRE_SIZE, 112);
    EXPECT_EQ(BSG_PLAYER_BINDER_WIRE_SIZE, 256);
    EXPECT_EQ(BSG_FOUNDATION_MAX_BODY_BYTES, 54928);
    EXPECT_EQ(BSG_FOUNDATION_RESERVE_BYTES, 8944);
}
