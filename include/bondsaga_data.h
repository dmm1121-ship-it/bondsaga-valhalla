#ifndef GUARD_BONDSAGA_DATA_H
#define GUARD_BONDSAGA_DATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Wire sizes are contracts; the layout of the C views below is not. */
#define BSG_CREATURE_WIRE_SIZE 112u
#define BSG_CREATURE_PROFILE_VERSION 1u
#define BSG_OWNED_CAPACITY 384u
#define BSG_STORE_HEADER_SIZE 16u
#define BSG_STORE_MAX_WIRE_SIZE (BSG_STORE_HEADER_SIZE + BSG_OWNED_CAPACITY * BSG_CREATURE_WIRE_SIZE)
#define BSG_PLAYER_BINDER_WIRE_SIZE 256u
#define BSG_DATA_SECTION_VERSION 1u
#define BSG_EXPEDITION_CAPACITY 8u
#define BSG_DEPLOYMENT_REFERENCE_CAPACITY 8u
#define BSG_ROSTER_HEADER_SIZE 16u
#define BSG_ROSTER_WIRE_SIZE 820u
#define BSG_RECORD_NONE UINT16_MAX
#define BSG_NICKNAME_BYTES 12u
#define BSG_PLAYER_NAME_BYTES 16u
#define BSG_APTITUDE_SLOTS 6u
#define BSG_TRAIT_SLOTS 4u
#define BSG_MOVE_SLOTS 4u
#define BSG_ARCHETYPE_POSITIONS 16u

#define BSG_CREATURE_ALPHA 0x01u
#define BSG_CREATURE_APEX 0x02u
#define BSG_CREATURE_VARIANT 0x04u
#define BSG_CREATURE_EXCEPTIONAL_MASK 0x07u

enum BsgDataResult
{
    BSG_DATA_OK,
    BSG_DATA_ARGUMENT,
    BSG_DATA_SIZE,
    BSG_DATA_VERSION,
    BSG_DATA_RESERVED,
    BSG_DATA_SERIAL,
    BSG_DATA_COUNT,
    BSG_DATA_REFERENCE,
    BSG_DATA_DUPLICATE,
    BSG_DATA_REGISTRY,
    BSG_DATA_NAME,
};

/* These are callback selectors, never serialized enum ordinals. */
enum BsgRegistryKind
{
    BSG_REGISTRY_SPECIES,
    BSG_REGISTRY_LINEAGE,
    BSG_REGISTRY_ORIGIN,
    BSG_REGISTRY_LOCATION,
    BSG_REGISTRY_APPEARANCE,
    BSG_REGISTRY_PERSONALITY,
    BSG_REGISTRY_RECOVERY,
    BSG_REGISTRY_MOVE,
    BSG_REGISTRY_TRAIT,
    BSG_REGISTRY_AVATAR,
    BSG_REGISTRY_DIFFICULTY,
    BSG_REGISTRY_BINDER_TECHNIQUE,
};

struct BsgRegistry
{
    bool (*contains)(void *context, enum BsgRegistryKind kind, uint16_t id);
    void *context;
};

/* Unresolved numerical fields preserve their full width; no gameplay caps,
 * formulas, trait activation, condition rules, or alphabet are selected here.
 * Name buffers are opaque single-byte codes. Their unused tail is zero.
 * Trait slots: innate/specimen, developed, Bond, exceptional (storage only).
 */
struct BsgCreature
{
    uint32_t serial;
    uint16_t species;
    uint16_t lineage;
    uint16_t origin;
    uint16_t acquisitionLocation;
    uint32_t totalExp;
    uint16_t earnedLevel;
    uint16_t acquisitionLevel;
    uint16_t currentHp;
    uint16_t condition;
    uint16_t appearance;
    uint16_t personality;
    uint8_t exceptionalFlags;
    uint8_t nicknameLength;
    uint16_t recoveryProfile;
    uint32_t recoveryProgress;
    uint32_t bondProgress;
    uint8_t nickname[BSG_NICKNAME_BYTES];
    uint16_t moves[BSG_MOVE_SLOTS];
    uint8_t moveResources[BSG_MOVE_SLOTS];
    uint8_t aptitudes[BSG_APTITUDE_SLOTS];
    uint16_t development[BSG_APTITUDE_SLOTS];
    uint16_t traits[BSG_TRAIT_SLOTS];
    uint16_t lineageMilestones;
    uint8_t awakenedOutcomes[8];
    uint32_t history[2];
    uint8_t reserved[4];
};

struct BsgPlayerBinder
{
    uint32_t playerSerial;
    uint8_t name[BSG_PLAYER_NAME_BYTES];
    uint8_t nameLength;
    uint8_t language;
    uint16_t avatar;
    uint8_t customization[16];
    uint32_t options;
    uint16_t difficulty;
    uint16_t accessibility;
    uint32_t playtime;
    uint32_t currency;
    uint32_t rngContext;
    /* Zero means the allocator is exhausted. It must never wrap or recycle. */
    uint32_t nextSpecimenSerial;
    uint32_t binderProgress;
    uint16_t binderRank;
    uint8_t reservedProgression[10];
    uint8_t techniqueMembership[32];
    uint8_t capabilityMembership[8];
    uint32_t archetypeMastery[BSG_ARCHETYPE_POSITIONS];
    uint8_t manifestationMilestones[8];
    uint16_t currentHp;
    uint16_t condition;
    uint16_t recoveryProfile;
    uint16_t selectedTechniques[4];
    uint8_t reservedAlignment[2];
    uint32_t recoveryProgress;
    uint8_t reserved[44];
};

