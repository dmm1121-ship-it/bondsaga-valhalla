/* Host-only persistence tests. No test fixtures or fake flash are linked into ROM. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bondsaga_save.h"
#include "bondsaga_data.h"
#include "bondsaga_foundation.h"

static unsigned sAssertions;
static unsigned sTests;

#define CHECK(expression) do { \
    ++sAssertions; \
    if (!(expression)) { \
        fprintf(stderr, "%s:%u: %s\n", __FILE__, (unsigned)__LINE__, #expression); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#define RUN(test) do { test(); ++sTests; printf("PASS %s\n", #test); } while (0)

/* Model FLASH1M: writes can only clear bits; erasing is the only way to set them.
 * Fault positions count individual programmed bytes, including bytes in a
 * multi-byte callback. This intentionally makes a commit record non-atomic. */
struct FakeFlash
{
    uint8_t bytes[131072];
    int64_t failProgramAfter;
    int32_t failEraseAt;
    uint32_t tornEraseBytes;
    uint32_t programmed;
    uint32_t erased;
    uint32_t readCalls;
    int failRead;
    int silentProgram;
    int silentErase;
};

static struct FakeFlash sFlash;
static uint8_t sSavedFlash[131072];
static uint8_t sPreviousBank[65536];

static void ResetFaults(void)
{
    sFlash.failProgramAfter = -1;
    sFlash.failEraseAt = -1;
    sFlash.tornEraseBytes = 0;
    sFlash.programmed = 0;
    sFlash.erased = 0;
    sFlash.readCalls = 0;
    sFlash.failRead = 0;
    sFlash.silentProgram = 0;
    sFlash.silentErase = 0;
}

static void ResetFlash(void)
{
    memset(sFlash.bytes, 0xFF, sizeof(sFlash.bytes));
    ResetFaults();
}

static bool FlashRead(void *context, uint32_t offset, uint8_t *out, uint32_t size)
{
    struct FakeFlash *flash = context;
    CHECK(offset <= sizeof(flash->bytes));
    CHECK(size <= sizeof(flash->bytes) - offset);
    ++flash->readCalls;
    if (flash->failRead)
        return 0;
    memcpy(out, flash->bytes + offset, size);
    return 1;
}

static bool FlashErase(void *context, uint16_t sector)
{
    struct FakeFlash *flash = context;
    uint32_t index = flash->erased++;
    CHECK(sector < 32);
    if (flash->failEraseAt >= 0 && index == (uint32_t)flash->failEraseAt)
    {
        CHECK(flash->tornEraseBytes <= 4096);
        memset(flash->bytes + sector * 4096u, 0xFF, flash->tornEraseBytes);
        return flash->silentErase;
    }
    memset(flash->bytes + sector * 4096u, 0xFF, 4096);
    return 1;
}

static bool FlashProgram(void *context, uint32_t offset, const uint8_t *data, uint32_t size)
{
    struct FakeFlash *flash = context;
    uint32_t i;
    CHECK(offset <= sizeof(flash->bytes));
    CHECK(size <= sizeof(flash->bytes) - offset);
    for (i = 0; i < size; ++i)
    {
        if (flash->failProgramAfter >= 0 && flash->programmed == (uint32_t)flash->failProgramAfter)
            return flash->silentProgram;
        flash->bytes[offset + i] &= data[i];
        ++flash->programmed;
    }
    return 1;
}

static void Put16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

static void Put32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
    out[2] = (uint8_t)(value >> 16);
    out[3] = (uint8_t)(value >> 24);
}

static uint32_t LogicalOffset(uint8_t slot, uint32_t offset)
{
    return slot * 65536u + (offset / 4032u) * 4096u + 32u + offset % 4032u;
}

static void ReadLogical(uint8_t slot, uint32_t offset, uint8_t *out, uint32_t size)
{
    uint32_t i;
    for (i = 0; i < size; ++i)
        out[i] = sFlash.bytes[LogicalOffset(slot, offset + i)];
}

static void WriteLogical(uint8_t slot, uint32_t offset, const uint8_t *data, uint32_t size)
{
    uint32_t i;
    for (i = 0; i < size; ++i)
        sFlash.bytes[LogicalOffset(slot, offset + i)] = data[i];
}

