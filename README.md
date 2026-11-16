# zmk-config — Corne + nice!nano v2

Firmware ZMK (v0.3) compilado via GitHub Actions.

- `config/corne.keymap` — layout (camadas: default, lower, raise)
- `config/corne.conf` — opções (sleep, bateria, OLED)
- `build.yaml` — o que compilar (left, right, settings_reset)

## Gravar
1. Baixe o artefato `firmware` na aba Actions.
2. Conecte a metade ESQUERDA via USB e aperte RST duas vezes rápido → aparece o drive `NICENANO`.
3. Copie `corne_left-nice_nano_v2-zmk.uf2` para o drive (ele reinicia sozinho).
4. Repita com a metade DIREITA e `corne_right-...uf2`.

A esquerda é a central (conecta ao PC). Bluetooth: Lower + tecla BT1..BT5 seleciona perfil, Lower + BTCLR limpa.

Se as metades não se conectarem: grave `settings_reset` nas duas, depois o firmware normal de novo, e resete as duas ao mesmo tempo.
