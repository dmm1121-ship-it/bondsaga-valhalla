# Bondsaga: Valhalla — Battle-Bound Combat Specification V1

Status: **APPROVED PROTOTYPE AUTHORITY** for the first playable combat vertical slice. This specification defines the canonical Battle-Bound model. It does not finalize balance values, canon creature content, narrative, art, or long-term progression formulas.

## 1. Core identity

Battle-Bound is the defining combat mechanic of Bondsaga.

- Any eligible owned creature may be designated as the **Bound Partner** outside battle.
- Only one creature is designated as Bound at a time.
- The Bound Partner is still a normal persistent creature with its own species, level, evolution, traits, personality, Aptitudes/development, Alpha/Apex/Variant state, moves/history, and Bond progression.
- Battle-Bound does **not** permanently assign one creature to the role. The player may change the designated Bound Partner between battles or in other approved safe contexts.
- During a Battle-Bound encounter, the Bound Partner does not deploy as an ordinary creature. Instead it manifests as the Trainer's Armament and contributes to a temporary battle-only combatant.
- The Trainer fights beside a separate normally deployed creature on the standard four-position doubles battlefield.

Canonical field arrangement for the player side:

1. **Active Creature** — ordinary deployed creature.
2. **Trainer + Bound Partner Armament** — temporary battle-only Battle-Bound combatant.

Enemy Battle-Bound Trainers use the same conceptual structure.

## 2. Terminology

Use these terms consistently:

- **Trainer**: the human combatant/user. Do not use "Binder" in player-facing terminology for this mechanic.
- **Bound Partner**: the single owned creature designated to manifest during Battle-Bound combat.
- **Battle-Bound Form**: the temporary Trainer + Bound Partner combat manifestation.
- **Armament**: the weapon/equipment manifestation produced by the Bound Partner. Armaments inherit the creature's design language but are not literal possessed objects with creature faces/eyes pasted onto them.
- **Active Creature**: the ordinary creature fighting beside the Trainer.
- **Bond Break**: the Battle-Bound Form ending because the Trainer/Battle-Bound combatant reaches zero HP.

## 3. Technical representation for prototype

For the first vertical slice, prefer the least invasive architecture compatible with the battle engine:

- Use one existing doubles battler slot for the temporary Trainer + Armament combatant.
- Construct a **battle-only Pokémon-compatible runtime combat structure** derived from Trainer state + the designated Bound Partner rather than persisting the Trainer as a fake species or seventh creature.
- The engine may route ordinary battle calculations through compatible battler structures where practical, but Battle-Bound-specific behavior must remain explicit and must not serialize as an owned creature.
- The Bound Partner remains referenced by the existing canonical roster Bound reference and must not be duplicated.
- Do not alter the 112-byte persistent creature wire record solely to make the prototype easier.

## 4. Bound Partner eligibility and selection

For Prototype 0.1:

- The player can designate any eligible expedition creature as Bound in a simple prototype menu or debug-facing UI.
- The Bound Partner must be a member of the expedition roster.
- The Bound Partner cannot simultaneously occupy the normal deployed-creature slot while Battle-Bound.
- Changing Bound Partner during battle is **not required** in Prototype 0.1.
- Rebinding, emergency swapping, multiple simultaneous Bounds, and legendary multi-stage manifestations are future design space and must not be implemented yet.

If engine simplicity benefits from internally normalizing the Bound Partner to a predictable party/roster index, that is acceptable only if it is invisible to the player and does not permanently reorder ownership identity.

## 5. Prototype combat action economy

Prototype 0.1 must preserve the feel of a doubles battle:

- The Active Creature receives one normal battle action each turn.
- The Battle-Bound Form receives one Battle-Bound action each turn.
- Both sides choose actions through the ordinary doubles turn structure where feasible.
- The Trainer/Armament is independently targetable by opponents.
- The Active Creature is independently targetable.
- The opposing Trainer/Armament and opposing Active Creature are independently targetable.
- Standard turn order/speed infrastructure may be reused for the prototype.

Do not finalize long-term action-resource systems, stamina, Resonance meters, combo gauges, reaction windows, parries, or special timing mechanics in this pass.

## 6. Prototype Battle-Bound stats

The long-term game will derive Battle-Bound combat performance from Trainer progression + Bound Partner characteristics + Armament archetype + equipment/traits as approved later.

For Prototype 0.1 only:

- Implement a **clearly isolated temporary derivation function** that creates usable HP, Attack, Defense, Special Attack, Special Defense, and Speed-compatible values for the runtime battler.
- Use simple documented placeholder formulas derived from the Trainer prototype level and Bound Partner battle stats/level.
- Do not encode these placeholder formulas into persistent data or treat them as final balance authority.
- Keep the derivation interface replaceable so later approved systems can substitute Trainer progression, traits, Aptitudes, Armament archetypes, and equipment without rewriting the battler lifecycle.

## 7. Prototype Battle-Bound moves

Prototype 0.1 needs only enough moves to prove interaction.

- Give the Battle-Bound Form a small temporary Armament technique set, ideally four prototype actions to fit the existing interface.
- At minimum include:
  1. one direct offensive technique;
  2. one defensive/protective technique;
  3. one technique that meaningfully interacts with or supports the Active Creature;
  4. one second offensive/control option.
