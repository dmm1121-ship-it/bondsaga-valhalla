# Persistent-data foundation V1

Implementation status: prototype foundation for review. The governing authorities are [Save Architecture V1](SAVE_ARCHITECTURE_V1.md), [Technical Architecture V1](TECHNICAL_ARCHITECTURE_V1.md), and [Design Bible V1](DESIGN_BIBLE_V1.md). This document records technical encodings and integration boundaries; it does not assign unresolved gameplay semantics.

## Scope and entry points

- `bondsaga_data.h/.c`: explicit creature/player/roster encoders, canonical owned-record storage and reference validation.
- `bondsaga_save.h/.c`: CRC32C, physical container, compatibility selection, interrupted-write-safe transactions and synthetic transfer.
- `bondsaga_foundation.h/.c`: complete V1 section budget, source validation, streaming profile validation and save/load wrappers.
- `bondsaga_save_gba.h/.c`: adapter to the existing FLASH1M driver; protection against Emerald save/recovery routines interpreting or overwriting Bondsaga data.

Use `BsgSaveFoundation` and `BsgLoadFoundation` for the canonical profile. `BsgWriteSnapshot` is the lower-level container primitive and validates container structure/integrity; a typed caller supplies its `validateSnapshot` callback for record/reference validation before commitment. The foundation wrapper supplies that callback automatically.

The caller owns the canonical store, player block, roster, opaque global sections and workspace. No second persistent creature array or full 64 KiB image is allocated by these modules. The source must remain frozen across all preflight, checksum, serialization and verification passes. Read/write callbacks must not reenter the transaction or modify the workspace descriptors. Output snapshot handles are usable only on success.

This stage does not initialize an authored player/world or route Emerald's playable Save/Continue frontend into Bondsaga. Such routing would require an approved initial state and runtime ownership integration. Ordinary reference-ROM data still uses its separate legacy routines; a recognized Bondsaga container is refused by those routines, including failed-save wiping. There is no Emerald fallback inside the Bondsaga APIs, no conversion of ordinary Emerald saves, and no silent legacy reset of a Bondsaga file.

## Physical and logical contract

The battery save remains exactly 131,072 bytes. Snapshot A occupies sectors 0–15; B occupies 16–31. They are recovery copies of one logical save.

| Sector byte range | Meaning |
|---|---|
| `000–01F` | Header: `BSEC`, physical version 1, slot/index/count, generation u64, lineage 16 bytes |
| `020–FDF` | 4,032 logical payload bytes |
| `FE0–FE3` | CRC32C of header and payload |
| `FE4–FE7` | Inverse CRC |
| `FE8–FEF` | Erased `FF` reserved bytes |
| `FF0–FFF` | `BCMT` + generation u64 + image CRC, only in the final sector; erased elsewhere |

All multibyte integers are little-endian. C layouts, pointers, native enum ordering and compiler bitfields are never copied onto flash. CRC32C uses reflected polynomial `82F63B78`, initial/final XOR `FFFFFFFF`; `123456789` yields `E3069283`. It is integrity checking, not authentication or an anti-edit policy.

Sixteen payloads concatenate to 64,512 logical bytes. The image starts with `BONDSAGA`, a 128-byte header and sixteen 32-byte directory entries, leaving 63,872 section-body bytes. The byte offsets are explicitly implemented in `EncodeHeader`/`DecodeHeader` and covered by fixtures. The header carries envelope/schema/minimum-reader versions, game and source-game IDs, generation, lineage, registry and ruleset versions, creature profile, transfer contract, eligibility marker, producing-build ID and image CRC. Eligibility is opaque to the container.

The image CRC covers all logical bytes, including header/directory, with its own field at offset 72 treated as zero. Each section has another CRC. Padding and unused directory entries must be canonical zero; reserved physical footer bytes must remain erased. Offset arithmetic, sorted unique IDs, lengths, alignment, overlap, section counts and physical sector identity are validated before section use.

Section IDs 1–11 are permanently assigned to creatures, player/Binder, roster, inventory, research, rivals, contracts, saga state, local world, extensions and transfer receipt. Scope and required/preservation flags are explicit. V1 deliberately accepts only implemented versions of known required sections. Unknown optional shared sections can be retained by the generic container; the fixed foundation writer refuses additional sections rather than dropping them. Future support requires a preserving versioned writer.

## Save and recovery ordering

1. Validate both banks, choose the highest intact generation within the same lineage, then check compatibility. An intact unsupported newest schema/game is an incompatibility, not permission to load an older supported bank.
2. Validate and measure the frozen source before any erase. Preserve the full future-system allowances even when they contain only zero placeholders.
3. Erase and verify the inactive bank's commit sector first, then the other inactive sectors. The source bank is never erased.
4. Serialize sector-by-sector and program the target without its final marker.
5. Read back and validate all bytes, sections and profile references. A read-only provisional view supplies the expected marker for this check while verifying that the physical marker is still erased.
6. Program the 16 marker bytes last, read back, and revalidate the committed result. Report success only after this completes.

