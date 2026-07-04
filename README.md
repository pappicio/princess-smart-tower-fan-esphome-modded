# 🌀 Princess Smart Tower Fan – ESPHome Local Control

Firmware ESPHome che sostituisce il modulo WiFi originale (ESP-WROOM-02) del **Princess Smart Tower Fan (350000)**, eliminando completamente la dipendenza dal cloud del produttore. Controllo 100% locale via Home Assistant, dashboard web integrata, automazioni, e integrazione vocale Alexa/Matter — nessun account, nessun server esterno, nessuna latenza cloud.

## ✨ Perché questo progetto

Il fan originale comunica con l'app "HomeWizard Climate" solo tramite server del produttore, anche per comandi impartiti da touch fisico. Questo firmware **ricrea fedelmente ogni comando originale** (velocità, oscillazione, modalità natural/sleep, timer 0-8h) via protocollo UART nativo del controller, sostituendo il modulo WiFi con un ESP32 che parla direttamente con l'hardware — zero cloud, zero telemetria esterna.

## 🚀 Funzionalità

- ✅ Controllo velocità (3 livelli)
- ✅ Oscillazione (swing) on/off
- ✅ Modalità **Normal / Natural / Sleep**
- ✅ Timer programmabile 0–8h (step 0.5h)
- ✅ **Feedback bidirezionale**: i comandi impartiti dai tasti touch fisici si riflettono in tempo reale su Home Assistant
- ✅ Nessuna sovrascrittura di stato: ogni comando (speed/swing/mode/timer) preserva gli altri campi
- ✅ Esposizione nativa **Alexa** e **Matter** tramite [Home Assistant Matter Hub](https://github.com/t0bst4r/home-assistant-matter-hub)
- ✅ Dashboard web locale integrata (`web_server`)
- ✅ Log diagnostico comandi inviati/ricevuti

## 🔧 Hardware richiesto

- ESP32-S2 (testato su modulo "S2 mini" / supermini)
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

Esposizione vocale tramite [Home Assistant Matter Hub](https://github.com/t0bst4r/home-assistant-matter-hub), che mappa automaticamente l'entity fan nativa sul Matter Fan device type — nessuna configurazione aggiuntiva richiesta oltre al bridge standard.

## 🙏 Crediti

- Protocollo UART originariamente documentato da **frank-f** e **JojoS62** nella [discussione Tasmota #13664](https://github.com/arendst/Tasmota/discussions/13664)
- Ispirazione strutturale per il custom UART fan component da [yeelight-c900](https://github.com/0neday/yeelight-c900)
- anche se i pochi bytes, erano riferiti alla sola velocità 1, quindi per arrivare a combinare tutte le opzioni tra velocità, swing, mode e sleep, hanno portato altro lavoro, ma alla fine, è COMPLETO!!!!
## ⚠️ Disclaimer

Questo progetto richiede l'apertura e la modifica hardware di un elettrodomestico collegato alla rete elettrica 220-240V. Procedi solo se hai esperienza con elettronica e sicurezza elettrica. Nessuna garanzia, uso a proprio rischio.
