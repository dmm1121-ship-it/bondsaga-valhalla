#include "bondsaga_data.h"
#include <string.h>

_Static_assert(BSG_STORE_MAX_WIRE_SIZE == 43024, "Bondsaga store budget changed");
_Static_assert(BSG_ROSTER_HEADER_SIZE + BSG_OWNED_CAPACITY * 2 + BSG_EXPEDITION_CAPACITY * 2 + 2 + BSG_DEPLOYMENT_REFERENCE_CAPACITY * 2 + 2 == BSG_ROSTER_WIRE_SIZE, "Bondsaga roster budget changed");

uint16_t BsgReadU16(const uint8_t *src)
{
    return (uint16_t)((uint16_t)src[0] | ((uint16_t)src[1] << 8));
}

uint32_t BsgReadU32(const uint8_t *src)
{
    return (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

void BsgWriteU16(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)value;
    dst[1] = (uint8_t)(value >> 8);
}

void BsgWriteU32(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)value;
    dst[1] = (uint8_t)(value >> 8);
    dst[2] = (uint8_t)(value >> 16);
    dst[3] = (uint8_t)(value >> 24);
}

static bool IsZero(const uint8_t *src, size_t size)
{
    size_t i;
    for (i = 0; i < size; ++i)
        if (src[i] != 0)
            return false;
    return true;
}

static bool Contains(const struct BsgRegistry *registry, enum BsgRegistryKind kind, uint16_t id)
{
    return registry == NULL || (registry->contains != NULL && registry->contains(registry->context, kind, id));
}

static enum BsgDataResult ValidateCreature(const struct BsgCreature *src, const struct BsgRegistry *registry)
{
    size_t i;
    if (src->serial == 0)
        return BSG_DATA_SERIAL;
    if (src->nicknameLength > BSG_NICKNAME_BYTES || !IsZero(src->nickname + src->nicknameLength, BSG_NICKNAME_BYTES - src->nicknameLength))
        return BSG_DATA_NAME;
    if ((src->exceptionalFlags & ~BSG_CREATURE_EXCEPTIONAL_MASK) != 0 || !IsZero(src->reserved, sizeof(src->reserved)))
        return BSG_DATA_RESERVED;
    if (!Contains(registry, BSG_REGISTRY_SPECIES, src->species)
     || !Contains(registry, BSG_REGISTRY_LINEAGE, src->lineage)
     || !Contains(registry, BSG_REGISTRY_ORIGIN, src->origin)
     || !Contains(registry, BSG_REGISTRY_LOCATION, src->acquisitionLocation)
     || !Contains(registry, BSG_REGISTRY_APPEARANCE, src->appearance)
     || !Contains(registry, BSG_REGISTRY_PERSONALITY, src->personality)
     || !Contains(registry, BSG_REGISTRY_RECOVERY, src->recoveryProfile))
        return BSG_DATA_REGISTRY;
    for (i = 0; i < BSG_MOVE_SLOTS; ++i)
        if (!Contains(registry, BSG_REGISTRY_MOVE, src->moves[i]))
            return BSG_DATA_REGISTRY;
    for (i = 0; i < BSG_TRAIT_SLOTS; ++i)
        if (!Contains(registry, BSG_REGISTRY_TRAIT, src->traits[i]))
            return BSG_DATA_REGISTRY;
    return BSG_DATA_OK;
}

