# Battle-Bound Prototype 0.1

Implementation report, not new design authority. The five V1 authority documents
remain controlling. This branch implements the disposable combat milestone in
`BATTLE_BOUND_SPEC_V1.md`, not final creatures, formulas, progression or content.

## Playing the build

Build Emerald with `make -j4 all`, or download `pokeemerald.gba` from the draft
PR's **Bondsaga persistence / bondsaga-persistence-measurements** artifact. The
artifact also includes the exact ELF, linker map, measurements and emulator
screenshots/reports. The exported ROM is padded to 32 MiB.

Press START through the inherited title sequence. In the small test room, use
the D-pad to walk the `T` marker. SELECT cycles the three eligible expedition
references. Walk right to TEST and press A. After either result, SELECT changes
the Bound Partner and A starts another fresh disposable encounter.

The two commands each turn are for the **Active Creature** and **TRAINER**.
Use FIGHT, choose a move, and select a target when the move supports selection.
Slash can target either opposing position. Protect is self-directed; Helping
Hand supports the normal ally; Icy Wind attacks both opposing positions and
lowers Speed. The ordinary creature can use PARTY to switch to the reserve.
The Trainer cannot switch. Bag, running and inherited Pokémon gimmicks are
disabled in this encounter. Bond Break visibly removes the Trainer position;
normal resources continue fighting. The Trainer can also continue if the last
normal creature falls first.

This build has **no Save/Continue**. It starts a disposable RAM session and
blocks legacy flash access while that session runs. Connecting a full canonical
session and recovery UI is the separate integration boundary explicitly allowed
by specification section 12. Emulator save states are not Bondsaga save files.

## Disposable fixtures and representation

Three 112-byte canonical wire records use Combusken, Marshtomp and Grovyle at
level 20, with unique serials 1–3. These are temporary engine species, not canon
Bondsaga creatures or starter proposals. Their ordinary first attacks are Ember,
Water Gun and Absorb respectively, followed by Swift, Protect and Quick Attack.
The enemy uses Grovyle as its normal creature and Marshtomp as its fixed Bound
source. The player has one ordinary reserve; the enemy has none in this room.
These fixture choices carry no final balance authority.

`BsgRoster.bound` is the canonical designation. A Bound record is never staged
as an ordinary participant. At entry, two eligible ordinary player creatures
are staged in legacy runtime party slots 0 and 2. Slot 1 is a transient
Pokémon-compatible Trainer adapter. The enemy stages an ordinary creature and
its own adapter. Right-hand battlefield positions identify Trainer roles;
species IDs never identify those roles. Ownership order and serials do not
change. The room clears all runtime parties on exit and does not write back
battle HP, EXP, exhaustion, rewards or a fake Trainer record. The inherited
2,400-byte replay party backup is skipped; no extra copy of the adapters is
retained after battle. The smoke check verifies equal free heap space before
and after the encounter.

The adapter uses an existing Smeargle compatibility shell, with explicit human
Youngster graphics and a plain Armament bar, a TRAINER/RIVAL-ARM nickname,
neutral ability and temporary Normal typing. It has a separate move set from
the Bound creature. No persistent species was added and the 112-byte wire
format is unchanged.

`BsgBbDeriveStats` is the replaceable formula boundary. With prototype Trainer
level 20, HP is `30 + Trainer level + partner max HP / 2`; each other stat is
`Trainer level + corresponding partner stat / 2`, using integer division.
All results live in battle runtime. No Aptitude, trait, equipment, personality,
Bond or reserved-byte gameplay meanings are assigned.

Bond Break has an explicit battle script and battle-only broken-position mask.
Both direct-damage and script/indirect faint paths reach it. It skips ordinary
creature faint bookkeeping and does not fill the Trainer position with a
reserve. Existing doubles ordering, move effects, targeting and win/loss
resolution remain in use. EXP is disabled for the disposable encounter.

## Validation

The production-source host persistence suite remains unchanged: 18 groups and
938,454 assertions, with Linux CI ASan/UBSan. Four inherited Bondsaga GBA tests
exercise the foundation on the emulator target.