/* Hand-authored little-endian golden record, independent of the encoder. */
static const uint8_t sCreatureGolden[112] =
{
    0x78, 0x56, 0x34, 0x12, 0x34, 0x12, 0x45, 0x23,
    0x01, 0x00, 0x56, 0x34, 0x98, 0xBA, 0xDC, 0xFE,
    0xC8, 0x00, 0x65, 0x00, 0x00, 0x00, 0x5A, 0xA5,
    0x67, 0x45, 0x78, 0x56, 0x07, 0x0C, 0x89, 0x67,
    0x44, 0x33, 0x22, 0x11, 0x88, 0x77, 0x66, 0x55,
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4A, 0x4B, 0x4C, 0x23, 0x01, 0x45, 0x23,
    0x67, 0x45, 0x89, 0x67, 0xFF, 0x01, 0x7F, 0x00,
    0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xFF, 0xFF,
    0x34, 0x12, 0x78, 0x56, 0xBC, 0x9A, 0xF0, 0xDE,
    0x21, 0x43, 0x01, 0x10, 0x02, 0x20, 0x03, 0x30,
    0x04, 0x40, 0x55, 0xAA, 0x01, 0x23, 0x45, 0x67,
    0x89, 0xAB, 0xCD, 0xEF, 0xEF, 0xBE, 0xAD, 0xDE,
    0x04, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t sPlayerGolden[256] =
{
    [0] = 0x44, 0x33, 0x22, 0x11,
    [4] = 'S', 'A', 'G', 'A',
    [20] = 4, 2, 0x34, 0x12,
    [24] = 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    [40] = 4, 3, 2, 1, 0x21, 0x43, 0x65, 0x87,
    [48] = 0xFF, 0xFF, 0xFF, 0xFF, 0x78, 0x56, 0x34, 0x12,
    [56] = 0xEF, 0xCD, 0xAB, 0x90, 0x89, 0x67, 0x45, 0x23,
    [64] = 0xEF, 0xBE, 0xAD, 0xDE, 0xC8, 0,
    [80] = 1, [95] = 0x80, [111] = 0xFF,
    [112] = 0xAA, [119] = 0x55,
    [120] = 0x78, 0x56, 0x34, 0x12,
    [148] = 0xEF, 0xCD, 0xAB, 0x90,
    [180] = 0xFF, 0xFF, 0xFF, 0xFF,
    [184] = 0xA5, [191] = 0x5A,
    [192] = 0, 0, 0x5A, 0xA5, 0x89, 0x67,
    [198] = 0x23, 0x01, 0x45, 0x23, 0x67, 0x45, 0x89, 0x67,
    [208] = 0x44, 0x33, 0x22, 0x11,
};

static struct BsgCreatureStore sStore;
static struct BsgCreatureStore sDecodedStore;
static uint8_t sStoreWire[BSG_STORE_MAX_WIRE_SIZE];

static struct BsgCreature GoldenCreature(void)
{
    struct BsgCreature creature;
    CHECK(BsgCreatureDecode(&creature, sCreatureGolden, sizeof(sCreatureGolden), NULL) == BSG_DATA_OK);
    return creature;
}

static void TestCreatureGolden(void)
{
    static const uint16_t expectedMoves[4] = {0x0123, 0x2345, 0x4567, 0x6789};
    static const uint16_t expectedDevelopment[6] = {0xFFFF, 0x1234, 0x5678, 0x9ABC, 0xDEF0, 0x4321};
    static const uint16_t expectedTraits[4] = {0x1001, 0x2002, 0x3003, 0x4004};
    struct BsgCreature creature = GoldenCreature();
    struct BsgCreature sentinel;
    uint8_t encoded[112];
    uint8_t malformed[112];
    unsigned flags;
    CHECK(BSG_CREATURE_WIRE_SIZE == 112);
    CHECK(creature.serial == UINT32_C(0x12345678));
    CHECK(creature.species == 0x1234 && creature.lineage == 0x2345);
    CHECK(creature.origin == 1 && creature.acquisitionLocation == 0x3456);
    CHECK(creature.totalExp == UINT32_C(0xFEDCBA98));
    CHECK(creature.earnedLevel == 200 && creature.acquisitionLevel == 101);
    CHECK(creature.currentHp == 0 && creature.condition == 0xA55A);
    CHECK(creature.appearance == 0x4567 && creature.personality == 0x5678);
    CHECK(creature.nicknameLength == 12 && memcmp(creature.nickname, "ABCDEFGHIJKL", 12) == 0);
    CHECK(creature.recoveryProfile == 0x6789 && creature.recoveryProgress == UINT32_C(0x11223344));
    CHECK(creature.bondProgress == UINT32_C(0x55667788));
    CHECK(creature.moves[0] == 0x123 && creature.moves[3] == 0x6789);
    CHECK(creature.moveResources[0] == 255 && creature.moveResources[3] == 0);
    CHECK(creature.aptitudes[0] == 1 && creature.aptitudes[5] == 255);
    CHECK(creature.development[0] == 65535 && creature.development[5] == 0x4321);
    CHECK(creature.traits[0] == 0x1001 && creature.traits[3] == 0x4004);
    CHECK(creature.lineageMilestones == 0xAA55 && creature.awakenedOutcomes[7] == 0xEF);
    CHECK(creature.history[0] == UINT32_C(0xDEADBEEF) && creature.history[1] == UINT32_C(0x01020304));
    for (flags = 0; flags < BSG_MOVE_SLOTS; ++flags)
    {
        CHECK(creature.moves[flags] == expectedMoves[flags]);
        CHECK(creature.moveResources[flags] == sCreatureGolden[60 + flags]);
        CHECK(creature.traits[flags] == expectedTraits[flags]);
    }
    for (flags = 0; flags < BSG_APTITUDE_SLOTS; ++flags)
    {
        CHECK(creature.aptitudes[flags] == sCreatureGolden[64 + flags]);
        CHECK(creature.development[flags] == expectedDevelopment[flags]);
    }
    CHECK(memcmp(creature.awakenedOutcomes, sCreatureGolden + 92, 8) == 0);
    CHECK(BsgCreatureEncode(encoded, sizeof(encoded), &creature, NULL) == BSG_DATA_OK);
    CHECK(memcmp(encoded, sCreatureGolden, sizeof(encoded)) == 0);
    for (flags = 0; flags <= BSG_CREATURE_EXCEPTIONAL_MASK; ++flags)
    {
        creature.exceptionalFlags = (uint8_t)flags;
        CHECK(BsgCreatureEncode(encoded, sizeof(encoded), &creature, NULL) == BSG_DATA_OK);
        CHECK(encoded[28] == flags && encoded[29] == 12);
        CHECK(BsgCreatureDecode(&creature, encoded, sizeof(encoded), NULL) == BSG_DATA_OK);
        CHECK(creature.exceptionalFlags == flags && creature.appearance == 0x4567);
    }
    creature.earnedLevel = UINT16_MAX;
    creature.acquisitionLevel = UINT16_MAX;
    creature.totalExp = UINT32_MAX;
    CHECK(BsgCreatureEncode(encoded, sizeof(encoded), &creature, NULL) == BSG_DATA_OK);
    CHECK(BsgCreatureDecode(&creature, encoded, sizeof(encoded), NULL) == BSG_DATA_OK);
    CHECK(creature.earnedLevel == UINT16_MAX && creature.acquisitionLevel == UINT16_MAX && creature.totalExp == UINT32_MAX);
    memset(&sentinel, 0xA5, sizeof(sentinel));
    creature = sentinel;
    memcpy(malformed, sCreatureGolden, sizeof(malformed));
    malformed[108] = 1;
    CHECK(BsgCreatureDecode(&creature, malformed, sizeof(malformed), NULL) == BSG_DATA_RESERVED);
    CHECK(memcmp(&creature, &sentinel, sizeof(creature)) == 0);
    CHECK(BsgCreatureValidateWire(sCreatureGolden, 111, NULL) == BSG_DATA_SIZE);
    malformed[108] = 0;
    malformed[29] = 13;
    CHECK(BsgCreatureValidateWire(malformed, sizeof(malformed), NULL) != BSG_DATA_OK);
    malformed[29] = 12;
    malformed[28] = 0x80;
    CHECK(BsgCreatureValidateWire(malformed, sizeof(malformed), NULL) == BSG_DATA_RESERVED);
    memcpy(malformed, sCreatureGolden, sizeof(malformed));
    memset(malformed, 0, 4);
    CHECK(BsgCreatureValidateWire(malformed, sizeof(malformed), NULL) == BSG_DATA_SERIAL);
    memcpy(malformed, sCreatureGolden, sizeof(malformed));
    malformed[29] = 11; /* A nonzero unused name tail must not be silently dropped. */
    CHECK(BsgCreatureValidateWire(malformed, sizeof(malformed), NULL) == BSG_DATA_NAME);
}

static void TestPlayerGolden(void)
{
    struct BsgPlayerBinder player;
    struct BsgPlayerBinder unchanged;
    uint8_t encoded[256];
    uint8_t malformed[256];
    CHECK(BSG_PLAYER_BINDER_WIRE_SIZE == 256);
    CHECK(BsgPlayerBinderDecode(&player, sPlayerGolden, sizeof(sPlayerGolden), NULL) == BSG_DATA_OK);
    CHECK(player.playerSerial == UINT32_C(0x11223344));
    CHECK(player.nameLength == 4 && memcmp(player.name, "SAGA", 4) == 0);
    CHECK(player.language == 2 && player.avatar == 0x1234);
    CHECK(player.customization[0] == 0x80 && player.customization[15] == 0x8F);
    CHECK(player.options == UINT32_C(0x01020304) && player.difficulty == 0x4321 && player.accessibility == 0x8765);
    CHECK(player.playtime == UINT32_MAX && player.currency == UINT32_C(0x12345678));
    CHECK(player.rngContext == UINT32_C(0x90ABCDEF) && player.nextSpecimenSerial == UINT32_C(0x23456789));
    CHECK(player.binderProgress == UINT32_C(0xDEADBEEF) && player.binderRank == 200);
    CHECK(player.techniqueMembership[0] == 1 && player.techniqueMembership[31] == 255);
    CHECK(player.capabilityMembership[0] == 0xAA && player.capabilityMembership[7] == 0x55);
    CHECK(player.archetypeMastery[0] == UINT32_C(0x12345678));
    CHECK(player.archetypeMastery[7] == UINT32_C(0x90ABCDEF) && player.archetypeMastery[15] == UINT32_MAX);
    CHECK(player.manifestationMilestones[0] == 0xA5 && player.manifestationMilestones[7] == 0x5A);
    CHECK(player.currentHp == 0 && player.condition == 0xA55A && player.recoveryProfile == 0x6789);
    CHECK(player.selectedTechniques[0] == 0x123 && player.selectedTechniques[3] == 0x6789);
    CHECK(player.recoveryProgress == UINT32_C(0x11223344));
    CHECK(BsgPlayerBinderEncode(encoded, sizeof(encoded), &player, NULL) == BSG_DATA_OK);
    CHECK(memcmp(encoded, sPlayerGolden, sizeof(encoded)) == 0);
    unchanged = player;
    memcpy(malformed, sPlayerGolden, sizeof(malformed));
    malformed[70] = 1;
    CHECK(BsgPlayerBinderDecode(&player, malformed, sizeof(malformed), NULL) == BSG_DATA_RESERVED);
    CHECK(memcmp(&player, &unchanged, sizeof(player)) == 0);
    malformed[70] = 0;
    malformed[255] = 1;
    CHECK(BsgPlayerBinderDecode(&player, malformed, sizeof(malformed), NULL) == BSG_DATA_RESERVED);
    CHECK(BsgPlayerBinderDecode(&player, sPlayerGolden, 255, NULL) == BSG_DATA_SIZE);
}

static bool RejectSpecies(void *context, enum BsgRegistryKind kind, uint16_t id)
{
    unsigned *calls = context;
    ++*calls;
    return kind != BSG_REGISTRY_SPECIES || id != 0x1234;
}

static void TestRegistryRejection(void)
{
    unsigned calls = 0;
    struct BsgRegistry registry = {RejectSpecies, &calls};
    struct BsgCreature creature = GoldenCreature();
    uint8_t encoded[112];
    uint8_t unchanged[112];
    memset(encoded, 0xA5, sizeof(encoded));
    memcpy(unchanged, encoded, sizeof(encoded));
    CHECK(BsgCreatureEncode(encoded, sizeof(encoded), &creature, &registry) == BSG_DATA_REGISTRY);
    CHECK(calls > 0 && memcmp(encoded, unchanged, sizeof(encoded)) == 0);
    CHECK(BsgCreatureValidateWire(sCreatureGolden, sizeof(sCreatureGolden), &registry) == BSG_DATA_REGISTRY);
}

static void PopulateStore(unsigned count)
{
    struct BsgCreature creature = GoldenCreature();
    unsigned i;
    CHECK(count <= BSG_OWNED_CAPACITY);
    BsgCreatureStoreInitialize(&sStore);
    for (i = 0; i < count; ++i)
    {
        creature.serial = i + 1;
        creature.exceptionalFlags = (uint8_t)(i & 7u);
        CHECK(BsgCreatureEncode(sStore.records[i], BSG_CREATURE_WIRE_SIZE, &creature, NULL) == BSG_DATA_OK);
    }
    sStore.count = (uint16_t)count;
}

static void TestCapacityAndSerials(void)
{
    struct BsgCreature extra = GoldenCreature();
    uint8_t header[16];
    uint16_t count;
    uint32_t serials[] = {1, 2, UINT32_MAX};
    CHECK(BSG_OWNED_CAPACITY == 384);
    CHECK(BSG_STORE_MAX_WIRE_SIZE == 43024);
    PopulateStore(383);
    extra.serial = 384;
    CHECK(BsgCreatureStoreAppend(&sStore, &extra, 385, NULL) == BSG_DATA_OK);
    CHECK(sStore.count == 384);
    CHECK(BsgCreatureStoreEncode(sStoreWire, sizeof(sStoreWire), &sStore, 385, NULL) == BSG_DATA_OK);
    extra.serial = 385;
    CHECK(BsgCreatureStoreAppend(&sStore, &extra, 386, NULL) == BSG_DATA_COUNT);
    CHECK(sStore.count == 384);
    CHECK(BsgCreatureStoreDecode(&sDecodedStore, sStoreWire, sizeof(sStoreWire), 385, NULL) == BSG_DATA_OK);
    CHECK(sDecodedStore.count == 384 && memcmp(sStore.records, sDecodedStore.records, sizeof(sStore.records)) == 0);
    sStoreWire[BSG_STORE_HEADER_SIZE + 383 * BSG_CREATURE_WIRE_SIZE + 108] = 1;
    CHECK(BsgCreatureStoreDecode(&sDecodedStore, sStoreWire, sizeof(sStoreWire), 385, NULL) == BSG_DATA_RESERVED);
    CHECK(sDecodedStore.count == 384 && memcmp(sStore.records, sDecodedStore.records, sizeof(sStore.records)) == 0);
    sStoreWire[BSG_STORE_HEADER_SIZE + 383 * BSG_CREATURE_WIRE_SIZE + 108] = 0;
    CHECK(BsgCreatureStoreValidate(&sStore, 384, NULL) == BSG_DATA_SERIAL);
    CHECK(BsgCreatureStoreValidate(&sStore, 0, NULL) == BSG_DATA_OK);
    CHECK(BsgValidateSerialReferences(serials, 3, 0) == BSG_DATA_OK);
    CHECK(BsgValidateSerialReferences(serials, 3, UINT32_MAX) == BSG_DATA_SERIAL);
    memcpy(sStore.records[383], sStore.records[0], BSG_CREATURE_WIRE_SIZE);
    CHECK(BsgCreatureStoreValidate(&sStore, 385, NULL) == BSG_DATA_DUPLICATE);
    memcpy(header, sStoreWire, sizeof(header));
    CHECK(BsgStoreValidateWireHeader(header, sizeof(header), sizeof(sStoreWire), &count) == BSG_DATA_OK && count == 384);
    CHECK(BsgStoreEncodeWireHeader(header, sizeof(header), 385) == BSG_DATA_COUNT);
    CHECK(BsgCreatureStoreWireSize(385) == 0);
    CHECK(BsgCreatureStoreDecode(&sDecodedStore, sStoreWire, sizeof(sStoreWire) - 1, 385, NULL) == BSG_DATA_SIZE);
    PopulateStore(1);
    sStore.records[1][0] = 1;
    CHECK(BsgCreatureStoreValidate(&sStore, 2, NULL) == BSG_DATA_RESERVED);
}

static void TestRosterReferences(void)
{
    struct BsgRoster roster;
    struct BsgRoster decoded;
    uint8_t wire[BSG_ROSTER_WIRE_SIZE];
    unsigned i;
    BsgRosterInitialize(&roster);
    CHECK(BsgRosterValidate(&roster, 0) == BSG_DATA_OK);
    for (i = 0; i < BSG_OWNED_CAPACITY; ++i)
        roster.placement[i] = (uint16_t)i;
    CHECK(BsgRosterValidate(&roster, 384) == BSG_DATA_OK); /* Ownership does not force expedition membership. */
    for (i = 0; i < BSG_EXPEDITION_CAPACITY; ++i)
        roster.expedition[i] = (uint16_t)i;
    roster.bound = 7;
    roster.deployed[0] = 0;
    roster.deployed[1] = 1;
    CHECK(BsgRosterValidate(&roster, 384) == BSG_DATA_OK);
    CHECK(BsgRosterEncode(wire, sizeof(wire), &roster, 384) == BSG_DATA_OK);
    CHECK(BsgRosterDecode(&decoded, wire, sizeof(wire), 384) == BSG_DATA_OK);
    CHECK(memcmp(&decoded, &roster, sizeof(roster)) == 0);
    roster.placement[383] = 384;
    CHECK(BsgRosterValidate(&roster, 384) == BSG_DATA_REFERENCE);
    roster.placement[383] = 0;
    CHECK(BsgRosterValidate(&roster, 384) == BSG_DATA_DUPLICATE);
    roster.placement[383] = BSG_RECORD_NONE;
    CHECK(BsgRosterValidate(&roster, 384) != BSG_DATA_OK);
    roster.placement[383] = 383;
    roster.bound = 8; /* Storage reference cannot masquerade as expedition. */
    CHECK(BsgRosterValidate(&roster, 384) == BSG_DATA_REFERENCE);
    roster.bound = 7;
    roster.deployed[0] = 7;
    CHECK(BsgRosterValidate(&roster, 384) != BSG_DATA_OK);
    roster.deployed[0] = 1;
    CHECK(BsgRosterValidate(&roster, 384) == BSG_DATA_DUPLICATE);
    roster.deployed[0] = 8;
    CHECK(BsgRosterValidate(&roster, 384) == BSG_DATA_REFERENCE);
    CHECK(BsgRosterValidate(&roster, 385) == BSG_DATA_COUNT);
}

static BsgIo sIo = {&sFlash, FlashRead, FlashErase, FlashProgram};
static BsgWorkspace sWorkspace;
static BsgReader sReader;
static BsgImageSpec sSpec;
static uint8_t sRosterWire[1280]; /* Preserve the full planned roster allowance. */
static uint8_t sLogicalImage[BSG_IMAGE_BYTES];
static unsigned sFixtureRevision;
static int sUnstableFixture;
static int sRejectFixture;
static const uint8_t sLineage[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

static bool ReadFixture(void *context, uint32_t id, uint32_t offset, uint8_t *out, uint32_t size)
{
    uint32_t i;
    const uint8_t *source = NULL;
    (void)context;
    if (sRejectFixture)
        return false;
    if (id == BSG_SECTION_CREATURES)
        source = sStoreWire;
    else if (id == BSG_SECTION_PLAYER)
        source = sPlayerGolden;
    else if (id == BSG_SECTION_ROSTER)
        source = sRosterWire;
    if (source != NULL)
        memcpy(out, source + offset, size);
    else
        for (i = 0; i < size; ++i)
            out[i] = (uint8_t)(id * 17u + offset + i + sFixtureRevision);
    if (sUnstableFixture && sFlash.erased != 0 && id == BSG_SECTION_CREATURES && offset == 0 && size > 0)
        out[0] ^= 1; /* Change frozen source only after preflight; never validly commit it. */
    return true;
}

static void InitializeFixture(void)
{
    static const uint32_t lengths[10] = {43024, 256, 1280, 2048, 2064, 1040, 512, 1024, 3072, 512};
    struct BsgRoster roster;
    unsigned i;
    sReader = BsgDefaultReader((1u << BSG_GAME_PART_I) | (1u << BSG_GAME_PART_II));
    memset(&sSpec, 0, sizeof(sSpec));
    BsgInitMetadata(&sSpec.metadata, BSG_GAME_PART_I, sLineage);
    for (i = 0; i < sizeof(sSpec.metadata.buildId); ++i)
        sSpec.metadata.buildId[i] = (uint8_t)(0xA0u + i);
    sSpec.metadata.eligibility = 0x12345678; /* Deliberately synthetic policy token. */
    sSpec.readSection = ReadFixture;
    sSpec.sectionCount = 10;
    for (i = 0; i < 10; ++i)
    {
        sSpec.sections[i].id = i + 1;
        sSpec.sections[i].major = 1;
        sSpec.sections[i].length = lengths[i];
        sSpec.sections[i].flags = BSG_SCOPE_SHARED | BSG_REQUIRED_RESUME | BSG_REQUIRED_TRANSFER;
    }
    sSpec.sections[8].flags = BSG_SCOPE_LOCAL | BSG_REQUIRED_RESUME;
    sSpec.sections[0].count = 384;
    sSpec.sections[1].count = 1;
    PopulateStore(384);
    CHECK(BsgCreatureStoreEncode(sStoreWire, sizeof(sStoreWire), &sStore, 385, NULL) == BSG_DATA_OK);
    BsgRosterInitialize(&roster);
    for (i = 0; i < 384; ++i)
        roster.placement[i] = (uint16_t)i;
    for (i = 0; i < BSG_EXPEDITION_CAPACITY; ++i)
        roster.expedition[i] = (uint16_t)i;
    roster.bound = 7;
    roster.deployed[0] = 0;
    roster.deployed[1] = 1;
    memset(sRosterWire, 0, sizeof(sRosterWire));
    CHECK(BsgRosterEncode(sRosterWire, BSG_ROSTER_WIRE_SIZE, &roster, 384) == BSG_DATA_OK);
    sFixtureRevision = 0;
    sUnstableFixture = 0;
    sRejectFixture = 0;
}

/* Re-sign an intentionally edited fixture so structural checks, not an outer
 * CRC mismatch, must reject the malformed field. Section CRCs are independent
 * and remain unchanged unless a test explicitly updates them. */
static void RepairSnapshotIntegrity(uint8_t slot)
{
    uint32_t crc;
    unsigned sector;
    ReadLogical(slot, 0, sLogicalImage, sizeof(sLogicalImage));
    memset(sLogicalImage + 72, 0, 4);
    crc = BsgCrc32c(sLogicalImage, sizeof(sLogicalImage));
    Put32(sLogicalImage + 72, crc);
    WriteLogical(slot, 0, sLogicalImage, sizeof(sLogicalImage));
    Put32(sFlash.bytes + slot * 65536u + 15u * 4096u + 0xFFCu, crc);
    for (sector = 0; sector < 16; ++sector)
    {
        uint8_t *physical = sFlash.bytes + slot * 65536u + sector * 4096u;
        uint32_t sectorCrc = BsgCrc32c(physical, 0xFE0);
        Put32(physical + 0xFE0, sectorCrc);
        Put32(physical + 0xFE4, ~sectorCrc);
    }
}

static void TestCrc32c(void)
{
    uint32_t running;
    static const uint8_t text[] = "123456789";
    CHECK(BsgCrc32c(text, 9) == UINT32_C(0xE3069283));
    CHECK(BsgCrc32c(text, 0) == 0);
    running = BsgCrc32cUpdate(UINT32_MAX, text, 4);
    running = BsgCrc32cUpdate(running, text + 4, 5);
    CHECK((running ^ UINT32_MAX) == UINT32_C(0xE3069283));
}

static void EstablishTwoSnapshots(BsgSnapshot *latest)
{
    ResetFlash();
    InitializeFixture();
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, latest) == BSG_OK);
    CHECK(latest->slot == 0 && latest->metadata.generation == 1);
    sFixtureRevision = 1;
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, latest) == BSG_OK);
    CHECK(latest->slot == 1 && latest->metadata.generation == 2);
    memcpy(sSavedFlash, sFlash.bytes, sizeof(sSavedFlash));
    memcpy(sPreviousBank, sFlash.bytes + 65536, sizeof(sPreviousBank));
    ResetFaults();
    sFixtureRevision = 2;
}