- These moves are prototype-only and must not become canon names, balance, or final archetype design by accident.
- The Bound Partner's ordinary creature moves do not automatically become the Trainer's Armament moves.

## 8. Bond Break

When the Trainer/Battle-Bound Form reaches zero HP:

- trigger **Bond Break** rather than ordinary creature fainting semantics;
- remove/disable the Battle-Bound battler for the remainder of the prototype battle;
- the underlying Bound Partner remains owned and is not treated as defeated/captured/lost;
- mark the Bound Partner with a temporary prototype Exhausted/Bond-Broken battle state as needed for battle logic;
- the player's remaining ordinary creatures may continue the battle;
- Bond Break alone does **not** automatically cause match loss;
- the battle is lost only when the player's remaining valid battle resources can no longer continue under the prototype victory logic.

Do not implement long-term injury/recovery timing in this pass.

## 9. Switching and deployment

- The Active Creature may switch among eligible expedition creatures using the existing switching system where possible.
- The designated Bound Partner must be excluded from ordinary switch/deployment eligibility while manifested.
- The Battle-Bound Form itself does not switch like a Pokémon.
- Prototype enemy Trainers may use a fixed Bound Partner and a small ordinary creature roster.

## 10. Victory, defeat, capture, and battle types

Prototype 0.1 is a **Trainer-vs-Trainer Battle-Bound proof**, not the final wild-capture loop.

- Do not implement Subdual/capture yet.
- Do not implement wild Battle-Bound enemies.
- Do not implement Nemesis adaptation yet.
- Ordinary victory occurs when the opposing side has no valid remaining battle resources.
- Ordinary defeat occurs when the player side has no valid remaining battle resources.
- If the Trainer suffers Bond Break but an Active Creature/eligible reserves remain, combat continues.

## 11. Presentation requirements

The prototype must visually distinguish the two player-side battlers clearly on a 240×160 GBA screen.

For 0.1, placeholder art is explicitly acceptable.

Required presentation:

- the Trainer + Armament occupies its own battler position;
- the normal Active Creature occupies the other;
- HP/status UI distinguishes Trainer/Armament from creature;
- command flow makes it obvious whether the player is choosing the creature's action or the Battle-Bound action;
- Bond Break receives at least a basic visible message/effect;
- the Bound Partner itself should not appear as a second ordinary creature while manifested.

Do not spend significant production time on final sprites, canon creature design, elaborate Armament animation, final UI skin, or story presentation in this pass.

## 12. Prototype playable slice

The first player-facing milestone should be a small disposable test area/build containing:

- a minimal controllable overworld entry point;
- a small prototype expedition roster with at least two selectable candidate Bound Partners;
- a method to designate the Bound Partner before battle;
- one Battle-Bound Trainer encounter;
- player Trainer + Bound Partner Armament + one Active Creature;
- enemy Trainer + Bound Partner Armament + one Active Creature;
- target selection for all four battlers;
- Battle-Bound action selection;
- ordinary creature action selection;
- switching of ordinary creatures if practical within the minimal roster;
- Bond Break behavior;
- normal win/loss completion;
- an ordinary in-game save and reload if the persistence/runtime integration required for the slice can be completed without derailing the combat milestone.

If Save/Continue integration would materially delay the first combat proof, report that boundary and produce the smallest bootable battle slice first rather than expanding scope silently.

## 13. Acceptance criteria

Prototype 0.1 is successful when all of the following are true:

1. The ROM builds and boots in mGBA.
2. The player can designate one of multiple eligible creatures as Bound before the test battle.
3. The designated creature is excluded from ordinary deployment.
4. A temporary Trainer + Armament battler is created without creating a second persistent owned creature.
5. The Trainer/Armament and Active Creature each receive independent battle actions.
6. Both opposing battlers can be targeted independently.
7. Enemy Trainer/Armament can likewise act and be targeted.
8. Bond Break removes the Trainer/Armament combatant without automatically ending the match if ordinary battle resources remain.
9. Normal victory and defeat can resolve without corrupting party/roster state.
10. Changing the designated Bound Partner before a new battle changes which creature supplies the Battle-Bound derivation.
11. Existing inherited engine tests and Bondsaga persistence tests continue to pass, or any regression is explicitly identified and justified before merge.
12. The implementation remains architecturally replaceable for future final Battle-Bound formulas, Armament archetypes, traits, progression and visuals.

## 14. Explicit non-goals for Prototype 0.1

Do not implement or finalize:

- canon Bondsaga species;
- the final starter trio;
- final Armament archetype art;
- long-term Trainer progression formulas;
- final Battle-Bound stat formulas;
- final Battle-Bound move libraries;
- Subdual/capture;
- Alpha/Apex encounters;
- research/Pokédex systems;
- Guilds/hunts;
- Nemesis rivals;
- story/world/narrative;
- advanced injury/recovery;
- Part II/Asgard mechanics;
- mid-battle Rebinding;
- multi-Bound combat;
- full-armor legendary manifestation;
- final save/continue UX beyond what is strictly required for a disposable test build.

## 15. Design guardrail

Battle-Bound is not under evaluation as an optional feature. It is the core game mechanic.

The prototype exists to discover the best implementation and interaction model for the required mechanic, not to decide whether to remove or replace it with ordinary Pokémon combat.

If the current GBA architecture creates a conflict, report the conflict and propose implementation alternatives that preserve the canonical Battle-Bound fantasy rather than deleting or trivializing it.
