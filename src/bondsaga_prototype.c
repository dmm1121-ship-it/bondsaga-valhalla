#include "global.h"
#include "bondsaga_battle_bound.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "bg.h"
#include "gpu_regs.h"
#include "main.h"
#include "menu.h"
#include "new_game.h"
#include "overworld.h"
#include "palette.h"
#include "sprite.h"
#include "scanline_effect.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "util.h"
#include "constants/moves.h"
#include "constants/opponents.h"
#include "constants/rgb.h"

/* Three disposable canonical wire records. This deliberately does not allocate
 * a second 384-record owner beside Emerald's runtime. Full session/save UI is
 * deferred; the existing 112-byte format and roster reference are unchanged. */
static EWRAM_DATA u8 sRecords[3][BSG_CREATURE_WIRE_SIZE] = {0};
static EWRAM_DATA struct BsgRoster sRoster = {0};
static EWRAM_DATA bool8 sRunning = FALSE;
static EWRAM_DATA u8 sX = 0;
static EWRAM_DATA u8 sY = 0;
static EWRAM_DATA u8 sLastResult = 0;
static EWRAM_DATA u16 sTilemap[32 * 32] = {0};

static const struct BgTemplate sBg = { .bg = 0, .charBaseIndex = 0, .mapBaseIndex = 31 };
static const struct WindowTemplate sWindows[] = {
    { .bg = 0, .width = 30, .height = 20, .paletteNum = 0, .baseBlock = 1 },
    DUMMY_WIN_TEMPLATE,
};
static const u16 sPalette[16] = { RGB_BLACK, RGB(2, 5, 10), RGB_WHITE, RGB(12, 16, 22) };
static const u8 sColors[] = {1, 2, 3};
static const u16 sSpecies[] = {SPECIES_COMBUSKEN, SPECIES_MARSHTOMP, SPECIES_GROVYLE};
static const u16 sOrdinaryAttacks[] = {MOVE_EMBER, MOVE_WATER_GUN, MOVE_ABSORB};

static void InitRoom(void);
static void RoomFrame(void);

bool32 BsgPrototypeRunning(void) { return sRunning; }

static void Print(u32 x, u32 y, const u8 *text)
{
    AddTextPrinterParameterized3(0, FONT_SMALL, x, y, sColors, TEXT_SKIP_DRAW, text);
}

static void DrawRoom(void)
{
    struct BsgCreature bound;
    BsgCreatureDecode(&bound, sRecords[sRoster.bound], BSG_CREATURE_WIRE_SIZE, NULL);
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    Print(5, 1, COMPOUND_STRING("BONDSAGA / BATTLE-BOUND 0.1"));
    Print(5, 16, COMPOUND_STRING("D-pad: walk   SELECT: Bound Partner"));
    Print(5, 29, COMPOUND_STRING("A at TEST: battle   No saving in this build"));
    Print(5, 44, COMPOUND_STRING("Bound:"));
    Print(42, 44, GetSpeciesName(bound.species));
    Print(5, 57, COMPOUND_STRING("Trainer actions: Slash / Protect"));
    Print(5, 69, COMPOUND_STRING("Helping Hand / Icy Wind (placeholders)"));
    FillWindowPixelRect(0, PIXEL_FILL(3), 5, 86, 230, 49);
    FillWindowPixelRect(0, PIXEL_FILL(1), 7, 88, 226, 45);
    Print(189, 104, COMPOUND_STRING("TEST"));
    Print(15 + sX * 24, 88 + sY * 12, COMPOUND_STRING("T"));
    if (sLastResult == B_OUTCOME_WON)
        Print(5, 140, COMPOUND_STRING("VICTORY. Choose another partner and retry."));
    else if (sLastResult)
        Print(5, 140, COMPOUND_STRING("DEFEAT. Test roster restored; try again."));
    else
        Print(5, 140, COMPOUND_STRING("Walk right to TEST. T = your Trainer."));
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);
}

static void VBlank(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void InitRoom(void)
{
    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
    SetMainCallback1(NULL);
    ScanlineEffect_Stop();
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    ResetTasks();
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetPaletteFade();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, &sBg, 1);
    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    memset(sTilemap, 0, sizeof(sTilemap));
    SetBgTilemapBuffer(0, sTilemap);
    InitWindows(sWindows);
    DeactivateAllTextPrinters();
    LoadPalette(sPalette, 0, sizeof(sPalette));
    DrawRoom();
    ShowBg(0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0_ON);
    SetVBlankCallback(VBlank);
    SetMainCallback2(RoomFrame);
}