static void RestoreTwoSnapshots(void)
{
    memcpy(sFlash.bytes, sSavedFlash, sizeof(sSavedFlash));
    ResetFaults();
}

static void TestSnapshotRoundTripAndNewest(void)
{
    BsgSnapshot snapshot;
    BsgSnapshot checked;
    uint8_t record[112];
    uint8_t crossing[200];
    uint8_t expected[200];
    bool recovered = true;
    ResetFlash();
    InitializeFixture();
    CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &snapshot, &recovered) == BSG_EMPTY && !recovered);
    EstablishTwoSnapshots(&snapshot);
    CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &checked, &recovered) == BSG_OK);
    CHECK(checked.slot == 1 && checked.metadata.generation == 2 && !recovered);
    CHECK(checked.usedExtent == 55472);
    CHECK(BSG_IMAGE_BYTES - checked.usedExtent == 9040);
    CHECK(BsgReadSection(&sIo, &checked, BSG_SECTION_CREATURES, 16, record, sizeof(record)) == BSG_OK);
    CHECK(memcmp(record, sStore.records[0], sizeof(record)) == 0);
    CHECK(BsgReadLogical(&sIo, 1, 4000, crossing, sizeof(crossing)) == BSG_OK);
    ReadLogical(1, 4000, expected, sizeof(expected));
    CHECK(memcmp(crossing, expected, sizeof(crossing)) == 0);
    CHECK(BsgReadLogical(&sIo, 2, 0, crossing, 1) == BSG_BAD_ARGUMENT);
    CHECK(BsgReadLogical(&sIo, 1, UINT32_MAX, crossing, 1) == BSG_BAD_ARGUMENT);
    CHECK(BsgReadLogical(&sIo, 1, BSG_IMAGE_BYTES - 1, crossing, 2) == BSG_BAD_ARGUMENT);
    CHECK(BsgReadSection(&sIo, &checked, BSG_SECTION_CREATURES, UINT32_MAX, record, 1) == BSG_BAD_ARGUMENT);
    CHECK(BsgReadSection(&sIo, &checked, 999, 0, record, 1) == BSG_BAD_ARGUMENT);
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) == BSG_OK);
    CHECK(snapshot.slot == 0 && snapshot.metadata.generation == 3);
    CHECK(memcmp(sPreviousBank, sFlash.bytes + 65536, sizeof(sPreviousBank)) == 0);
    CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &checked, &recovered) == BSG_OK);
    CHECK(checked.slot == 0 && checked.metadata.generation == 3 && !recovered);
}

