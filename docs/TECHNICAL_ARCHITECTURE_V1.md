# Bondsaga: Valhalla — Technical Architecture V1

## Purpose
This document converts the Design Bible and initial Astra reconnaissance into implementation boundaries for `valhalla-core`. It is intentionally conservative: define the architecture first, then prove the signature systems in a disposable vertical slice before producing canon content.

## Non-Negotiable Platform Target
- Primary runtime: Game Boy Advance compatible `.gba` ROM.
- Primary user emulator target: Delta on iPhone.
- ROM budget: conventional 32 MiB GBA address space.
- Part I and Part II are separate ROMs sharing a deliberate save-transfer contract.
- No design may silently require abandoning GBA/Delta compatibility.

## Development Strategy
1. Preserve `master` as a clean upstream `pokeemerald-expansion` reference.
2. Use `valhalla-core` for shared engine work and architecture.
3. Prove risky systems in a disposable vertical slice before canon production.
4. Fork stable shared systems into Part I production only after vertical-slice acceptance.
5. Design Part II compatibility from day one, but build Part II content after Part I proves the systems in real play.

## Core Architectural Principle
The project must not treat Bondsaga as a cosmetic Emerald hack. Its signature systems alter assumptions in battle, persistence, progression, party management, and UI. Therefore systems are introduced through explicit engine abstractions rather than scattered one-off patches.

## 1. Binder Representation
### Decision
The Binder is a first-class battle participant, not merely trainer graphics layered over a creature battler.

### Requirements
- A battle-side abstraction must distinguish `CREATURE` and `BINDER` combatants while still using the existing four-position doubles battlefield where practical.
- Binder combat stats derive from:
  - Binder progression
  - current Bound creature
  - Armament archetype
  - compatible equipment/relic modifiers
  - temporary battle state
- Binder targeting, damage, status, AI evaluation, defeat, animation, controller flow, and result handling must be explicit rather than faked through an ordinary party creature.
- The Bound creature may not simultaneously occupy a normal creature battler slot.

### Bond Break
- Binder reaching 0 HP causes Bond Break, not immediate match defeat.
- Bound creature returns in an Exhausted state.
- The battle may continue using remaining deployed creatures.
- Exact re-entry and exhaustion duration rules will be implemented conservatively in the vertical slice and then playtested before canon lock.

## 2. Battle-Bound / Armament Architecture
### Decision
Use a limited set of shared Armament archetypes with species-line-specific manifestations.

### Target
Approximately 15 archetypes, subject to technical validation.

### Rules
- Evolutionary lines remain within the same broad archetype family unless there is a strong authored reason to diverge.
- Species identity must still be visually obvious through weapon morphology, motifs, palette, silhouette details, and advanced manifestation features.
- Armaments do not retain literal creature eyes/faces as possessed-object decoration.
- Progression may escalate from weapon manifestation to partial armor and, for suitable Mythic/Legendary/True Legendary bonds, major or full-body manifestation.

## 3. Level Architecture
### Decision
Part I targets approximately levels 1–100. Part II continues approximately 100–200.

### Engineering Rule
Do not attempt to support 200 by changing `MAX_LEVEL` alone.

### Required Work
- Widen persistent level/EXP-related storage where necessary.
- Define Bondsaga experience curves deliberately rather than extending incompatible vanilla formulas blindly.
- Audit all level assumptions, level comparisons, move learning, evolution, AI, encounter generation, UI, three-digit displays, level caps, and serialization.
- Over-leveling is allowed within reason. Use diminishing EXP efficiency rather than hard progression caps.

## 4. Party / Expedition Architecture
### Design Target
- Expedition roster target: 8 creatures.
- Major battle deployment target: Binder + selected deployed creatures, with the Bound creature reserved from ordinary deployment.

### Engineering Rule
Do not implement this through `PARTY_SIZE 8` alone.

### Required Audit
- party arrays
- menus
- EXP recipient counters
- switching
- healing
- storage transfer
- field access
- AI
- battle result flow
- save structures
- scripts and UI assumptions

The exact final expedition/deployment count may be adjusted only if real play or hard hardware constraints show the current target harms the experience.

## 5. Persistent Creature Data
### Principle
Save capacity is the primary persistent-data constraint, not ROM capacity.

### Rule
Do not bolt numerous independent fields onto every stored creature without a byte budget.

### Direction
Prefer compact/packed representations and derived data where possible.

Persistent systems requiring budgeting include:
- Aptitudes / development
- multiple trait state
- personality/nature reinterpretation
- Alpha/Apex/Variant state
- Awakened Move history
- Bond-relevant progression
- equipment compatibility state only where it truly must be per-creature

### Integrity
Critical continuation state must be checksum-protected. Do not place saga-critical data into unprotected extension bytes without adding integrity/version handling.

## 6. Aptitudes and Development
Traditional IV/EV min-maxing will not be preserved as the primary player-facing system.

### Design Goals
- individuals feel meaningfully different
- no creature is permanently garbage because of hidden birth rolls
- growth reflects use and experience
- diminishing returns prevent mandatory grinding
- player-facing information remains understandable without calculators

Final axes/ranges will be specified before implementation. Astra must not invent them from necessity.

