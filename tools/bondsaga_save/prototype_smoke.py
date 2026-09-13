"""Drive an immutable Prototype 0.1 ROM through mGBA using only button input.

Requires mgba_probe (matching mGBA 0.10 headers/library), and `arm-none-eabi-nm
-n pokeemerald.elf` output from the exact ROM build. RAM reads observe results;
there are no memory writes, forced victories, or loaded emulator save states.
"""
import argparse
import json
import os
import re
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--probe', required=True)
p.add_argument('--rom', required=True)
p.add_argument('--symbols', required=True)
p.add_argument('--output', required=True)
p.add_argument('--dll-dir')
p.add_argument('--control', action='store_true', help='Use spread/control actions after the first turn')
p.add_argument('--bound', type=int, choices=(0, 1, 2), default=1)
p.add_argument('--focus', action='store_true', help='Focus both selected attacks on the opposing Trainer')
p.add_argument('--support', action='store_true', help='Use Helping Hand with the normal creature using Swift')
p.add_argument('--spread', action='store_true', help='Use Swift and Icy Wind from the first turn')
args = p.parse_args()
symbols = {}
all_symbols = {}
for line in Path(args.symbols).read_text(encoding='utf-8-sig').splitlines():
    fields = line.split()
    if len(fields) == 3:
        symbols[fields[2]] = int(fields[0], 16)
        all_symbols.setdefault(fields[2], set()).add(int(fields[0], 16))
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)
env = os.environ.copy()
if args.dll_dir:
    env['PATH'] = str(Path(args.dll_dir).resolve()) + os.pathsep + env.get('PATH', '')
