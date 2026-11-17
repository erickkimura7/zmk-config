#!/bin/bash
# Espera o nice!nano entrar no bootloader (curto RST-GND 2x) e copia o .uf2.
# Uso: tools/flash.sh arquivo.uf2   (espera ate 5 min)
F="$1"
[ -f "$F" ] || { echo "arquivo nao encontrado: $F"; exit 1; }
for i in $(seq 1 3000); do
  if lsusb -d 239a:00b3 >/dev/null 2>&1; then
    DEV=$(lsblk -rno NAME,LABEL | awk 'toupper($2)=="NICENANO"{print "/dev/"$1; exit}')
    if [ -n "$DEV" ]; then
      M=$(lsblk -rno MOUNTPOINT "$DEV" | head -1)
      [ -z "$M" ] && M=$(udisksctl mount -b "$DEV" --no-user-interaction 2>/dev/null | sed -n 's/.* at \(.*\)$/\1/p')
      if [ -n "$M" ] && cp "$F" "$M/"; then sync; echo "COPIED to $M"; sleep 6; lsusb | grep -iE "239a|1d50"; exit 0; fi
    fi
  fi
  sleep 0.1
done; echo TIMEOUT
