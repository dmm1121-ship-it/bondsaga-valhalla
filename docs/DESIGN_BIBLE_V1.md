# Bondsaga: Valhalla — Design Bible V1

## Project Thesis
Bondsaga: Valhalla is a Delta-first GBA monster-collecting RPG built from the pokeemerald-expansion codebase. It is aimed at adults who grew up with classic Pokémon and wanted the genre to mature mechanically and narratively without becoming grimdark, tedious, or obsessed with competitive min-maxing.

The governing rule is simple: depth where depth creates interesting decisions; convenience where complexity would only waste the player’s time.

## Core Experience Pillars
1. Discovery matters again. The player should regularly encounter creatures, evolutions, abilities, locations, and mechanics they do not already know.
2. Creatures are organisms first, game pieces second. Systems should model growth, individuality, ecology, and bonds without turning play into spreadsheet homework.
3. Battle-Bound combat is the signature mechanic. The player fights alongside a creature while another chosen partner manifests as a weapon/armament.
4. The world is Nordic-inspired and mythologically grounded, but not every creature, town, or quest is a Norse reference.
5. Mature means adult-targeted and believable, not juvenile-edgy. Death, violence, politics, religion, betrayal, relationships, profanity, and moral ambiguity may exist where appropriate.
6. Difficulty should reward understanding without forcing hardcore play. Over-leveling is allowed within reason, saves can be abused freely, and nothing is permanently lost through ordinary play.
7. Optional content must be meaningful. Guild hunts, rival arcs, Pokédex research, secrets, Apex hunting, and mythic encounters should reward exploration rather than pad runtime.
8. Fun and coherence outrank feature count.

## Technical Target
- Platform: Game Boy Advance ROM
- Primary emulator target: Delta on iPhone
- Base: pokeemerald-expansion
- ROM budget: conventional 32 MB GBA space per game
- Structure: two-part saga sharing a deliberate save-transfer schema
- Part I: Mortal Realms, approximately level 1–100
- Part II: Asgard / Ragnarök, approximately level 100–200
- Part I target playtime: roughly 40–60+ hours for a substantial exploratory playthrough, without padding
- Part II target: full continuation RPG, not DLC or an epilogue

## Creature Roster
- Saga target: approximately 150–250 original creatures total
- Strong preference for deliberate evolutionary families over isolated one-off species
- Evolution chains may exceed three stages where conceptually justified
- One-stage species should be uncommon
- Legendary and mythic species may evolve if the concept supports it
- Official Pokémon are not required; quality and coherence matter more than nostalgia

## Creature Design Rules
- Visual language should broadly evoke the strongest Gen III–VI monster-design sensibilities: strong silhouette, readable anatomy, restrained detail, ecological plausibility, and clear evolutionary continuity.
- Avoid literal object-creature reskins such as teapots, appliances, or household items with faces.
- Object-like motifs are acceptable when biologically integrated into the organism.
- Battle-Bound manifestations are exempt from the object rule because they are temporary armament forms, but they must not simply paste the creature’s face or eyes onto a weapon.
- Battle-Bound forms should inherit the creature’s design language, materials, motifs, motion, and personality rather than literal anatomy.
- Nordic fauna, folklore, myth, climate, and material culture should strongly influence the roster, but should not dominate every family.

## Battle-Bound System
- Most important battles are doubles-focused.
- One active combatant is a conventional creature.
- The second active combatant is the human Binder using one preselected Bound creature as an Armament.
- The Bound creature cannot simultaneously participate as a conventional battlefield creature.
- Approximately 15 core Armament archetypes should cover implementation needs while preserving species-unique manifestations.
- Evolutionary families generally remain within the same Armament archetype, with manifestations becoming more sophisticated as the creature evolves.
- Progression can expand from weapon-only manifestations to weapon + partial armor, advanced armament, major armor, and rare full-body manifestations for high-tier mythic/legendary Bonds.

## Binder Progression
The protagonist has independent RPG progression.

Binder progression may affect:
- Vitality
- Guard
- Resonance
- Focus
- Battle-Bound techniques
- Armament mastery
- defensive manifestation
- advanced synchronization
- traversal capability
- late-game mythic/full-manifestation techniques

Trainer combat stats are influenced by personal progression, Bound creature, Armament archetype, and equipment.

