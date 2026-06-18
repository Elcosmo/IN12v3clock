# IN12v3clock

Firmware alternativo per la scheda Nixie GeekStyles IN12 V3, nota anche come
JM NixieClock IN12 V4.2.

## Hardware testato

RC9 e' stato preparato e documentato per la seguente configurazione reale:

- scheda `GeekStyles IN12 V3` / `JM NixieClock IN12 V4.2 20190514`;
- MCU `STM8S003F3P6`;
- RTC `DS3231`;
- tubi `IN-12`;
- programmatore `ST-Link V2`;
- batteria tampone RTC `CR1220`.

Foto della scheda:

[![Frontside Preview](Photo/pcb_front_preview.JPG)](Photo/pcb_front_fullsize.JPG)
[![Backside Preview](Photo/pcb_back_preview.JPG)](Photo/pcb_back_fullsize.JPG)

## Stato del firmware

Questa repo conserva il firmware originale come base, ma aggiunge modifiche
specifiche per l'uso quotidiano della scheda:

- lettura e validazione rigorosa del DS3231;
- lettura e impostazione manuale del weekday DS3231;
- convenzione weekday: `1=lunedi`, `2=martedi`, ..., `7=domenica`;
- schedule feriale/weekend configurabile da menu;
- rimozione della vecchia modalita' notte;
- spegnimento programmato di Nixie, RGB e colon;
- safe boot della catena HC595, con uscite spente prima dell'abilitazione;
- colon digitale stabile, senza PWM: 1 secondo acceso / 1 secondo spento;
- anti-avvelenamento automatico ogni 6 minuti, visibile solo dentro schedule;
- test host per schedule, EEPROM, DS3231, formato S19, gate uscite e safe boot;
- build riproducibile con Docker e SDCC.

Il firmware continua a mostrare solo ore e minuti sui quattro Nixie. I secondi
vengono letti internamente dal DS3231 e usati per sincronizzare il colon.

## Schedule feriale/weekend

Lo schedule usa il weekday del DS3231 e agisce come gate degli elementi visibili
nel normale funzionamento.

Default lunedi-venerdi:

- fascia 1: `07:00` inclusa -> `09:00` esclusa;
- fascia 2: `17:00` inclusa -> `00:00` esclusa.

Default sabato-domenica:

- fascia 1: `07:00` inclusa -> `12:00` esclusa;
- fascia 2: `19:00` inclusa -> `01:00` esclusa del giorno successivo.

I menu `3..10` permettono di cambiare solo l'ora intera di inizio/fine di ogni
fascia. Le regole sono:

- start incluso;
- end escluso;
- `start < end`: fascia nella stessa giornata;
- `start > end`: fascia che attraversa la mezzanotte;
- `start == end`: fascia disabilitata;
- la parte dopo mezzanotte usa il tipo del giorno precedente.

Esempi con i default: lunedi `00:30` resta acceso per la prosecuzione della
domenica; martedi `00:30` e sabato `00:30` sono spenti.

Fuori fascia, durante il normale funzionamento:

- Nixie spenti;
- RGB spenti;
- colon spento;
- anti-avvelenamento non visibile;
- RTC e microcontrollore restano attivi.

I menu restano utilizzabili anche fuori fascia: i Nixie possono essere mostrati
per configurare la scheda, mentre RGB e colon restano spenti dal gate.

## Comandi e menu RC9

Tasti:

- `K1`: tasto superiore;
- `K2`: tasto inferiore.

Comandi rapidi:

- pressione breve `K1`: avvia anti-avvelenamento manuale;
- pressione breve `K2`: commuta RGB globale;
- pressione lunga `K1`: impostazione ora;
- pressione lunga `K2`: regolazione colore RGB;
- pressione lunga `K1 + K2`: menu impostazioni.

Impostazione ora:

- `K1` incrementa ore o minuti;
- `K2` passa da ore a minuti e poi salva;
- al salvataggio i secondi vengono scritti a `00` nel DS3231.

Menu impostazioni RC9:

