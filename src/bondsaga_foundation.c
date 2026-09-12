#include "bondsaga_foundation.h"
#include <string.h>

_Static_assert(BSG_STORE_MAX_WIRE_SIZE + BSG_PLAYER_BINDER_WIRE_SIZE + BSG_ROSTER_SECTION_BYTES
             + BSG_INVENTORY_SECTION_BYTES + BSG_RESEARCH_SECTION_BYTES + BSG_RIVALS_SECTION_BYTES
             + BSG_CONTRACTS_SECTION_BYTES + BSG_SAGA_SECTION_BYTES + BSG_LOCAL_SECTION_BYTES
             + BSG_EXTENSIONS_SECTION_BYTES + BSG_TRANSFER_RECEIPT_BYTES == BSG_FOUNDATION_MAX_BODY_BYTES, "Bondsaga future-state budget changed");
_Static_assert(BSG_FOUNDATION_RESERVE_BYTES >= 4096, "Bondsaga planning reserve exhausted");
_Static_assert(BSG_ROSTER_WIRE_SIZE <= BSG_ROSTER_SECTION_BYTES, "Bondsaga roster exceeds its envelope");

static uint32_t SectionCapacity(uint32_t id)
{
    switch (id)
    {
    case BSG_SECTION_CREATURES: return BSG_STORE_MAX_WIRE_SIZE;
    case BSG_SECTION_PLAYER: return BSG_PLAYER_BINDER_WIRE_SIZE;
    case BSG_SECTION_ROSTER: return BSG_ROSTER_SECTION_BYTES;
    case BSG_SECTION_INVENTORY: return BSG_INVENTORY_SECTION_BYTES;
    case BSG_SECTION_RESEARCH: return BSG_RESEARCH_SECTION_BYTES;
    case BSG_SECTION_RIVALS: return BSG_RIVALS_SECTION_BYTES;
    case BSG_SECTION_CONTRACTS: return BSG_CONTRACTS_SECTION_BYTES;
    case BSG_SECTION_SAGA: return BSG_SAGA_SECTION_BYTES;
    case BSG_SECTION_LOCAL: return BSG_LOCAL_SECTION_BYTES;
    case BSG_SECTION_EXTENSIONS: return BSG_EXTENSIONS_SECTION_BYTES;
    case BSG_SECTION_TRANSFER_RECEIPT: return BSG_TRANSFER_RECEIPT_BYTES;
    default: return 0;
    }
}

static uint32_t SectionFlags(uint32_t id)
{
    if (id == BSG_SECTION_LOCAL)
        return BSG_SCOPE_LOCAL | BSG_REQUIRED_RESUME | BSG_PRESERVE;
    return BSG_SCOPE_SHARED | BSG_REQUIRED_RESUME | BSG_REQUIRED_TRANSFER | BSG_PRESERVE;
}

static BsgResult DataResult(enum BsgDataResult result)
{
    switch (result)
    {
    case BSG_DATA_OK: return BSG_OK;
    case BSG_DATA_ARGUMENT: return BSG_BAD_ARGUMENT;
    case BSG_DATA_COUNT: return BSG_CAPACITY;
    case BSG_DATA_VERSION: case BSG_DATA_REGISTRY: return BSG_UNSUPPORTED;
    case BSG_DATA_SERIAL: case BSG_DATA_REFERENCE: case BSG_DATA_DUPLICATE: return BSG_BAD_REFERENCE;
    default: return BSG_INVALID;
    }
}

static bool CopyPadded(uint8_t *out, uint32_t offset, uint32_t length, const uint8_t *data, uint32_t dataLength, uint32_t capacity)
{
    uint32_t copy = 0;
    if (offset > capacity || length > capacity - offset)
        return false;
    if (offset < dataLength)
    {
        copy = dataLength - offset;
        if (copy > length)
            copy = length;
        memcpy(out, data + offset, copy);
    }
    memset(out + copy, 0, length - copy);
    return true;
}

