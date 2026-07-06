# 🌀 Princess Smart Tower Fan – ESPHome Local Control

Firmware ESPHome che sostituisce il modulo WiFi originale (ESP-WROOM-02) del **Princess Smart Tower Fan (modello: 350000; part number: 01.350000.01.001)**, eliminando completamente la dipendenza dal cloud del produttore. Controllo 100% locale via Home Assistant, dashboard web integrata, automazioni, e integrazione vocale Alexa/Matter — nessun account, nessun server esterno, nessuna latenza cloud.

## ✨ Perché questo progetto

Il fan originale comunica (o meglio, nel mio caso, dovrebbe comunicare, cosa non fattibile per: 1 app obsoleta e nn compatibile con versioen android del mio cellulare; 2 anche installandola su cellulare datato, non c'e stato verso di fare il pairing!!!!) con l'app "HomeWizard Climate" solo tramite server del produttore, anche per comandi impartiti da touch fisico. Questo firmware **ricrea fedelmente ogni comando originale** (velocità, oscillazione, modalità natural/sleep, timer 0-8h) via protocollo UART nativo del controller, sostituendo il modulo WiFi con un ESP32 che parla direttamente con l'hardware — zero cloud, zero telemetria esterna.

## 🚀 Funzionalità

- ✅ Controllo velocità (3 livelli)
- ✅ Oscillazione (swing) on/off
- ✅ Modalità **Normal / Natural / Sleep**
- ✅ Timer programmabile 0–8h (step 0.5h)
- ✅ **Feedback bidirezionale in tempo reale**: ogni comando impartito dai tasti touch fisici del fan (o eventualmente da un telecomando IR) viene letto dal bus UART e sincronizzato immediatamente su Home Assistant — velocità, oscillazione, modalità e timer residuo sono sempre allineati allo stato reale dell'hardware, indipendentemente da dove parte il comando (touch fisico, dashboard, automazione, Alexa)
- ✅ Nessuna sovrascrittura di stato: ogni comando (speed/swing/mode/timer) preserva gli altri campi
- ✅ Esposizione nativa **Alexa** e **Matter** tramite [Home Assistant Matter Hub](https://github.com/RiDDiX/home-assistant-matter-hub/)
- ✅ Dashboard web locale integrata (`web_server`)
- ✅ Log diagnostico comandi inviati/ricevuti
- ✅ Aggiunta funzione ***beeper*** ad invio comandi. <nessun beep / beep breve / beep lungo>

## 🔧 Hardware richiesto

- ESP8266/ESP32 (testato su modulo "S2 mini" / supermini)
- Accesso fisico al bus UART interno del fan (pad TX/RX del modulo ESP-WROOM-02 originale)

### Collegamento (sostituzione 1:1 del modulo originale)

| Pad scheda fan | ESP32-S2 |
|---|---|
| RX | TX (GPIO17) |
| TX | RX (GPIO18) |
| GND | GND |
| 3.3V | 3.3V |

> ⚠️ Nessun isolamento galvanico nel design originale del fan (alimentazione a condensatore). Alimenta l'ESP da un alimentatore USB isolato, mai direttamente dal PC durante i test da banco.

## 📡 Protocollo UART reverse-engineered

Basato sul lavoro di reverse engineering condiviso nella [discussione Tasmota #13664](https://github.com/arendst/Tasmota/discussions/13664) (frank-f, JojoS62), esteso e corretto in questo progetto per gestire tutti i campi **simultaneamente** senza sovrascritture.

- UART, 9600 baud, 8N1
- Datagramma a 4 byte, header fisso `AA A0`
- Ogni campo (speed/swing/mode/timer) occupa bit dedicati senza sovrapposizioni
- Formula timer: `steps = ore × 30` (minuti/2), distribuiti su 6 bit (byte3) + 2 bit (byte4)

## 🔬 Composizione dei byte del protocollo

Ogni comando/feedback è un datagramma a 4 byte: `AA A0 [byte3] [byte4]`.
I primi due byte sono un header fisso di sincronizzazione. Gli ultimi due
contengono tutti i parametri, codificati bit a bit **senza sovrapposizioni**:
ogni informazione ha il proprio posto fisso, indipendentemente dalle altre.

### Byte 3 — Timer (parte bassa) + Swing
Bit:     7   6   5   4   3   2   1   0
[SW][0 ][      TIMER (6 bit)    ]
SW  (bit 7) = Swing/oscillazione: 1=ON, 0=OFF
bit 6       = sempre 0 (non usato)
bit 5-0     = 6 bit meno significativi del contatore timer (0-63)

**Esempi:**
0x00 = 00000000 → swing OFF, timer_low = 0
0x40 = 01000000 → swing ON,  timer_low = 0
0x3C = 00111100 → swing OFF, timer_low = 60 (max, = 8 ore)
0x7C = 01111100 → swing ON,  timer_low = 60

### Byte 4 — Velocità + Modalità + Timer (parte alta)
Bit:     7   6   5   4   3   2   1   0
[   TH  ][0 ][SL][NA][  SPEED  ]
TH (bit 7-6) = 2 bit più significativi del timer (moltiplicano per 64/128/192)
bit 5        = sempre 0 (non usato)
SL (bit 4)   = Modalità Sleep: 1=ON, 0=OFF
NA (bit 3)   = Modalità Natural: 1=ON, 0=OFF
bit 2-0      = Velocità: 011=speed1, 101=speed2, 111=speed3, 010=OFF

**Esempi:**
0x03 = 00000011 → speed1, nessuna modalità, timer_high=0
0x05 = 00000101 → speed2, nessuna modalità, timer_high=0
0x0B = 00001011 → speed1, modalità Natural (bit3=1)
0x13 = 00010011 → speed1, modalità Sleep (bit4=1)
0x02 = 00000010 → OFF (spegnimento)

### Timer: come si ricompone il valore completo

Il contatore timer è a 8 bit totali, ma **spezzato in due pezzi** tra i due byte:
timer_totale = (timer_low << 2) | timer_high
minuti_residui = timer_totale × 2

**Esempio pratico — Timer impostato a 3 ore (180 minuti):**
steps = 180 / 2 = 90  (in binario: 01011010)
timer_low  = 90 >> 2        = 22   (0x16, va nei bit 5-0 del byte3)
timer_high = 90 & 0x03      = 2    (va nei bit 7-6 del byte4, shiftato: 2 << 6 = 0x80)
byte3 = 0x16 (timer_low, swing OFF)
byte4 = 0x80 | speed_code    (esempio con speed1: 0x80 | 0x03 = 0x83)
Pacchetto finale: AA A0 16 83

### Perché nessun campo si sovrappone

Ogni valore possibile di ciascun campo resta dentro il proprio range di bit
dedicato, quindi combinarli con un semplice OR bit a bit (`|`) non causa mai
collisioni:

| Campo | Byte | Bit occupati | Range valori |
|---|---|---|---|
| Swing | 3 | bit 7 | 0-1 |
| Timer (basso) | 3 | bit 5-0 | 0-63 |
| Timer (alto) | 4 | bit 7-6 | 0-3 |
| Sleep | 4 | bit 4 | 0-1 |
| Natural | 4 | bit 3 | 0-1 |
| Velocità | 4 | bit 2-0 | 3/5/7 (OFF=2) |

Questo permette di **modificare un solo parametro alla volta** (es. solo la
velocità) senza dover conoscere o toccare lo stato degli altri campi — il
firmware ricostruisce sempre il pacchetto completo combinando lo stato
corrente di tutti i parametri ad ogni invio.

## 📦 Installazione

1. Installa [ESPHome](https://esphome.io)
2. Copia `princess-fan.yaml` e il tuo `secrets.yaml`
3. Compila e flasha via USB la prima volta
4. Apri il fan, sostituisci il modulo ESP-WROOM-02 con il tuo ESP32-S2 secondo lo schema sopra
5. Aggiungi il dispositivo in Home Assistant tramite l'integrazione ESPHome

## 🏠 Integrazione Home Assistant

Una volta aggiunto, il dispositivo espone:
- `fan.ventilatore` — entity fan nativa (speed, oscillating, preset_mode)
- `number.fan_timer_ore` — timer 0-8h
- `text_sensor.fan_mode` — diagnostica modalità corrente
- Sensori diagnostici (uptime, IP, ultimo riavvio)

## 🗣️ Alexa & Matter

Esposizione vocale tramite [Home Assistant Matter Hub](https://github.com/RiDDiX/home-assistant-matter-hub/), che mappa automaticamente l'entity fan nativa sul Matter Fan device type — nessuna configurazione aggiuntiva richiesta oltre al bridge standard.

## 🙏 Crediti

- Protocollo UART originariamente documentato da **frank-f** e **JojoS62** nella [discussione Tasmota #13664](https://github.com/arendst/Tasmota/discussions/13664)
- Ispirazione strutturale per il custom UART fan component da [yeelight-c900](https://github.com/0neday/yeelight-c900)
- anche se i pochi bytes, erano riferiti alla sola velocità 1, quindi per arrivare a combinare tutte le opzioni tra velocità, swing, mode e sleep, hanno portato altro lavoro, ma alla fine, è COMPLETO!!!!
## ⚠️ Disclaimer

Questo progetto richiede l'apertura e la modifica hardware di un elettrodomestico collegato alla rete elettrica 220-240V. Procedi solo se hai esperienza con elettronica e sicurezza elettrica. Nessuna garanzia, uso a proprio rischio.