| Menu | Funzione | Valori |
|------|----------|--------|
| 0 | Zero iniziale ora | `0` disabilitato, `1` abilitato |
| 1 | Formato ora | `0` 24h; il 12h resta disabilitato dal firmware |
| 2 | Luminosita' normale Nixie | `5..100` |
| 3 | Feriale fascia 1 inizio | ora `0..23` |
| 4 | Feriale fascia 1 fine | ora `0..23` |
| 5 | Feriale fascia 2 inizio | ora `0..23` |
| 6 | Feriale fascia 2 fine | ora `0..23` |
| 7 | Weekend fascia 1 inizio | ora `0..23` |
| 8 | Weekend fascia 1 fine | ora `0..23` |
| 9 | Weekend fascia 2 inizio | ora `0..23` |
| 10 | Weekend fascia 2 fine | ora `0..23` |
| 11 | Colon | `0` 1 secondo ON / 1 secondo OFF, `1` sempre ON, `2` sempre OFF |
| 12 | Weekday DS3231 | `1..7`, lunedi..domenica |

Nei menu `3..10`, `K1` incrementa con wrap `23 -> 0`; `K2` salva il byte EEPROM
corrispondente e passa alla voce successiva. Il menu 12 scrive solo il registro
weekday del DS3231 e non usa EEPROM.

## EEPROM e migrazione RC9

Il vecchio firmware usava gli indirizzi EEPROM `3..10` per modalita' notte:
luminosita' notte, enable notte, intervallo notte, RGB notte e anti-poisoning
solo notte.

RC9 riusa gli stessi 8 byte per lo schedule:

| EEPROM logico | Indirizzo fisico | Nuovo significato | Default |
|---------------|------------------|-------------------|---------|
| 3 | `0x4003` | Feriale fascia 1 inizio | `07` |
| 4 | `0x4004` | Feriale fascia 1 fine | `09` |
| 5 | `0x4005` | Feriale fascia 2 inizio | `17` |
| 6 | `0x4006` | Feriale fascia 2 fine | `00` |
| 7 | `0x4007` | Weekend fascia 1 inizio | `07` |
| 8 | `0x4008` | Weekend fascia 1 fine | `12` |
| 9 | `0x4009` | Weekend fascia 2 inizio | `19` |
| 10 | `0x400A` | Weekend fascia 2 fine | `01` |
| 17 | `0x4011` | Versione configurazione schedule | `01` |

Al primo avvio con vecchio formato, il firmware inizializza solo questi 8 orari
ai default e scrive il byte versione. Luminosita' normale, RGB globale, colori,
colon e altre impostazioni non collegate allo schedule restano conservati. Se in
un avvio successivo uno degli 8 orari e' maggiore di `23`, tutti gli 8 orari
vengono ripristinati ai default. Il byte versione evita scritture ripetute a
ogni boot.

`0x4011 = 01` significa che la migrazione schedule RC9 e' completata. Non serve
programmare Data Memory: il byte viene scritto dal firmware al primo boot valido.

## Configurazione consigliata

1. Montare una CR1220 buona prima di configurare ora e weekday.
2. Impostare ora e minuti con pressione lunga di `K1`.
3. Impostare il weekday con menu `12`.
4. Usare menu `1 = 0` per formato 24h.
5. Regolare la luminosita' normale con menu `2`.
6. Lasciare i menu `3..10` ai default, oppure configurare le fasce desiderate.
7. Usare menu `11 = 0` per colon digitale: acceso sui secondi pari, spento sui
   secondi dispari.
8. Durante gli aggiornamenti firmware, programmare solo Program Memory.

Se il DS3231 contiene valori invalidi, il firmware tiene spenti Nixie, RGB e
colon nel normale funzionamento. I menu restano accessibili: impostare ora e
weekday, poi alla lettura valida successiva il funzionamento riprende.

## Anti-avvelenamento

L'anti-avvelenamento esegue una rotazione controllata dei catodi dei Nixie.

- automatico ogni 6 minuti;
- pressione breve `K1` lo avvia manualmente;
- fuori dalle fasce attive dello schedule non produce accensioni visibili.

## Batteria CR1220

La CR1220 alimenta il DS3231 quando la scheda e' scollegata. Se e' scarica o
assente, ora e weekday possono andare persi. In caso di ora corrotta, il firmware
non mostra valori RTC invalidi: usare i menu per reimpostare ora e weekday.

Per RC9, sulla scheda testata e' risultata empiricamente necessaria la rimozione
temporanea della CR1220 durante la programmazione e il primo avvio di migrazione.
La procedura consigliata sotto documenta questo comportamento osservato.

## Build Docker / SDCC