enum BsgDataResult BsgCreatureEncode(uint8_t *dst, size_t size, const struct BsgCreature *src, const struct BsgRegistry *registry)
{
    size_t i;
    enum BsgDataResult result;
    if (dst == NULL || src == NULL)
        return BSG_DATA_ARGUMENT;
    if (size != BSG_CREATURE_WIRE_SIZE)
        return BSG_DATA_SIZE;
    result = ValidateCreature(src, registry);
    if (result != BSG_DATA_OK)
        return result;
    BsgWriteU32(dst, src->serial);
    BsgWriteU16(dst + 4, src->species);
    BsgWriteU16(dst + 6, src->lineage);
    BsgWriteU16(dst + 8, src->origin);
    BsgWriteU16(dst + 10, src->acquisitionLocation);
    BsgWriteU32(dst + 12, src->totalExp);
    BsgWriteU16(dst + 16, src->earnedLevel);
    BsgWriteU16(dst + 18, src->acquisitionLevel);
    BsgWriteU16(dst + 20, src->currentHp);
    BsgWriteU16(dst + 22, src->condition);
    BsgWriteU16(dst + 24, src->appearance);
    BsgWriteU16(dst + 26, src->personality);
    BsgWriteU16(dst + 28, (uint16_t)(src->exceptionalFlags | ((uint16_t)src->nicknameLength << 8)));
    BsgWriteU16(dst + 30, src->recoveryProfile);
    BsgWriteU32(dst + 32, src->recoveryProgress);
    BsgWriteU32(dst + 36, src->bondProgress);
    memcpy(dst + 40, src->nickname, BSG_NICKNAME_BYTES);
    for (i = 0; i < BSG_MOVE_SLOTS; ++i)
        BsgWriteU16(dst + 52 + i * 2, src->moves[i]);
    memcpy(dst + 60, src->moveResources, BSG_MOVE_SLOTS);
    memcpy(dst + 64, src->aptitudes, BSG_APTITUDE_SLOTS);
    for (i = 0; i < BSG_APTITUDE_SLOTS; ++i)
        BsgWriteU16(dst + 70 + i * 2, src->development[i]);
    for (i = 0; i < BSG_TRAIT_SLOTS; ++i)
        BsgWriteU16(dst + 82 + i * 2, src->traits[i]);
    BsgWriteU16(dst + 90, src->lineageMilestones);
    memcpy(dst + 92, src->awakenedOutcomes, 8);
    BsgWriteU32(dst + 100, src->history[0]);
    BsgWriteU32(dst + 104, src->history[1]);
    memset(dst + 108, 0, 4);
    return BSG_DATA_OK;
}

enum BsgDataResult BsgCreatureDecode(struct BsgCreature *dst, const uint8_t *src, size_t size, const struct BsgRegistry *registry)
{
    struct BsgCreature value;
    enum BsgDataResult result;
    uint16_t flags;
    size_t i;
    if (dst == NULL || src == NULL)
        return BSG_DATA_ARGUMENT;
    if (size != BSG_CREATURE_WIRE_SIZE)
        return BSG_DATA_SIZE;
    flags = BsgReadU16(src + 28);
    if ((flags & ~0x0F07u) != 0 || !IsZero(src + 108, 4))
        return BSG_DATA_RESERVED;
    memset(&value, 0, sizeof(value));
    value.serial = BsgReadU32(src);
    value.species = BsgReadU16(src + 4);
    value.lineage = BsgReadU16(src + 6);
    value.origin = BsgReadU16(src + 8);
    value.acquisitionLocation = BsgReadU16(src + 10);
    value.totalExp = BsgReadU32(src + 12);
    value.earnedLevel = BsgReadU16(src + 16);
    value.acquisitionLevel = BsgReadU16(src + 18);
    value.currentHp = BsgReadU16(src + 20);
    value.condition = BsgReadU16(src + 22);
    value.appearance = BsgReadU16(src + 24);
    value.personality = BsgReadU16(src + 26);
    value.exceptionalFlags = (uint8_t)(flags & BSG_CREATURE_EXCEPTIONAL_MASK);
    value.nicknameLength = (uint8_t)((flags >> 8) & 0x0Fu);
    value.recoveryProfile = BsgReadU16(src + 30);
    value.recoveryProgress = BsgReadU32(src + 32);
    value.bondProgress = BsgReadU32(src + 36);
    memcpy(value.nickname, src + 40, BSG_NICKNAME_BYTES);
    for (i = 0; i < BSG_MOVE_SLOTS; ++i)
        value.moves[i] = BsgReadU16(src + 52 + i * 2);
    memcpy(value.moveResources, src + 60, BSG_MOVE_SLOTS);
    memcpy(value.aptitudes, src + 64, BSG_APTITUDE_SLOTS);
    for (i = 0; i < BSG_APTITUDE_SLOTS; ++i)
        value.development[i] = BsgReadU16(src + 70 + i * 2);
    for (i = 0; i < BSG_TRAIT_SLOTS; ++i)
        value.traits[i] = BsgReadU16(src + 82 + i * 2);
    value.lineageMilestones = BsgReadU16(src + 90);
    memcpy(value.awakenedOutcomes, src + 92, 8);
    value.history[0] = BsgReadU32(src + 100);
    value.history[1] = BsgReadU32(src + 104);
    result = ValidateCreature(&value, registry);
    if (result == BSG_DATA_OK)
        *dst = value;
    return result;
}