Nine targeted Battle-Bound test groups cover canonical designation, immutable
partner derivation, four independent actors/targets, guard/support effects,
both sides' Bond Break, normal reserve switching after Bond Break, continuing
with only a Trainer, and final victory/defeat. They also reject every inherited
gimmick in the prototype. Run `make check TESTS="Battle-Bound"`. CI separately
runs Bondsaga, Helping Hand and Protect filters, plus the repository's existing
full build/test workflow. Check the draft PR for exact-head results.

`tools/bondsaga_save/mgba_probe.c` and `prototype_smoke.py` exercise the actual
ROM with buttons, not memory writes, forced outcomes or emulator save states.
They cold boot, select a Bound reference, walk to the encounter, operate both
command menus, observe Bond Break and the outcome, check the original 336
canonical bytes and designation, then enter another battle after changing Bound
and verify different derived Trainer HP. Read-only RAM observations use the
current GBA ABI and exact-build `arm-none-eabi-nm -n` symbols; this is a prototype
test aid, not a portable saga serialization format.

On a host with matching mGBA 0.10 headers/library and libpng:

```sh
cc -std=c11 -O2 tools/bondsaga_save/mgba_probe.c -lmgba -lpng -o build/mgba-probe
arm-none-eabi-nm -n pokeemerald.elf > build/prototype-symbols.txt
python3 tools/bondsaga_save/prototype_smoke.py \
  --probe build/mgba-probe --rom pokeemerald.gba \
  --symbols build/prototype-symbols.txt --output build/prototype-smoke --bound 1
```

Use an immutable ROM while the emulator runs; do not rebuild that mapped file.
Local Windows validation used mGBA 0.10.5 and ARM GCC 16.1. CI uses its installed
ARM GCC for both baseline and candidate. The local button tests demonstrated
victory, defeat, Bond Break continuation, unchanged owned records, and repeat
entry with different partner-derived stats. They are not Delta device tests.

The visual test caught an inherited integration trap: directly created enemy
party records lack the trainer loader's `BLOCK_AI_DYNAMAX` marker, so the AI
could Dynamax. An explicit prototype guard in `CanActivateGimmick` now excludes
all inherited gimmicks for both sides. This preserves the specified four-action
combat scope without changing the ordinary engine outside Battle-Bound.

## Memory and remaining gates

The workflow compares `831c6ad4` with the candidate using one toolchain and
publishes exact ROM/static RAM deltas. Local final gameplay build uses 26,741,228
linker ROM bytes, 229,616 EWRAM bytes and 28,388 IWRAM bytes; these compiler-specific
totals must not be subtracted from a differently compiled baseline. An earlier
same-toolchain CI comparison measured +3,196 bytes EWRAM and no IWRAM increase;
the exact candidate ROM delta is in the workflow artifact.

The new static EWRAM includes 336 canonical wire bytes, an 804-byte roster view,
a 2,048-byte room tilemap, and small lifecycle/UI state plus alignment. The room
also allocates a 19,200-byte window pixel buffer and frees it before battle.
The existing runtime parties and doubles battle allocations are reused. No
second 384-record owner or 64 KiB snapshot buffer is allocated. Compiler frame
estimates for the new modules are published with the artifact; they do not
measure complete call-chain stack or peak heap usage.

Persistent usage is unchanged: the approved worst-case snapshot allowance is
56,592 / 65,536 bytes, leaving 8,944 bytes per bank. Both banks stay within the
128 KiB FLASH1M target. All future-system reservations remain charged.

Remaining review/device gates:

- Delta/iPhone cold boot, controls and presentation need device confirmation.
  The ROM is intended for an exploratory combat test, not a save-transfer test.
- The runtime adapter is certified only for this encounter's move set. Final
  forced-switch effects, transformations, abilities, revival, equipment and
  advanced status interactions need explicit lifecycle policy and tests before
  they enter the Battle-Bound move/content set.
- Full owned-session adapters, Save/Continue/recovery, peak stack/heap profiling
  and save latency remain separate integration work. No Emerald serialization
  is used as a substitute for the approved container.
- Final eligibility, Trainer growth, formulas, Armament archetypes, progression,
  exhaustion/recovery and all narrative/content decisions remain with design
  authority. No such decisions are required to test this disposable slice.

Stop at Prototype 0.1 review; this implementation does not authorize broader
game production.