Flash callbacks must never implicitly erase during `program`. The GBA adapter uses the existing byte-program and explicit sector-erase operations; `ProgramFlashSector` cannot be used for commitment because that driver erases internally. Neither erase, sector programming nor the final marker is assumed atomic.

Blank media initializes generation 1 in A using a caller-supplied nonzero saga lineage. Nonblank invalid/unsupported media is never automatically reset. If the first save is torn, there is no previous save to recover; an explicit recovery/new-game flow remains frontend work. A corrupt bank with an older valid bank reports recovery through the `recovered` output. Mixed lineages or conflicting equal generations are refused. Generation exhaustion never wraps.

Newest-generation selection applies to the understood physical envelope. A recognizable but unsupported physical version or header geometry conservatively blocks loading/writing either bank, even when its age or complete integrity cannot be established. An older reader must not erase an envelope it cannot safely interpret; recovery across future physical formats requires explicit support.

CRC protects against corruption and torn writes, not destruction of both banks or replacement of the entire external file. A save-file import replaces transport data before the ROM sees it; no receipt can enforce lifetime import counts after the user restores an earlier file.

## Creature and player encodings

The canonical owned store has 384 total records across every location. Its section consists of a 16-byte versioned header and `count × 112` bytes. The source type has capacity for all 384 canonical serialized records; inactive memory records are zero. No hidden daycare/fusion population is admitted outside that count.

| Creature offset | Bytes | Field |
|---:|---:|---|
| 0 | 4 | Specimen serial |
| 4, 6, 8, 10 | 2 each | Species/form, lineage, origin, acquisition location |
| 12 | 4 | Earned EXP |
| 16, 18 | 2 each | Earned and acquisition levels |
| 20, 22, 24, 26 | 2 each | HP, opaque condition, appearance, personality profile |
| 28 | 2 | Alpha/Apex/Variant bits 0–2; name length bits 8–11; other bits reserved |
| 30 | 2 | Recovery profile |
| 32, 36 | 4 each | Recovery and individual Bond progression |
| 40 | 12 | Opaque nickname glyph bytes, unused tail zero |
| 52 | 8 | Four move IDs |
| 60 | 4 | Four durable resource values |
| 64 | 6 | Six unnamed Aptitude slots |
| 70 | 12 | Six developed values |
| 82 | 8 | Four trait identities: innate/specimen, developed, Bond, exceptional |
| 90 | 2 | Lineage milestones |
| 92 | 8 | Stable lineage outcomes |
| 100, 104 | 4 each | Two opaque history accumulators |
| 108 | 4 | Reserved zero |

There is no level-100 clamp, EXP-curve calculation, trait activation, injury formula, new personality rule or hidden stat normalization. Alpha, Apex and Variant combine independently. Equipment remains global; individual compatibility defaults and battle stats remain derived. Registry callbacks can reject unknown IDs without substitution. A null registry permits structural tests without inventing a creature/content registry.

Serials are nonzero and unique within the current saved timeline. A nonzero allocator exceeds all currently owned serials; zero denotes exhaustion, never wraparound. Released serials are not recycled by the allocation contract. Restoring an earlier save restores its allocator, so diverging timelines cannot be merged by serial alone.

The player/Binder encoding occupies 256 bytes. Player identity/name/customization, options, difficulty/accessibility, playtime/currency, RNG context and allocator use the first 64 bytes. The remaining 192 encode Binder progression/rank, technique/capability memberships, sixteen mastery positions, manifestation milestones, health/recovery and selected techniques. Unspecified progression-component bytes remain reserved zero. No proposed attribute names or spendable-stat rules are implemented.

Roster ownership placement and expedition selection are separate: all 384 creatures may remain outside the expedition. Ownership placement is a permutation of the occupied record indices; empty references use `FFFF`. Up to eight expedition references, a Bound reference and deployment references point to the canonical records. Bound/deployment must reference expedition members; Bound cannot also be deployed. An eight-reference deployment envelope is not authorization for eight simultaneous battlers. Compaction must remap indices while preserving serials; no automatic compaction or gameplay movement is introduced here.

## Guaranteed save budget

No compression or typical-case sparsity is assumed. Empty future systems still reserve their entire allowance.

| Section | Worst-case bytes |
|---|---:|
| 384 creatures and header | 43,024 |
| Player/Binder | 256 |
| Roster allocation, including padding | 1,280 |
| Inventory/equipment | 2,048 |
| Species research, retaining capacity allowance for 256 rows | 2,064 |
| Rivals | 1,040 |
| Contracts | 512 |
| Saga decisions/provenance | 1,024 |
| Local world | 3,072 |
| Extensions | 512 |
| Transfer receipt | 96 |
| **Bodies used** | **54,928** |
| Header and directory | 640 |
| Physical framing | 1,024 |
| **Total occupied allowance per snapshot** | **56,592** |
| **Remaining per snapshot** | **8,944** |