enum BsgDataResult BsgCreatureValidateWire(const uint8_t *src, size_t size, const struct BsgRegistry *registry)
{
    struct BsgCreature value;
    return BsgCreatureDecode(&value, src, size, registry);
}

static enum BsgDataResult ValidatePlayerBinder(const struct BsgPlayerBinder *src, const struct BsgRegistry *registry)
{
    size_t i;
    if (src->playerSerial == 0)
        return BSG_DATA_SERIAL;
    if (src->nameLength > BSG_PLAYER_NAME_BYTES || !IsZero(src->name + src->nameLength, BSG_PLAYER_NAME_BYTES - src->nameLength))
        return BSG_DATA_NAME;
    if (!IsZero(src->reservedProgression, sizeof(src->reservedProgression))
     || !IsZero(src->reservedAlignment, sizeof(src->reservedAlignment))
     || !IsZero(src->reserved, sizeof(src->reserved)))
        return BSG_DATA_RESERVED;
    if (!Contains(registry, BSG_REGISTRY_AVATAR, src->avatar)
     || !Contains(registry, BSG_REGISTRY_DIFFICULTY, src->difficulty)
     || !Contains(registry, BSG_REGISTRY_RECOVERY, src->recoveryProfile))
        return BSG_DATA_REGISTRY;
    for (i = 0; i < 4; ++i)
        if (!Contains(registry, BSG_REGISTRY_BINDER_TECHNIQUE, src->selectedTechniques[i]))
            return BSG_DATA_REGISTRY;
    return BSG_DATA_OK;
}

enum BsgDataResult BsgPlayerBinderEncode(uint8_t *dst, size_t size, const struct BsgPlayerBinder *src, const struct BsgRegistry *registry)
{
    enum BsgDataResult result;
    size_t i;
    if (dst == NULL || src == NULL)
        return BSG_DATA_ARGUMENT;
    if (size != BSG_PLAYER_BINDER_WIRE_SIZE)
        return BSG_DATA_SIZE;
    result = ValidatePlayerBinder(src, registry);
    if (result != BSG_DATA_OK)
        return result;
    BsgWriteU32(dst, src->playerSerial);
    memcpy(dst + 4, src->name, BSG_PLAYER_NAME_BYTES);
    dst[20] = src->nameLength;
    dst[21] = src->language;
    BsgWriteU16(dst + 22, src->avatar);
    memcpy(dst + 24, src->customization, 16);
    BsgWriteU32(dst + 40, src->options);
    BsgWriteU16(dst + 44, src->difficulty);
    BsgWriteU16(dst + 46, src->accessibility);
    BsgWriteU32(dst + 48, src->playtime);
    BsgWriteU32(dst + 52, src->currency);
    BsgWriteU32(dst + 56, src->rngContext);
    BsgWriteU32(dst + 60, src->nextSpecimenSerial);
    BsgWriteU32(dst + 64, src->binderProgress);
    BsgWriteU16(dst + 68, src->binderRank);
    memset(dst + 70, 0, 10);
    memcpy(dst + 80, src->techniqueMembership, 32);
    memcpy(dst + 112, src->capabilityMembership, 8);
    for (i = 0; i < BSG_ARCHETYPE_POSITIONS; ++i)
        BsgWriteU32(dst + 120 + i * 4, src->archetypeMastery[i]);
    memcpy(dst + 184, src->manifestationMilestones, 8);
    BsgWriteU16(dst + 192, src->currentHp);
    BsgWriteU16(dst + 194, src->condition);
    BsgWriteU16(dst + 196, src->recoveryProfile);
    for (i = 0; i < 4; ++i)
        BsgWriteU16(dst + 198 + i * 2, src->selectedTechniques[i]);
    memset(dst + 206, 0, 2);
    BsgWriteU32(dst + 208, src->recoveryProgress);
    memset(dst + 212, 0, 44);
    return BSG_DATA_OK;
}