If the Binder reaches 0 HP during battle, the default state is Bond Break rather than instant defeat. The manifestation collapses, the Bound creature becomes exhausted, and remaining creatures may continue the battle. Full defeat occurs only when the remaining combat resources are also defeated.

## Levels and Difficulty
- Part I is designed around level 1–100.
- Part II continues directly from the imported levels, generally spanning approximately 100–200.
- No forced reset between games.
- Over-leveling is permitted.
- Soft progression curves, diminishing low-level EXP, and encounter design should discourage absurd grinding without imposing hard caps.
- Difficulty settings should primarily affect AI quality, team construction, tactical pressure, and forgiveness rather than stripping player tools.
- No Nuzlocke or mandatory permadeath philosophy.

## Capture / Subdual
Capturing should remain straightforward and combat-forward.

Core rule:
When a capturable wild creature would be reduced to 0 HP, the player may be given a contextual choice to Subdue rather than finish it.

Goals:
- Prevent accidental critical-hit loss of rare targets
- Preserve the satisfaction of overcoming a creature in battle
- Avoid personality-dating-sim capture systems
- Avoid dozens of redundant capture-item types
- Allow rarity, Binder skill, research, Alpha/Apex status, and status conditions to influence reliability without requiring obscure calculators

## Individuality and Growth
Traditional IV/EV systems should be reconsidered.

Preferred direction:
- visible natural Aptitudes rather than hidden genetic lottery values
- development shaped by how a creature is actually used
- diminishing returns to prevent mandatory grind loops
- no permanently worthless specimen simply because it rolled poor invisible numbers

Potential Aptitude axes include Power, Resilience, Agility, Focus, Instinct, and Bond.

## Traits / Abilities
Creatures should not be conceptually limited to one arbitrary Ability if a richer biological system is technically and mechanically feasible.

Preferred structure:
- Innate Trait
- Developed Trait
- Bond Trait
- rare exceptional additions for Apex or special individuals

Traits must be complementary, logical, and readable rather than stacked multiplier nonsense.

## Equipment
Held items should be reinterpreted into more sensible equipment logic.

Possible categories:
- Carried consumables/provisions
- Worn equipment
- Relics

Species compatibility should matter. Creatures may carry multiple sensible consumables rather than being restricted to one berry solely because inherited rules say so.

## Natures / Personality
Natures should describe actual behavior rather than only hidden stat math.

Examples of meaningful effects may include:
- protection behavior
- Bond growth
- resistance tendencies
- learning tendencies
- weakened-target behavior
- overworld/Pokédex interactions

## Move Learning
Three main acquisition paths:
1. Natural Moves — learned through growth and evolution
2. Taught Moves — deliberately taught by trainers, analogous to TMs
3. Awakened Moves — discovered through ancestry, battle history, environment, Bond use, or unusual triggers

Awakened Moves should preserve mystery. The game may provide ecological/research hints rather than exposing every formula.

## Alpha / Apex / Variants
Color variants are separate from combat status.

- Variant: rare coloration/pattern
- Alpha: uncommon exceptional specimen
- Apex: very rare exceptional specimen with meaningful gameplay differences

Alpha target rarity may be around 1/25 before modifiers.
Apex should be substantially rarer, roughly on the order of 1/250 before modifiers, subject to later balancing.

Apex differences may include size, traits, Aptitude potential, rare Awakened-Move ancestry, markings, and distinct Battle-Bound cosmetics rather than only flat stat inflation.

## Pokédex / Research
The Pokédex is a research system, not a checkbox list.

Progress may come from:
- encountering
- battling
- capturing
- observing habitats
- discovering evolutions
- documenting Alpha/Apex variation
- discovering Awakened Moves
- Battle-Bounding a species

Species Mastery should grant useful knowledge and tools such as tracking, evolution clues, Alpha detection, Awakened-Move hints, or training benefits.

## Traversal
Traversal should be capability-based, not HM-slave-based.

Potential capabilities:
- Mount
- Climb
- Swim
- Dive
- Glide/Fly
- Break
- Illuminate
- Burrow
- Extreme climate traversal

Any biologically appropriate creature may perform a capability once the Binder has learned the relevant technique. Traversal should not consume combat move slots.

## Guild / Waystations
Pokémon-Center-style functionality is replaced or expanded through Guild Halls, Lodges, and Waystations.

