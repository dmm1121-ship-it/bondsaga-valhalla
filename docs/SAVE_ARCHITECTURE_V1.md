# Bondsaga: Valhalla — Save Architecture V1

Status: **APPROVED TECHNICAL AUTHORITY FOR PROTOTYPE WORK**

This document promotes the core of Astra's persistence proposal into project authority, with design-level decisions made here to preserve gameplay flexibility and safe engineering margins.

## 1. Governing principles

- Bondsaga does **not** require compatibility with ordinary Pokémon Emerald saves.
- The 128 KiB FLASH1M device remains the target.
- The save format is explicitly serialized and versioned; raw C structs, pointers, enum order, and inherited Emerald save layout are not wire contracts.
- Part I and Part II share a stable saga identity/registry contract.
- Delta `.sav` export/import is transport only; Part II performs an explicit versioned migration.
- Save integrity and rollback safety are mandatory. Reloading and save-file use remain player-friendly; the format is not an anti-edit or anti-reload mechanism.

## 2. Physical save model

Use two complete 64 KiB recovery snapshots inside the existing 128 KiB flash save.

Each snapshot uses:

- 16 sectors × 4096 bytes
- 32-byte sector headers
- 4032-byte logical payload per sector
- CRC32C integrity checks
- a final commit record written only after the complete snapshot validates

A save transaction always writes the inactive snapshot, verifies it, commits it, and leaves the previous valid snapshot untouched as recovery material.

Do not add a third full save image, enlarge the physical save device, or rely on emulator save states.

## 3. Owned-creature capacity

**V1 authority: 384 total owned creature records across all locations, including the active expedition.**

Rationale:

- The saga currently targets roughly 150–250 original creatures across both games.
- 384 permits one of every species plus substantial room for duplicates, Alpha/Apex/Variant collecting, alternate evolutions, and favorites.
- It preserves materially more save headroom than retaining Emerald's inherited 420-box-slot comparison.
- Storage UX may present any convenient number of lodges/pages/boxes; UI grouping does not redefine the 384-record ownership ceiling.

If future validated content demonstrates that 384 meaningfully harms the intended collecting experience, capacity may be revisited only with a complete rebudget. Do not silently reduce creature-state fidelity to increase capacity.

## 4. Creature wire record

**V1 authority: target 112 bytes per owned creature.**

The record must explicitly support:

- immutable specimen serial
- stable current species/form ID
- stable lineage ID
- origin/provenance references
- 32-bit total EXP
- 16-bit earned and acquisition levels
- persistent HP/condition/recovery state
- explicit appearance identity independent of Alpha/Apex status
- personality profile
- independent Alpha, Apex, and Variant flags
- individual Bond progression
- nickname
- four selected move IDs and durable move-resource state for the prototype contract
- six natural Aptitude values
- six developed-growth values
- up to four persistent trait identities
- authored evolution/lineage milestones
- up to 64 lineage-stable Awakened outcomes
- two persistent history accumulators
- finite reserved bytes

Do not persist derived battle stats, species base data, type defaults, learnsets, Armament archetypes, or equipment-compatibility tables per creature.

### 4.1 Aptitudes and development

For the prototype/save schema, reserve exactly **six Aptitude axes** and six corresponding developed values.

The semantic names and balance formulas are not yet frozen. Engineering must not bake axis meaning into the wire layout beyond stable slot identifiers.

### 4.2 Traits

Reserve **four persistent trait identity slots**:

1. innate/specimen trait
2. developed trait
3. Bond trait
4. exceptional/rare trait

This is storage authority, not permission to make all four active simultaneously without combat-design approval.

### 4.3 Awakened Moves

Reserve **64 stable lineage outcomes** per creature plus two persistent sufficient-statistic counters.

Do not store arbitrary battle history. If a future Awakening design requires more than these lossless outcomes/counters, return for rebudget instead of overloading meanings.

## 5. Player/Binder envelope

Reserve **256 bytes** for the combined player/Binder core.

It must accommodate:

- player identity/customization
- options/accessibility/difficulty profile
- playtime and primary currency
- specimen serial allocator
- Binder progression/rank
- technique membership
- traversal/capability unlocks
- mastery for approximately 16 Armament archetype positions
- manifestation milestones
- persistent Binder health/condition/recovery where required
- selected Binder techniques where required
- reserved space for finalized Binder systems

