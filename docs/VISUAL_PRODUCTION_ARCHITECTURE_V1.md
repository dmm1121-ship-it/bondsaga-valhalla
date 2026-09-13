# Bondsaga — Visual Production Architecture V1

**Status: technical proposal for design-authority review; not approved visual canon or authorization for production.** Prepared 2026-09-13. Scope: source-art contracts, conversion, validation, runtime constraints and a small future calibration batch. No creatures, archetype identities, palette aesthetics, environments or gameplay are designed here.

## 1. Authority and accepted baseline

The [Design Bible](DESIGN_BIBLE_V1.md), [Technical Architecture](TECHNICAL_ARCHITECTURE_V1.md), [Save Architecture](SAVE_ARCHITECTURE_V1.md), [Persistence Foundation](PERSISTENCE_FOUNDATION_V1.md) and [Battle-Bound specification](BATTLE_BOUND_SPEC_V1.md) govern this proposal. The latest specification's player-facing term is **Trainer**, even where older technical structures say Binder. The user's production instructions supplement these documents; this proposal does not supersede them.

The user reports that corrected Prototype 0.1 passed real Delta/iPhone testing and accepts it as the technical proof. This is user-reported device acceptance, not a new independently performed device test. [PR #2](https://github.com/dmm1121-ship-it/bondsaga-valhalla/pull/2) remains draft and unmerged, preserved on `codex/battle-bound-prototype-01` at `d1c2f3e6e5c5c746269acbbb5589454032e46423`. Its obedience correction is commit `6f594f38`. This proposal branch descends from that accepted head; it does not replace or rewrite it. Eventual integration into `valhalla-core` remains a separate reviewed operation.

Verified evidence:

- [Full CI](https://github.com/dmm1121-ship-it/bondsaga-valhalla/actions/runs/34736960252): successful builds and 5,412 test passes; 16 unchanged known failures, 421 TODOs and nine expected failures.
- [Bondsaga CI](https://github.com/dmm1121-ship-it/bondsaga-valhalla/actions/runs/34736960229): successful persistence, Battle-Bound, inherited Sleep and actual-ROM button tests.
- [Accepted artifact](https://github.com/dmm1121-ship-it/bondsaga-valhalla/actions/runs/34736960229/artifacts/10311296768): ROM SHA-256 `5747c9b78cfb8bd42d7bb73da98131c0e0056030abe478d14ce1aaca89bef1a8`. The ELF uses GitHub's synthetic merge `bd9a705844b10782a002485359a94a5e2c41884b`, testing the accepted head against `valhalla-core` `831c6ad4`; this is not a merged PR.

No runtime, assets, game configuration or save structures change in this proposal. Preserve the early Bondsaga obedience bypass, legitimate combat Sleep, one canonical Bound reference, battle-only Trainer representation and existing regression tests during later asset integration. Save usage remains **56,592 bytes per 65,536-byte snapshot, with 8,944 bytes reserve**, in the same 128 KiB FLASH1M medium. Art IDs and animation data belong in ROM registries; art revisions must not consume reserved creature bytes or duplicate owned records.

## 2. Creative contract for technical production

The design authority supplies the franchise style guide, approved concepts and evolutionary relationships. Technical artists translate those approvals into readable assets; they do not fill gaps with new anatomy, ecology, personalities, mythology, progression or archetypes.

Carry these constraints into every brief and review:

- Creatures are coherent organisms or mythic beings, not appliances, furniture, containers or other manufactured objects with faces. An Armament manifestation does not redefine the underlying creature as an object.
- Each later stage remains a recognizable descendant. Family graphs may exceed three stages; single-stage designs are uncommon and purposeful. Mythic/Legendary status does not imply evolutionlessness.
- Nordic geography, ecology and mythology provide foundations without restricting the ecosystem to literal Norse creatures. No new mythology or naming decisions are made by conversion staff.
- Prioritize silhouette, anatomy, posture, proportion, controlled detail and sensible color relationships. Consistency does not mean giving every species the same body shape or rendered size.
- Armaments inherit approved motifs and materials, without pasted-on creature faces or eyes. Evolution must affect the approved manifestation; approximately 15 shared motion families remain a technical direction, not a finalized archetype list.
- Alpha, Apex and Variant are independent future systems. Reserve measurable asset allowances and test surfaces; do not choose their mechanics, appearance mapping or final palette treatments here.
- Existing Pokémon graphics are engineering placeholders and measurement samples only. Neither these samples, random internet Fakemon nor community/generated sprites become final Bondsaga art by convenience.

## 3. Audited engine constraints

These are facts about the accepted source, not creative requirements imposed by Pokémon tradition.

| Surface | Current contract | Production implication |
|---|---|---|
| Display | 240×160 pixels | Review assets at native resolution in the actual doubles layout, including the command panel and health boxes. |
| Creature front | 64×64, 4bpp; normally two vertically stacked frames, PNG 64×128 | 2,048 decoded bytes per frame; frame 0 must work as the static portrait. |
| Creature back | 64×64, 4bpp; sampled normal backs use one frame | 2,048 decoded bytes; requires independently approved rear anatomy and pose, not a flipped front. |
| Icon | 32×32, 4bpp; two frames, PNG 32×64 | 512 bytes per frame, 1,024 bytes raw per icon; the loader directly uses raw tile data. |
| Battle palette | 16 indices, including transparent index 0; RGB555 | Up to 15 visible colors per ordinary sprite palette. Front/back and alternate frames must obey a consistent index contract. |
| Icon palettes | Six shared palettes; species selects one through a 3-bit index | Not a private arbitrary palette for each icon. A new approved shared icon palette set needs simultaneous-menu review. |
| Battler animation buffers | `MAX_MON_PIC_FRAMES = 2` and four battlers | 16,384 bytes of decompressed battler frame storage, plus descriptors and the separate 4,096-byte battle-bar font buffer. |
| Graphics memory | 96 KiB VRAM; in tile modes, 64 KiB BG and 32 KiB OBJ | These are shared screen resources, not available independently to each creature. |
| Palette memory | 512 bytes BG + 512 bytes OBJ | Sixteen 16-color banks in each pool; battle, UI, effects and icons compete for assignments. |
| Sprite capacity | Engine `MAX_SPRITES = 64`; 32 affine matrices; hardware OAM is 1,024 bytes | Extra Armament layers, shadows and effects consume finite entries and per-scanline rendering time. Metasprites are not free. |
| Field objects | `OBJECT_EVENTS_COUNT = 16` | A crowded scene's player, NPCs and creature objects share the existing field-object pool. |

Evidence: [pixel/tile/memory constants](../include/gba/defines.h), [picture constants](../include/constants/pokemon.h), [SpeciesInfo](../include/pokemon.h), [sprite limits](../include/sprite.h), [battle allocation/loading](../src/battle_gfx_sfx_util.c), [icon loader](../src/pokemon_icon.c), [field-object count](../include/constants/global.h).

`P_GBA_STYLE_SPECIES_GFX` is currently **FALSE**: the engine often uses Gen IV/V-style source drawings within GBA-sized buffers. Do not mistake that setting for Bondsaga's art direction. `OW_POKEMON_OBJECT_EVENTS` is TRUE, while followers are disabled. Available support does not authorize follower gameplay.

### Front and back delivery profiles

Recommended first calibration profile: two unique front frames, one back frame and two icon frames, all at the dimensions above. This is a testable starting profile, not a permanent ceiling on expressive quality. Shared procedural movement, short holds and carefully authored pose changes should be evaluated before ordering many unique raster frames.

Deliver each frame separately in the source package; generate the vertically stacked engine sheet deterministically. Include facing, frame order, durations, ground/contact anchor, drawn bounds, front/back Y offsets, elevation and shadow placement in the manifest. The engine's size/offset fields describe the drawn area within a 64×64 canvas; they do not enlarge that canvas. Record intentional edge contact rather than silently clipping protrusions. Long tails, wings, weapons and tall bodies require on-screen review with motion extrema.

One frame is 64 tiles × 32 bytes. Increasing the global frame count from two to four adds **16,384 bytes** to the four-battler decompression allocation alone, before additional scratch buffers or alternate forms. The ordinary front-frame scripts, back animation callbacks, summary display and move-effect copies also require audit. Back animations commonly transform a single image; simply appending a second back frame does not prove it will be selected correctly.

Treat more frames, larger canvases or split large creatures as explicit later implementation proposals. GBA sprite primitives and metasprites permit alternatives, but the current battler loaders are not an unrestricted large-sprite pipeline. If an approved design cannot read within the initial profile, present pixel studies and measured alternatives to design authority; do not amputate or redesign it to fit.

### Palette and icon delivery

Use indexed PNG with all used indices in 0–15, a stable 16-slot palette, and index 0 reserved for transparency. PNG storage bit depth need not equal the runtime's four bits per pixel. Deliver the approved RGB555 palette as well as the editable source palette. Review colors after quantization: distinct source colors can collapse to the same hardware value. Reject fractional alpha and unintended antialiased fringe pixels in engine exports.

An alternate palette must preserve index roles across the relevant images. A palette-only appearance costs 32 raw bytes, but a changed silhouette or marking that cannot be expressed by index replacement needs additional tiles or a tested overlay. It is not automatically a cheap palette swap. No new appearance is authored solely because this budget includes a placeholder allowance.

Icons are purpose-drawn reductions, not automatically resized battle sprites. Require recognizable silhouettes at 32×32, clean animation and a documented shared palette assignment. Six existing palettes occupy 192 bytes of ROM color data; their simultaneous OBJ allocations matter more than that small ROM size. Do not change a shared palette without checking every user. More than six palettes or per-icon palettes require a loader/menu allocation proposal, not just a new index value.

## 4. Battle-Bound presentation architecture

[The accepted adapter](../src/bondsaga_battle_bound.c) currently draws a Youngster front image and a temporary bar in both Trainer positions, repeats the image into both frame buffers and loads the position's OBJ palette plus its BG animation copy. This proves combat roles, not final facing, posing, equipment composition or manifestation animation. Ordinary four/five-frame Trainer throw backsprites are a separate entry-animation system and must not be assumed to extend the Battle-Bound battler buffer.

Required future art package for each approved calibration manifestation:

- Front-facing enemy view and rear-facing player view, referenced to an approved Trainer pose/body template.
- Species/stage identity, manifestation asset key and an approved motion-family key, resolved from ROM data using the current Bound reference. No species alias is persisted for the Trainer.
- Grip, root/contact and effect attachment anchors; front/behind-body masks where needed; canvas bounds and palette-index contracts.
- Pose/timing compatibility for the prototype's four action categories, idle, hit reaction, entry and Bond Break. These are presentation tests, not new actions or combat mechanics.
- A comparison sheet showing that evolution changes the manifestation while retaining the approved family identity. Extensive/full armor remains future work with its own cost estimate.

Recommended implementation experiment after approval: keep source layers separate, then compare **precomposed frames** with **composition into the existing battler buffer**. Neither is implemented here.

| Approach | Benefit | Cost/risk to measure |
|---|---|---|
| Precomposed Trainer + Armament | Reuses one normal battler image and palette; simplest lifecycle | Unique frames multiply with body/outfit/manifestation/view/pose combinations. All components must fit one 15-visible-color palette. |
| Compose approved layers on load/pose change into that same image | Reuses Trainer and Armament source tiles without adding a combatant or extra resident OAM layers | Bounded composition time and scratch storage; palette union must still fit; masks, anchors and both views need validation. Compose only when needed, not a whole image every frame. |
| Separate synchronized OBJ layers | Potentially separate palettes and independently moving weapon/body | Additional OAM, tile/palette allocation, priority, affine transforms, clipping, fades, hit effects, shadows and cleanup. Requires renderer integration and a worst-scene test. |

Do not assume a shared Trainer palette and every creature's palette can be combined into 16 slots. If they cannot, return approved palette/pose studies or the measured multi-layer alternative. Full-body manifestations must remain recognizable and independently targetable beside the Active Creature; a technical limitation does not justify removing Battle-Bound.

### Motion-family reuse

Share timelines, attachment conventions, hit/effect anchor semantics, reusable effect tiles and transform callbacks where approved motion is compatible. Keep timing separate from raster frame indices and combat mechanics. A species-stage manifestation may reference shared motion while retaining unique morphology, motifs and palette. Similarity must be authored, not inferred from a filename.

For illustration only, 15 shared packs at 16 KiB each cost 245,760 bytes. Duplicating a 16 KiB pack for each of 200 stages would cost 3,276,800 bytes; sharing that assumed data saves 3,031,040 bytes. These are allocation assumptions, not measured final animation sizes. They do **not** eliminate the cost of distinct manifestation art, complex armor, additional Trainer bodies or unique cinematics.

## 5. Evolution-family reuse without design distortion

Use an authored graph of stable species and lineage keys. Do not encode lineage as three mandatory folders or deduce evolution from stage number, directory order, mythic flags or asset names. Source evolution tables are terminated lists, not a universal three-stage rule. However, inherited interfaces still have presentation limits: [HGSS Pokédex code](../src/pokedex_plus_hgss.c) has `MAX_EVOLUTION_ICONS = 8` and a per-node display clamp. This is an eventual research/UI engineering issue, not a reason to shorten families or expose hidden evolution information.

Reuse approved palette roles, silhouette landmarks, material swatches, attachment conventions, compatible motion scripts and genuinely identical tiles. Each stage still receives its own approved front/back/icon and manifestation review. Mirroring is allowed only where asymmetry, equipment handedness and lighting permit it. No shared master edit may silently change an already approved stage; resolve and preview all affected dependencies.

For Part II, retain stable registry mappings for every transferable Part I identity even if that creature is not locally encounterable. Visual files may be repacked without renumbering saved identities. Do not infer canon IDs from temporary Pokémon enum values. Removing old assets later requires a reference/migration audit, not a save-schema shortcut.

## 6. Overworld, environments and UI

### Creature and NPC objects

Existing NPCs commonly use 16×32 frames; creature objects commonly use 32×32. The Treecko engineering sample has six 32×32 frames in a 192×32 horizontal source strip, converted with four-tile-wide/four-tile-high metatile ordering. The following animation table supports a mirrored side view; its asymmetric variant has distinct right-facing frames. Do not globally apply the battle sheet's vertical ordering to field sheets.

Define dimensions and directional frame semantics per profile, with explicit mirroring approval. Test walking in all four directions, stationary pose, feet/ground anchors, occlusion by tall grass and roofs, bridges, reflections, shadows, palette transitions and object unloading. Large bodies or mounts may need subsprites, larger collision/occlusion policies and more memory. Those policies and traversal mechanics are not settled here.

Budget field assets separately even though the current room uses none of the future original roster. A six-frame 32×32 set is 3,072 raw tile bytes, plus palettes and metadata; asymmetric eight-frame or larger sets cost more. No universal six-frame aesthetic or follower feature is mandated.

Evidence: [object graphics descriptors](../src/data/object_events/object_event_graphics_info.h), [field animation tables](../src/data/object_events/object_event_anims.h), [follower image tables](../src/data/object_events/object_event_pic_tables_followers.h), [field palette/loader code](../src/event_object_movement.c).

### Tilesets and environment art

The current Emerald map path combines primary and secondary tilesets. [fieldmap.h](../include/fieldmap.h) permits 1,024 8×8 tiles and 1,024 metatiles total, with a 512/512 primary/secondary split and 13 terrain palette banks split 6/7. These are engine map allocations, distinct from the hardware's 16 BG palette banks. FRLG uses different primary splits; do not apply those to the Emerald production target.

A normal metatile is 16×16 pixels represented by eight 8×8 tiles across two layers, plus behavior/layer attributes. At full capacity the raw tile pixels alone cost 32,768 bytes; metatile definitions add 16,384 bytes and Emerald u16 attributes another 2,048 bytes, before palettes, maps, animations and compression. Per-map border grids, elevation/collision and script/object data remain separate. `MAX_MAP_DATA_SIZE = 10240` bounds the existing buffered map data, so large environment canvases are not unlimited maps.

Plan primary shared materials and secondary location sets only after environment art direction is supplied. Reuse exact tiles, flips and compatible palette roles; budget animated water/foliage frames and DMA updates separately. Verify daytime/nighttime palettes, transitions between neighboring sets, layering, collision readability and weather. Do not automatically reuse one palette across environments whose approved lighting differs. This task authors no region, map, biome list or tileset replacement.

### UI

UI is a combination of 4bpp tiles, tilemaps, palettes, sprites and runtime-rendered text/window buffers, not one high-resolution screen image. A full 240×160 4bpp pixel surface would be 19,200 bytes before tilemap/palette data; actual allocations are window-specific. Fonts, localization widths, status icons, target cursors, health boxes and text all require their own space.

Validate readability at 1× and normal Delta display scale, using a provisional neutral UI until the visual authority supplies a skin. Protect the Trainer/creature distinction, target highlights and status legibility from decorative art. Reserve adequate text widths for eventual three-digit levels and authored names; this does not implement Lv.200 gameplay. Do not repurpose battle palette banks or window tiles by assuming they are unused in an idle screenshot.

## 7. Source-art and conversion workflow

Recommended source package:

- Approved concept sheets with front/side/rear anatomical information, proportions, silhouette landmarks, color/material roles, asymmetries and family comparisons. A layered master or vector source is preferred; roughly 1,024–2,048 pixels per view is a useful reference scale, not an engine requirement or a substitute for sprite tests.
- A native-size pixel study approved early, before polishing details that cannot survive at 64×64/32×32. Review both native pixels and nearest-neighbor enlargement. Do not judge only a filtered zoom.
- Editable pixel-art source with named frames/layers, explicit palette and no hidden unapproved anatomy. Keep the original approved master immutable by revision; engine exports are reproducible derivatives.

Approval sequence:

1. **Brief approved:** creator supplies concept revision, family graph, identity landmarks and required views. Missing rear anatomy, grip behavior or colors return to authority; conversion staff do not invent them.
2. **Pixel translation approved:** technical artist redraws deliberately at target resolution. Nearest-neighbor scaling helps previews, but neither automatic downsampling nor generative reinterpretation is an acceptable final conversion. Simplification is documented in a concept-to-pixel comparison. Any change to anatomy, silhouette identity, coloration or manifestation fantasy requires creative approval.
3. **Palette/pose approved:** review RGB555 output, both facings, animation extrema, icon reduction and family lineup. Avoid adding detail or changing hues merely to improve a compression result.
4. **Machine validation:** validate exports, pack frames, convert tiles/palettes, compress and compare decoded bytes to approved exports.
5. **Engine preview approved:** test the exact packaged asset in doubles, menus and applicable field scenes; capture native screenshots and short motion clips. Technical pass and creative acceptance are separate sign-offs.
6. **Versioned integration:** commit approved sources, manifest and approval evidence; CI rebuilds derivatives, reports asset and linker deltas, and retains the ROM/ELF/previews. A later creative change repeats affected approvals.

Store author, provenance/usage rights, source hashes, approval references, exporter/tool versions and export parameters. No mass conversion from unrelated third-party designs. Choosing a drawing application or generation service is outside this proposal; existing deterministic repo converters remain the implementation baseline.

## 8. Proposed organization and naming

The following tree is proposed, not created by this task. Angle-bracket keys are placeholders, not new canon names or allocated IDs.

```text
art/bondsaga/
  authority/style-guide/<approved-revision>/
  authority/calibration/<batch-key>/
  source/creatures/<family-key>/<species-key>/
    concept/<revision>/                 # approved references and provenance
    pixel/                             # editable native-size source
    manifest.json                      # profiles, dependency and approval refs
  source/trainers/<presentation-key>/
  source/armaments/<family-key>/<species-key>/
  source/motion/<motion-key>/
  source/overworld/<asset-key>/
  source/environments/<tileset-key>/
  source/ui/<screen-or-component-key>/
  registries/                          # stable asset/lineage keys; explicit mappings
  reviews/<batch-key>/<revision>/       # comparisons and separate sign-offs
graphics/bondsaga/                     # reviewed, lossless engine-input exports
  creatures/<species-key>/
  trainers/<presentation-key>/
  armaments/<species-key>/
  overworld/<asset-key>/
  environments/<tileset-key>/
  ui/<component-key>/
src/data/bondsaga/                     # future explicit ROM registry bindings
tools/bondsaga_assets/                 # future validators/export manifest tooling
build/assets/graphics/bondsaga/        # ignored, reproducibly generated binary data
```

Use lowercase ASCII `snake_case` keys and filenames, with no spaces or renamed IDs tied to display names. Example export names: `battle_front.png`, `battle_back.png`, `icon.png`, `battle_base.pal`, `overworld.png`; manifest source frames may be `front_idle_00.png` and `front_idle_01.png`. Distinguish `player_back` and `enemy_front` Armament views explicitly. Variant appearance keys and canonical numeric mappings remain unassigned until authority supplies them.

Manifest fields should include a manifest schema version, asset key, species/lineage mapping reference, approved revision/hash, usage-rights reference, asset profile, dimensions/frame count/order, palette key/index roles, anchors/bounds, animation/motion dependencies, mirroring policy, compression mode and estimated/actual bytes. Dependencies are explicit references, not inferred stage numbering. Keep approval status machine-readable and independent of file existence. Calibration-only test keys must never be issued as live saga IDs.

Recommended storage policy: commit reviewed PNG/palette exports and small manifests with the source code. Large layered masters may use a reviewed versioned archive or Git LFS, with pinned hashes and a documented retrieval process. Decide that storage workflow before receiving masters; an ordinary ROM build should not require a paid editor, a cloud generation call or downloading an unpinned master.

## 9. Compression and build integration

The existing [Makefile](../Makefile) converts `.png` to tile data through `gbagfx`, `.pal` to `.gbapal`, and supports `.lz`, `.smol`, `.fastSmol` and `.smolTM`. [Graphics declarations](../src/data/graphics/pokemon.h) use `INCGFX` paths: creature fronts/backs commonly use `.4bpp.smol`, icons `.4bpp`, palettes `.gbapal`. [Runtime decompression](../src/decompress.c) dispatches through headers; raw icons must not simply be renamed to a compressed extension.

Retain the existing lossless codec path initially. Measure compressed ROM bytes, decoded bytes, temporary allocation and cycles separately. `.fastSmol` is an existing option to benchmark where latency matters; a smaller file is not automatically faster. Palette data is already only 32 bytes per bank and is commonly raw. Tilemap-aware compression and exact tile reuse can help environments; arbitrary tile deduplication requires a matching map/frame descriptor, not reordering battle tiles behind the loader's back.

Compression reduces ROM, not the decoded frame or VRAM capacity. `MAX_DECOMPRESSION_BUFFER_SIZE = 0x4000` is a generic helper bound, not permission to write 16 KiB into a 4 KiB battler buffer. Runtime decompressor entry points accept a destination pointer without its capacity; build-time bounds and valid-stream checks are therefore important.

Reuse data only when the bytes, palette-index interpretation and animation use are compatible. Verify actual linker retention after registry changes: compiled files in `build/assets` are not proof of linked cost or safe removability. Removing official placeholder content later can reclaim space, but must be a separate reference audit across species, forms, trainers, scripts, cries and UI. No removal credit is assumed below.

## 10. Measured ROM baseline and roster estimates

### Current linked usage

Measurements are from the accepted CI artifact's ELF/map/build log, not the padded file or an estimate from source PNG sizes.

| Measure | Bytes |
|---|---:|
| Conventional ROM address budget | 33,554,432 |
| Current linker-used ROM | 26,760,148 |
| Current ROM headroom | **6,794,284 (6.480 MiB)** |
| Exported padded `.gba` | 33,554,432 |
| Static EWRAM | 229,612 |
| Static IWRAM | 28,376 |

Static EWRAM includes the existing reserved heap region; its unoccupied linker tail is not the same as usable battle heap. These totals and the accepted room's 96,736 free-heap observation do not certify worst-case animated-scene stack/heap usage. The proposal itself changes zero runtime/ROM/RAM/save bytes.

### Measured engineering samples

`arm-none-eabi-nm -S --size-sort` on the accepted ELF provides these symbol payload sizes. Front is the complete two-frame compressed sheet; back is the compressed single frame. Every listed icon is 1,024 bytes and the two listed palettes total 64 bytes. Animation descriptors, code, shadows, field sprites, cries and alignment between symbols are excluded. These existing assets provide engineering comparisons only, not visual references for final creatures.

| Existing sample | Front bytes | Back bytes | Icon + two palettes | Total bytes |
|---|---:|---:|---:|---:|
| Treecko | 856 | 476 | 1,088 | 2,420 |
| Grovyle | 1,424 | 636 | 1,088 | 3,148 |
| Sceptile | 1,768 | 700 | 1,088 | 3,556 |
| Butterfree | 1,180 | 920 | 1,088 | 3,188 |
| Onix | 1,484 | 796 | 1,088 | 3,368 |
| Swinub | 484 | 364 | 1,088 | 1,936 |
| Wailord | 1,136 | 236 | 1,088 | 2,460 |
| Gardevoir | 976 | 584 | 1,088 | 2,648 |
| Metagross | 1,488 | 780 | 1,088 | 3,356 |
| Groudon | 2,144 | 712 | 1,088 | 3,944 |
| Rayquaza | 1,976 | 684 | 1,088 | 3,748 |
| Lugia | 1,872 | 580 | 1,088 | 3,540 |

Sample median is **3,272 bytes**, maximum **3,944**. This deliberately varied but small sample is not a compression guarantee for original Bondsaga art. Raw standard-profile tiles total `2×2048 + 2048 + 2×512 = 7168` bytes; two palettes and a provisional 128-byte descriptor/timeline allowance raise that to **7,360 bytes** before packaging margin.

For context, deduplicating address/size pairs within each linked symbol prefix yields front pictures 1,578,616 bytes, backs 897,080, icons 1,454,080, base palettes 46,968 and alternate palettes 44,952: **4,021,696 bytes across those groups**. Counts include forms and alternate presentations, not just base species. This is an audit lead, not guaranteed reclaimable space; it excludes other species costs and does not authorize deleting any dependencies.

Reproduction: extract artifact 10311296768, read `build/bondsaga-measurements/measurements.json`, and run `arm-none-eabi-nm -S --size-sort pokeemerald.elf`. For each sample, sum `gMonFrontPic_<Name>`, `gMonBackPic_<Name>`, `gMonIcon_<Name>`, `gMonPalette_<Name>` and `gMonShinyPalette_<Name>` sizes. The sampled source PNGs are 64×128, 64×64 and 32×64 respectively. Do not measure a different toolchain's ROM and subtract this baseline.

### Planning envelopes, not quality caps

Count **individual creature stages**, not families. Budget every transferable stage in the Part II compatibility set; do not assume each ROM only needs half the saga roster.

- **4 KiB/stage provisional compressed profile:** standard geometry above, two palettes charged even though no exceptional treatment is approved, plus descriptors and short animation data. The sample supports this starting average; complex approved stages may exceed it.
- **8 KiB/stage conservative profile:** the same geometry using raw tile accounting plus palettes, descriptors and packaging margin. It avoids depending on favorable sprite compression. This is not an envelope for unlimited extra frames/forms.
- **3 KiB/stage manifestation allowance:** an illustrative two-view/two-pose 32×32 component set is 2,048 raw bytes, leaving room for palettes, anchors and margin. Assumes shared Trainer bodies/motion and future tested composition. It does not certify that every approved weapon or armor can use 32×32 components.
- **4 KiB/stage field allowance:** a simple six-frame 32×32 set, palettes, metadata and margin. Larger/asymmetric sets require explicit additions.
- **240 KiB shared motion allowance:** 15 × 16 KiB, intended to cover bounded shared body poses, motion/effect data and small presentation infrastructure. This is a planning placeholder, not measured implementation cost or approval of 15 final archetypes.

The following **additive** scenarios keep all currently linked content and assume no reclamation. Expanded totals charge creature bundles, manifestation/field allowances and the shared 240 KiB pack once per ROM.

| Stages | Creature bundles at 4 / 8 KiB | Expanded total at 4 KiB | Expanded total at 8 KiB | Headroom left after expanded 8 KiB case |
|---:|---:|---:|---:|---:|
| 150 | 614,400 / 1,228,800 | 1,935,360 | 2,549,760 | 4,244,524 |
| 200 | 819,200 / 1,638,400 | 2,498,560 | 3,317,760 | 3,476,524 |
| 250 | 1,024,000 / 2,048,000 | 3,061,760 | 4,085,760 | 2,708,524 |

Formula: `N × (creature KiB + 3 + 4) × 1024 + 15 × 16384`. Amounts are exact products of provisional allowances, not predictions precise to a byte.

**Remaining headroom is not all available for more creatures.** Original environments, maps, NPC/Trainer customization, UI/fonts, audio/cries/music, future mechanics, migration support, localization and substantial manifestation variants still need ledgers. Proposed interim planning rule: hold at least **2 MiB of the remaining ROM** uncommitted for these unmeasured needs until real production estimates replace it. This is a technical contingency proposal, not an approved allocation of final content. At 250 stages in the conservative scenario only **611,372 bytes** remain beyond that hold; full production is not cleared by the table.

Sensitivity examples: every added unique 64×64 frame costs 2,048 decoded/raw bytes; every 32×32 icon/overlay frame 512; every palette bank 32. Two additional 64×64 frames across 250 stages add 1,024,000 raw bytes before compression. For `E` complex manifestations, charge `E × (measured actual bytes − 3072)` above the assumed component allowance. A precomposed presentation model with `T` distinct Trainer bodies instead of shared layers multiplies the relevant pose/view bundles by `T`; do not hide that multiplier in the 240 KiB allowance. Approved pattern/silhouette variants and Part II-exclusive transformations also add bundles.

Conclusion for planning: 150–250 stages are plausible under a bounded, reusable pipeline, but current evidence does not certify a finished 250-stage saga. Calibrate the approved style, measure exceptional packages and the rest of the game, then revise the ledger. Quality is not traded away to hit the upper count.

## 11. Proposed automated validation

These are recommendations and future implementation tasks; no new asset validator or gameplay feature is implemented in this document-only branch.

| Gate | Required checks | Failure handling |
|---|---|---|
| Intake/authority | Manifest schema, unique stable keys, source hashes, provenance, required approved views, separate technical/creative status | Reject missing approval; never fill missing concept views automatically. |
| Pixels | Exact profile dimensions; tile alignment; indexed export; used indices 0–15; transparency; expected frame count; nonempty required frames | Reject malformed files and mismatched profiles. Approved exceptions use a new explicit profile. |
| Palette | 16 active slots; RGB555 round-trip; index-0 policy; shared index roles; valid icon palette reference; approved matching front/back | Reject invalid indices; show color-collapse warnings and previews for artist review rather than auto-recoloring. |
| Metadata/animation | Bounds/anchors in range, supported OAM geometry, frame references in range, durations and loops valid, declared mirroring, all dependencies resolve | Reject out-of-range reads, missing references and unterminated scripts. Explicitly allow valid looping idle sequences. |
| Conversion | Pinned tools/flags; deterministic sheet ordering; exact decoded sizes; valid codec/header/alignment; byte-exact decode versus approved raw tiles | Reject output that fails round-trip; do not trust compressed length or file extension alone. |
| Robustness | Negative fixtures: truncated PNG/stream, illegal palette index, huge/overflowed dimensions/header length, wrong frame count, bad anchors and missing registry IDs | Run bounded host checks with sanitizer/time/memory limits. Do not feed unchecked corrupt streams directly to the GBA decompressor. |
| Budget | Per-asset raw/compressed sizes and hashes; dependency sharing; ELF/map retained bytes; ROM delta; decoded-buffer, OBJ/BG palette and tile budgets | Report warnings for planning overruns; reject real hardware/buffer overflow. Bring quality tradeoffs to authority. |
| Runtime | Front/back/icon/field preview, four battlers, both Trainer roles, both targets, support/defense/offense/control, Bond Break, switching, repeat entry | Detect missing layers, wrong frame/palette, status/fade interference, leaks and persistent identity mutation. |
| Regression | Existing obedience, legitimate Sleep, Battle-Bound and persistence suites; full builds for engine/loader changes; mGBA and Delta screenshots/button tests | No integration that regresses accepted combat or save behavior. |

Future CI should regenerate exports/build products in a clean environment, check deterministic hashes, link the ROM, and publish a manifest-indexed byte report plus native-resolution previews and ROM/ELF. The same manifest should drive validators and build declarations so dimensions cannot drift between two independent hand-maintained tables.

For compression verification, provide a bounded host decoder or a controlled target harness with a destination sized from a validated profile; compare against raw converted tiles. The game's pointer-only decompression routine is not a safe host-side validator for arbitrary input. Test known-good outputs on GBA/mGBA as well as the host, because host round-trip success alone does not certify target alignment, scratch use or performance.

Use the [existing read-only mGBA probe](../tools/bondsaga_save/mgba_probe.c) and [button smoke framework](../tools/bondsaga_save/prototype_smoke.py) as starting infrastructure. Expand resource observations only in a separate approved implementation task. Record emulator/tool versions, ROM hash, heap/stack high-water measurements, palette/OAM/tile occupancy, frame-time spikes and upload timing. No unmeasured peak-memory or frame-rate claim is made here.

## 12. Future calibration batch and acceptance criteria

The authority selects **2–3 families and approximately 6–12 total stages**, with distinct body plans, relative sizes, silhouettes and complexity. No actual species or mythological identities are supplied by this proposal. Three families are the straightforward way to test three potential Armament motion families while keeping each lineage within its approved broad family. If only two creature families are approved, request an authorized third engineering presentation fixture or authored exception; do not invent an archetype-changing evolution to satisfy the test matrix.

Require coverage of one high-complexity late evolution, one organism suitable for later exceptional-appearance evaluation, both compact and large-on-screen bodies, and asymmetric anatomy or a design that reveals mirroring errors. A justified chain longer than three stages should be tested if supplied; otherwise test a synthetic registry graph with non-canon IDs to verify tooling does not assume three stages. That does not authorize adding an evolution to a creature's design.

Batch deliveries: approved concepts and family graph; editable sources; all front/back/icon exports; at least representative field direction sets; both views of approved manifestations; shared motion references; palettes/anchors; provenance and approval metadata; per-asset and full-batch size report; native-resolution lineup and in-engine previews.

Acceptance gates:

1. **Creative coherence:** design authority accepts a contact sheet with all stages together as one franchise. Each family retains recognizable landmarks through evolution, with meaningful escalation and no unrelated replacement designs. Technical staff do not self-certify creative coherence.
2. **Readability:** each stage is distinguishable at native size, in grayscale/silhouette studies and on light/dark test backgrounds. Back views and icons remain identifiable. Evaluate actual doubles UI and Delta scale, not just a large concept sheet.
3. **Faithful translation:** no unexplained anatomy, color, equipment or proportion changes from approved concepts. Every simplification affecting identity has explicit approval and comparison evidence.
4. **Motion:** no clipping, unwanted mirroring, foot/grip drift, palette flicker, sudden scale changes or frame-index errors; meaningful motion remains legible without extra detail. Test repeated plays and both sides.
5. **Battle-Bound identity:** approved manifestations are distinct by family/stage, visibly change through evolution and remain a Trainer + Armament beside another creature. Both positions and target indicators remain readable; Bond Break cleans up every layer.
6. **Exceptional-treatment readiness:** manifest and preview routes can represent an additional approved palette or tile package independently of Alpha/Apex/Variant mechanics. Actual treatment waits for authority; a capacity test must not become a final visual rule.
7. **Technical validity:** all machine checks pass; decoded data fits its consumer buffers; no new combat/save failures or memory leaks. Test the worst approved combination of both Trainers, both normal creatures, health boxes and the busiest selected effect, not each asset alone.
8. **Measured cost:** publish actual median, high-end and maximum bundle sizes, sharing ratios, retained ROM delta and worst-scene performance. Replace the provisional table with those observations before scaling production. An over-budget asset triggers review, not automatic detail removal.
9. **Device acceptance:** user/design authority accepts native Delta/iPhone readability, color, controls and animation behavior on the exact candidate ROM. Prototype 0.1 acceptance is the baseline, not certification of new art/loaders.

Stop after this calibration is reviewed in a later authorized task. This document does not authorize generating even the calibration creatures, implementing the visual overhaul, producing the full roster or starting Part I world content.

## 13. Decisions required before original generation

Only decisions that affect the first approved art batch need answers now; the final roster and all 15 archetypes do not need to be invented to unblock tooling.

| Required authority input | Why it is needed |
|---|---|
| Approved visual style guide and a small set of franchise reference sheets | Establish silhouette/detail, line/shading, light direction, materials and color discipline; an engineer cannot choose the final identity from an upstream graphics flag. |
| Calibration concepts, evolutionary graph and identity landmarks | Select the 2–3 families/6–12 stages, including recognizable development and body-plan/size coverage. Supply missing views instead of asking conversion to invent anatomy. |
| Approved sprite-scale/pose studies | Confirm that the initial 64×64 battle and 32×32 icon interpretation preserves approved anatomy and relative size; identify designs needing a larger-renderer proposal early. |
| Calibration Trainer presentation and manifestation briefs | Define both facing views, permitted mirroring/handedness, attachments and the three representative motion families without finalizing the entire Armament taxonomy. |
| Initial shared icon color studies | Approve a coherent small palette set without forcing unrelated creatures into unsuitable colors. Loader alternatives remain available if measured palette sharing fails. |
| Scope of presentation variations in the batch | Identify which Trainer bodies/outfits or alternate appearances are actually approved for the test, so composition and multiplicative asset costs can be measured. No need to finalize Alpha/Apex rules now. |

Production-owner decisions also needed: source master storage/retrieval, provenance and sign-off process, responsibility for the ROM ledger, and acceptance of the provisional profiles/contingency. These are workflow decisions, not creature-design authority. Final archetype names, mythology, trait/appearance mechanics, evolution triggers, research, progression and world content stay outside this task.

## 14. Principal risks and review outcome

- **Palette composition:** a final Trainer/outfit/Armament union may not fit one bank. Test approved studies; compare composition or extra layers without changing creature identity.
- **Multiplicative content:** body customization, views, poses, armor and exceptional silhouettes can exceed simple per-stage estimates. Track every independent bundle and dependency.
- **Runtime pressure:** extra frames/layers consume decoded RAM, OAM, affine matrices, VRAM, palette banks and upload time even when ROM compression is excellent. Budget the full scene; no global frame-count increase is authorized.
- **Inherited species assumptions:** rear/front metadata, species-specific spots/forms, shadows, icon palette selection and evolution UI limits need explicit registry integration. Do not treat default fallback Pokémon graphics as valid original production output.
- **Inconsistent conversion:** autonomous recoloring, rescaling or detail addition can erode approved designs across hundreds of assets. Require revisioned reference comparisons and separate creative sign-off.
- **Premature capacity claims:** the measured 6.480 MiB headroom and favorable sample compression do not cover all remaining content. Charge future systems and appearances, and measure removals before spending their possible savings.
- **Scope creep:** the accepted combat proof remains intact. This proposal makes no new gameplay, narrative, progression, canon creature or Battle-Bound aesthetic decisions.

Recommended next authorized milestone: approve or revise this architecture and obtain the style/concept inputs above, then implement the smallest validator/export harness and review a calibration batch. The current task stops at the proposal for review.
