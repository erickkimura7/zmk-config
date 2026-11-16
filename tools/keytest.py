#!/usr/bin/env python3
"""Testador de teclas do Corne (ZMK) — valida solda de switches/diodos.

Lê o teclado direto do /dev/input (sem libs externas) e marca cada tecla
pressionada num mapa do Corne. Enquanto roda, o teclado fica "capturado":
as teclas NÃO digitam em lugar nenhum.

Uso:  python3 tools/keytest.py
Sai:  sozinho quando todas as teclas forem testadas, após 120 s sem
      apertar nada, ou Ctrl+C num terminal/SSH.

Lower e Raise não enviam tecla sozinhos. Para testá-los:
  - Lower: segure Lower e aperte Q (deve sair "1").
  - Raise: segure Raise e aperte H (deve sair "-").
  Não aperte Lower + teclas da fileira do meio esquerda (Ctrl, A..G):
  são os atalhos de Bluetooth (Lower+Ctrl apaga o pareamento).
"""
import fcntl
import glob
import os
import select
import struct
import sys
import time

EVIOCGRAB = 0x40044590
EV_KEY = 1
EVENT = struct.Struct("llHHi")
CHATTER_MS = 40      # dois "press" da mesma tecla em menos que isso = solda/switch suspeito
IDLE_TIMEOUT = 120

# (rótulo, keycode Linux) — layout padrão do config/corne.keymap
LEFT = [
    [("TAB", 15), ("Q", 16), ("W", 17), ("E", 18), ("R", 19), ("T", 20)],
    [("CTL", 29), ("A", 30), ("S", 31), ("D", 32), ("F", 33), ("G", 34)],
    [("SFT", 42), ("Z", 44), ("X", 45), ("C", 46), ("V", 47), ("B", 48)],
    [None, None, None, ("GUI", 125), ("LWR", "lower"), ("SPC", 57)],
]
RIGHT = [
    [("Y", 21), ("U", 22), ("I", 23), ("O", 24), ("P", 25), ("BSP", 14)],
    [("H", 35), ("J", 36), ("K", 37), ("L", 38), (";", 39), ("'", 40)],
    [("N", 49), ("M", 50), (",", 51), (".", 52), ("/", 53), ("ESC", 1)],
    [("ENT", 28), ("RSE", "raise"), ("ALT", 100), None, None, None],
]
# Teclas que só existem na camada Lower / Raise -> provam que o Lower/Raise funciona
LOWER_ONLY = set(range(2, 12)) | {103, 105, 106, 108}  # 1..0 e setas
RAISE_ONLY = {12, 13, 26, 27, 41, 43}                    # - = [ ] ` \

ALL_KEYS = [k for half in (LEFT, RIGHT) for row in half for k in row if k]
KEY_BY_CODE = {code: label for label, code in ALL_KEYS if isinstance(code, int)}

GREEN, RED, DIM, BOLD, RESET = "\033[42;30m", "\033[41;97m", "\033[2m", "\033[1m", "\033[0m"


def find_device():
    paths = sorted(glob.glob("/dev/input/by-id/*ZMK*event-kbd"))
    if not paths:
        sys.exit("Teclado ZMK não encontrado. Ele está conectado via USB?")
    return os.path.realpath(paths[0])


def cell(key, done, chatter):
    if key is None:
        return "     "
    label, code = key
    text = f"{label:^5}"
    if code in chatter:
        return f"{RED}{text}{RESET}"
    if code in done:
        return f"{GREEN}{text}{RESET}"
    return f"{DIM}{text}{RESET}"


def draw(done, chatter, counts, last):
    out = ["\033[H\033[2J", f"{BOLD}Corne — teste de teclas{RESET}   "
           f"{len(done)}/{len(ALL_KEYS)} ok   (verde = ok, vermelho = repique)\n"]
    for lrow, rrow in zip(LEFT, RIGHT):
        left = "".join(cell(k, done, chatter) for k in lrow)
        right = "".join(cell(k, done, chatter) for k in rrow)
        out.append(f"  {left}     {right}\n")
    out.append(f"\n  Última tecla: {last or '-'}\n")
    if chatter:
        names = ", ".join(KEY_BY_CODE.get(c, c) if isinstance(c, int) else c for c in chatter)
        out.append(f"  {RED} Repique em: {names} {RESET}  -> refazer solda do switch/diodo\n")
    out.append(f"\n  {DIM}Lower: segure LWR + Q   |   Raise: segure RSE + H   |   Ctrl+C sai{RESET}\n")
    sys.stdout.write("".join(out))
    sys.stdout.flush()


def main():
    dev = find_device()
    fd = os.open(dev, os.O_RDONLY)
    try:
        fcntl.ioctl(fd, EVIOCGRAB, 1)
    except OSError:
        print("Aviso: não consegui capturar o teclado; as teclas também vão digitar.")
    done, chatter, counts, last_press = set(), set(), {}, {}
    last = None
    last_activity = time.time()
    draw(done, chatter, counts, last)
    try:
        while len(done) < len(ALL_KEYS):
            r, _, _ = select.select([fd], [], [], 1.0)
            if not r:
                if time.time() - last_activity > IDLE_TIMEOUT:
                    break
                continue
            data = os.read(fd, EVENT.size * 64)
            for i in range(0, len(data), EVENT.size):
                sec, usec, etype, code, value = EVENT.unpack_from(data, i)
                if etype != EV_KEY or value != 1:   # só "press"
                    continue
                last_activity = time.time()
                t = sec + usec / 1e6
                if code in LOWER_ONLY:
                    target = "lower"
                elif code in RAISE_ONLY:
                    target = "raise"
                else:
                    target = code
                if target in last_press and (t - last_press[target]) * 1000 < CHATTER_MS:
                    chatter.add(target)
                last_press[target] = t
                counts[target] = counts.get(target, 0) + 1
                done.add(target)
                last = {"lower": "Lower (via camada)", "raise": "Raise (via camada)"}.get(
                    target, KEY_BY_CODE.get(code, f"código {code}"))
            draw(done, chatter, counts, last)
    except KeyboardInterrupt:
        pass
    finally:
        try:
            fcntl.ioctl(fd, EVIOCGRAB, 0)
        except OSError:
            pass
        os.close(fd)

    missing = [label for label, code in ALL_KEYS if code not in done]
    print()
    if not missing and not chatter:
        print(f"{GREEN} Todas as {len(ALL_KEYS)} teclas OK {RESET}")
    else:
        if missing:
            print(f"Sem resposta ({len(missing)}): {', '.join(missing)}")
        if chatter:
            print("Com repique: " + ", ".join(
                KEY_BY_CODE.get(c, c) if isinstance(c, int) else c for c in chatter))


if __name__ == "__main__":
    main()