## 7. Traits / Abilities
### Direction
Creatures may possess multiple logical traits rather than one mutually exclusive ability.

Tentative conceptual slots:
- innate/species trait
- developed trait
- Bond trait

This structure is not yet a license to stack arbitrary modifiers. Traits must be complementary, readable, and balanceable. Implementation must include a deterministic interaction/order model once finalized.

## 8. Equipment
Held-item logic will evolve toward logical equipment/provision categories rather than one universal held slot.

Tentative categories include:
- carried consumable/provision
- worn equipment
- relic/special equipment

Not every creature can use every equipment form. Equipment rules should reflect creature morphology and gameplay readability without becoming inventory simulation.

## 9. Subdual / Capture
### Principle
Catching remains combat-forward and simple.

### Core Rule
When an eligible direct attack would reduce a capturable wild creature to 0 HP, the player may choose to Subdue instead of finish.

### Requirements
- no dating-sim temperament puzzle
- no huge family of capture-device variants
- no accidental critical-hit destruction of a desired creature once the Subdue decision is available
- wild doubles, indirect damage, multihits, storage-full handling, and capture cancellation require explicit rules

Upstream Victory Catch may be mined for implementation ideas but must not define Bondsaga behavior.

## 10. Research / Pokédex
The Pokédex is a research and mastery system, not only a seen/caught checklist.

Information disclosure must be staged. Evolution, Awakened Move, ecology, Alpha/Apex and other advanced information should be revealable through research rather than automatically exposed by upstream optional Pokédex features.

## 11. Alpha / Apex / Variant
These are independent concepts.
- Variant: rare coloration/pattern identity.
- Alpha: uncommon exceptional individual.
- Apex: much rarer exceptional individual with deeper mechanical identity.

A creature may combine these states. Exact rates are balance data, not architectural constants.

## 12. Traversal
Traversal is capability-based rather than move-slot taxation.

Compatible creatures may provide traversal capabilities once Binder progression/training unlocks the relationship. The system must prevent softlocks and should not require an exact species unless authored for a special case.

## 13. Guild / Hunt Framework
Guild halls/waystations provide the practical replacement for generic Pokémon Center repetition.

Expected service families:
- healing / rehabilitation
- roster / storage management
- supplies / equipment
- research services
- travel / local information
- optional contracts / hunts

Contracts should support meaningful categories such as Apex hunts, rescues, bounties, surveys, myth hunts, and longer saga contracts.

## 14. Rival / Nemesis Framework
Recurring rivals are authored persistent characters with adaptive state, not procedurally generated disposable NPCs.

The system may track a compact set of observations such as:
- results against player
- common player battle tendencies
- recurring creature/Armament usage
- relationship/disposition state
- major authored milestones

Adaptation must be bounded and narratively authored. Some worthy rivals may die and later reappear in Valhalla/Asgard in stronger forms when story conditions justify it.

## 15. Failure / Recovery
- No permadeath for the player's owned creatures.
- Normal KO is recoverable.
- Serious injury/incapacitation may temporarily limit availability and recover through gameplay progression rather than real-time waiting.
- Manual saving/loading remains unrestricted enough to respect single-player autonomy.

## 16. Part I / Part II Save Contract
### Principle
Part II is a continuation, not a reset or expansion-state copy.

Transfer must preserve at minimum:
- player identity/customization
- Binder progression
- creature identity and nicknames
- levels and experience
- relevant aptitudes/development
- traits/personality
- move state including Awakened Move history
- Alpha/Apex/Variant state
- relevant equipment/relic continuity
- research state where meaningful
- rival/Nemesis state
- story decisions required by Part II

### Requirements
- stable identifiers shared across both games
- explicit schema version
- import eligibility marker
- integrity validation
- repeat-import policy
- corrupted/incompatible save handling
- destination-world initialization distinct from copied Part I map flags

Delta save import/export is transport only, not the data contract itself.

## 17. CI and Test Strategy
- `valhalla-core` should receive CI on push.
- mGBA automated tests remain useful for engine validation.
- Delta/iPhone remains a required target-device validation gate for playable milestones.
- The disposable vertical slice must prove:
  1. Binder as a real combatant
  2. one Battle-Bound archetype
  3. Bond Break
  4. Subdual
  5. Binder progression
  6. expanded expedition behavior
  7. save/load stability
  8. one recurring rival state update

## 18. Content Production Gate
No canon creature roster, final maps, or large-scale narrative scripting should begin until the vertical slice proves the shared architecture is viable and fun enough to justify production.

## 19. Do Not Improvise Boundaries for Implementation Agents
Implementation agents may optimize code and propose alternatives, but must not independently redefine:
- story/world canon
- creature philosophy
- Battle-Bound fantasy
- progression philosophy
- capture philosophy
- Part I / Part II continuity
- difficulty philosophy
- player-autonomy philosophy

If engine constraints force a material design tradeoff, stop and escalate the decision instead of silently substituting a different game.

## 20. Quality Gate
Every major feature must answer:
1. Is it fun in active play?
2. Is it coherent with the world and adjacent systems?
3. Does it add meaningful choice, discovery, character, or progression?
4. Is its ROM/RAM/save/complexity cost justified?

If not, redesign or remove it.
