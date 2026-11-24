#!/bin/bash
# Procura um teclado BLE chamado "Corne" e pareia/confia/conecta com este computador.
# Uso: tools/bt-pair.sh [segundos]   (padrao 120)
T=${1:-120}
bluetoothctl --timeout 2 agent NoInputNoOutput >/dev/null 2>&1
bluetoothctl --timeout 2 default-agent >/dev/null 2>&1
coproc BT { bluetoothctl 2>&1; }
echo "scan on" >&"${BT[1]}"
end=$((SECONDS + T)); MAC=""
while [ $SECONDS -lt $end ] && [ -z "$MAC" ]; do
  MAC=$(bluetoothctl devices | awk '$3 ~ /^Corne/ {print $2; exit}')
  sleep 1
done
echo "scan off" >&"${BT[1]}"; echo "quit" >&"${BT[1]}"; wait "$BT_PID" 2>/dev/null
[ -z "$MAC" ] && { echo "Corne NAO encontrado em ${T}s"; exit 1; }
echo "Encontrado: $MAC"
bluetoothctl --timeout 20 pair "$MAC"
bluetoothctl trust "$MAC"
bluetoothctl --timeout 15 connect "$MAC"
bluetoothctl info "$MAC" | grep -E "Name|Paired|Bonded|Trusted|Connected"