static void TestCorruptionAndRecovery(void)
{
    BsgSnapshot snapshot;
    bool recovered = false;
    static const uint32_t faults[] = {0, 31, 32, 32 + 72, 32 + 640, 0xFE0, 0xFE4, 0xFE8, 0xFF0, 65535};
    unsigned i;
    EstablishTwoSnapshots(&snapshot);
    for (i = 0; i < sizeof(faults) / sizeof(faults[0]); ++i)
    {
        RestoreTwoSnapshots();
        sFlash.bytes[65536u + faults[i]] ^= 1;
        CHECK(BsgValidateSnapshot(&sIo, 1, &sWorkspace, &snapshot) == BSG_INVALID);
        CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &snapshot, &recovered) == BSG_OK);
        CHECK(snapshot.metadata.generation == 1 && snapshot.slot == 0 && recovered);
    }
    RestoreTwoSnapshots();
    sFlash.bytes[LogicalOffset(1, BSG_BODY_OFFSET + 24)] ^= 1;
    RepairSnapshotIntegrity(1); /* Section CRC still detects changed contents. */
    CHECK(BsgValidateSnapshot(&sIo, 1, &sWorkspace, &snapshot) == BSG_INVALID);
    RestoreTwoSnapshots();
    sFlash.bytes[LogicalOffset(1, BSG_IMAGE_BYTES - 1)] = 1;
    RepairSnapshotIntegrity(1);
    CHECK(BsgValidateSnapshot(&sIo, 1, &sWorkspace, &snapshot) == BSG_INVALID);
    ResetFlash();
    sFlash.bytes[0] = 0;
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) == BSG_INVALID);
    CHECK(sFlash.erased == 0 && sFlash.programmed == 0);
}