enum BsgDataResult BsgPlayerBinderDecode(struct BsgPlayerBinder *dst, const uint8_t *src, size_t size, const struct BsgRegistry *registry)
{
    struct BsgPlayerBinder value;
    enum BsgDataResult result;
    size_t i;
    if (dst == NULL || src == NULL)
        return BSG_DATA_ARGUMENT;
    if (size != BSG_PLAYER_BINDER_WIRE_SIZE)
        return BSG_DATA_SIZE;
    if (!IsZero(src + 70, 10) || !IsZero(src + 206, 2) || !IsZero(src + 212, 44))
        return BSG_DATA_RESERVED;
    memset(&value, 0, sizeof(value));
    value.playerSerial = BsgReadU32(src);
    memcpy(value.name, src + 4, BSG_PLAYER_NAME_BYTES);
    value.nameLength = src[20];
    value.language = src[21];
    value.avatar = BsgReadU16(src + 22);
    memcpy(value.customization, src + 24, 16);
    value.options = BsgReadU32(src + 40);
    value.difficulty = BsgReadU16(src + 44);
    value.accessibility = BsgReadU16(src + 46);
    value.playtime = BsgReadU32(src + 48);
    value.currency = BsgReadU32(src + 52);
    value.rngContext = BsgReadU32(src + 56);
    value.nextSpecimenSerial = BsgReadU32(src + 60);
    value.binderProgress = BsgReadU32(src + 64);
    value.binderRank = BsgReadU16(src + 68);
    memcpy(value.techniqueMembership, src + 80, 32);
    memcpy(value.capabilityMembership, src + 112, 8);
    for (i = 0; i < BSG_ARCHETYPE_POSITIONS; ++i)
        value.archetypeMastery[i] = BsgReadU32(src + 120 + i * 4);
    memcpy(value.manifestationMilestones, src + 184, 8);
    value.currentHp = BsgReadU16(src + 192);
    value.condition = BsgReadU16(src + 194);
    value.recoveryProfile = BsgReadU16(src + 196);
    for (i = 0; i < 4; ++i)
        value.selectedTechniques[i] = BsgReadU16(src + 198 + i * 2);
    value.recoveryProgress = BsgReadU32(src + 208);
    result = ValidatePlayerBinder(&value, registry);
    if (result == BSG_DATA_OK)
        *dst = value;
    return result;
}

void BsgCreatureStoreInitialize(struct BsgCreatureStore *store)
{
    if (store != NULL)
        memset(store, 0, sizeof(*store));
}

size_t BsgCreatureStoreWireSize(uint16_t count)
{
    return count <= BSG_OWNED_CAPACITY ? BSG_STORE_HEADER_SIZE + (size_t)count * BSG_CREATURE_WIRE_SIZE : 0;
}

enum BsgDataResult BsgStoreEncodeWireHeader(uint8_t *dst, size_t size, uint16_t count)
{
    if (dst == NULL)
        return BSG_DATA_ARGUMENT;
    if (size != BSG_STORE_HEADER_SIZE)
        return BSG_DATA_SIZE;
    if (count > BSG_OWNED_CAPACITY)
        return BSG_DATA_COUNT;
    memcpy(dst, "BSGC", 4);
    BsgWriteU16(dst + 4, BSG_DATA_SECTION_VERSION);
    BsgWriteU16(dst + 6, BSG_CREATURE_WIRE_SIZE);
    BsgWriteU16(dst + 8, count);
    BsgWriteU16(dst + 10, BSG_OWNED_CAPACITY);
    memset(dst + 12, 0, 4);
    return BSG_DATA_OK;
}

enum BsgDataResult BsgStoreValidateWireHeader(const uint8_t *src, size_t size, size_t sectionSize, uint16_t *count)
{
    uint16_t records;
    if (src == NULL || count == NULL)
        return BSG_DATA_ARGUMENT;
    if (size != BSG_STORE_HEADER_SIZE)
        return BSG_DATA_SIZE;
    if (memcmp(src, "BSGC", 4) != 0 || BsgReadU16(src + 4) != BSG_DATA_SECTION_VERSION
     || BsgReadU16(src + 6) != BSG_CREATURE_WIRE_SIZE || BsgReadU16(src + 10) != BSG_OWNED_CAPACITY)
        return BSG_DATA_VERSION;
    if (!IsZero(src + 12, 4))
        return BSG_DATA_RESERVED;
    records = BsgReadU16(src + 8);
    if (records > BSG_OWNED_CAPACITY)
        return BSG_DATA_COUNT;
    if (sectionSize != BsgCreatureStoreWireSize(records))
        return BSG_DATA_SIZE;
    *count = records;
    return BSG_DATA_OK;
}

