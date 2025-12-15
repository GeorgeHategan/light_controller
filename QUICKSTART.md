# 🚀 Quick Start Guide - AC Dimmer Controller

## Pasul 1: Alege Platforma

| Întrebare | ATmega328 | ESP-01 |
|-----------|-----------|--------|
| Ai nevoie de WiFi? | ❌ | ✅ |
| Vrei consum ultra-mic în sleep? | ✅ | ❌ |
| Ai experință cu I2C? | Nu contează | Recomandat |

📂 Folder: `atmega328_dimmer/` sau `esp01_dimmer/`

---

## Pasul 2: Componente Esențiale

### Lista de Cumpărături

```
✓ 1x Placă ATmega328 (Arduino Nano) SAU ESP-01
✓ 1x Optocuplor 4N35 (pentru ZCD)
✓ 2x Optocuplor MOC3021 (pentru triace)
✓ 2x Triac BT136 (sau BTA16)
✓ 2x Modul TTP223 (butoane touch)
✓ 1x TSOP4838 (receptor IR 38kHz)
✓ 2x Rezistor 100kΩ
✓ 2x Rezistor 330Ω
✓ 1x Rezistor 10kΩ
✓ 2x Rezistor 100Ω (snubber)
✓ 2x Condensator 100nF/400V (snubber)
✓ [ESP-01 only] 1x PCF8574 I2C expander
```

---

## Pasul 3: Conexiuni

### ATmega328 (Arduino Nano)

```
                    Arduino Nano
                   ┌─────────────┐
     ZCD Signal ──►│ D2          │
                   │             │
      → TRIAC1 ◄──│ D3          │
      → TRIAC2 ◄──│ D4          │
                   │             │
    Button 1 ─────►│ D5          │
    Button 2 ─────►│ D6          │
                   │             │
     IR Recv ─────►│ D7          │
                   └─────────────┘
```

### ESP-01 cu PCF8574

```
     ESP-01                 PCF8574
    ┌───────┐              ┌───────┐
    │GPIO0──│──── SDA ────►│SDA    │
    │GPIO2──│──── SCL ────►│SCL    │
    │GPIO3◄─│──── ZCD      │P0────►│── TRIAC1
    │GPIO1◄─│──── IR       │P1────►│── TRIAC2
    └───────┘              │P2◄────│── Button1
                           │P3◄────│── Button2
                           └───────┘
```

---

## Pasul 4: Programare

### Pregătire PlatformIO

1. Instalează [VS Code](https://code.visualstudio.com/)
2. Instalează extensia **PlatformIO IDE**
3. Deschide folderul proiectului

### Upload

**ATmega328:**
```bash
cd atmega328_dimmer
pio run -t upload
```

**ESP-01:**
```bash
cd esp01_dimmer
pio run -t upload
```

> ⚠️ Pentru ESP-01, ai nevoie de adaptor USB-Serial cu GPIO0 la GND la pornire!

---

## Pasul 5: Testare

### Test 1: ZCD
- Conectează ZCD-ul la 220V
- Deschide Serial Monitor (115200 baud)
- Ar trebui să vezi mesaje ZCD la fiecare 10ms

### Test 2: Butoane
- Apasă fiecare buton
- Serial Monitor: `Button X pressed`

### Test 3: Dimming
- Conectează un bec (ATENȚIE LA 220V!)
- Apasă scurt un buton → becul pornește la 50%
- Ține apăsat → nivelul crește/scade

### Test 4: IR
- Triple-click când becul e stins
- LED clipește 1x
- Apasă pe telecomandă
- LED clipește 2x = succes!

---

## Comenzi Rapide

| Acțiune | Cum faci |
|---------|----------|
| **ON/OFF** | Click scurt |
| **Ajustează nivel** | Ține apăsat |
| **Învață IR** | 3x click rapid (bec OFF) |
| **Sleep mode** | Automat când ambele OFF |

---

## ⚠️ SAFETY FIRST!

```
╔══════════════════════════════════════════════════════════╗
║  ⚡ TENSIUNEA DE 220V POATE FI MORTALĂ! ⚡               ║
║                                                          ║
║  • DECONECTEAZĂ alimentarea înainte de orice lucrare    ║
║  • Folosește ÎNTOTDEAUNA izolație galvanică             ║
║  • NU atinge componentele când sistemul e alimentat      ║
║  • Verifică conexiunile de 2-3 ori înainte de pornire   ║
╚══════════════════════════════════════════════════════════╝
```

---

## Probleme Frecvente

| Problemă | Soluție |
|----------|---------|
| Bec tremură | Verifică ZCD, ajustează timing |
| IR nu merge | Mărește IR_TOLERANCE la 30% |
| ESP-01 boot loop | Verifică GPIO0/GPIO2 pull-up |
| Nivel nu se salvează | Verifică EEPROM write |

---

## Structura Proiectului

```
esp_controller/
├── DOCS.md              ← Documentație completă
├── QUICKSTART.md        ← Acest fișier
│
├── atmega328_dimmer/
│   ├── atmega328_dimmer.ino    ← Cod principal
│   ├── platformio.ini          ← Config PlatformIO
│   └── README.md               ← Documentație specifică
│
└── esp01_dimmer/
    ├── esp01_dimmer.ino        ← Cod principal
    ├── platformio.ini          ← Config PlatformIO
    └── README.md               ← Documentație specifică
```

---

## Următorii Pași

După ce funcționează:

1. 📖 Citește **DOCS.md** pentru funcționalități avansate
2. 🔧 Personalizează parametrii în secțiunea CONFIGURARE
3. ➕ Adaugă funcționalități noi (vezi secțiunea "Extensii")
4. 📦 Montează în carcasă izolată

---

**Good luck! 🎉**