static void SetLogical16(uint8_t slot, uint32_t offset, uint16_t value)
{
    uint8_t bytes[2];
    Put16(bytes, value);
    WriteLogical(slot, offset, bytes, sizeof(bytes));
}

static void SetLogical32(uint8_t slot, uint32_t offset, uint32_t value)
{
    uint8_t bytes[4];
    Put32(bytes, value);
    WriteLogical(slot, offset, bytes, sizeof(bytes));
}

static void TestMalformedRanges(void)
{
    /* Each change has valid outer CRCs, so malformed structures are actually parsed. */
    static const struct { uint32_t offset; uint32_t value; } changes[] =
    {
        {128 + 12, 636},        /* Section overlaps the header/directory. */
        {128 + 12, 641},        /* Misaligned section. */
        {128 + 12, UINT32_MAX}, /* Offset overflow. */
        {128 + 16, UINT32_MAX}, /* Length overflow. */
        {128 + 16, 0},         /* Zero length occupied section. */
        {128 + 32 + 12, 640},   /* Sections overlap. */
        {128 + 32, 1},         /* Duplicate directory ID. */
        {128 + 32, 0},         /* Invalid directory ID. */
        {128 + 28, 1},         /* Reserved directory bytes. */
        {128 + 8, BSG_SCOPE_SHARED | BSG_SCOPE_LOCAL},
        {128 + 8, 0},          /* Undeclared persistence scope. */
        {56, BSG_IMAGE_BYTES + 4},
        {112, 1},              /* Reserved image header bytes. */
        {128 + 10 * 32, 1},    /* Unused directory entries must be canonical zero. */
    };
    BsgSnapshot snapshot;
    unsigned i;
    EstablishTwoSnapshots(&snapshot);
    for (i = 0; i < sizeof(changes) / sizeof(changes[0]); ++i)
    {
        RestoreTwoSnapshots();
        SetLogical32(1, changes[i].offset, changes[i].value);
        RepairSnapshotIntegrity(1);
        CHECK(BsgValidateSnapshot(&sIo, 1, &sWorkspace, &snapshot) == BSG_INVALID);
    }
    RestoreTwoSnapshots();
    SetLogical16(1, 18, 17);
    RepairSnapshotIntegrity(1);
    CHECK(BsgValidateSnapshot(&sIo, 1, &sWorkspace, &snapshot) == BSG_INVALID);
}

static void TestUnsupportedNewest(void)
{
    static const uint32_t shortFields[] = {8, 10, 22, 24, 26, 68, 70, 100, 102};
    static const uint32_t longFields[] = {28, 76, 80, 108};
    BsgSnapshot snapshot;
    bool recovered;
    unsigned i;
    EstablishTwoSnapshots(&snapshot);
    for (i = 0; i < sizeof(shortFields) / sizeof(shortFields[0]); ++i)
    {
        RestoreTwoSnapshots();
        SetLogical16(1, shortFields[i], 99);
        RepairSnapshotIntegrity(1);
        CHECK(BsgValidateSnapshot(&sIo, 1, &sWorkspace, &snapshot) == BSG_OK);
        CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &snapshot, &recovered) == BSG_UNSUPPORTED);
        CHECK(snapshot.metadata.generation == 2 && !recovered);
        CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) == BSG_UNSUPPORTED);
        CHECK(sFlash.erased == 0 && sFlash.programmed == 0);
        CHECK(memcmp(sPreviousBank, sSavedFlash + 65536, sizeof(sPreviousBank)) == 0);
    }
    for (i = 0; i < sizeof(longFields) / sizeof(longFields[0]); ++i)
    {
        RestoreTwoSnapshots();
        SetLogical32(1, longFields[i], 99);
        RepairSnapshotIntegrity(1);
        CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &snapshot, NULL) == BSG_UNSUPPORTED);
    }
    RestoreTwoSnapshots();
    SetLogical16(1, 20, BSG_GAME_PART_II);
    RepairSnapshotIntegrity(1);
    {
        BsgReader partIReader = BsgDefaultReader(1u << BSG_GAME_PART_I);
        CHECK(BsgSelectSnapshot(&sIo, &partIReader, &sWorkspace, &snapshot, NULL) == BSG_UNSUPPORTED);
    }
}

static void TestSectionCompatibility(void)
{
    BsgSnapshot snapshot;
    EstablishTwoSnapshots(&snapshot);
    SetLogical16(1, 128 + 4, 2);
    RepairSnapshotIntegrity(1);
    CHECK(BsgValidateSnapshot(&sIo, 1, &sWorkspace, &snapshot) == BSG_OK);
    CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &snapshot, NULL) == BSG_UNSUPPORTED);
    RestoreTwoSnapshots();
    SetLogical32(1, 128 + 8, BSG_SCOPE_SHARED | 0x80000000u);
    RepairSnapshotIntegrity(1);
    CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &snapshot, NULL) == BSG_UNSUPPORTED);
    RestoreTwoSnapshots();
    SetLogical32(1, 128 + 9 * 32, 99); /* Unknown required section. */
    RepairSnapshotIntegrity(1);
    CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &snapshot, NULL) == BSG_UNSUPPORTED);
    SetLogical32(1, 128 + 9 * 32 + 8, BSG_SCOPE_SHARED | BSG_PRESERVE);
    RepairSnapshotIntegrity(1);
    CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &snapshot, NULL) == BSG_OK);
    CHECK(snapshot.metadata.generation == 2 && BsgFindSection(&snapshot, 99) != NULL);
}

static void SetSnapshotGeneration(uint8_t slot, uint64_t generation)
{
    uint8_t encoded[8];
    unsigned sector;
    Put32(encoded, (uint32_t)generation);
    Put32(encoded + 4, (uint32_t)(generation >> 32));
    WriteLogical(slot, 32, encoded, sizeof(encoded));
    for (sector = 0; sector < 16; ++sector)
        memcpy(sFlash.bytes + slot * 65536u + sector * 4096u + 8, encoded, sizeof(encoded));
    memcpy(sFlash.bytes + slot * 65536u + 15 * 4096u + 0xFF4, encoded, sizeof(encoded));
    RepairSnapshotIntegrity(slot);
}

static void TestLineageAndGeneration(void)
{
    BsgSnapshot snapshot;
    unsigned sector;
    EstablishTwoSnapshots(&snapshot);
    sFlash.bytes[LogicalOffset(1, 40)] ^= 0x80;
    for (sector = 0; sector < 16; ++sector)
        sFlash.bytes[65536u + sector * 4096u + 16] ^= 0x80;
    RepairSnapshotIntegrity(1);
    CHECK(BsgValidateSnapshot(&sIo, 1, &sWorkspace, &snapshot) == BSG_OK);
    CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &snapshot, NULL) == BSG_AMBIGUOUS);
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) == BSG_AMBIGUOUS);
    CHECK(sFlash.erased == 0 && sFlash.programmed == 0);
    RestoreTwoSnapshots();
    SetSnapshotGeneration(0, 2);
    CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &snapshot, NULL) == BSG_AMBIGUOUS);
    RestoreTwoSnapshots();
    SetSnapshotGeneration(1, UINT64_MAX);
    CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &snapshot, NULL) == BSG_OK);
    CHECK(snapshot.metadata.generation == UINT64_MAX);
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) == BSG_CAPACITY);
    CHECK(sFlash.erased == 0 && sFlash.programmed == 0);
}