enum BsgDataResult BsgValidateSerialReferences(const uint32_t *serials, uint16_t count, uint32_t nextSerial)
{
    uint16_t i, j;
    if (count > BSG_OWNED_CAPACITY)
        return BSG_DATA_COUNT;
    if (serials == NULL && count != 0)
        return BSG_DATA_ARGUMENT;
    for (i = 0; i < count; ++i)
    {
        if (serials[i] == 0 || (nextSerial != 0 && serials[i] >= nextSerial))
            return BSG_DATA_SERIAL;
        for (j = 0; j < i; ++j)
            if (serials[i] == serials[j])
                return BSG_DATA_DUPLICATE;
    }
    return BSG_DATA_OK;
}

static enum BsgDataResult ValidateRecords(const uint8_t *records, uint16_t count, uint32_t nextSerial, const struct BsgRegistry *registry)
{
    enum BsgDataResult result;
    uint16_t i, j;
    if (count > BSG_OWNED_CAPACITY)
        return BSG_DATA_COUNT;
    for (i = 0; i < count; ++i)
    {
        const uint8_t *record = records + (size_t)i * BSG_CREATURE_WIRE_SIZE;
        result = BsgCreatureValidateWire(record, BSG_CREATURE_WIRE_SIZE, registry);
        if (result != BSG_DATA_OK)
            return result;
        uint32_t serial = BsgReadU32(record);
        if (nextSerial != 0 && serial >= nextSerial)
            return BSG_DATA_SERIAL;
        for (j = 0; j < i; ++j)
            if (serial == BsgReadU32(records + (size_t)j * BSG_CREATURE_WIRE_SIZE))
                return BSG_DATA_DUPLICATE;
    }
    return BSG_DATA_OK;
}

enum BsgDataResult BsgCreatureStoreValidate(const struct BsgCreatureStore *store, uint32_t nextSerial, const struct BsgRegistry *registry)
{
    if (store == NULL)
        return BSG_DATA_ARGUMENT;
    if (store->count > BSG_OWNED_CAPACITY)
        return BSG_DATA_COUNT;
    if (!IsZero((const uint8_t *)store->records + (size_t)store->count * BSG_CREATURE_WIRE_SIZE,
                (BSG_OWNED_CAPACITY - store->count) * BSG_CREATURE_WIRE_SIZE))
        return BSG_DATA_RESERVED;
    return ValidateRecords((const uint8_t *)store->records, store->count, nextSerial, registry);
}

enum BsgDataResult BsgCreatureStoreEncode(uint8_t *dst, size_t size, const struct BsgCreatureStore *src, uint32_t nextSerial, const struct BsgRegistry *registry)
{
    enum BsgDataResult result;
    if (dst == NULL || src == NULL)
        return BSG_DATA_ARGUMENT;
    if (src->count > BSG_OWNED_CAPACITY)
        return BSG_DATA_COUNT;
    if (size != BsgCreatureStoreWireSize(src->count))
        return BSG_DATA_SIZE;
    result = BsgCreatureStoreValidate(src, nextSerial, registry);
    if (result != BSG_DATA_OK)
        return result;
    BsgStoreEncodeWireHeader(dst, BSG_STORE_HEADER_SIZE, src->count);
    memcpy(dst + BSG_STORE_HEADER_SIZE, src->records, size - BSG_STORE_HEADER_SIZE);
    return BSG_DATA_OK;
}

enum BsgDataResult BsgCreatureStoreDecode(struct BsgCreatureStore *dst, const uint8_t *src, size_t size, uint32_t nextSerial, const struct BsgRegistry *registry)
{
    enum BsgDataResult result;
    uint16_t count;
    if (dst == NULL || src == NULL)
        return BSG_DATA_ARGUMENT;
    if (size < BSG_STORE_HEADER_SIZE)
        return BSG_DATA_SIZE;
    result = BsgStoreValidateWireHeader(src, BSG_STORE_HEADER_SIZE, size, &count);
    if (result != BSG_DATA_OK)
        return result;
    result = ValidateRecords(src + BSG_STORE_HEADER_SIZE, count, nextSerial, registry);
    if (result != BSG_DATA_OK)
        return result;
    BsgCreatureStoreInitialize(dst);
    dst->count = count;
    memcpy(dst->records, src + BSG_STORE_HEADER_SIZE, size - BSG_STORE_HEADER_SIZE);
    return BSG_DATA_OK;
}

