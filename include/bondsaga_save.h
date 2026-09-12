#ifndef BONDSAGA_SAVE_H
#define BONDSAGA_SAVE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Explicit wire constants; neither these structs nor compiler enums are saved. */
#define BSG_FLASH_BYTES 131072u
#define BSG_SECTOR_BYTES 4096u
#define BSG_SECTORS_PER_SNAPSHOT 16u
#define BSG_SNAPSHOT_BYTES 65536u
#define BSG_PAYLOAD_BYTES 4032u
#define BSG_IMAGE_BYTES 64512u
#define BSG_IMAGE_HEADER_BYTES 128u
#define BSG_DIRECTORY_CAPACITY 16u
#define BSG_DIRECTORY_ENTRY_BYTES 32u
#define BSG_BODY_OFFSET 640u
#define BSG_BODY_BYTES 63872u
#define BSG_SCHEMA_MAJOR 1u
#define BSG_SCHEMA_MINOR 0u
#define BSG_REGISTRY_VERSION 1u
#define BSG_TRANSFER_CONTRACT 1u
#define BSG_CREATURE_PROFILE 1u
#define BSG_GAME_PART_I 1u
#define BSG_GAME_PART_II 2u
#define BSG_SCOPE_SHARED 1u
#define BSG_SCOPE_LOCAL 2u
#define BSG_REQUIRED_RESUME 4u
#define BSG_REQUIRED_TRANSFER 8u
#define BSG_PRESERVE 16u

enum BsgSectionId {
    BSG_SECTION_CREATURES = 1, BSG_SECTION_PLAYER = 2,
    BSG_SECTION_ROSTER = 3, BSG_SECTION_INVENTORY = 4,
    BSG_SECTION_RESEARCH = 5, BSG_SECTION_RIVALS = 6,
    BSG_SECTION_CONTRACTS = 7, BSG_SECTION_SAGA = 8,
    BSG_SECTION_LOCAL = 9, BSG_SECTION_EXTENSIONS = 10,
    BSG_SECTION_TRANSFER_RECEIPT = 11
};
#define BSG_TRANSFER_RECEIPT_BYTES 96u
typedef enum {
    BSG_OK, BSG_EMPTY, BSG_INVALID, BSG_IO_ERROR, BSG_UNSUPPORTED,
    BSG_AMBIGUOUS, BSG_CAPACITY, BSG_BAD_REFERENCE, BSG_BAD_ARGUMENT,
    BSG_NOT_ELIGIBLE, BSG_ALREADY_IMPORTED
} BsgResult;

typedef struct {
    void *context;
    bool (*read)(void *, uint32_t offset, uint8_t *out, uint32_t length);
    bool (*erase)(void *, uint16_t sector);
    /* Programs erased bytes (flash 1->0), never implicitly erases a sector. */
    bool (*program)(void *, uint32_t offset, const uint8_t *, uint32_t length);
} BsgIo;

typedef struct {
    uint16_t envelopeMajor, envelopeMinor, game, schemaMajor, schemaMinor;
    uint16_t minimumReaderMinor, creatureProfile, imageFlags;
    uint16_t transferContract, sourceGame;
    uint32_t requiredFeatures, registryVersion, rulesetVersion;
    uint32_t eligibility, profileFlags;
    uint64_t generation;
    uint8_t lineage[16], buildId[16];
} BsgMetadata;

typedef struct {
    uint32_t id;
    uint16_t major, minor;
    uint32_t flags, offset, length, crc, count;
} BsgSection;

typedef struct {
    BsgMetadata metadata;
    uint32_t imageCrc, usedExtent;
    uint16_t sectionCount;
    uint8_t slot;
    BsgSection sections[BSG_DIRECTORY_CAPACITY];
} BsgSnapshot;

typedef struct {
    uint16_t schemaMajor, readerMinor, creatureProfile;
    uint32_t gameMask, knownFeatures, registryVersion, rulesetVersion;
} BsgReader;

typedef struct BsgWorkspace BsgWorkspace;

typedef struct {
    BsgMetadata metadata;
    uint16_t sectionCount;
    BsgSection sections[BSG_DIRECTORY_CAPACITY];
    void *context;
    /* Caller freezes all logical state throughout preflight/write/readback. */
    bool (*readSection)(void *, uint32_t id, uint32_t offset, uint8_t *, uint32_t);
    BsgResult (*validateSnapshot)(const BsgIo *, const BsgSnapshot *, BsgWorkspace *, void *);
    void *validationContext;
} BsgImageSpec;

/* Allocate in EWRAM/heap, not on the GBA system stack. Internal descriptors
 * remain live across nested validation and migration passes. */
struct BsgWorkspace {
    _Alignas(uint64_t) uint8_t sector[BSG_SECTOR_BYTES];
    uint8_t scratch[128];
    BsgSnapshot selection, transaction, donor;
    BsgImageSpec migration;
};

uint32_t BsgCrc32cUpdate(uint32_t running, const uint8_t *, uint32_t length);
uint32_t BsgCrc32c(const uint8_t *, uint32_t length);
BsgReader BsgDefaultReader(uint32_t gameMask);
void BsgInitMetadata(BsgMetadata *, uint16_t game, const uint8_t lineage[16]);
BsgResult BsgReadLogical(const BsgIo *, uint8_t slot, uint32_t offset, uint8_t *, uint32_t);
const BsgSection *BsgFindSection(const BsgSnapshot *, uint32_t id);
BsgResult BsgReadSection(const BsgIo *, const BsgSnapshot *, uint32_t id, uint32_t offset, uint8_t *, uint32_t);
/* Structural/integrity validation deliberately precedes schema compatibility. */
BsgResult BsgValidateSnapshot(const BsgIo *, uint8_t slot, BsgWorkspace *, BsgSnapshot *);
BsgResult BsgCheckCompatibility(const BsgSnapshot *, const BsgReader *);
BsgResult BsgSelectSnapshot(const BsgIo *, const BsgReader *, BsgWorkspace *, BsgSnapshot *, bool *recovered);
/* Blank media may initialize at generation1; nonblank invalid media is never reset. */
BsgResult BsgWriteSnapshot(const BsgIo *, const BsgReader *, const BsgImageSpec *, BsgWorkspace *, BsgSnapshot *);
/* A signature probe, not a validator: incomplete containers are protected too. */
bool BsgLegacyAccessAllowed(const BsgIo *);

/* Synthetic migration policy: no built-in story eligibility or world initializer. */
typedef struct {
    void *context;
    bool (*eligible)(void *, const BsgMetadata *);
    bool (*readLocal)(void *, uint32_t offset, uint8_t *, uint32_t);
    uint32_t localLength, initializerVersion;
    uint8_t destinationBuildId[16]; /* Zero is an unassigned synthetic build. */
    BsgResult (*validateSnapshot)(const BsgIo *, const BsgSnapshot *, BsgWorkspace *, void *);
    void *validationContext;
} BsgTransferPolicy;
BsgResult BsgTransferPartII(const BsgIo *, const BsgReader *, const BsgTransferPolicy *, BsgWorkspace *, BsgSnapshot *);

#endif
