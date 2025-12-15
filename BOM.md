# 📋 Bill of Materials (BOM)

## Lista Completă de Componente

### Microcontroller (alege una)

| Componentă | Cantitate | Cumpără de la | Preț Aprox. |
|------------|-----------|---------------|-------------|
| Arduino Nano (ATmega328) | 1 | AliExpress/Optimus | ~15 RON |
| **SAU** ESP-01 (ESP8266) | 1 | AliExpress/Optimus | ~8 RON |
| PCF8574 I2C Expander | 1* | AliExpress/Optimus | ~5 RON |

> *Doar pentru versiunea ESP-01

---

### Circuit ZCD (Zero Crossing Detector)

| Componentă | Valoare | Cantitate | Package | Notă |
|------------|---------|-----------|---------|------|
| Optocuplor | 4N35 | 1 | DIP-6 | Sau H11AA1 pentru bidirectional |
| Rezistor | 100kΩ | 2 | 1/4W | În serie pentru siguranță |
| Rezistor | 10kΩ | 1 | 1/4W | Pull-up pe ieșire |

---

### Circuit Triac (pentru FIECARE canal)

| Componentă | Valoare | Cantitate/canal | Package | Notă |
|------------|---------|-----------------|---------|------|
| Optocuplor Triac | MOC3021 | 1 | DIP-6 | Random phase, pentru sarcini rezistive |
| Triac | BT136-600E | 1 | TO-220 | 4A/600V, suficient pentru becuri |
| Rezistor | 330Ω | 1 | 1/4W | LED driver MOC3021 |
| Rezistor | 100Ω | 1 | 1W | Snubber |
| Condensator | 100nF/400V | 1 | Film | Snubber, IMPORTANT: rating 400V+ |

**Pentru 2 canale, multiplică × 2!**

---

### Interfață Utilizator

| Componentă | Cantitate | Package | Notă |
|------------|-----------|---------|------|
| TTP223 Touch Button Module | 2 | Modul | Cu LED indicator |
| TSOP4838 IR Receiver | 1 | 3-pin | 38kHz, sau TSOP1838 |

---

### Alimentare

| Componentă | Valoare | Cantitate | Package | Notă |
|------------|---------|-----------|---------|------|
| Modul alimentare | HLK-PM01 | 1 | Modul | 5V/3W, izolat |
| **SAU** | HLK-PM03 | 1 | Modul | 3.3V/3W pentru ESP-01 |
| Regulator | AMS1117-3.3 | 1* | SOT-223 | Dacă folosești HLK-PM01 + ESP-01 |
| Condensator | 100µF/25V | 1 | Electrolitic | Filtrare alimentare |
| Condensator | 100nF | 2 | Ceramic | Decuplare |

---

### Conectică și Diverse

| Componentă | Cantitate | Notă |
|------------|-----------|------|
| Cleme șir | 1 set | Pentru conexiuni 220V |
| Cablaj | - | Preferabil 0.5mm² pentru 220V |
| PCB prototip | 1 | 5×7 cm suficient |
| Carcasă | 1 | Izolantă, cu loc pentru ventilație |
| Siguranță 1A | 1 | Protecție circuit |

---

## Rezumat Costuri

| Categorie | Preț Aprox. (RON) |
|-----------|-------------------|
| Microcontroller | 8-15 |
| Circuit ZCD | 5-10 |
| Circuite Triac (×2) | 15-25 |
| Interfață (butoane + IR) | 10-15 |
| Alimentare | 15-25 |
| Diverse | 10-15 |
| **TOTAL** | **~60-100 RON** |

---

## Cod Componente pentru Comandă

### AliExpress Search Terms

```
- "Arduino Nano V3 ATmega328"
- "ESP-01 ESP8266 WiFi module"
- "PCF8574 I2C IO expander"
- "4N35 optocoupler DIP"
- "MOC3021 triac optocoupler"
- "BT136-600E triac TO-220"
- "TTP223 touch sensor module"
- "TSOP4838 IR receiver 38kHz"
- "HLK-PM01 AC DC 5V 3W"
- "100nF 400V film capacitor"
```

### Optimus Digital / Robofun

```
- Arduino Nano
- ESP8266 ESP-01
- Optocuplor 4N35
- Optocuplor MOC3021
- Triac BT136
- Modul touch capacitiv
- Receptor IR TSOP1838
- Sursă alimentare Hi-Link 5V
```

---

## Alternative Componente

| Original | Alternativă | Notă |
|----------|-------------|------|
| 4N35 | 4N25, H11AA1 | H11AA1 = AC input |
| MOC3021 | MOC3020, MOC3022 | 3020/3022 similar |
| BT136 | BTA16, BT139 | BTA16 = 16A (overkill dar merge) |
| TSOP4838 | TSOP1838, VS1838 | Toate la 38kHz |

---

## Verificare Pre-Comandă

- [ ] Am ales platforma (ATmega328 sau ESP-01)
- [ ] Am numărat corect cantitățile pentru 2 canale
- [ ] Am verificat tensiunile (condensatoare 400V+ pentru 220V AC)
- [ ] Am comandat și componente de rezervă (mai ales optocuploare)
- [ ] Am inclus carcasă izolantă pentru siguranță

---

**💡 Sfat:** Comandă 2-3 bucăți din fiecare componentă critică (optocuploare, triace) - sunt ieftine și pot fi defecte sau le poți arde accidental la testare!