The 96-byte receipt is a separately charged technical section; it does not overwrite opaque saga or extension bytes. Neither the second recovery bank nor the four formerly auxiliary Emerald sectors are counted again as spare space.

The future global allowances are technical reservations, not finalized research counters, inventory caps, rival formulas or contract counts. Filling them with actual mechanics requires their own approved specifications and a worst-case rebudget if necessary. Static assertions protect the 384/112 budgets and a minimum 4 KiB planning reserve.

## Synthetic transfer

`BsgTransferPartII` requires an explicit eligibility callback and a deterministic, versioned local-world byte provider. There is no default eligibility or destination. The provider must reproduce the same initialization from its approved context across retries; durable random initialization beyond that contract requires separate design.

All shared sections are copied byte-for-byte except the dedicated technical transfer receipt. The approved local section is replaced through the callback. Creature counts, identities, EXP, earned levels, nickname bytes, flags, growth, traits, Bond and health remain unchanged. Receipt fields record donor lineage/generation/CRC, source game/schema, contract/transformation/registry/ruleset versions, build identity and initializer version. The caller supplies the destination build ID separately; it never inherits a misleading Part I producer ID. Zero build IDs denote unassigned synthetic builds.

An existing Part II snapshot is continued (`BSG_ALREADY_IMPORTED`) instead of importing its older Part I recovery bank again. Retrying an interrupted import reads the unchanged donor and replaces the inactive target; it does not append creatures, grant rewards or normalize state. Real eligibility, release support, destination state and any reward behavior remain outside this implementation.

## Validation and measurements

The portable test runner is `python tools/bondsaga_save/run_tests.py`. On a supported GCC/Clang host add `--sanitize` for AddressSanitizer and UndefinedBehaviorSanitizer. Tests compile the production C sources, with a fake 128 KiB flash device enforcing 1-to-0 programming and injected read/erase/program failures. The CI workflow additionally builds the approved baseline and current GBA ROM and reports ELF region usage, padded ROM size, object sections and compiler stack estimates.

The review suite passes 18 host test groups, including 192 interrupted-program/erase boundaries, silent I/O failures, completed-commit controls, exact wire fixtures, capacity/reference rejection, unsupported-newest refusal and synthetic migration/retry. It was run with strict C11 warnings on Clang and GCC, and with instrumented UBSan. Linux CI also runs AddressSanitizer; Windows Zig accepting an ASan flag is not evidence of address instrumentation. Four GBA test-runner cases cover CRC32C, sequel progression/flags, the admitted budget and refusal of an unidentified flash device.

ARM GCC's APCS layout measurements give: canonical store 43,012 bytes, player/Binder view 256, roster view 804, snapshot descriptor 540, and workspace 6,392. These are runtime C object sizes, distinct from their wire sizes. The four foundation modules introduce no static mutable data allocation. The workspace belongs in EWRAM/heap, not the system stack; it contains all nested container descriptors and reuses its sector buffer for streamed validation. The largest measured individual foundation frame is the 848-byte section-encoding callback. Callback chains, caller frames, IRQ use and actual stack high-water marks still require measurement when a live frontend is connected.

Exact baseline/candidate ROM and static-RAM deltas, compiler versions, build logs, target-ABI sizes and stack estimates are emitted by the [Bondsaga persistence workflow](../.github/workflows/bondsaga-save.yml) as the `bondsaga-persistence-measurements` artifact. It compares against approved baseline `71c589a0822a8fbef8100c8f89bf8f051366c353` using the same toolchain. Report linker-used ROM separately from the padded `.gba` file and compiled-but-discarded objects; an unused library's zero retained ROM cost does not mean its eventual integration is free.

Compiler stack estimates are not emulator stack high-water measurements. Delta/iPhone cold boot, flash flushing, export/import and user-facing recovery remain a required device gate before a playable milestone.

## Remaining integration decisions

- Authored initial player/world state, actual registry contents, release-specific transfer eligibility and destination initializer.
- Gameplay meanings/ranges for anonymous Aptitudes, personality, traits, research, recovery, equipment and future global records.
- Runtime ownership adapters replacing legacy storage/party/save copies before allocating and binding a live canonical Bondsaga session.
- Save/Continue and recovery UI, explicit replacement of invalid media, allocator/lineage creation and optional autosave policy.
- Device-measured stack/heap peaks and save latency in supported stable out-of-battle contexts.

Battle-Bound combat, content, balancing and narrative are not implemented by this foundation.
