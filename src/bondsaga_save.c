#include "bondsaga_save.h"
#include <string.h>

#define SECTOR_MAGIC 0x43455342u /* BSEC */
#define COMMIT_MAGIC 0x544d4342u /* BCMT */
#define CRC_OFFSET 0xfe0u
#define COMMIT_OFFSET 0xff0u
#define IMAGE_CRC_OFFSET 72u

static uint16_t Get16(const uint8_t *p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }
static uint32_t Get32(const uint8_t *p) { return (uint32_t)Get16(p) | (uint32_t)Get16(p + 2) << 16; }
static uint64_t Get64(const uint8_t *p) { return (uint64_t)Get32(p) | (uint64_t)Get32(p + 4) << 32; }
static void Put16(uint8_t *p, uint16_t n) { p[0] = n; p[1] = n >> 8; }
static void Put32(uint8_t *p, uint32_t n) { Put16(p, n); Put16(p + 2, n >> 16); }
static void Put64(uint8_t *p, uint64_t n) { Put32(p, n); Put32(p + 4, n >> 32); }
static bool Filled(const uint8_t *p, uint32_t n, uint8_t value)
{
    while (n--) if (*p++ != value) return false;
    return true;
}

uint32_t BsgCrc32cUpdate(uint32_t crc, const uint8_t *p, uint32_t n)
{
    while (n--) {
        unsigned bit;
        crc ^= *p++;
        for (bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ ((crc & 1) ? 0x82f63b78u : 0);
    }
    return crc;
}

uint32_t BsgCrc32c(const uint8_t *p, uint32_t n)
{
    return BsgCrc32cUpdate(UINT32_MAX, p, n) ^ UINT32_MAX;
}

BsgReader BsgDefaultReader(uint32_t games)
{
    BsgReader r = {BSG_SCHEMA_MAJOR, BSG_SCHEMA_MINOR, BSG_CREATURE_PROFILE,
                   games, 0, BSG_REGISTRY_VERSION, 1};
    return r;
}

void BsgInitMetadata(BsgMetadata *m, uint16_t game, const uint8_t lineage[16])
{
    memset(m, 0, sizeof(*m));
    m->envelopeMajor = 1;
    m->game = game;
    m->schemaMajor = BSG_SCHEMA_MAJOR;
    m->schemaMinor = BSG_SCHEMA_MINOR;
    m->creatureProfile = BSG_CREATURE_PROFILE;
    m->registryVersion = BSG_REGISTRY_VERSION;
    m->rulesetVersion = 1;
    m->transferContract = BSG_TRANSFER_CONTRACT;
    memcpy(m->lineage, lineage, 16);
}

bool BsgLegacyAccessAllowed(const BsgIo *io)
{
    uint8_t prefix[40];
    unsigned sector;
    if (!io || !io->read) return false;
    for (sector = 0; sector < 32; sector++) {
        if (!io->read(io->context, sector * BSG_SECTOR_BYTES, prefix, sizeof(prefix))) return false;
        if (!memcmp(prefix, "BSEC", 4) || !memcmp(prefix + 32, "BONDSAGA", 8)) return false;
    }
    return true;
}

BsgResult BsgReadLogical(const BsgIo *io, uint8_t slot, uint32_t off, uint8_t *out, uint32_t len)
{
    if (!io || !io->read || slot > 1 || !out || off > BSG_IMAGE_BYTES || len > BSG_IMAGE_BYTES - off)
        return BSG_BAD_ARGUMENT;
    while (len) {
        uint32_t within = off % BSG_PAYLOAD_BYTES;
        uint32_t n = BSG_PAYLOAD_BYTES - within;
        uint32_t physical = slot * BSG_SNAPSHOT_BYTES + off / BSG_PAYLOAD_BYTES * BSG_SECTOR_BYTES + 32 + within;
        if (n > len) n = len;
        if (!io->read(io->context, physical, out, n)) return BSG_IO_ERROR;
        off += n; out += n; len -= n;
    }
    return BSG_OK;
}

const BsgSection *BsgFindSection(const BsgSnapshot *s, uint32_t id)
{
    unsigned i;
    for (i = 0; i < s->sectionCount; i++) if (s->sections[i].id == id) return &s->sections[i];
    return NULL;
}

BsgResult BsgReadSection(const BsgIo *io, const BsgSnapshot *s, uint32_t id, uint32_t off, uint8_t *out, uint32_t len)
{
    const BsgSection *d = BsgFindSection(s, id);
    if (!d || off > d->length || len > d->length - off) return BSG_BAD_ARGUMENT;
    return BsgReadLogical(io, s->slot, d->offset + off, out, len);
}

static void EncodeHeader(uint8_t *p, const BsgSnapshot *s)
{
    const BsgMetadata *m = &s->metadata;
    memset(p, 0, BSG_BODY_OFFSET);
    memcpy(p, "BONDSAGA", 8);
    Put16(p + 8, m->envelopeMajor); Put16(p + 10, m->envelopeMinor);
    Put16(p + 12, 128); Put16(p + 14, 32);
    Put16(p + 16, 16); Put16(p + 18, s->sectionCount);
    Put16(p + 20, m->game); Put16(p + 22, m->schemaMajor);
    Put16(p + 24, m->schemaMinor); Put16(p + 26, m->minimumReaderMinor);
    Put32(p + 28, m->requiredFeatures); Put64(p + 32, m->generation);
    memcpy(p + 40, m->lineage, 16); Put32(p + 56, s->usedExtent);
    Put32(p + 60, 128); Put32(p + 64, 512);
    Put16(p + 68, m->creatureProfile); Put16(p + 70, m->imageFlags);
    Put32(p + 72, s->imageCrc); Put32(p + 76, m->registryVersion);
    Put32(p + 80, m->rulesetVersion); memcpy(p + 84, m->buildId, 16);
    Put16(p + 100, m->transferContract); Put16(p + 102, m->sourceGame);
    Put32(p + 104, m->eligibility); Put32(p + 108, m->profileFlags);
    {
        unsigned i;
        for (i = 0; i < s->sectionCount; i++) {
            const BsgSection *d = &s->sections[i];
            uint8_t *e = p + 128 + i * 32;
            Put32(e, d->id); Put16(e + 4, d->major); Put16(e + 6, d->minor);
            Put32(e + 8, d->flags); Put32(e + 12, d->offset); Put32(e + 16, d->length);
            Put32(e + 20, d->crc); Put32(e + 24, d->count);
        }
    }
}

static BsgResult DecodeHeader(const uint8_t *p, BsgSnapshot *s)
{
    BsgMetadata *m = &s->metadata;
    unsigned i;
    uint32_t end = BSG_BODY_OFFSET, lastId = 0;
    if (memcmp(p, "BONDSAGA", 8)) return BSG_INVALID;
    m->envelopeMajor = Get16(p + 8); m->envelopeMinor = Get16(p + 10);
    if (Get16(p + 12) != 128 || Get16(p + 14) != 32 || Get16(p + 16) != 16
        || Get32(p + 60) != 128 || Get32(p + 64) != 512) return BSG_UNSUPPORTED;
    s->sectionCount = Get16(p + 18);
    if (s->sectionCount > 16 || !Filled(p + 112, 16, 0)) return BSG_INVALID;
    m->game = Get16(p + 20); m->schemaMajor = Get16(p + 22);
    m->schemaMinor = Get16(p + 24); m->minimumReaderMinor = Get16(p + 26);
    m->requiredFeatures = Get32(p + 28); m->generation = Get64(p + 32);
    memcpy(m->lineage, p + 40, 16); s->usedExtent = Get32(p + 56);
    m->creatureProfile = Get16(p + 68); m->imageFlags = Get16(p + 70);
    s->imageCrc = Get32(p + 72); m->registryVersion = Get32(p + 76);
    m->rulesetVersion = Get32(p + 80); memcpy(m->buildId, p + 84, 16);
    m->transferContract = Get16(p + 100); m->sourceGame = Get16(p + 102);
    m->eligibility = Get32(p + 104); m->profileFlags = Get32(p + 108);
    if (!m->generation || Filled(m->lineage, 16, 0)) return BSG_INVALID;
    for (i = 0; i < 16; i++) {
        const uint8_t *e = p + 128 + i * 32;
        BsgSection *d = &s->sections[i];
        if (i >= s->sectionCount) {
            if (!Filled(e, 32, 0)) return BSG_INVALID;
            continue;
        }
        d->id = Get32(e); d->major = Get16(e + 4); d->minor = Get16(e + 6);
        d->flags = Get32(e + 8); d->offset = Get32(e + 12); d->length = Get32(e + 16);
        d->crc = Get32(e + 20); d->count = Get32(e + 24);
        if (!d->id || d->id <= lastId || !d->length || (d->offset & 3) || (d->length & 3)
            || d->offset < end || d->offset > BSG_IMAGE_BYTES || d->length > BSG_IMAGE_BYTES - d->offset
            || Get32(e + 28) || !(d->flags & (BSG_SCOPE_SHARED | BSG_SCOPE_LOCAL))
            || (d->flags & (BSG_SCOPE_SHARED | BSG_SCOPE_LOCAL)) == (BSG_SCOPE_SHARED | BSG_SCOPE_LOCAL)) return BSG_INVALID;
        end = d->offset + d->length; lastId = d->id;
    }
    return s->usedExtent == end ? BSG_OK : BSG_INVALID;
}

BsgResult BsgValidateSnapshot(const BsgIo *io, uint8_t slot, BsgWorkspace *w, BsgSnapshot *s)
{
    uint32_t imageCrc = UINT32_MAX;
    unsigned i;
    bool allErased = true;
    BsgResult headerResult = BSG_INVALID;
    if (!io || !io->read || !w || !s || slot > 1) return BSG_BAD_ARGUMENT;
    memset(s, 0, sizeof(*s)); s->slot = slot;
    /* First distinguish a wholly erased bank from a partially initialized one. */
    for (i = 0; i < 16; i++) {
        if (!io->read(io->context, slot * BSG_SNAPSHOT_BYTES + i * 4096, w->sector, 4096)) return BSG_IO_ERROR;
        if (!Filled(w->sector, 4096, 0xff)) allErased = false;
    }
    if (allErased) return BSG_EMPTY;
    for (i = 0; i < 16; i++) {
        uint8_t *p = w->sector;
        uint32_t crc;
        if (!io->read(io->context, slot * BSG_SNAPSHOT_BYTES + i * 4096, p, 4096)) return BSG_IO_ERROR;
        crc = BsgCrc32c(p, CRC_OFFSET);
        if (Get32(p) != SECTOR_MAGIC || Get32(p + CRC_OFFSET) != crc
            || Get32(p + CRC_OFFSET + 4) != ~crc || !Filled(p + 0xfe8, 8, 0xff)) return BSG_INVALID;
        if (p[4] != 1) return BSG_UNSUPPORTED;
        if (p[5] != slot || p[6] != i || p[7] != 16) return BSG_INVALID;
        if (i == 0) {
            headerResult = DecodeHeader(p + 32, s);
            if (headerResult != BSG_OK) return headerResult;
        }
        if (Get64(p + 8) != s->metadata.generation || memcmp(p + 16, s->metadata.lineage, 16)) return BSG_INVALID;
        if (i == 15) {
            if (Get32(p + COMMIT_OFFSET) != COMMIT_MAGIC || Get64(p + COMMIT_OFFSET + 4) != s->metadata.generation
                || Get32(p + COMMIT_OFFSET + 12) != s->imageCrc) return BSG_INVALID;
        } else if (!Filled(p + COMMIT_OFFSET, 16, 0xff)) return BSG_INVALID;
        if (i == 0) Put32(p + 32 + IMAGE_CRC_OFFSET, 0);
        imageCrc = BsgCrc32cUpdate(imageCrc, p + 32, BSG_PAYLOAD_BYTES);
    }
    if ((imageCrc ^ UINT32_MAX) != s->imageCrc) return BSG_INVALID;
    for (i = 0; i < s->sectionCount; i++) {
        const BsgSection *d = &s->sections[i];
        uint32_t off, crc = UINT32_MAX;
        for (off = 0; off < d->length;) {
            uint32_t n = d->length - off;
            if (n > sizeof(w->scratch)) n = sizeof(w->scratch);
            if (BsgReadLogical(io, slot, d->offset + off, w->scratch, n) != BSG_OK) return BSG_IO_ERROR;
            crc = BsgCrc32cUpdate(crc, w->scratch, n); off += n;
        }
        if ((crc ^ UINT32_MAX) != d->crc) return BSG_INVALID;
    }
    /* Unused ranges are canonical zero, including gaps between sections. */
    {
        uint32_t off = BSG_BODY_OFFSET;
        for (i = 0; i <= s->sectionCount; i++) {
            uint32_t end = i == s->sectionCount ? BSG_IMAGE_BYTES : s->sections[i].offset;
            while (off < end) {
                uint32_t n = end - off;
                if (n > sizeof(w->scratch)) n = sizeof(w->scratch);
                if (BsgReadLogical(io, slot, off, w->scratch, n) != BSG_OK) return BSG_IO_ERROR;
                if (!Filled(w->scratch, n, 0)) return BSG_INVALID;
                off += n;
            }
            if (i < s->sectionCount) off = s->sections[i].offset + s->sections[i].length;
        }
    }
    return BSG_OK;
}

BsgResult BsgCheckCompatibility(const BsgSnapshot *s, const BsgReader *r)
{
    const BsgMetadata *m = &s->metadata;
    unsigned i;
    if (!r || m->game >= 32 || !(r->gameMask & (1u << m->game)) || m->sourceGame > BSG_GAME_PART_II
        || m->envelopeMajor != 1 || m->envelopeMinor
        || m->schemaMajor != r->schemaMajor || m->schemaMinor > r->readerMinor || m->minimumReaderMinor > r->readerMinor
        || m->creatureProfile != r->creatureProfile || (m->requiredFeatures & ~r->knownFeatures)
        || m->registryVersion != r->registryVersion || m->rulesetVersion != r->rulesetVersion
        || m->transferContract != BSG_TRANSFER_CONTRACT || m->imageFlags || m->profileFlags) return BSG_UNSUPPORTED;
    for (i = 0; i < s->sectionCount; i++) {
        const BsgSection *d = &s->sections[i];
        if (d->flags & ~(BSG_SCOPE_SHARED | BSG_SCOPE_LOCAL | BSG_REQUIRED_RESUME | BSG_REQUIRED_TRANSFER | BSG_PRESERVE)) return BSG_UNSUPPORTED;
        if (d->id <= BSG_SECTION_TRANSFER_RECEIPT) {
            if (d->major != 1 || d->minor) return BSG_UNSUPPORTED;
        } else if (d->flags & (BSG_REQUIRED_RESUME | BSG_REQUIRED_TRANSFER)) return BSG_UNSUPPORTED;
    }
    return BSG_OK;
}

BsgResult BsgSelectSnapshot(const BsgIo *io, const BsgReader *r, BsgWorkspace *w, BsgSnapshot *out, bool *recovered)
{
    BsgSnapshot *other;
    BsgResult a, b;
    if (!out || !r || !w || out == &w->selection) return BSG_BAD_ARGUMENT;
    other = &w->selection;
    if (recovered) *recovered = false;
    a = BsgValidateSnapshot(io, 0, w, out);
    b = BsgValidateSnapshot(io, 1, w, other);
    if (a == BSG_IO_ERROR || b == BSG_IO_ERROR) return BSG_IO_ERROR;
    /* Unknown physical envelopes cannot safely be ordered or overwritten. */
    if (a == BSG_UNSUPPORTED || b == BSG_UNSUPPORTED) return BSG_UNSUPPORTED;
    if (a != BSG_OK && b != BSG_OK) return a == BSG_EMPTY && b == BSG_EMPTY ? BSG_EMPTY : BSG_INVALID;
    if (a == BSG_OK && b == BSG_OK) {
        if (memcmp(out->metadata.lineage, other->metadata.lineage, 16)) return BSG_AMBIGUOUS;
        if (out->metadata.generation == other->metadata.generation && out->imageCrc != other->imageCrc) return BSG_AMBIGUOUS;
        if (other->metadata.generation > out->metadata.generation) *out = *other;
    } else {
        if (b == BSG_OK) *out = *other;
        if (recovered) *recovered = a == BSG_INVALID || b == BSG_INVALID;
    }
    /* Do not first filter to supported schemas: that would permit rollback. */
    return BsgCheckCompatibility(out, r);
}

static BsgResult Prepare(const BsgImageSpec *spec, BsgWorkspace *w, BsgSnapshot *s)
{
    unsigned i;
    uint32_t end = BSG_BODY_OFFSET, last = 0;
    if (!spec || !spec->readSection || !spec->sectionCount || spec->sectionCount > 16) return BSG_BAD_ARGUMENT;
    s->sectionCount = spec->sectionCount;
    for (i = 0; i < spec->sectionCount; i++) {
        BsgSection *d = &s->sections[i];
        uint32_t off, crc = UINT32_MAX;
        *d = spec->sections[i]; d->offset = end;
        if (!d->id || d->id <= last || !d->length || (d->length & 3)
            || !(d->flags & (BSG_SCOPE_SHARED | BSG_SCOPE_LOCAL))
            || (d->flags & (BSG_SCOPE_SHARED | BSG_SCOPE_LOCAL)) == (BSG_SCOPE_SHARED | BSG_SCOPE_LOCAL)) return BSG_BAD_ARGUMENT;
        if (d->length > BSG_IMAGE_BYTES - end) return BSG_CAPACITY;
        for (off = 0; off < d->length;) {
            uint32_t n = d->length - off;
            if (n > sizeof(w->scratch)) n = sizeof(w->scratch);
            if (!spec->readSection(spec->context, d->id, off, w->scratch, n)) return BSG_INVALID;
            crc = BsgCrc32cUpdate(crc, w->scratch, n); off += n;
        }
        d->crc = crc ^ UINT32_MAX; end += d->length; last = d->id;
    }
    s->usedExtent = end;
    return BSG_OK;
}

static bool MakePayload(const BsgImageSpec *spec, const BsgSnapshot *s, unsigned index, uint8_t *p)
{
    unsigned i;
    uint32_t begin = index * BSG_PAYLOAD_BYTES, end = begin + BSG_PAYLOAD_BYTES;
    memset(p, 0, BSG_PAYLOAD_BYTES);
    if (!index) EncodeHeader(p, s);
    for (i = 0; i < s->sectionCount; i++) {
        const BsgSection *d = &s->sections[i];
        uint32_t from = d->offset > begin ? d->offset : begin;
        uint32_t to = d->offset + d->length < end ? d->offset + d->length : end;
        if (from < to && !spec->readSection(spec->context, d->id, from - d->offset, p + from - begin, to - from)) return false;
    }
    return true;
}

/* Readback view inserts the expected marker only for pre-commit validation. */
typedef struct { const BsgIo *io; uint32_t commitAddress; uint8_t marker[16]; } PendingRead;
static bool ReadPending(void *ctx, uint32_t off, uint8_t *out, uint32_t n)
{
    PendingRead *p = ctx;
    uint32_t i;
    if (!p->io->read(p->io->context, off, out, n)) return false;
    for (i = 0; i < n; i++) if (off + i >= p->commitAddress && off + i < p->commitAddress + 16) {
        if (out[i] != 0xff) return false;
        out[i] = p->marker[off + i - p->commitAddress];
    }
    return true;
}

BsgResult BsgWriteSnapshot(const BsgIo *io, const BsgReader *reader, const BsgImageSpec *spec, BsgWorkspace *w, BsgSnapshot *out)
{
    BsgSnapshot *plan;
    BsgResult result;
    uint32_t crc = UINT32_MAX;
    unsigned i;
    PendingRead pending;
    BsgIo pendingIo;
    if (!io || !io->read || !io->erase || !io->program || !spec || !w || !out
        || out == &w->transaction || out == &w->selection) return BSG_BAD_ARGUMENT;
    plan = &w->transaction;
    memset(plan, 0, sizeof(*plan));
    result = BsgSelectSnapshot(io, reader, w, out, NULL);
    if (result != BSG_OK && result != BSG_EMPTY) return result;
    plan->metadata = spec->metadata;
    if (Filled(plan->metadata.lineage, 16, 0)) return BSG_BAD_ARGUMENT;
    plan->slot = result == BSG_EMPTY ? 0 : 1 - out->slot;
    if (result == BSG_OK) {
        if (memcmp(plan->metadata.lineage, out->metadata.lineage, 16)) return BSG_AMBIGUOUS;
        if (out->metadata.generation == UINT64_MAX) return BSG_CAPACITY;
        plan->metadata.generation = out->metadata.generation + 1;
        if (spec->validateSnapshot) {
            result = spec->validateSnapshot(io, out, w, spec->validationContext);
            if (result != BSG_OK) return result;
        }
    } else plan->metadata.generation = 1;
    result = BsgCheckCompatibility(plan, reader);
    if (result != BSG_OK) return result;
    result = Prepare(spec, w, plan);
    if (result != BSG_OK) return result;
    result = BsgCheckCompatibility(plan, reader);
    if (result != BSG_OK) return result;
    for (i = 0; i < 16; i++) {
        if (!MakePayload(spec, plan, i, w->sector + 32)) return BSG_INVALID;
        crc = BsgCrc32cUpdate(crc, w->sector + 32, BSG_PAYLOAD_BYTES);
    }
    plan->imageCrc = crc ^ UINT32_MAX;
    /* Invalidate stale commit before touching any other inactive sector. */
    for (i = 0; i < 16; i++) {
        uint16_t sector = plan->slot * 16 + (i == 0 ? 15 : i - 1);
        if (!io->erase(io->context, sector)
            || !io->read(io->context, sector * 4096u, w->sector, 4096)
            || !Filled(w->sector, 4096, 0xff)) return BSG_IO_ERROR;
    }
    for (i = 0; i < 16; i++) {
        uint8_t *p = w->sector;
        memset(p, 0xff, 4096);
        Put32(p, SECTOR_MAGIC); p[4] = 1; p[5] = plan->slot; p[6] = i; p[7] = 16;
        Put64(p + 8, plan->metadata.generation); memcpy(p + 16, plan->metadata.lineage, 16);
        if (!MakePayload(spec, plan, i, p + 32)) return BSG_INVALID;
        crc = BsgCrc32c(p, CRC_OFFSET); Put32(p + CRC_OFFSET, crc); Put32(p + CRC_OFFSET + 4, ~crc);
        if (!io->program(io->context, plan->slot * BSG_SNAPSHOT_BYTES + i * 4096, p, COMMIT_OFFSET)) return BSG_IO_ERROR;
    }
    pending.io = io;
    pending.commitAddress = plan->slot * BSG_SNAPSHOT_BYTES + 15 * 4096 + COMMIT_OFFSET;
    Put32(pending.marker, COMMIT_MAGIC); Put64(pending.marker + 4, plan->metadata.generation); Put32(pending.marker + 12, plan->imageCrc);
    pendingIo = *io; pendingIo.context = &pending; pendingIo.read = ReadPending;
    result = BsgValidateSnapshot(&pendingIo, plan->slot, w, out);
    if (result != BSG_OK || out->imageCrc != plan->imageCrc) return result == BSG_OK ? BSG_INVALID : result;
    if (spec->validateSnapshot) {
        result = spec->validateSnapshot(&pendingIo, out, w, spec->validationContext);
        if (result != BSG_OK) return result;
    }
    for (i = 0; i < 16; i++)
        if (!io->program(io->context, pending.commitAddress + i, pending.marker + i, 1)) return BSG_IO_ERROR;
    result = BsgValidateSnapshot(io, plan->slot, w, out);
    if (result == BSG_OK && spec->validateSnapshot)
        result = spec->validateSnapshot(io, out, w, spec->validationContext);
    return result;
}

typedef struct {
    const BsgIo *io;
    const BsgSnapshot *source;
    const BsgTransferPolicy *policy;
    uint8_t receipt[BSG_TRANSFER_RECEIPT_BYTES];
} TransferSource;
static bool ReadTransfer(void *ctx, uint32_t id, uint32_t off, uint8_t *out, uint32_t n)
{
    TransferSource *t = ctx;
    if (id == BSG_SECTION_LOCAL) return t->policy->readLocal(t->policy->context, off, out, n);
    if (id == BSG_SECTION_TRANSFER_RECEIPT) {
        if (off > sizeof(t->receipt) || n > sizeof(t->receipt) - off) return false;
        memcpy(out, t->receipt + off, n);
        return true;
    }
    return BsgReadSection(t->io, t->source, id, off, out, n) == BSG_OK;
}

BsgResult BsgTransferPartII(const BsgIo *io, const BsgReader *reader, const BsgTransferPolicy *policy, BsgWorkspace *w, BsgSnapshot *out)
{
    BsgSnapshot *source;
    BsgImageSpec *spec;
    TransferSource ctx;
    BsgResult result;
    unsigned i;
    bool inserted = false;
    if (!w || !out || !policy || !policy->eligible || !policy->readLocal || !policy->initializerVersion) return BSG_BAD_ARGUMENT;
    source = &w->donor;
    spec = &w->migration;
    result = BsgSelectSnapshot(io, reader, w, source, NULL);
    if (result != BSG_OK) return result;
    if (policy->validateSnapshot) {
        result = policy->validateSnapshot(io, source, w, policy->validationContext);
        if (result != BSG_OK) return result;
    }
    if (source->metadata.game == BSG_GAME_PART_II) { *out = *source; return BSG_ALREADY_IMPORTED; }
    if (source->metadata.game != BSG_GAME_PART_I || !policy->eligible(policy->context, &source->metadata)) return BSG_NOT_ELIGIBLE;
    if (!BsgFindSection(source, BSG_SECTION_LOCAL)) return BSG_INVALID;
    memset(spec, 0, sizeof(*spec));
    spec->metadata = source->metadata; spec->metadata.game = BSG_GAME_PART_II;
    spec->metadata.sourceGame = BSG_GAME_PART_I;
    memcpy(spec->metadata.buildId, policy->destinationBuildId, 16);
    for (i = 0; i < source->sectionCount; i++) {
        BsgSection d = source->sections[i];
        if (d.id >= BSG_SECTION_TRANSFER_RECEIPT && !inserted) {
            BsgSection receipt = {BSG_SECTION_TRANSFER_RECEIPT, 1, 0,
                BSG_SCOPE_SHARED | BSG_REQUIRED_RESUME | BSG_REQUIRED_TRANSFER | BSG_PRESERVE,
                0, BSG_TRANSFER_RECEIPT_BYTES, 0, 1};
            if (spec->sectionCount == 16) return BSG_CAPACITY;
            spec->sections[spec->sectionCount++] = receipt;
            inserted = true;
        }
        if (d.id == BSG_SECTION_TRANSFER_RECEIPT) continue;
        /* Only the declared local section can be replaced by this V1 scaffold. */
        if (d.id == BSG_SECTION_LOCAL) d.length = policy->localLength;
        else if (!(d.flags & BSG_SCOPE_SHARED)) return BSG_UNSUPPORTED;
        if (spec->sectionCount == 16) return BSG_CAPACITY;
        spec->sections[spec->sectionCount++] = d;
    }
    if (!inserted) {
        BsgSection receipt = {BSG_SECTION_TRANSFER_RECEIPT, 1, 0,
            BSG_SCOPE_SHARED | BSG_REQUIRED_RESUME | BSG_REQUIRED_TRANSFER | BSG_PRESERVE,
            0, BSG_TRANSFER_RECEIPT_BYTES, 0, 1};
        if (spec->sectionCount == 16) return BSG_CAPACITY;
        spec->sections[spec->sectionCount++] = receipt;
    }
    ctx.io = io; ctx.source = source; ctx.policy = policy;
    memset(ctx.receipt, 0, sizeof(ctx.receipt));
    memcpy(ctx.receipt, source->metadata.lineage, 16);
    Put64(ctx.receipt + 16, source->metadata.generation);
    Put32(ctx.receipt + 24, source->imageCrc);
    Put16(ctx.receipt + 28, source->metadata.game);
    Put16(ctx.receipt + 30, source->metadata.schemaMajor);
    Put16(ctx.receipt + 32, source->metadata.schemaMinor);
    Put16(ctx.receipt + 34, source->metadata.transferContract);
    Put32(ctx.receipt + 36, 1); /* Wire transformation version, not gameplay. */
    Put32(ctx.receipt + 40, source->metadata.registryVersion);
    Put32(ctx.receipt + 44, source->metadata.rulesetVersion);
    memcpy(ctx.receipt + 48, source->metadata.buildId, 16);
    Put32(ctx.receipt + 64, policy->initializerVersion);
    spec->context = &ctx; spec->readSection = ReadTransfer;
    spec->validateSnapshot = policy->validateSnapshot;
    spec->validationContext = policy->validationContext;
    return BsgWriteSnapshot(io, reader, spec, w, out);
}