Binder combat stats remain derived from Binder progression, Bound creature, Armament archetype, equipment, and temporary battle state.

## 6. Canonical ownership and references

There is one authoritative owned-creature store.

Expedition, storage, rehabilitation, Bound designation, deployment, and other facilities reference canonical records; they do not duplicate full creatures.

Roster-placement references may use compact validated table indices. Equipment bindings use stable specimen identities where persistence requires it.

## 7. Equipment persistence

Do **not** allocate equipment directly inside every creature record.

Equipment ownership and assignments live in global inventory/equipment sections.

Species compatibility is ROM data unless a future mechanic explicitly requires individual permanent exceptions.

Consumables/provisions must not duplicate global quantities merely because a creature has an assignment.

## 8. Research, rivals, contracts, and world state

These remain global/player-level systems rather than per-creature data.

For V1 budgeting:

- species research targets a registry capable of at least 256 species entries
- recurring rivals use bounded authored state, not complete battle transcripts
- contracts use compact completion/progress records
- Part I/Part II share only authored saga decisions and transferable progression
- raw Part I map flags, map IDs, script pointers, and local variables do not become Part II world state

Detailed gameplay semantics remain separate design work.

## 9. Part I → Part II transfer

The transfer contract must preserve:

- player identity/customization
- Binder-earned progression
- all owned creature identities, nicknames, levels, EXP, Aptitudes/development, traits, exceptional states, Awakened outcomes, Bond progression, and persistent health/recovery state
- relevant equipment/relic ownership
- research accomplishments
- authored rival state
- approved story decisions

Part II constructs fresh local-world state from an approved initializer.

No level normalization, creature replacement, silent healing, state deletion, or unknown-ID substitution is permitted merely to make import succeed.

Imported levels remain exactly earned, including values around/above Part I's intended endgame range.

## 10. Save scope

V1 saves stable **out-of-battle** state.

Mid-battle resume is not part of the initial architecture.

The two snapshots are recovery copies of one logical save, not two user-facing save slots.

Manual save and optional autosave behavior may share the same logical state, but retention policy must not compromise recovery guarantees.

## 11. Legacy Emerald systems

The new format may omit inherited serialization for systems Bondsaga does not use, including legacy contest records, Battle Frontier data, Mystery Gift payloads, record mixing, Secret Base data, Easy Chat, Emerald-specific TV/news/minigame records, and other compatibility-only state.

Removing legacy serialization does **not** automatically prohibit analogous Bondsaga features later. Any adopted feature receives its own explicit budget.

Breeding/ancestry remains undecided as gameplay; no Emerald daycare model is preserved merely for compatibility.

## 12. Safety margins

The project will not intentionally ship a save layout with only token headroom.

With 384 × 112-byte creature records, the creature store requires 43,008 bytes before its small section header, leaving materially more capacity than the 428-record draft scenario for research, inventory, rival state, contracts, world state, future migrations, and debugging safety.

A minimum planning reserve of approximately 4 KiB remains desirable after worst-case admitted state is defined. Prefer more.

No compression ratio is assumed in guaranteed capacity calculations.

## 13. Prototype implementation order

Implementation should proceed in this order:

1. explicit serializer/deserializer and versioned snapshot container
2. dual-snapshot transaction and integrity checks
3. canonical creature store with 112-byte wire records and 384-record capacity
4. minimal player/Binder block
5. roster/placement references for an eight-creature expedition target
6. migration/transfer scaffolding sufficient for synthetic Part I → Part II tests
7. golden fixtures, torn-write tests, malformed-save rejection, and capacity tests
8. only after persistence passes: Battle-Bound combat prototype

## 14. Non-authorized gameplay decisions

This document does not finalize:

- names/formulas of the six Aptitude axes
- full personality system
- exact trait activation/stacking rules
- detailed research tasks/rewards
- exact equipment slot rules
- exact injury/exhaustion recovery formulas
- breeding/ancestry gameplay
- final contract counts
- final rival adaptation model
- final autosave UX

Those are design matters and will be specified before relevant implementation.

## 15. Quality rule

If implementation pressure exceeds this save budget, return a visible tradeoff proposal. Never solve overflow by silently deleting earned state, auto-unequipping creatures, truncating names/history, reducing collection capacity, or weakening the documented game design.
