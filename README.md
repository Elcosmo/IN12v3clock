# IN12v3clock

Firmware alternativo per la scheda Nixie GeekStyles IN12 V3, nota anche come
JM NixieClock IN12 V4.2.

Questa variante e' destinata alla scheda reale con:

- serigrafia frontale: `GeekStyles IN12 V3`;
- serigrafia posteriore: `JM NixieClock IN12 V4.2 20190514`;
- MCU `STM8S003F3P6`;
- RTC `DS3231`;
- tubi `IN-12`;
- catena di shift register 74HC595 e driver 2003A/ULN2003A;
- batteria tampone RTC `CR1220`.

Foto della scheda:

[![Frontside Preview](Photo/pcb_front_preview.JPG)](Photo/pcb_front_fullsize.JPG)
[![Backside Preview](Photo/pcb_back_preview.JPG)](Photo/pcb_back_fullsize.JPG)

## Stato del firmware

Questa repo conserva il firmware originale come base, ma aggiunge una serie di
modifiche specifiche per l'uso quotidiano della scheda:

- lettura e validazione rigorosa del DS3231;
- lettura e impostazione manuale del weekday DS3231;
- convenzione weekday: `1=lunedi`, `2=martedi`, ..., `7=domenica`;
- spegnimento programmato di Nixie, RGB e colon;
- menu 12 per impostare il weekday;
- safe boot della catena HC595, con uscite spente prima dell'abilitazione;
- colon digitale stabile, senza PWM: 1 secondo acceso / 1 secondo spento;
- test host per schedule, DS3231, formato S19, gate uscite e safe boot;
- build riproducibile con Docker e SDCC.

Il firmware continua a mostrare solo ore e minuti sui quattro Nixie. I secondi
vengono letti internamente dal DS3231 e usati per sincronizzare il colon.

## Schedule feriale/weekend

Lo schedule usa il weekday del DS3231 e agisce come gate degli elementi visibili
nel normale funzionamento.

Lunedi-venerdi:

- acceso dalle `07:00` incluse alle `09:00` escluse;
- acceso dalle `17:00` incluse alle `24:00` escluse.

Sabato-domenica:

- acceso dalle `07:00` incluse alle `12:00` escluse;
- acceso dalle `19:00` incluse alle `01:00` escluse del giorno successivo.

Lo schedule considera anche la fascia iniziata il giorno precedente. Per esempio
lunedi `00:30` resta acceso per la prosecuzione della domenica, mentre martedi
`00:30` e' spento.

Fuori fascia, durante il normale funzionamento:

- Nixie spenti;
- RGB spenti;
- colon spento;
- anti-avvelenamento non visibile;
- RTC e microcontrollore restano attivi.

I menu restano utilizzabili anche fuori fascia: i Nixie possono essere mostrati
per configurare la scheda, mentre RGB e colon restano gestiti dal gate.

## Comandi e menu

Tasti:

- `K1`: tasto superiore;
- `K2`: tasto inferiore.

Comandi rapidi:

- pressione breve `K1`: avvia anti-avvelenamento;
- pressione breve `K2`: commuta RGB globale, o RGB notte quando la modalita'
  notte e' attiva;
- pressione lunga `K1`: impostazione ora;
- pressione lunga `K2`: regolazione colore RGB;
- pressione lunga `K1 + K2`: menu impostazioni.

Impostazione ora:

- `K1` incrementa ore o minuti;
- `K2` passa da ore a minuti e poi salva;
- al salvataggio i secondi vengono scritti a `00` nel DS3231.

Menu impostazioni:

| Menu | Funzione | Valori |
|------|----------|--------|
| 0 | Zero iniziale ora | `0` disabilitato, `1` abilitato |
| 1 | Formato ora | `0` 24h, `1` 12h |
| 2 | Luminosita' normale Nixie | `5..100` |
| 3 | Luminosita' notte Nixie | `5..100` |
| 4 | Luminosita' notte abilitata | `0` no, `1` si |
| 5 | Ora inizio notte | `0..23` o `1..12` secondo formato |
| 6 | Minuto inizio notte | `0..59` |
| 7 | Ora fine notte | `0..23` o `1..12` secondo formato |
| 8 | Minuto fine notte | `0..59` |
| 9 | RGB in modalita' notte | `0` spento, `1` acceso |
| 10 | Anti-avvelenamento solo notte | `0` normale, `1` solo notte |
| 11 | Colon | `0` 1 secondo ON / 1 secondo OFF, `1` sempre ON, `2` sempre OFF |
| 12 | Weekday DS3231 | `1..7`, lunedi..domenica |

Il menu 12 scrive solo il registro weekday del DS3231. Non usa EEPROM.

## Configurazione consigliata

1. Montare una CR1220 buona prima di configurare ora e weekday.
2. Impostare ora e minuti con pressione lunga di `K1`.
3. Impostare il weekday con menu `12`.
4. Usare menu `1 = 0` per formato 24h, salvo preferenze diverse.
5. Usare menu `11 = 0` per colon digitale: acceso sui secondi pari, spento sui
   secondi dispari.
6. Regolare luminosita' normale e notte con menu `2` e `3`.
7. Lasciare invariati EEPROM e Option Byte durante gli aggiornamenti firmware.

Se il DS3231 contiene valori invalidi, il firmware tiene spenti Nixie, RGB e
colon nel normale funzionamento. I menu restano accessibili: impostare ora e
weekday, poi alla lettura valida successiva il funzionamento riprende.

## Anti-avvelenamento

L'anti-avvelenamento esegue una rotazione controllata dei catodi dei Nixie.
Fuori dalle fasce attive dello schedule non produce accensioni visibili.

Il menu `10` controlla quando viene eseguito:

- `0`: comportamento normale, ogni 6 minuti;
- `1`: solo nella fascia notte configurata, ogni 2 minuti.

## Batteria CR1220

La CR1220 alimenta il DS3231 quando la scheda e' scollegata. Se e' scarica o
assente, ora e weekday possono andare persi. In caso di ora corrotta, il firmware
non mostra valori RTC invalidi: usare i menu per reimpostare ora e weekday.

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

- test esaustivo dello schedule settimanale;
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

## Collegamento ST-Link

Connettore debug della scheda:

| Pin | Segnale | Descrizione |
|-----|---------|-------------|
| 1 | 3.3V | alimentazione/riferimento |
| 2 | SWIM | interfaccia programmazione |
| 3 | NRST | reset |
| 4 | GND | massa |

Collegare almeno `SWIM`, `NRST` e `GND`; usare 3.3 V solo se l'alimentazione
della scheda e del programmatore lo richiede ed e' sicura. Non applicare 5 V alla
logica STM8.

Pin STM8 rilevanti:

- `PD1`: SWIM;
- `NRST`: reset;
- `PB5/PB4`: I2C DS3231;
- `PD6`: colon;
- `PC5/PC7/PD2/PD3`: catena HC595;
- `PC3/PC4/PC6`: RGB.

![SWD pinout](Photo/swd_pinout.jpg)

## Programmazione

Per aggiornare una scheda gia' configurata con RC recenti, programmare soltanto
la Program Memory con `main.s19`.

Non programmare Data EEPROM e Option Byte durante aggiornamenti ordinari:

- la EEPROM contiene le impostazioni salvate;
- gli Option Byte includono configurazioni hardware delicate;
- ROP puo' impedire il readback/backup del firmware originale;
- AFR0 deve restare coerente con la scheda per non rompere le funzioni dei pin
  usati dalla catena di controllo.

Prima di ogni flash, tentare un backup del firmware originale. Se ROP e' attivo,
il backup puo' non essere possibile senza cancellare il dispositivo.

## Licenza

Questo progetto e' distribuito con licenza MIT.