enum BsgDataResult BsgCreatureStoreAppend(struct BsgCreatureStore *store, const struct BsgCreature *creature, uint32_t nextSerial, const struct BsgRegistry *registry)
{
    uint8_t record[BSG_CREATURE_WIRE_SIZE];
    uint16_t i;
    enum BsgDataResult result;
    if (store == NULL || creature == NULL)
        return BSG_DATA_ARGUMENT;
    if (store->count >= BSG_OWNED_CAPACITY)
        return BSG_DATA_COUNT;
    result = BsgCreatureStoreValidate(store, nextSerial, registry);
    if (result != BSG_DATA_OK)
        return result;
    result = BsgCreatureEncode(record, sizeof(record), creature, registry);
    if (result != BSG_DATA_OK)
        return result;
    if (nextSerial != 0 && creature->serial >= nextSerial)
        return BSG_DATA_SERIAL;
    for (i = 0; i < store->count; ++i)
        if (BsgReadU32(store->records[i]) == creature->serial)
            return BSG_DATA_DUPLICATE;
    memcpy(store->records[store->count], record, sizeof(record));
    ++store->count;
    return BSG_DATA_OK;
}

void BsgRosterInitialize(struct BsgRoster *roster)
{
    if (roster != NULL)
        memset(roster, 0xFF, sizeof(*roster));
}

static uint16_t PlacementAt(const struct BsgRoster *roster, const uint8_t *wire, size_t i)
{
    return roster != NULL ? roster->placement[i] : BsgReadU16(wire + 16 + i * 2);
}

static uint16_t ExpeditionAt(const struct BsgRoster *roster, const uint8_t *wire, size_t i)
{
    return roster != NULL ? roster->expedition[i] : BsgReadU16(wire + 784 + i * 2);
}

static uint16_t DeploymentAt(const struct BsgRoster *roster, const uint8_t *wire, size_t i)
{
    return roster != NULL ? roster->deployed[i] : BsgReadU16(wire + 802 + i * 2);
}

static bool InExpedition(const struct BsgRoster *roster, const uint8_t *wire, uint16_t reference)
{
    size_t i;
    for (i = 0; i < BSG_EXPEDITION_CAPACITY; ++i)
        if (ExpeditionAt(roster, wire, i) == reference)
            return true;
    return false;
}

static enum BsgDataResult ValidateRosterReferences(const struct BsgRoster *roster, const uint8_t *wire, uint16_t ownedCount)
{
    uint8_t seen[(BSG_OWNED_CAPACITY + 7) / 8] = {0};
    uint16_t found = 0;
    uint16_t bound = roster != NULL ? roster->bound : BsgReadU16(wire + 800);
    size_t i, j;
    if (ownedCount > BSG_OWNED_CAPACITY)
        return BSG_DATA_COUNT;
    for (i = 0; i < BSG_OWNED_CAPACITY; ++i)
    {
        uint16_t record = PlacementAt(roster, wire, i);
        uint8_t mask;
        if (record == BSG_RECORD_NONE)
            continue;
        if (record >= ownedCount)
            return BSG_DATA_REFERENCE;
        mask = (uint8_t)(1u << (record % 8));
        if ((seen[record / 8] & mask) != 0)
            return BSG_DATA_DUPLICATE;
        seen[record / 8] |= mask;
        ++found;
    }
    if (found != ownedCount)
        return BSG_DATA_REFERENCE;
    for (i = 0; i < BSG_EXPEDITION_CAPACITY; ++i)
    {
        uint16_t record = ExpeditionAt(roster, wire, i);
        if (record == BSG_RECORD_NONE)
            continue;
        if (record >= ownedCount)
            return BSG_DATA_REFERENCE;
        for (j = 0; j < i; ++j)
            if (record == ExpeditionAt(roster, wire, j))
                return BSG_DATA_DUPLICATE;
    }
    if (bound != BSG_RECORD_NONE && !InExpedition(roster, wire, bound))
        return BSG_DATA_REFERENCE;
    for (i = 0; i < BSG_DEPLOYMENT_REFERENCE_CAPACITY; ++i)
    {
        uint16_t record = DeploymentAt(roster, wire, i);
        if (record == BSG_RECORD_NONE)
            continue;
        if (!InExpedition(roster, wire, record) || record == bound)
            return BSG_DATA_REFERENCE;
        for (j = 0; j < i; ++j)
            if (record == DeploymentAt(roster, wire, j))
                return BSG_DATA_DUPLICATE;
    }
    return BSG_DATA_OK;
}