static bool ReadFoundationSection(void *context, uint32_t id, uint32_t offset, uint8_t *out, uint32_t length)
{
    const struct BsgFoundationSource *source = context;
    uint8_t wire[BSG_ROSTER_WIRE_SIZE];
    uint32_t capacity = SectionCapacity(id);
    if (id == BSG_SECTION_CREATURES)
    {
        uint32_t copy = 0;
        capacity = (uint32_t)BsgCreatureStoreWireSize(source->store->count);
        if (offset > capacity || length > capacity - offset)
            return false;
        if (BsgStoreEncodeWireHeader(wire, BSG_STORE_HEADER_SIZE, source->store->count) != BSG_DATA_OK)
            return false;
        if (offset < BSG_STORE_HEADER_SIZE)
        {
            copy = BSG_STORE_HEADER_SIZE - offset;
            if (copy > length)
                copy = length;
            memcpy(out, wire + offset, copy);
        }
        if (copy < length)
            memcpy(out + copy, (const uint8_t *)source->store->records + offset + copy - BSG_STORE_HEADER_SIZE, length - copy);
        return true;
    }
    if (id == BSG_SECTION_PLAYER)
    {
        if (BsgPlayerBinderEncode(wire, BSG_PLAYER_BINDER_WIRE_SIZE, source->player, source->registry) != BSG_DATA_OK)
            return false;
        return CopyPadded(out, offset, length, wire, BSG_PLAYER_BINDER_WIRE_SIZE, capacity);
    }
    if (id == BSG_SECTION_ROSTER)
    {
        if (BsgRosterEncode(wire, BSG_ROSTER_WIRE_SIZE, source->roster, source->store->count) != BSG_DATA_OK)
            return false;
        return CopyPadded(out, offset, length, wire, BSG_ROSTER_WIRE_SIZE, capacity);
    }
    if (id >= BSG_SECTION_INVENTORY && id <= BSG_SECTION_EXTENSIONS)
    {
        const struct BsgFoundationBlob *blob = &source->opaque[id - BSG_SECTION_INVENTORY];
        return CopyPadded(out, offset, length, blob->data, blob->length, capacity);
    }
    if (id == BSG_SECTION_TRANSFER_RECEIPT)
        return CopyPadded(out, offset, length, source->transferReceipt.data, source->transferReceipt.length, capacity);
    return false;
}

static BsgResult ValidateSource(const struct BsgFoundationSource *source)
{
    uint8_t wire[BSG_PLAYER_BINDER_WIRE_SIZE];
    enum BsgDataResult result;
    uint32_t i;
    if (source == NULL || source->store == NULL || source->player == NULL || source->roster == NULL)
        return BSG_BAD_ARGUMENT;
    result = BsgPlayerBinderEncode(wire, sizeof(wire), source->player, source->registry);
    if (result != BSG_DATA_OK)
        return DataResult(result);
    result = BsgCreatureStoreValidate(source->store, source->player->nextSpecimenSerial, source->registry);
    if (result != BSG_DATA_OK)
        return DataResult(result);
    result = BsgRosterValidate(source->roster, source->store->count);
    if (result != BSG_DATA_OK)
        return DataResult(result);
    for (i = 0; i < BSG_FOUNDATION_OPAQUE_COUNT; ++i)
    {
        const struct BsgFoundationBlob *blob = &source->opaque[i];
        if (blob->length > SectionCapacity(i + BSG_SECTION_INVENTORY))
            return BSG_CAPACITY;
        if (blob->length != 0 && blob->data == NULL)
            return BSG_BAD_ARGUMENT;
    }
    if (source->transferReceipt.length != 0 && source->transferReceipt.length != BSG_TRANSFER_RECEIPT_BYTES)
        return BSG_INVALID;
    if (source->transferReceipt.length != 0)
    {
        if (source->transferReceipt.data == NULL)
            return BSG_BAD_ARGUMENT;
        for (i = 68; i < BSG_TRANSFER_RECEIPT_BYTES; ++i)
            if (source->transferReceipt.data[i] != 0)
                return BSG_INVALID;
    }
    return BSG_OK;
}