La build supportata usa Docker Compose e SDCC. Il submodule `STM8_headers` deve
essere inizializzato.

```bash
git submodule update --init --recursive
docker compose build
docker compose run --rm sdcc make clean
docker compose run --rm sdcc make
```

Artefatti principali:

- `SDCC/main.s19`;
- `SDCC/main.ihx`;
- `SDCC/main.map`.

Il microcontrollore ha 8192 byte di Program Memory. La CI fallisce se la Flash
calcolata dal map supera 8192 byte.

## Test

Lo script unico per test host e build STM8 e':

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/ci.ps1
```

Lo script esegue:

- test esaustivo dello schedule settimanale e degli orari personalizzati;
- test migrazione EEPROM e salvataggio menu `3..10`;
- test protocollo e validazione DS3231;
- test formato S19;
- test safe boot HC595;
- test gate uscite visibili e colon digitale;
- test recovery RTC/menu;
- build Docker/SDCC pulita;
- validazione del file `SDCC/main.s19`;
- controllo dimensione Flash.

## GitHub Actions

Il workflow `.github/workflows/firmware-ci.yml` gira su push e pull request verso
`master`, clona i submodule, esegue `scripts/ci.ps1` e carica `main.s19`,
`main.ihx` e `main.map` come artifact.

## Collegamento ST-Link V2

Collegamenti usati sulla scheda testata:

| ST-Link V2 | Scheda |
|------------|--------|
| SWIM | SWIM |
| NRST | RST |
| GND | GND |

Non collegare `3,3 V` o `5 V` dallo ST-Link. La scheda deve essere alimentata
dalla propria USB durante la programmazione.

Pin STM8 rilevanti:

- `PD1`: SWIM;
- `NRST`: reset;
- `PB5/PB4`: I2C DS3231;
- `PD6`: colon;
- `PC5/PC7/PD2/PD3`: catena HC595;
- `PC3/PC4/PC6`: RGB.

![SWD pinout](Photo/swd_pinout.jpg)

## Programmazione RC9

Per aggiornare una scheda gia' configurata con RC recenti, programmare soltanto
la Program Memory con `main.s19`. Non programmare Data Memory e non programmare
Option Byte durante aggiornamenti ordinari.

Procedura consigliata sulla scheda testata:

1. Spegnere la scheda.
2. Rimuovere temporaneamente la CR1220.
3. Collegare ST-Link: `SWIM -> SWIM`, `NRST -> RST`, `GND -> GND`.
4. Alimentare la scheda dalla propria USB.
5. Caricare il file S19 e programmare solo `PROGRAM MEMORY`.
6. Usare `Program -> Current tab`.
7. Impostare `Verify after programming = ON`.
8. Non usare `All tabs`, `Fill area`, Data Memory o Option Byte.
9. Spegnere, scollegare ST-Link e fare il primo avvio senza ST-Link e senza batteria.
10. Attendere 20-30 secondi per la migrazione EEPROM.
11. Verificare i menu `3..10`.
12. Spegnere e reinserire la CR1220.

`File Checksum` e `Device Checksum` possono differire. Non e' automaticamente un
errore: conta soprattutto il messaggio `PROGRAM MEMORY successfully verified`.

Option Byte corretti osservati sulla scheda testata:

```text
00 00 01 00 00 00
```

Durante aggiornamenti normali non vanno riprogrammati. Gli Option Byte includono
configurazioni hardware delicate: ROP puo' impedire readback/backup del firmware
originale e AFR0 deve restare coerente con la scheda.

Prima di ogni flash, tentare un backup del firmware originale. Se ROP e' attivo,
il backup puo' non essere possibile senza cancellare il dispositivo.

## Troubleshooting RC9

- Menu `3` mostra ancora la vecchia luminosita' notte: RC9 non e' stato avviato
  correttamente o la migrazione non e' stata eseguita.
- `0x4011 = 00`: migrazione schedule non completata.
- Checksum file/device diversi non indicano da soli un errore.
- Non usare `Fill area`.
- Non programmare Data Memory per forzare la migrazione: non serve.
- In caso di problemi, rimuovere la batteria, riprogrammare solo Program Memory e
  fare un boot autonomo senza ST-Link e senza CR1220, poi attendere 20-30 secondi.

## Licenza

Questo progetto e' distribuito con licenza MIT.