enum BsgDataResult BsgRosterValidate(const struct BsgRoster *roster, uint16_t ownedCount)
{
    if (roster == NULL)
        return BSG_DATA_ARGUMENT;
    return ValidateRosterReferences(roster, NULL, ownedCount);
}

enum BsgDataResult BsgRosterEncode(uint8_t *dst, size_t size, const struct BsgRoster *src, uint16_t ownedCount)
{
    enum BsgDataResult result;
    size_t i;
    if (dst == NULL || src == NULL)
        return BSG_DATA_ARGUMENT;
    if (size != BSG_ROSTER_WIRE_SIZE)
        return BSG_DATA_SIZE;
    result = BsgRosterValidate(src, ownedCount);
    if (result != BSG_DATA_OK)
        return result;
    memcpy(dst, "BSGR", 4);
    BsgWriteU16(dst + 4, BSG_DATA_SECTION_VERSION);
    BsgWriteU16(dst + 6, BSG_OWNED_CAPACITY);
    BsgWriteU16(dst + 8, BSG_EXPEDITION_CAPACITY);
    BsgWriteU16(dst + 10, BSG_DEPLOYMENT_REFERENCE_CAPACITY);
    memset(dst + 12, 0, 4);
    for (i = 0; i < BSG_OWNED_CAPACITY; ++i)
        BsgWriteU16(dst + BSG_ROSTER_HEADER_SIZE + i * 2, src->placement[i]);
    for (i = 0; i < BSG_EXPEDITION_CAPACITY; ++i)
        BsgWriteU16(dst + 784 + i * 2, src->expedition[i]);
    BsgWriteU16(dst + 800, src->bound);
    for (i = 0; i < BSG_DEPLOYMENT_REFERENCE_CAPACITY; ++i)
        BsgWriteU16(dst + 802 + i * 2, src->deployed[i]);
    memset(dst + 818, 0, 2);
    return BSG_DATA_OK;
}

enum BsgDataResult BsgRosterValidateWire(const uint8_t *src, size_t size, uint16_t ownedCount)
{
    if (src == NULL)
        return BSG_DATA_ARGUMENT;
    if (size != BSG_ROSTER_WIRE_SIZE)
        return BSG_DATA_SIZE;
    if (memcmp(src, "BSGR", 4) != 0 || BsgReadU16(src + 4) != BSG_DATA_SECTION_VERSION
     || BsgReadU16(src + 6) != BSG_OWNED_CAPACITY || BsgReadU16(src + 8) != BSG_EXPEDITION_CAPACITY
     || BsgReadU16(src + 10) != BSG_DEPLOYMENT_REFERENCE_CAPACITY)
        return BSG_DATA_VERSION;
    if (!IsZero(src + 12, 4) || !IsZero(src + 818, 2))
        return BSG_DATA_RESERVED;
    return ValidateRosterReferences(NULL, src, ownedCount);
}

enum BsgDataResult BsgRosterDecode(struct BsgRoster *dst, const uint8_t *src, size_t size, uint16_t ownedCount)
{
    enum BsgDataResult result;
    size_t i;
    if (dst == NULL)
        return BSG_DATA_ARGUMENT;
    result = BsgRosterValidateWire(src, size, ownedCount);
    if (result != BSG_DATA_OK)
        return result;
    for (i = 0; i < BSG_OWNED_CAPACITY; ++i)
        dst->placement[i] = BsgReadU16(src + BSG_ROSTER_HEADER_SIZE + i * 2);
    for (i = 0; i < BSG_EXPEDITION_CAPACITY; ++i)
        dst->expedition[i] = BsgReadU16(src + 784 + i * 2);
    dst->bound = BsgReadU16(src + 800);
    for (i = 0; i < BSG_DEPLOYMENT_REFERENCE_CAPACITY; ++i)
        dst->deployed[i] = BsgReadU16(src + 802 + i * 2);
    return BSG_DATA_OK;
}