proc = subprocess.Popen([str(Path(args.probe).resolve()), str(Path(args.rom).resolve())],
    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=(out / 'emulator.log').open('w'),
    text=True, env=env, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
assert proc.stdout.readline().strip() == 'READY'

report = {'actions': [0, 0], 'targets': [], 'bond_break_seen': False,
          'executed_moves': [[], [], [], []], 'obedience_messages': [], 'sleep_observations': 0}
last_messages = [None] * 4
string_names = re.findall(r'^\s+(STRINGID_\w+),',
                         Path('include/constants/battle_string_ids.h').read_text(), re.M)
forbidden = {string_names.index('STRINGID_' + name) for name in (
    'PKMNIGNORESASLEEP', 'PKMNIGNOREDORDERS', 'PKMNBEGANTONAP', 'PKMNLOAFING',
    'PKMNWONTOBEY', 'PKMNTURNEDAWAY', 'PKMNPRETENDNOTNOTICE')}

def observe(data):
    # Exact GBA ABI: BattlePokemon=140, status1=80; BattleResources.bufferA=16.
    # These are frame-by-frame controller observations, not inferred from HP.
    if not data[0]:
        last_messages[:] = [None] * 4
        return
    status = [int.from_bytes(data[1 + i*4:5 + i*4], 'little') for i in range(4)]
    if any(s & 7 for s in status):
        report['sleep_observations'] += 1
    flags = int.from_bytes(data[26:30], 'little')
    for battler in range(4):
        buffer = data[30 + battler*8:38 + battler*8]
        message = buffer if flags & (1 << battler) and buffer[0] == 15 else None
        if message and message != last_messages[battler]:
            string_id = int.from_bytes(buffer[2:4], 'little')
            if string_id in forbidden:
                report['obedience_messages'].append(string_names[string_id])
            if string_id == 4:  # STRINGID_USEDMOVE / CONTROLLER_PRINTSTRING
                executed = int.from_bytes(buffer[4:6], 'little')
                original = int.from_bytes(buffer[6:8], 'little')
                selected = int.from_bytes(data[18 + battler*2:20 + battler*2], 'little')
                report['executed_moves'][battler].append(executed)
                if battler in (0, 2) and (executed != selected or executed != original):
                    report.setdefault('substitutions', []).append([battler, selected, original, executed])
        last_messages[battler] = message

def command(text):
    proc.stdin.write(text + '\n')
    proc.stdin.flush()
    result = proc.stdout.readline().strip()
    while result.startswith('TRACE '):
        observe(bytes.fromhex(result[6:]))
        result = proc.stdout.readline().strip()
    if result == 'ERROR' or not result:
        raise RuntimeError((text, result))
    return result

# Observe every emulated frame, including narration and animations between taps.
command(f"watch {symbols['sActive']:x} 0 1 0")
for battler in range(4):
    command(f"watch {symbols['gBattleMons']:x} {140*battler + 80} 4 0")
command(f"watch {symbols['gBattlerAttacker']:x} 0 1 0")
command(f"watch {symbols['gChosenMoveByBattler']:x} 0 8 0")
command(f"watch {symbols['gBattleControllerExecFlags']:x} 0 4 0")
for battler in range(4):
    command(f"watch {symbols['gBattleResources']:x} {16 + battler*512} 8 1")

def run(frames=20, keys=0):
    command(f'run {frames} {keys}')

def tap(keys, wait=20):
    run(1, keys)
    run(wait)

def read(name, size=1, offset=0):
    return bytes.fromhex(command(f'read {symbols[name] + offset:x} {size}'))

def integer(name, size=1, offset=0):
    return int.from_bytes(read(name, size, offset), 'little')

def shot(name):
    # Probe accepts a path without spaces; use paths relative to the workspace.
    command('shot ' + (out / (name + '.png')).relative_to(Path.cwd()).as_posix())

def pointer_is(value, name):
    return (value & ~1) in all_symbols.get(name, set())

def heap_free():
    # GBA MemBlock ABI: 16-byte header, allocated bit 0, 18-bit size.
    head = symbols['gHeap']
    node = head
    free = 0
    for _ in range(1000):
        data = bytes.fromhex(command(f'read {node:x} 16'))
        if not (int.from_bytes(data[:2], 'little') & 1):
            free += int.from_bytes(data[4:8], 'little') & 0x3ffff
        node = int.from_bytes(data[12:16], 'little')
        if node == head:
            return free
        assert head <= node < head + 0x1c500, 'Malformed heap chain'
    raise AssertionError('Heap chain did not terminate')

try:
    for _ in range(30):
        if integer('sRunning'):
            break
        tap(8, 120)  # START skips intro and enters the disposable room.
    assert integer('sRunning') == 1, 'Prototype did not boot'
    run(120)
    records = read('sRecords', 336)
    report['room_heap_free'] = heap_free()
    assert integer('sRoster', 2, 784) == 0
    shot('01-room')
    for _ in range(args.bound):
        tap(4)  # SELECT cycles eligible canonical references.
    assert integer('sRoster', 2, 784) == args.bound
    assert read('sRecords', 336) == records
    shot('02-bound-selection')
    run(180, 16)
    run(1)
    assert integer('sX') == 7
    tap(1)
    saw_battle = False
    captured = set()
    for step in range(1600):
        if integer('sActive'):
            saw_battle = True
            if 'first_trainer_max_hp' not in report:
                report['first_trainer_max_hp'] = integer('gParties', 2, 188)
        elif saw_battle and pointer_is(integer('gMain', 4, 4), 'RoomFrame'):
            break
        broken = integer('sBroken')
        if broken and not report['bond_break_seen']:
            report['bond_break_seen'] = True
            report['first_break_mask'] = broken
            shot('06-after-bond-break')
        funcs = [integer('gBattlerControllerFuncs', 4, i * 4) for i in range(4)]
        handled = False
        for battler in (0, 2):
            if pointer_is(funcs[battler], 'HandleInputChooseAction'):
                # The fixture's opposing ordinary Grovyle has 53 max HP.
                # Catch accidental inherited Dynamax/stat recalculation.
                assert integer('gBattleMons', 2, 140 + 46) == 53
                report['actions'][battler // 2] += 1
                label = '03-creature-command' if battler == 0 else '04-trainer-command'
                if label not in captured:
                    shot(label)
                    captured.add(label)
                tap(1)
                handled = True
                break
            if pointer_is(funcs[battler], 'HandleInputChooseMove'):
                desired_move = (3 if battler == 2 else 1) if args.control and report['actions'][battler // 2] > 1 else 0
                if args.support:
                    desired_move = 2 if battler == 2 else 1
                if args.spread:
                    desired_move = 3 if battler == 2 else 1
                cursor = integer('gMoveSelectionCursor', 1, battler)
                if (cursor & 1) != (desired_move & 1):
                    tap(16 if desired_move & 1 else 32, 3)
                elif (cursor & 2) != (desired_move & 2):
                    tap(128 if desired_move & 2 else 64, 3)
                else:
                    tap(1)
                handled = True
                break
            if pointer_is(funcs[battler], 'HandleInputChooseTarget'):
                target = integer('gMultiUsePlayerCursor')
                desired = 3 if (battler == 2 or args.focus) and not (broken & 8) else 1
                if integer('gAbsentBattlerFlags') & (1 << desired):
                    desired = 1 if desired == 3 else 3
                if target != desired:
                    tap(32, 3)  # LEFT cycles target positions.
                else:
                    if target not in report['targets']:
                        report['targets'].append(target)
                        shot('05-target-' + str(target))
                    tap(1)
                handled = True
                break
        if handled:
            continue
        tasks = [integer('gTasks', 4, i * 40) for i in range(16)]
        if any(pointer_is(t, 'Task_HandleChooseMonInput') for t in tasks):
            if integer('gPartyMenu', 1, 9) != 2:
                tap(128, 4)  # Down to the ordinary reserve.
            else:
                tap(1)
        else:
            tap(1, 60)  # Advance narration; does not alter combat state.
    else:
        shot('stalled')
        raise AssertionError('Battle did not return to room within input budget')
    report['result'] = integer('sLastResult')
    assert report['result'] in (1, 2, 3), report
    assert report['actions'][0] > 0 and report['actions'][1] > 0, report
    assert report['bond_break_seen'], report
    assert not report['obedience_messages'], report
    assert not report['sleep_observations'], report  # Fixture has no Sleep-inducing moves.
    assert not report.get('substitutions'), report
    assert len(report['executed_moves'][0]) >= 2 and len(report['executed_moves'][2]) >= 2, report
    assert read('sRecords', 336) == records, 'Canonical records changed'
    assert integer('sRoster', 2, 784) == args.bound, 'Bound designation changed'
    assert integer('gPartiesCount') == 0, 'Runtime party leaked into room'
    report['return_room_heap_free'] = heap_free()
    assert report['return_room_heap_free'] == report['room_heap_free'], 'Battle allocation retained on return'
    shot('07-result')
    tap(4)
    assert integer('sRoster', 2, 784) == (args.bound + 1) % 3
    assert read('sRecords', 336) == records
    report['canonical_bytes_preserved'] = True
    tap(1)
    for _ in range(60):
        if integer('sActive'):
            break
        tap(1, 60)
    assert integer('sActive'), 'Repeat encounter did not start'
    report['repeat_trainer_max_hp'] = integer('gParties', 2, 188)
    assert report['repeat_trainer_max_hp'] != report['first_trainer_max_hp'], report
    assert read('sRecords', 336) == records
    report['repeat_bound'] = integer('sRoster', 2, 784)
    print(json.dumps(report, indent=2), flush=True)
finally:
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    proc.stdin.write('quit\n')
    proc.stdin.flush()
    proc.wait(timeout=10)