Core services may include:
- healing
- rehabilitation
- storage access
- roster management
- supplies
- research
- training
- travel support

Optional Hunt Boards may offer:
- Cull Hunts
- Apex Hunts
- Rescue contracts
- Bounties
- Surveys
- Myth Hunts
- Saga Contracts

Rewards must be proportionate and meaningful.

## Party / Expedition Structure
Current working direction:
- Expedition roster larger than six, initially targeted at eight creatures
- one designated Bound creature
- major battles may require a chosen deployment subset
- storage/roster access should be modernized and convenient outside restricted situations

Exact numbers remain subject to prototype testing.

## Injury and Recovery
- Normal knockout is temporary and recoverable
- Severe defeat may cause temporary Incapacitation or injury
- Recovery should occur through gameplay progression, battles, travel, facilities, or resources
- No real-time waiting
- No ordinary permadeath

## Mythic Hierarchy
Working creature tiers:
- Common / Uncommon / Rare
- Alpha
- Apex
- Mythic
- Legendary
- True Legendary

Legendary species need not mean literally one specimen exists in the world.
True Legendary status may refer to named individuals whose existence shaped mythology.

## World Structure
The world is Nordic-inspired and organized around the Nine Realms as a mythological spine.

Part I focuses on the mortal campaign, eight major progression challenges/Sigils, Elite Four, and Champion.
Asgard is the major continuation realm in Part II rather than a small postgame zone.

Part II escalates into the Pantheon, Valhalla, higher-order Bonds, mythic ecology, and Ragnarök-level stakes.

Norse names and mythology may be used directly where appropriate. Mythological artifacts can be reinterpreted as ancient or legendary Battle-Bound manifestations.

## Rival / Nemesis Philosophy
Recurring rivals should behave as persistent characters rather than scheduled exposition machines.

Trackable development may include:
- wins/losses versus player
- common player tactics
- preferred damage/type patterns
- Battle-Bound archetype
- relationship/disposition
- team evolution
- responses to prior defeats
- victories/losses against other rivals

Some worthy rivals may die during appropriately serious conflicts and later reappear in Valhalla/Asgard in stronger forms. This should be rare, narratively earned, and memorable.

## Save Philosophy
- unrestricted normal saving where technically reasonable
- optional autosave/checkpoint support if feasible
- no moralizing about reloading or scouting bosses
- the player may replay or reload encounters freely
- Part I → Part II transfer must preserve identity, roster, levels, growth, traits, personalities, Awakened Moves, Alpha/Apex/Variant state, Binder progression, research, significant equipment, rival state, and relevant story decisions

## Two-Part Saga Structure
### Part I — Mortal Realms
- complete RPG experience in its own right
- roughly Lv.1–100
- ~40–60+ hour substantial exploratory target
- eight major progression challenges
- Elite Four / Champion as a genuine climax
- ending opens the path to Asgard

### Part II — Asgard / Ragnarök
- direct continuation, not DLC
- imports the Part I save state
- roughly Lv.100–200
- no reset or normalization of earned creatures
- new ecosystems, evolutions, Pantheon encounters, Valhalla, advanced Armaments, and Ragnarök campaign
- intended to feel like a second full RPG

## Adult Tone Guardrail
The game is for adults who grew up with classic monster-collecting RPGs.

Allowed where appropriate:
- death
- violence
- grief
- politics
- religion
- alcohol
- betrayal
- crime
- profanity
- relationships
- morally compromised institutions and characters

Avoid:
- gratuitous edginess
- shock content for its own sake
- grimdark tonal monotony
- childish censorship that makes adults speak unnaturally

Charm, humor, cozy towns, strange NPCs, exploration, fishing, local culture, and moments of levity remain essential.

## Spoiler Preservation
The player/creative director should not need to know every creature, evolution, rival team, hidden area, boss composition, Awakened Move, Apex, mythic encounter, or story reveal.

Development should distinguish between:
- Director-facing decisions requiring approval
- Implementation/player-secret content that can remain hidden to preserve first-playthrough discovery

## Final Quality Gate
Every feature, creature, quest, area, and mechanic should be judged against four questions:
1. Is it fun?
2. Is it coherent with the world and systems?
3. Does it add meaningful choice, discovery, or character?
4. Is its cost justified within the GBA/Delta technical budget?

If the answer is no, cut or redesign it rather than defending sunk cost.