BsgResult BsgBuildFoundationSpec(BsgImageSpec *out, const BsgMetadata *metadata, const struct BsgFoundationSource *source)
{
    BsgMetadata savedMetadata;
    BsgResult result;
    uint32_t i;
    if (out == NULL || metadata == NULL)
        return BSG_BAD_ARGUMENT;
    if (metadata->creatureProfile != BSG_CREATURE_PROFILE)
        return BSG_UNSUPPORTED;
    result = ValidateSource(source);
    if (result != BSG_OK)
        return result;
    savedMetadata = *metadata;
    memset(out, 0, sizeof(*out));
    out->metadata = savedMetadata;
    out->sectionCount = BSG_FOUNDATION_SECTION_COUNT;
    out->context = (void *)source;
    out->readSection = ReadFoundationSection;
    out->validateSnapshot = BsgValidateFoundationCallback;
    out->validationContext = (void *)source->registry;
    for (i = 0; i < BSG_FOUNDATION_SECTION_COUNT; ++i)
    {
        BsgSection *section = &out->sections[i];
        section->id = i + 1;
        section->major = BSG_DATA_SECTION_VERSION;
        section->flags = SectionFlags(section->id);
        section->length = SectionCapacity(section->id);
        if (section->id == BSG_SECTION_CREATURES)
        {
            section->length = (uint32_t)BsgCreatureStoreWireSize(source->store->count);
            section->count = source->store->count;
        }
        else if (section->id == BSG_SECTION_PLAYER)
            section->count = 1;
        else if (section->id == BSG_SECTION_ROSTER)
            section->count = BSG_OWNED_CAPACITY;
        else if (section->id == BSG_SECTION_TRANSFER_RECEIPT)
            section->count = 1;
    }
    return BSG_OK;
}

BsgResult BsgSaveFoundation(const BsgIo *io, const BsgReader *reader, const BsgMetadata *metadata, const struct BsgFoundationSource *source, BsgWorkspace *workspace, BsgSnapshot *out)
{
    BsgResult result;
    if (workspace == NULL)
        return BSG_BAD_ARGUMENT;
    result = BsgBuildFoundationSpec(&workspace->migration, metadata, source);
    if (result != BSG_OK)
        return result;
    return BsgWriteSnapshot(io, reader, &workspace->migration, workspace, out);
}

static BsgResult ValidatePlayer(const BsgIo *io, const BsgSnapshot *snapshot, BsgWorkspace *workspace, const struct BsgRegistry *registry, uint32_t *nextSerial)
{
    struct BsgPlayerBinder player;
    BsgResult result = BsgReadSection(io, snapshot, BSG_SECTION_PLAYER, 0, workspace->sector, BSG_PLAYER_BINDER_WIRE_SIZE);
    if (result != BSG_OK)
        return result;
    result = DataResult(BsgPlayerBinderDecode(&player, workspace->sector, BSG_PLAYER_BINDER_WIRE_SIZE, registry));
    if (result == BSG_OK)
        *nextSerial = player.nextSpecimenSerial;
    return result;
}

static BsgResult ValidateCreatures(const BsgIo *io, const BsgSnapshot *snapshot, BsgWorkspace *workspace, const struct BsgRegistry *registry, uint32_t nextSerial, uint16_t *ownedCount)
{
    const BsgSection *section = BsgFindSection(snapshot, BSG_SECTION_CREATURES);
    uint16_t count, i, j;
    BsgResult result = BsgReadSection(io, snapshot, BSG_SECTION_CREATURES, 0, workspace->scratch, BSG_STORE_HEADER_SIZE);
    if (result != BSG_OK)
        return result;
    result = DataResult(BsgStoreValidateWireHeader(workspace->scratch, BSG_STORE_HEADER_SIZE, section->length, &count));
    if (result != BSG_OK)
        return result;
    if (section->count != count)
        return BSG_INVALID;
    for (i = 0; i < count; ++i)
    {
        result = BsgReadSection(io, snapshot, BSG_SECTION_CREATURES, BSG_STORE_HEADER_SIZE + (uint32_t)i * BSG_CREATURE_WIRE_SIZE, workspace->scratch, BSG_CREATURE_WIRE_SIZE);
        if (result != BSG_OK)
            return result;
        result = DataResult(BsgCreatureValidateWire(workspace->scratch, BSG_CREATURE_WIRE_SIZE, registry));
        if (result != BSG_OK)
            return result;
        uint32_t serial = BsgReadU32(workspace->scratch);
        if (nextSerial != 0 && serial >= nextSerial)
            return BSG_BAD_REFERENCE;
        for (j = 0; j < i; ++j)
            if (serial == BsgReadU32(workspace->sector + (uint32_t)j * 4))
                return BSG_BAD_REFERENCE;
        BsgWriteU32(workspace->sector + (uint32_t)i * 4, serial);
    }
    *ownedCount = count;
    return BSG_OK;
}