static void EndBattle(void)
{
    sLastResult = gBattleOutcome;
    BsgBbEnd();
    /* No battle-only proxy, HP, exhaustion, reward or EXP is written back to
     * owned state. Resetting a disposable test is not a saga healing rule. */
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    gBattleTypeFlags = 0;
    InitRoom();
}

static void CreateOrdinary(struct Pokemon *mon, u16 index)
{
    struct BsgCreature source;
    BsgCreatureDecode(&source, sRecords[index], BSG_CREATURE_WIRE_SIZE, NULL);
    CreateMonWithIVs(mon, source.species, source.earnedLevel, source.serial, OTID_STRUCT_PLAYER_ID, 15);
    SetMonMoveSlot(mon, sOrdinaryAttacks[index], 0);
    SetMonMoveSlot(mon, MOVE_SWIFT, 1);
    SetMonMoveSlot(mon, MOVE_PROTECT, 2);
    SetMonMoveSlot(mon, MOVE_QUICK_ATTACK, 3);
}

static void StartBattle(void)
{
    struct Pokemon partner;
    u32 next = 0;
    FreeAllWindowBuffers();
    SetVBlankCallback(NULL);
    SetMainCallback1(NULL);
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    for (u32 i = 0; i < ARRAY_COUNT(sRecords); i++)
        if (i != sRoster.bound)
        {
            CreateOrdinary(&gParties[B_TRAINER_PLAYER][next], i);
            next = 2; // slot 1 is a runtime Trainer adapter, never ownership.
        }
    CreateOrdinary(&partner, sRoster.bound);
    BsgBbCreateForm(&gParties[B_TRAINER_PLAYER][1], &partner, FALSE);
    CreateOrdinary(&gParties[B_TRAINER_OPPONENT_A][0], 2);
    CreateOrdinary(&partner, 1);
    BsgBbCreateForm(&gParties[B_TRAINER_OPPONENT_A][1], &partner, TRUE);
    gPartiesCount[B_TRAINER_PLAYER] = 3;
    gPartiesCount[B_TRAINER_OPPONENT_A] = 2;
    memset(&gTrainerBattleParameter, 0, sizeof(gTrainerBattleParameter));
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_CALVIN_1;
    TRAINER_BATTLE_PARAM.isDoubleBattle = TRUE;
    TRAINER_BATTLE_PARAM.defeatTextA = (u8 *)COMPOUND_STRING("Test encounter complete!");
    TRAINER_BATTLE_PARAM.victoryText = (u8 *)COMPOUND_STRING("Test encounter complete!");
    gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE;
    BsgBbBegin();
    gMain.savedCallback = EndBattle;
    SetMainCallback2(CB2_InitBattle);
}

static void RoomFrame(void)
{
    bool32 changed = FALSE;
    if (JOY_REPEAT(DPAD_LEFT) && sX > 0) { sX--; changed = TRUE; }
    if (JOY_REPEAT(DPAD_RIGHT) && sX < 7) { sX++; changed = TRUE; }
    if (JOY_REPEAT(DPAD_UP) && sY > 0) { sY--; changed = TRUE; }
    if (JOY_REPEAT(DPAD_DOWN) && sY < 2) { sY++; changed = TRUE; }
    if (JOY_NEW(SELECT_BUTTON))
    {
        BsgBbDesignate(&sRoster, ARRAY_COUNT(sRecords), (sRoster.bound + 1) % ARRAY_COUNT(sRecords));
        changed = TRUE;
    }
    if (JOY_NEW(A_BUTTON) && sX == 7)
    {
        StartBattle();
        return;
    }
    if (changed) DrawRoom();
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

void CB2_BsgPrototype(void)
{
    sRunning = TRUE; // Protect media before initializing disposable RAM state.
    NewGameInitData();
    StringCopy(gSaveBlock2Ptr->playerName, COMPOUND_STRING("TEST"));
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SET;
    BsgBbEnd();
    BsgRosterInitialize(&sRoster);
    for (u32 i = 0; i < ARRAY_COUNT(sRecords); i++)
    {
        struct BsgCreature creature = {0};
        creature.serial = i + 1;
        creature.species = sSpecies[i];
        creature.earnedLevel = BSG_BB_PROTOTYPE_LEVEL;
        BsgCreatureEncode(sRecords[i], BSG_CREATURE_WIRE_SIZE, &creature, NULL);
        sRoster.placement[i] = sRoster.expedition[i] = i;
    }
    BsgBbDesignate(&sRoster, ARRAY_COUNT(sRecords), 0);
    sX = 3;
    sY = 1;
    sLastResult = 0;
    InitRoom();
}