/* The sole full creature store contains canonical bytes, not a second set of
 * decoded creatures. These objects are caller-owned; this module allocates no
 * persistent/global storage. Unused record bytes must be zero.
 */
struct BsgCreatureStore
{
    uint16_t count;
    uint8_t records[BSG_OWNED_CAPACITY][BSG_CREATURE_WIRE_SIZE];
};

/* Each nonempty placement owns one record; no facility/UX meaning is assigned
 * to its index by this foundation. Expedition selection is separate so owning
 * 384 creatures never forces eight expedition members. Expedition, Bound and
 * deployment entries are references, not additional ownership. An eight
 * reference envelope does not authorize eight simultaneously deployed battlers.
 */
struct BsgRoster
{
    uint16_t placement[BSG_OWNED_CAPACITY];
    uint16_t expedition[BSG_EXPEDITION_CAPACITY];
    uint16_t bound;
    uint16_t deployed[BSG_DEPLOYMENT_REFERENCE_CAPACITY];
};

uint16_t BsgReadU16(const uint8_t *src);
uint32_t BsgReadU32(const uint8_t *src);
void BsgWriteU16(uint8_t *dst, uint16_t value);
void BsgWriteU32(uint8_t *dst, uint32_t value);

/* A NULL registry requests structural validation only. A non-NULL registry
 * checks every ID, including zero; the supplied immutable registry determines
 * which namespaces admit an empty ID. Unknown IDs are never substituted.
 * Encode/decode destinations remain unchanged on failure. Buffers must not
 * overlap their source objects, and all source bytes must remain stable.
 */
enum BsgDataResult BsgCreatureEncode(uint8_t *dst, size_t size, const struct BsgCreature *src, const struct BsgRegistry *registry);
enum BsgDataResult BsgCreatureDecode(struct BsgCreature *dst, const uint8_t *src, size_t size, const struct BsgRegistry *registry);
enum BsgDataResult BsgCreatureValidateWire(const uint8_t *src, size_t size, const struct BsgRegistry *registry);
enum BsgDataResult BsgPlayerBinderEncode(uint8_t *dst, size_t size, const struct BsgPlayerBinder *src, const struct BsgRegistry *registry);
enum BsgDataResult BsgPlayerBinderDecode(struct BsgPlayerBinder *dst, const uint8_t *src, size_t size, const struct BsgRegistry *registry);

void BsgCreatureStoreInitialize(struct BsgCreatureStore *store);
size_t BsgCreatureStoreWireSize(uint16_t count);
enum BsgDataResult BsgStoreEncodeWireHeader(uint8_t *dst, size_t size, uint16_t count);
enum BsgDataResult BsgStoreValidateWireHeader(const uint8_t *src, size_t size, size_t sectionSize, uint16_t *count);
enum BsgDataResult BsgCreatureStoreValidate(const struct BsgCreatureStore *store, uint32_t nextSerial, const struct BsgRegistry *registry);
enum BsgDataResult BsgCreatureStoreEncode(uint8_t *dst, size_t size, const struct BsgCreatureStore *src, uint32_t nextSerial, const struct BsgRegistry *registry);
enum BsgDataResult BsgCreatureStoreDecode(struct BsgCreatureStore *dst, const uint8_t *src, size_t size, uint32_t nextSerial, const struct BsgRegistry *registry);
/* Appends an already assigned serial; acquisition/allocator gameplay is not
 * implemented. Failed append preserves the store. */
enum BsgDataResult BsgCreatureStoreAppend(struct BsgCreatureStore *store, const struct BsgCreature *creature, uint32_t nextSerial, const struct BsgRegistry *registry);

void BsgRosterInitialize(struct BsgRoster *roster);
enum BsgDataResult BsgRosterValidate(const struct BsgRoster *roster, uint16_t ownedCount);
enum BsgDataResult BsgRosterValidateWire(const uint8_t *src, size_t size, uint16_t ownedCount);
enum BsgDataResult BsgRosterEncode(uint8_t *dst, size_t size, const struct BsgRoster *src, uint16_t ownedCount);
enum BsgDataResult BsgRosterDecode(struct BsgRoster *dst, const uint8_t *src, size_t size, uint16_t ownedCount);
/* Shared with streaming readers: caller supplies previously validated serials.
 * Zero nextSerial is exhausted; otherwise it must exceed every owned serial. */
enum BsgDataResult BsgValidateSerialReferences(const uint32_t *serials, uint16_t count, uint32_t nextSerial);

#endif /* GUARD_BONDSAGA_DATA_H */
