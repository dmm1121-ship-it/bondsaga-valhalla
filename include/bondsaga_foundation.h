#ifndef GUARD_BONDSAGA_FOUNDATION_H
#define GUARD_BONDSAGA_FOUNDATION_H

#include "bondsaga_data.h"
#include "bondsaga_save.h"

#define BSG_FOUNDATION_SECTION_COUNT 11u
#define BSG_FOUNDATION_OPAQUE_COUNT 7u
#define BSG_ROSTER_SECTION_BYTES 1280u
#define BSG_INVENTORY_SECTION_BYTES 2048u
#define BSG_RESEARCH_SECTION_BYTES 2064u
#define BSG_RIVALS_SECTION_BYTES 1040u
#define BSG_CONTRACTS_SECTION_BYTES 512u
#define BSG_SAGA_SECTION_BYTES 1024u
#define BSG_LOCAL_SECTION_BYTES 3072u
#define BSG_EXTENSIONS_SECTION_BYTES 512u
#define BSG_FOUNDATION_MAX_BODY_BYTES 54928u
#define BSG_FOUNDATION_RESERVE_BYTES (BSG_BODY_BYTES - BSG_FOUNDATION_MAX_BODY_BYTES)

/* Sections 4..10 have full budgeted capacity even when their future gameplay
 * serialization is unimplemented. Supplied opaque bytes are retained verbatim;
 * the rest of each allocation is zero. These envelopes assign no field or
 * reserved-byte semantics to research, equipment, contracts, or world content.
 */
struct BsgFoundationBlob
{
    const uint8_t *data;
    uint32_t length;
};

struct BsgFoundationSource
{
    const struct BsgCreatureStore *store;
    const struct BsgPlayerBinder *player;
    const struct BsgRoster *roster;
    const struct BsgRegistry *registry;
    struct BsgFoundationBlob opaque[BSG_FOUNDATION_OPAQUE_COUNT];
    /* Zero length initializes a blank receipt; subsequent saves supply the
     * existing complete receipt. It never aliases future saga/gameplay bytes. */
    struct BsgFoundationBlob transferReceipt;
};

/* Source pointers and their contents remain frozen until the transaction ends.
 * No global canonical store or full snapshot buffer is allocated here.
 */
BsgResult BsgBuildFoundationSpec(BsgImageSpec *out, const BsgMetadata *metadata, const struct BsgFoundationSource *source);
BsgResult BsgSaveFoundation(const BsgIo *io, const BsgReader *reader, const BsgMetadata *metadata, const struct BsgFoundationSource *source, BsgWorkspace *workspace, BsgSnapshot *out);
BsgResult BsgValidateFoundation(const BsgIo *io, const BsgSnapshot *snapshot, BsgWorkspace *workspace, const struct BsgRegistry *registry);
BsgResult BsgValidateFoundationCallback(const BsgIo *io, const BsgSnapshot *snapshot, BsgWorkspace *workspace, void *registry);
/* Selection/compatibility precede profile validation. A malformed or unsupported
 * newest committed profile is refused, never replaced with an older profile.
 * Load returns a validated snapshot handle; read/decode the selected canonical
 * sections once into the caller's eventual runtime owner, without a second copy.
 */
BsgResult BsgLoadFoundation(const BsgIo *io, const BsgReader *reader, BsgWorkspace *workspace, const struct BsgRegistry *registry, BsgSnapshot *out, bool *recovered);

#endif /* GUARD_BONDSAGA_FOUNDATION_H */