static void TestLegacyGuard(void)
{
    unsigned sector;
    ResetFlash();
    CHECK(BsgLegacyAccessAllowed(&sIo));
    for (sector = 0; sector < 32; ++sector)
    {
        ResetFlash();
        memcpy(sFlash.bytes + sector * 4096u, "BSEC", 4);
        CHECK(!BsgLegacyAccessAllowed(&sIo)); /* Even an otherwise incomplete snapshot. */
        ResetFlash();
        memcpy(sFlash.bytes + sector * 4096u + 32, "BONDSAGA", 8);
        CHECK(!BsgLegacyAccessAllowed(&sIo)); /* Protect a damaged outer sector header too. */
    }
    ResetFlash();
    sFlash.failRead = 1;
    CHECK(!BsgLegacyAccessAllowed(&sIo));
    CHECK(!BsgLegacyAccessAllowed(NULL));
}

static void TestWritePreflight(void)
{
    BsgSnapshot snapshot;
    EstablishTwoSnapshots(&snapshot);
    sSpec.sections[9].length = UINT32_MAX - 3;
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) == BSG_CAPACITY);
    CHECK(sFlash.erased == 0 && sFlash.programmed == 0);
    CHECK(memcmp(sFlash.bytes, sSavedFlash, sizeof(sSavedFlash)) == 0);
    sSpec.sections[9].length = 512;
    sRejectFixture = 1;
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) == BSG_INVALID);
    CHECK(sFlash.erased == 0 && sFlash.programmed == 0);
    sRejectFixture = 0;
    sFlash.failRead = 1;
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) == BSG_IO_ERROR);
    CHECK(sFlash.erased == 0 && sFlash.programmed == 0);
    ResetFaults();
    sUnstableFixture = 1;
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) != BSG_OK);
    CHECK(memcmp(sPreviousBank, sFlash.bytes + 65536, sizeof(sPreviousBank)) == 0);
    sUnstableFixture = 0;
    ResetFaults();
    CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &snapshot, NULL) == BSG_OK);
    CHECK(snapshot.metadata.generation == 2);
}

static void AssertFailedWritePreservesPrevious(void)
{
    BsgSnapshot selected;
    ResetFaults();
    CHECK(memcmp(sPreviousBank, sFlash.bytes + 65536, sizeof(sPreviousBank)) == 0);
    CHECK(BsgSelectSnapshot(&sIo, &sReader, &sWorkspace, &selected, NULL) == BSG_OK);
    CHECK(selected.slot == 1 && selected.metadata.generation == 2);
}

static void TestTornWrites(void)
{
    BsgSnapshot snapshot;
    uint32_t totalPrograms;
    uint32_t cut;
    unsigned sector;
    unsigned faultCases = 0;
    static const uint32_t boundaryDeltas[] = {0, 1, 31, 32, 33, 4063, 4064, 4067, 4068, 4079};
    unsigned delta;
    EstablishTwoSnapshots(&snapshot);
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) == BSG_OK);
    totalPrograms = sFlash.programmed;
    CHECK(totalPrograms >= 16);
    for (sector = 0; sector < 16; ++sector)
    {
        for (delta = 0; delta < sizeof(boundaryDeltas) / sizeof(boundaryDeltas[0]); ++delta)
        {
            RestoreTwoSnapshots();
            cut = sector * 4080u + boundaryDeltas[delta];
            CHECK(cut < totalPrograms);
            sFlash.failProgramAfter = cut;
            CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) != BSG_OK);
            AssertFailedWritePreservesPrevious();
            ++faultCases;
        }
    }
    for (cut = totalPrograms - 16; cut < totalPrograms; ++cut)
    {
        RestoreTwoSnapshots();
        sFlash.failProgramAfter = cut;
        CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) != BSG_OK);
        AssertFailedWritePreservesPrevious();
        ++faultCases;
    }
    for (sector = 0; sector < 16; ++sector)
    {
        RestoreTwoSnapshots();
        sFlash.failEraseAt = (int32_t)sector;
        sFlash.tornEraseBytes = 2048;
        CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) == BSG_IO_ERROR);
        AssertFailedWritePreservesPrevious();
        ++faultCases;
    }
    RestoreTwoSnapshots();
    sFlash.failEraseAt = 0;
    sFlash.tornEraseBytes = 0;
    sFlash.silentErase = 1;
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) == BSG_IO_ERROR);
    AssertFailedWritePreservesPrevious();
    RestoreTwoSnapshots();
    sFlash.failProgramAfter = 1234;
    sFlash.silentProgram = 1;
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) != BSG_OK);
    AssertFailedWritePreservesPrevious();
    RestoreTwoSnapshots();
    sFlash.failProgramAfter = totalPrograms;
    CHECK(BsgWriteSnapshot(&sIo, &sReader, &sSpec, &sWorkspace, &snapshot) == BSG_OK);
    CHECK(snapshot.metadata.generation == 3);
    CHECK(memcmp(sPreviousBank, sFlash.bytes + 65536, sizeof(sPreviousBank)) == 0);
    printf("  %u torn-write/erase boundaries plus silent-failure and complete-commit controls\n", faultCases);
}

static bool SyntheticEligible(void *context, const BsgMetadata *metadata)
{
    bool *allowed = context;
    return *allowed && metadata->eligibility == UINT32_C(0x12345678);
}

static bool SyntheticLocal(void *context, uint32_t offset, uint8_t *out, uint32_t size)
{
    uint32_t i;
    (void)context;
    if (offset > 3072 || size > 3072 - offset)
        return false;
    for (i = 0; i < size; ++i)
        out[i] = (uint8_t)(0xD0u ^ (offset + i));
    return true;
}

static void TestSyntheticSagaTransfer(void)
{
    BsgSnapshot source;
    BsgSnapshot target;
    bool allowed = false;
    BsgTransferPolicy policy = {
        .context = &allowed, .eligible = SyntheticEligible, .readLocal = SyntheticLocal,
        .localLength = 3072, .initializerVersion = 1,
    };
    uint8_t sourceBytes[128];
    uint8_t targetBytes[128];
    uint32_t offset;
    unsigned i;
    EstablishTwoSnapshots(&source);
    for (i = 0; i < sizeof(policy.destinationBuildId); ++i)
        policy.destinationBuildId[i] = (uint8_t)(0xB0u + i);
    CHECK(BsgTransferPartII(&sIo, &sReader, &policy, &sWorkspace, &target) == BSG_NOT_ELIGIBLE);
    CHECK(memcmp(sFlash.bytes, sSavedFlash, sizeof(sSavedFlash)) == 0);
    allowed = true;
    CHECK(BsgTransferPartII(&sIo, &sReader, &policy, &sWorkspace, &target) == BSG_OK);
    CHECK(target.metadata.game == BSG_GAME_PART_II && target.metadata.sourceGame == BSG_GAME_PART_I);
    CHECK(target.metadata.generation == source.metadata.generation + 1);
    CHECK(memcmp(target.metadata.buildId, policy.destinationBuildId, sizeof(target.metadata.buildId)) == 0);
    CHECK(memcmp(target.metadata.buildId, source.metadata.buildId, sizeof(target.metadata.buildId)) != 0);
    CHECK(memcmp(target.metadata.lineage, source.metadata.lineage, 16) == 0);
    CHECK(memcmp(sPreviousBank, sFlash.bytes + 65536, sizeof(sPreviousBank)) == 0);
    {
        uint8_t receipt[96];
        CHECK(BsgReadSection(&sIo, &target, BSG_SECTION_TRANSFER_RECEIPT, 0, receipt, sizeof(receipt)) == BSG_OK);
        CHECK(memcmp(receipt, source.metadata.lineage, 16) == 0);
        CHECK(BsgReadU32(receipt + 16) == source.metadata.generation && BsgReadU32(receipt + 20) == 0);
        CHECK(BsgReadU32(receipt + 24) == source.imageCrc);
        CHECK(BsgReadU16(receipt + 28) == BSG_GAME_PART_I);
        CHECK(BsgReadU16(receipt + 30) == source.metadata.schemaMajor);
        CHECK(BsgReadU16(receipt + 34) == BSG_TRANSFER_CONTRACT);
        CHECK(BsgReadU32(receipt + 36) == 1 && BsgReadU32(receipt + 64) == policy.initializerVersion);
        CHECK(memcmp(receipt + 48, source.metadata.buildId, sizeof(source.metadata.buildId)) == 0);
        for (i = 68; i < sizeof(receipt); ++i)
            CHECK(receipt[i] == 0);
    }
    for (i = 0; i < source.sectionCount; ++i)
    {
        const BsgSection *section = &source.sections[i];
        if (section->id == BSG_SECTION_LOCAL)
            continue;
        CHECK(BsgFindSection(&target, section->id)->length == section->length);
        CHECK(BsgFindSection(&target, section->id)->count == section->count);
        for (offset = 0; offset < section->length;)
        {
            uint32_t size = section->length - offset;
            if (size > sizeof(sourceBytes))
                size = sizeof(sourceBytes);
            CHECK(BsgReadSection(&sIo, &source, section->id, offset, sourceBytes, size) == BSG_OK);
            CHECK(BsgReadSection(&sIo, &target, section->id, offset, targetBytes, size) == BSG_OK);
            CHECK(memcmp(sourceBytes, targetBytes, size) == 0);
            offset += size;
        }
    }
    CHECK(BsgReadSection(&sIo, &target, BSG_SECTION_LOCAL, 0, targetBytes, sizeof(targetBytes)) == BSG_OK);
    CHECK(SyntheticLocal(NULL, 0, sourceBytes, sizeof(sourceBytes)));
    CHECK(memcmp(sourceBytes, targetBytes, sizeof(sourceBytes)) == 0);
    CHECK(BsgReadSection(&sIo, &target, BSG_SECTION_CREATURES, 0, sStoreWire, sizeof(sStoreWire)) == BSG_OK);
    CHECK(BsgCreatureStoreDecode(&sDecodedStore, sStoreWire, sizeof(sStoreWire), 385, NULL) == BSG_DATA_OK);
    CHECK(sDecodedStore.count == 384 && memcmp(sDecodedStore.records, sStore.records, sizeof(sStore.records)) == 0);
    memcpy(sSavedFlash, sFlash.bytes, sizeof(sSavedFlash));
    ResetFaults();
    CHECK(BsgTransferPartII(&sIo, &sReader, &policy, &sWorkspace, &target) == BSG_ALREADY_IMPORTED);
    CHECK(sFlash.erased == 0 && sFlash.programmed == 0);
    CHECK(memcmp(sSavedFlash, sFlash.bytes, sizeof(sSavedFlash)) == 0);
}