static BsgResult ValidateRoster(const BsgIo *io, const BsgSnapshot *snapshot, BsgWorkspace *workspace, uint16_t ownedCount)
{
    uint32_t i;
    BsgResult result = BsgReadSection(io, snapshot, BSG_SECTION_ROSTER, 0, workspace->sector, BSG_ROSTER_SECTION_BYTES);
    if (result != BSG_OK)
        return result;
    for (i = BSG_ROSTER_WIRE_SIZE; i < BSG_ROSTER_SECTION_BYTES; ++i)
        if (workspace->sector[i] != 0)
            return BSG_INVALID;
    return DataResult(BsgRosterValidateWire(workspace->sector, BSG_ROSTER_WIRE_SIZE, ownedCount));
}

BsgResult BsgValidateFoundation(const BsgIo *io, const BsgSnapshot *snapshot, BsgWorkspace *workspace, const struct BsgRegistry *registry)
{
    uint32_t id, nextSerial;
    uint16_t count;
    BsgResult result;
    if (io == NULL || snapshot == NULL || workspace == NULL)
        return BSG_BAD_ARGUMENT;
    if (snapshot->metadata.creatureProfile != BSG_CREATURE_PROFILE || snapshot->sectionCount != BSG_FOUNDATION_SECTION_COUNT)
        return BSG_UNSUPPORTED;
    for (id = BSG_SECTION_CREATURES; id <= BSG_SECTION_TRANSFER_RECEIPT; ++id)
    {
        const BsgSection *section = BsgFindSection(snapshot, id);
        uint32_t expectedCount = (id == BSG_SECTION_PLAYER || id == BSG_SECTION_TRANSFER_RECEIPT) ? 1 : (id == BSG_SECTION_ROSTER ? BSG_OWNED_CAPACITY : 0);
        if (section == NULL || section->major != BSG_DATA_SECTION_VERSION || section->minor != 0 || section->flags != SectionFlags(id))
            return BSG_UNSUPPORTED;
        if (id == BSG_SECTION_CREATURES)
        {
            if (section->count > BSG_OWNED_CAPACITY)
                return BSG_CAPACITY;
            if (section->length != BsgCreatureStoreWireSize((uint16_t)section->count))
                return BSG_INVALID;
        }
        else if (section->length != SectionCapacity(id) || section->count != expectedCount)
            return BSG_INVALID;
    }
    result = ValidatePlayer(io, snapshot, workspace, registry, &nextSerial);
    if (result != BSG_OK)
        return result;
    result = ValidateCreatures(io, snapshot, workspace, registry, nextSerial, &count);
    if (result != BSG_OK)
        return result;
    result = ValidateRoster(io, snapshot, workspace, count);
    if (result != BSG_OK)
        return result;
    result = BsgReadSection(io, snapshot, BSG_SECTION_TRANSFER_RECEIPT, 0, workspace->scratch, BSG_TRANSFER_RECEIPT_BYTES);
    if (result != BSG_OK)
        return result;
    for (id = 68; id < BSG_TRANSFER_RECEIPT_BYTES; ++id)
        if (workspace->scratch[id] != 0)
            return BSG_INVALID;
    return BSG_OK;
}

BsgResult BsgValidateFoundationCallback(const BsgIo *io, const BsgSnapshot *snapshot, BsgWorkspace *workspace, void *registry)
{
    return BsgValidateFoundation(io, snapshot, workspace, registry);
}

BsgResult BsgLoadFoundation(const BsgIo *io, const BsgReader *reader, BsgWorkspace *workspace, const struct BsgRegistry *registry, BsgSnapshot *out, bool *recovered)
{
    bool wasRecovered;
    BsgResult result;
    if (out == NULL || workspace == NULL)
        return BSG_BAD_ARGUMENT;
    result = BsgSelectSnapshot(io, reader, workspace, &workspace->donor, &wasRecovered);
    if (result != BSG_OK)
        return result;
    result = BsgValidateFoundation(io, &workspace->donor, workspace, registry);
    if (result == BSG_OK)
    {
        *out = workspace->donor;
        if (recovered != NULL)
            *recovered = wasRecovered;
    }
    return result;
}
