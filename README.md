# zmk-config — Corne + nice!nano v2 (clone)

Firmware ZMK (v0.3) compilado via GitHub Actions. O keymap completo, com
desenho de cada camada, está em `config/corne.keymap`.

## Uso no dia a dia

Sistema operacional configurado como **US Internacional**.

| Quero... | Como |
|---|---|
| á é í ó ú | `'` e depois a letra |
| ã õ | `~` (Raise + ESC) e depois a letra |
| â ê ô | `^` (Raise + Y) e depois a letra |
| à | `` ` `` (Raise + TAB) e depois a letra |
| ç | `'` e depois `c` (ou AltGr + `,`) |
| `'` `"` `` ` `` `~` `^` sozinhos | a tecla e depois **Espaço** |
| Números / setas | segurar **Lower** |
| Símbolos de código | segurar **Raise** |
| F1–F12, Bluetooth, mídia | segurar **Lower + Raise** |
| CONSTANTE_EM_MAIUSCULA | tocar **CAPW** (canto inferior direito) e digitar |

### Home row mods
Segurar A / S / D / F = GUI / Alt / Ctrl / Shift (e J / K / L / ; espelhado).
Tocar digita a letra normalmente. Só vira modificador se a próxima tecla for da
outra mão, então Ctrl+C = segurar **K** (Ctrl) e tocar **C**.

### Mac (perfil 2)
O modo Mac liga sozinho quando o perfil Bluetooth 2 está selecionado (Lower + Raise + S),
inclusive ao acordar do sono (módulo `src/mac_profile.c`). Na mão: Lower + Raise + X.
- Cmd = segurar K ou D (copiar = K + C, igual ao Windows); Control = segurar ; ou A.
- Lower + Z X C V = ⌘Z ⌘X ⌘C ⌘V; Lower + H / L = início / fim da linha (⌘← / ⌘→).
- Acentos: no Mac use a fonte de entrada "U.S. International – PC".

### Bluetooth e USB
- Lower + Raise + A..G: perfil Bluetooth 1..5; Lower + Raise + ESC: apaga o perfil atual.
- Lower + Raise + H: manda as teclas pelo **USB**; + J: pelo **Bluetooth**
  (útil para carregar no PC e continuar digitando no outro por Bluetooth).

### Gravar firmware sem curto no RST
Lower + Raise + **tecla do canto inferior** (esquerda: SHIFT, direita: CAPW) entra
no bootloader da metade onde a tecla está e monta o drive `NICENANO`.
Depois: `tools/flash.sh firmware/corne_left-nice_nano_v2-zmk.uf2` (ou a da direita).

## Particularidades destes clones (Pro Micro nRF52840)
Não têm o cristal de 32 kHz do nice!nano original. Sem ajuste, o Bluetooth anuncia
mas toda conexão cai. Correções em `config/corne.conf`:
- `CLOCK_CONTROL_NRF_K32SRC_SYNTH`: relógio de 32 kHz gerado do cristal de 32 MHz;
- `ZMK_KEEP_HFXO_ON` (módulo `src/keep_hfxo_on.c`): mantém o cristal de 32 MHz
  sempre ligado; sem isso a ligação cai na bateria (no USB funcionava);
- `ZMK_BLE_PASSKEY_ENTRY=n`: pareamento sem PIN.

## Ferramentas (`tools/`)
- `keytest.py [--left|--right]`: testa todas as teclas (solda de switch/diodo).
- `flash.sh arquivo.uf2`: espera o bootloader e copia o firmware.
- `bt-pair.sh`: pareia o teclado com o Linux.
- `pinprobe/`: firmware Arduino de diagnóstico de pinos.

Diagnóstico de reinícios: o build `corne_right_logging` grava o motivo de cada
reinício na flash (`src/reset_reason_log.c`) e mostra o histórico no log USB.