static struct BsgPlayerBinder sFoundationPlayer;
static struct BsgRoster sFoundationRoster;
static struct BsgFoundationSource sFoundationSource;
static uint8_t sOpaqueData[7][3072];

static void InitializeFoundationFixture(void)
{
    static const uint32_t lengths[7] = {2048, 2064, 1040, 512, 1024, 3072, 512};
    unsigned i, j;
    InitializeFixture();
    memset(&sFoundationSource, 0, sizeof(sFoundationSource));
    CHECK(BsgPlayerBinderDecode(&sFoundationPlayer, sPlayerGolden, sizeof(sPlayerGolden), NULL) == BSG_DATA_OK);
    BsgRosterInitialize(&sFoundationRoster);
    for (i = 0; i < 384; ++i)
        sFoundationRoster.placement[i] = (uint16_t)i;
    /* A full collection with one expedition member must remain representable. */
    sFoundationRoster.expedition[0] = 383;
    sFoundationSource.store = &sStore;
    sFoundationSource.player = &sFoundationPlayer;
    sFoundationSource.roster = &sFoundationRoster;
    for (i = 0; i < 7; ++i)
    {
        for (j = 0; j < lengths[i]; ++j)
            sOpaqueData[i][j] = (uint8_t)(i * 13 + j);
        sFoundationSource.opaque[i].data = sOpaqueData[i];
        sFoundationSource.opaque[i].length = lengths[i];
    }
}

static void RepairSectionIntegrity(uint8_t slot, const BsgSnapshot *snapshot, uint32_t id)
{
    const BsgSection *section = BsgFindSection(snapshot, id);
    unsigned i;
    uint32_t crc;
    CHECK(section != NULL);
    ReadLogical(slot, section->offset, sLogicalImage, section->length);
    crc = BsgCrc32c(sLogicalImage, section->length);
    for (i = 0; i < snapshot->sectionCount; ++i)
        if (snapshot->sections[i].id == id)
            SetLogical32(slot, 128 + i * 32 + 20, crc);
    RepairSnapshotIntegrity(slot);
}

static void TestFoundationProfile(void)
{
    BsgSnapshot snapshot;
    BsgSnapshot newest;
    BsgSnapshot unchanged;
    uint8_t bytes[128];
    const BsgSection *creatures;
    const BsgSection *roster;
    bool recovered = false;
    unsigned i;
    ResetFlash();
    InitializeFoundationFixture();
    CHECK(BsgSaveFoundation(&sIo, &sReader, &sSpec.metadata, &sFoundationSource, &sWorkspace, &snapshot) == BSG_OK);
    CHECK(BsgLoadFoundation(&sIo, &sReader, &sWorkspace, NULL, &newest, &recovered) == BSG_OK);
    CHECK(newest.metadata.generation == 1 && !recovered);
    CHECK(newest.sectionCount == BSG_FOUNDATION_SECTION_COUNT);
    CHECK(newest.usedExtent == BSG_BODY_OFFSET + BSG_FOUNDATION_MAX_BODY_BYTES);
    CHECK(BSG_IMAGE_BYTES - newest.usedExtent == BSG_FOUNDATION_RESERVE_BYTES);
    CHECK(BSG_FOUNDATION_RESERVE_BYTES >= 4096);
    for (i = 0; i < 7; ++i)
    {
        const BsgSection *section = BsgFindSection(&newest, i + BSG_SECTION_INVENTORY);
        CHECK(section != NULL && section->length == sFoundationSource.opaque[i].length);
        CHECK(BsgReadSection(&sIo, &newest, i + BSG_SECTION_INVENTORY, 0, bytes, sizeof(bytes)) == BSG_OK);
        CHECK(memcmp(bytes, sOpaqueData[i], sizeof(bytes)) == 0);
    }
    ++sFoundationPlayer.currency;
    CHECK(BsgSaveFoundation(&sIo, &sReader, &sSpec.metadata, &sFoundationSource, &sWorkspace, &snapshot) == BSG_OK);
    CHECK(snapshot.metadata.generation == 2 && snapshot.slot == 1);
    memcpy(sSavedFlash, sFlash.bytes, sizeof(sSavedFlash));
    memcpy(sPreviousBank, sFlash.bytes + 65536, sizeof(sPreviousBank));
    ResetFaults();
    sStore.count = 385;
    CHECK(BsgSaveFoundation(&sIo, &sReader, &sSpec.metadata, &sFoundationSource, &sWorkspace, &newest) == BSG_CAPACITY);
    CHECK(sFlash.erased == 0 && sFlash.programmed == 0);
    sStore.count = 384;
    sFoundationRoster.placement[383] = 0;
    CHECK(BsgSaveFoundation(&sIo, &sReader, &sSpec.metadata, &sFoundationSource, &sWorkspace, &newest) == BSG_BAD_REFERENCE);
    CHECK(sFlash.erased == 0 && sFlash.programmed == 0);
    sFoundationRoster.placement[383] = 383;
    sFoundationSource.opaque[0].length = 2049;
    CHECK(BsgSaveFoundation(&sIo, &sReader, &sSpec.metadata, &sFoundationSource, &sWorkspace, &newest) == BSG_CAPACITY);
    sFoundationSource.opaque[0].length = 2048;
    CHECK(memcmp(sSavedFlash, sFlash.bytes, sizeof(sSavedFlash)) == 0);
    creatures = BsgFindSection(&snapshot, BSG_SECTION_CREATURES);
    roster = BsgFindSection(&snapshot, BSG_SECTION_ROSTER);
    CHECK(creatures != NULL && roster != NULL);
    /* A complete, checksummed newest snapshot with a bad reference must not
     * silently roll back to the older valid gameplay profile. */
    SetLogical16(1, roster->offset + BSG_ROSTER_HEADER_SIZE + 2, 0);
    RepairSectionIntegrity(1, &snapshot, BSG_SECTION_ROSTER);
    CHECK(BsgValidateSnapshot(&sIo, 1, &sWorkspace, &newest) == BSG_OK);
    memset(&newest, 0xA5, sizeof(newest));
    unchanged = newest;
    CHECK(BsgLoadFoundation(&sIo, &sReader, &sWorkspace, NULL, &newest, &recovered) == BSG_BAD_REFERENCE);
    CHECK(memcmp(&newest, &unchanged, sizeof(newest)) == 0);
    CHECK(BsgSaveFoundation(&sIo, &sReader, &sSpec.metadata, &sFoundationSource, &sWorkspace, &newest) == BSG_BAD_REFERENCE);
    CHECK(sFlash.erased == 0 && sFlash.programmed == 0);
    RestoreTwoSnapshots();
    SetLogical32(1, creatures->offset + BSG_STORE_HEADER_SIZE + BSG_CREATURE_WIRE_SIZE, 1);
    RepairSectionIntegrity(1, &snapshot, BSG_SECTION_CREATURES);
    CHECK(BsgLoadFoundation(&sIo, &sReader, &sWorkspace, NULL, &newest, &recovered) == BSG_BAD_REFERENCE);
    RestoreTwoSnapshots();
    SetLogical32(1, 128 + 24, 385);
    RepairSnapshotIntegrity(1);
    CHECK(BsgLoadFoundation(&sIo, &sReader, &sWorkspace, NULL, &newest, &recovered) == BSG_CAPACITY);
    RestoreTwoSnapshots();
    {
        unsigned calls = 0;
        struct BsgRegistry registry = {RejectSpecies, &calls};
        CHECK(BsgLoadFoundation(&sIo, &sReader, &sWorkspace, &registry, &newest, &recovered) == BSG_UNSUPPORTED);
        CHECK(calls > 0);
    }
}

static void TestFoundationTransferAndRetry(void)
{
    BsgSnapshot donor;
    BsgSnapshot imported;
    BsgSnapshot selected;
    bool allowed = true;
    BsgTransferPolicy policy = {
        .context = &allowed, .eligible = SyntheticEligible, .readLocal = SyntheticLocal,
        .localLength = BSG_LOCAL_SECTION_BYTES, .initializerVersion = 23,
        .validateSnapshot = BsgValidateFoundationCallback,
    };
    uint8_t receipt[96];
    uint8_t savedReceipt[96];
    uint8_t local[BSG_LOCAL_SECTION_BYTES];
    uint8_t bytes[256];
    unsigned i;
    ResetFlash();
    InitializeFoundationFixture();
    CHECK(BsgSaveFoundation(&sIo, &sReader, &sSpec.metadata, &sFoundationSource, &sWorkspace, &donor) == BSG_OK);
    CHECK(BsgSaveFoundation(&sIo, &sReader, &sSpec.metadata, &sFoundationSource, &sWorkspace, &donor) == BSG_OK);
    memcpy(sSavedFlash, sFlash.bytes, sizeof(sSavedFlash));
    memcpy(sPreviousBank, sFlash.bytes + 65536, sizeof(sPreviousBank));
    ResetFaults();
    sFlash.failProgramAfter = 16 * 4080 + 8; /* Half of the final commit record. */
    CHECK(BsgTransferPartII(&sIo, &sReader, &policy, &sWorkspace, &imported) == BSG_IO_ERROR);
    AssertFailedWritePreservesPrevious();
    CHECK(BsgLoadFoundation(&sIo, &sReader, &sWorkspace, NULL, &selected, NULL) == BSG_OK);
    CHECK(selected.metadata.game == BSG_GAME_PART_I && selected.metadata.generation == donor.metadata.generation);
    CHECK(BsgTransferPartII(&sIo, &sReader, &policy, &sWorkspace, &imported) == BSG_OK);
    CHECK(imported.metadata.game == BSG_GAME_PART_II && imported.metadata.generation == donor.metadata.generation + 1);
    CHECK(BsgLoadFoundation(&sIo, &sReader, &sWorkspace, NULL, &selected, NULL) == BSG_OK);
    CHECK(selected.usedExtent == BSG_BODY_OFFSET + BSG_FOUNDATION_MAX_BODY_BYTES);
    CHECK(BsgReadSection(&sIo, &selected, BSG_SECTION_CREATURES, 0, sStoreWire, sizeof(sStoreWire)) == BSG_OK);
    CHECK(BsgCreatureStoreDecode(&sDecodedStore, sStoreWire, sizeof(sStoreWire), sFoundationPlayer.nextSpecimenSerial, NULL) == BSG_DATA_OK);
    CHECK(sDecodedStore.count == 384 && memcmp(sStore.records, sDecodedStore.records, sizeof(sStore.records)) == 0);
    CHECK(BsgReadSection(&sIo, &selected, BSG_SECTION_PLAYER, 0, bytes, sizeof(bytes)) == BSG_OK);
    CHECK(memcmp(bytes, sPlayerGolden, sizeof(bytes)) == 0);
    CHECK(BsgReadSection(&sIo, &selected, BSG_SECTION_TRANSFER_RECEIPT, 0, receipt, sizeof(receipt)) == BSG_OK);
    CHECK(BsgReadU32(receipt + 64) == 23 && BsgReadU32(receipt + 24) == donor.imageCrc);
    memcpy(savedReceipt, receipt, sizeof(receipt));
    CHECK(BsgReadSection(&sIo, &selected, BSG_SECTION_LOCAL, 0, local, sizeof(local)) == BSG_OK);
    for (i = 0; i < sizeof(local); ++i)
        CHECK(local[i] == (uint8_t)(0xD0u ^ i));
    sFoundationSource.opaque[BSG_SECTION_LOCAL - BSG_SECTION_INVENTORY].data = local;
    sFoundationSource.transferReceipt.data = receipt;
    sFoundationSource.transferReceipt.length = sizeof(receipt);
    ++sFoundationPlayer.currency; /* Subsequent Part II save retains transfer provenance. */
    CHECK(BsgSaveFoundation(&sIo, &sReader, &imported.metadata, &sFoundationSource, &sWorkspace, &selected) == BSG_OK);
    CHECK(selected.metadata.generation == imported.metadata.generation + 1);
    CHECK(BsgReadSection(&sIo, &selected, BSG_SECTION_TRANSFER_RECEIPT, 0, receipt, sizeof(receipt)) == BSG_OK);
    CHECK(memcmp(receipt, savedReceipt, sizeof(receipt)) == 0);
    ResetFaults();
    CHECK(BsgTransferPartII(&sIo, &sReader, &policy, &sWorkspace, &imported) == BSG_ALREADY_IMPORTED);
    CHECK(sFlash.erased == 0 && sFlash.programmed == 0);
    {
        const BsgSection *roster = BsgFindSection(&selected, BSG_SECTION_ROSTER);
        CHECK(roster != NULL);
        SetLogical16(selected.slot, roster->offset + BSG_ROSTER_HEADER_SIZE + 2, 0);
        RepairSectionIntegrity(selected.slot, &selected, BSG_SECTION_ROSTER);
        CHECK(BsgTransferPartII(&sIo, &sReader, &policy, &sWorkspace, &imported) == BSG_BAD_REFERENCE);
        CHECK(sFlash.erased == 0 && sFlash.programmed == 0);
    }
}

int main(void)
{
    RUN(TestCrc32c);
    RUN(TestCreatureGolden);
    RUN(TestPlayerGolden);
    RUN(TestRegistryRejection);
    RUN(TestCapacityAndSerials);
    RUN(TestRosterReferences);
    RUN(TestSnapshotRoundTripAndNewest);
    RUN(TestCorruptionAndRecovery);
    RUN(TestMalformedRanges);
    RUN(TestUnsupportedNewest);
    RUN(TestSectionCompatibility);
    RUN(TestLineageAndGeneration);
    RUN(TestLegacyGuard);
    RUN(TestWritePreflight);
    RUN(TestTornWrites);
    RUN(TestSyntheticSagaTransfer);
    RUN(TestFoundationProfile);
    RUN(TestFoundationTransferAndRetry);
    printf("%u tests, %u assertions passed\n", sTests, sAssertions);
    return EXIT_SUCCESS;
}
