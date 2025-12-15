# 📚 Documentație Completă - AC Dimmer Controller

## 📖 Cuprins

1. [Prezentare Generală](#prezentare-generală)
2. [Alegerea Platformei](#alegerea-platformei)
3. [Hardware Necesar](#hardware-necesar)
4. [Scheme Electrice](#scheme-electrice)
5. [Funcționalități](#funcționalități)
6. [Cum să Folosești](#cum-să-folosești)
7. [Configurare și Personalizare](#configurare-și-personalizare)
8. [Adăugare Funcționalități Noi](#adăugare-funcționalități-noi)
9. [Troubleshooting](#troubleshooting)
10. [Exemple de Extensii](#exemple-de-extensii)

---

## Prezentare Generală

Controller pentru **2 becuri cu dimming** pe rețea 220V/50Hz, cu următoarele caracteristici:

| Caracteristică | Descriere |
|----------------|-----------|
| **Canale** | 2 (extensibil) |
| **Dimming** | 5% - 95% |
| **Control** | Butoane touch TTP223 + Telecomandă IR |
| **Memorare** | EEPROM (persistent la restart) |
| **Economie energie** | Sleep mode când becurile sunt stinse |

---

## Alegerea Platformei

### ATmega328 (Arduino Uno/Nano)

**Avantaje:**
- ✅ Suficienți pini GPIO pentru toate funcțiile
- ✅ Power-down sleep foarte eficient (~0.1µA)
- ✅ EEPROM integrat (1KB)
- ✅ Cost redus
- ✅ Programare simplă

**Dezavantaje:**
- ❌ Nu are WiFi integrat
- ❌ Memorie limitată (32KB flash, 2KB RAM)

**Recomandare:** Ideal pentru aplicații fără conectivitate.

### ESP-01 (ESP8266)

**Avantaje:**
- ✅ WiFi integrat (dezactivat în acest proiect)
- ✅ Mai multă memorie (1MB flash, 80KB RAM)
- ✅ OTA updates posibile

**Dezavantaje:**
- ❌ Doar 4 pini GPIO disponibili
- ❌ Necesită I2C expander pentru funcționalitate completă
- ❌ Consum mai mare chiar și în light sleep (~0.5mA)
- ❌ Mai complex de programat

**Recomandare:** Folosește doar dacă ai nevoie de WiFi în viitor.

---

## Hardware Necesar

### Lista Componente

| Componentă | Cantitate | Notă |
|------------|-----------|------|
| ATmega328/ESP-01 | 1 | Microcontroller |
| Optocuplor 4N35 | 1 | Pentru ZCD |
| Optocuplor MOC3021 | 2 | Pentru triace |
| Triac BT136 | 2 | Control AC |
| TTP223 | 2 | Butoane touch |
| TSOP4838 | 1 | Receptor IR |
| Rezistențe 100kΩ | 2 | Pentru ZCD |
| Rezistențe 330Ω | 2 | Pentru MOC3021 |
| Rezistență 10kΩ | 1 | Pull-up ZCD |
| Condensator 100nF/400V | 2 | Snubber |
| Rezistență 100Ω | 2 | Snubber |
| PCF8574 | 1 | Doar pentru ESP-01 |

### Pinout ATmega328

```
                   ┌────────────────────┐
                   │     ATmega328      │
                   │                    │
    ZCD Input ────►│ D2 (INT0)          │
                   │                    │
   TRIAC 1 ◄──────│ D3                 │
   TRIAC 2 ◄──────│ D4                 │
                   │                    │
   Button 1 ──────►│ D5                 │
   Button 2 ──────►│ D6                 │
                   │                    │
    IR Recv ──────►│ D7 (PCINT23)       │
                   │                    │
                   └────────────────────┘
```

### Pinout ESP-01 cu PCF8574

```
    ESP-01                    PCF8574
   ┌──────────┐             ┌──────────┐
   │ GPIO0 ───┼── SDA ─────►│ SDA      │
   │ GPIO2 ───┼── SCL ─────►│ SCL      │
   │ GPIO3 ◄──┼── ZCD       │ P0 ──────┼──► TRIAC 1
   │ GPIO1 ◄──┼── IR        │ P1 ──────┼──► TRIAC 2
   └──────────┘             │ P2 ◄─────┼─── Button 1
                            │ P3 ◄─────┼─── Button 2
                            └──────────┘
```

---

## Scheme Electrice

### Circuit ZCD (Zero Crossing Detector)

```
     220V AC (L) ───┬──[R1 100kΩ]──┬──[R2 100kΩ]──┬─── GND
                    │              │              │
                    │         ┌────┴────┐         │
                    │         │  4N35   │         │
                    │    1 ●──┤ LED     ├──● 6    │
                    │         │   ↓     │  │      │
                    │    2 ●──┤         ├──● 5 ───┼─── VCC (5V/3.3V)
                    │         │         │  │      │    prin R 10kΩ
     220V AC (N) ───┘         │ Photo   │  │      │
                              │ Trans   │  │      │
                         GND──┤         ├──● 4 ───┴─── Output ZCD
                              └─────────┘              (către D2/GPIO3)
```

**Funcționare:**
- Când tensiunea AC trece prin zero, LED-ul din 4N35 se stinge
- Fototranzistorul blochează, ieșirea merge HIGH (pull-up)
- Detectăm frontul RISING pentru a marca zero crossing

### Circuit Triac (per canal)

```
                    ┌─────────────────────────────────────────────────┐
                    │                                                 │
    GPIO ──[R 330Ω]─┤                                                 │
                    │    MOC3021                                      │
                    │   ┌────────┐        ┌────────┐                  │
                ────┤ 1─┤ LED    ├─2 ────┤        │                  │
                    │   │   ↓    │        │ BT136  │                  │
                    │ 6─┤ TRIAC  ├─4 ────┤  MT1   ├──── LOAD (Bec)   │
                    │   │        │    ┌───┤  MT2   ├──── 220V AC (L)  │
                    │   └────────┘    │   │  GATE  │                  │
                    │                 │   └────────┘                  │
                    │                 │        │                      │
                    │            [100Ω]   [100nF/400V]                │
                    │                 │        │                      │
                    │                 └────┬───┘                      │
                    │                      │                          │
                    └──────────────────────┴──────────────────────────┘
                                           │
                                      220V AC (N)
```

**Snubber (R 100Ω + C 100nF):**
- Protejează triacul de spike-uri
- **OBLIGATORIU** pentru sarcini inductive
- Recomandat și pentru becuri

---

## Funcționalități

### Tabel Comenzi

| Acțiune | Buton Fizic | Telecomandă IR |
|---------|-------------|----------------|
| Toggle ON/OFF | Click scurt | Click scurt |
| Ajustare nivel | Apăsare lungă (>0.8s) | Apăsare lungă |
| Învățare IR | Triple-click (bec stins) | - |

### Diagrama Stări

```
                         ┌──────────────┐
                         │     IDLE     │
                         │  (bec stins) │
                         └──────┬───────┘
                                │
            ┌───────────────────┼───────────────────┐
            │                   │                   │
            ▼                   ▼                   ▼
    ┌───────────────┐   ┌───────────────┐   ┌───────────────┐
    │  Click scurt  │   │ Apăsare lungă │   │ Triple-click  │
    └───────┬───────┘   └───────┬───────┘   └───────┬───────┘
            │                   │                   │
            ▼                   ▼                   ▼
    ┌───────────────┐   ┌───────────────┐   ┌───────────────┐
    │ Așteaptă 1s   │   │ Ramp up/down  │   │  Confirmare   │
    │ alte clickuri │   │   continuu    │   │  (1 clipire)  │
    └───────┬───────┘   └───────┬───────┘   └───────┬───────┘
            │                   │                   │
            ▼                   ▼                   ▼
    ┌───────────────┐   ┌───────────────┐   ┌───────────────┐
    │  Soft Start   │   │ Salvare nivel │   │ Așteaptă IR   │
    │  / Stop       │   │  în EEPROM    │   │   (10 sec)    │
    └───────────────┘   └───────────────┘   └───────┬───────┘
                                                    │
                                    ┌───────────────┼───────────────┐
                                    │               │               │
                                    ▼               ▼               ▼
                            ┌───────────────┐   ┌───────────────┐
                            │   Cod primit  │   │    Timeout    │
                            │   Salvare     │   │    (ieșire)   │
                            └───────┬───────┘   └───────────────┘
                                    │
                                    ▼
                            ┌───────────────┐
                            │  Confirmare   │
                            │  (2 clipiri)  │
                            └───────────────┘
```

---

## Cum să Folosești

### Pornire/Oprire Bec

1. **Apasă scurt** butonul corespunzător
2. Așteaptă 1 secundă (sistemul verifică dacă urmează alte clickuri)
3. Becul pornește/oprește cu efect soft

### Ajustare Nivel Dimming

1. **Ține apăsat** butonul corespunzător (>0.8 secunde)
2. Becul va crește gradual luminozitatea până la maxim
3. Apoi va scădea gradual până la minim
4. Repetă până găsești nivelul dorit
5. **Eliberează butonul** - nivelul se salvează automat

### Învățare Cod IR

1. Asigură-te că becul este **STINS**
2. **Apasă rapid de 3 ori** butonul corespunzător (în 3 secunde)
3. Becul va **clipi o dată** pentru confirmare
4. Ai **10 secunde** să apeși butonul de pe telecomandă
5. După recepționare, becul **clipește de 2 ori** = succes!

### Control prin Telecomandă

- **Apăsare scurtă**: Toggle on/off
- **Apăsare lungă**: Ajustare nivel (ca butonul fizic)

---

## Configurare și Personalizare

### Parametri Modificabili

Deschide fișierul `.ino` și modifică secțiunea CONFIGURARE:

```cpp
// TIMING
#define LONG_PRESS_MS       800     // Timp pentru apăsare lungă
#define TRIPLE_CLICK_WINDOW 3000    // Fereastră pentru 3 clickuri
#define CLICK_WAIT_MS       1000    // Așteptare după click
#define RAMP_DELAY_MS       30      // Viteză ramp (mai mic = mai rapid)
#define SOFT_STEP_DELAY_MS  20      // Viteză soft start/stop

// DIMMING
#define DIM_MIN             5       // Nivel minim (%)
#define DIM_MAX             95      // Nivel maxim (%)
#define DIM_DEFAULT         50      // Nivel implicit (%)
```

### Schimbare Pini (ATmega328)

```cpp
#define PIN_ZCD         2    // NU MODIFICA (INT0)!
#define PIN_TRIAC1      3    // Poate fi orice pin digital
#define PIN_TRIAC2      4    
#define PIN_BUTTON1     5    
#define PIN_BUTTON2     6    
#define PIN_IR_RECV     7    // Recomandat să rămână pe PCINT
```

---

## Adăugare Funcționalități Noi

### 1. Adăugare Canal 3

```cpp
// 1. Adaugă pin
#define PIN_TRIAC3      8
#define PIN_BUTTON3     9

// 2. Mărește array-urile
BulbState bulb[3];        // era [2]
ButtonState button[3];    // era [2]

// 3. Adaugă în EEPROM
#define EEPROM_DIM3         141
#define EEPROM_IR3_LEN      142
#define EEPROM_IR3_DATA     143

// 4. Modifică loop-urile (ex: în readButtons)
#define NUM_CHANNELS 3
for (int i = 0; i < NUM_CHANNELS; i++) { ... }

// 5. Inițializează în setup()
pinMode(PIN_TRIAC3, OUTPUT);
pinMode(PIN_BUTTON3, INPUT);
```

### 2. Adăugare Senzor de Lumină

```cpp
// Definire pin
#define PIN_LDR         A0

// Funcție citire
uint16_t readAmbientLight() {
    return analogRead(PIN_LDR);
}

// Ajustare automată în loop()
void autoAdjustBrightness() {
    uint16_t ambient = readAmbientLight();
    // Mai întuneric afară → mai multă lumină de la becuri
    uint8_t autoLevel = map(ambient, 0, 1023, DIM_MAX, DIM_MIN);
    // Aplică la becuri...
}
```

### 3. Adăugare Mod Noapte

```cpp
// Definiții
#define NIGHT_MODE_LEVEL    20      // Nivel redus pentru noapte
bool nightModeActive = false;

// Activare prin combinație de butoane
// (ex: ambele butoane apăsate simultan)
void checkNightModeActivation() {
    if (button[0].currentState && button[1].currentState) {
        nightModeActive = !nightModeActive;
        if (nightModeActive) {
            // Reduce toate becurile la NIGHT_MODE_LEVEL
            for (int i = 0; i < 2; i++) {
                if (bulb[i].isOn) {
                    bulb[i].currentLevel = NIGHT_MODE_LEVEL;
                }
            }
        }
    }
}
```

### 4. Adăugare Timer Auto-Off

```cpp
// Definiții
#define AUTO_OFF_MS     3600000     // 1 oră
unsigned long bulbOnTime[2] = {0, 0};

// În softStart(), salvează timestamp
bulbOnTime[bulbIndex] = millis();

// În loop(), verifică timeout
void checkAutoOff() {
    for (int i = 0; i < 2; i++) {
        if (bulb[i].isOn) {
            if (millis() - bulbOnTime[i] > AUTO_OFF_MS) {
                softStop(i);
            }
        }
    }
}
```

---

## Troubleshooting

### Dimming tremură/flicker

**Cauze posibile:**
1. Sincronizare ZCD greșită
2. Timing-uri incorecte
3. Sarcină prea mică pentru triac

**Soluții:**
- Verifică semnalul ZCD cu osciloscopul
- Ajustează `HALF_CYCLE_US` și `TRIAC_PULSE_US`
- Adaugă rezistență de sarcină minimă

### IR nu funcționează

**Verificări:**
1. Testează receptorul IR cu un LED și alt sketch
2. Verifică polaritatea conexiunilor
3. Asigură-te că nu există lumină directă pe receptor

**Ajustări:**
- Mărește `IR_TOLERANCE` (ex: 30%)
- Mărește `IR_MAX_LEN` pentru protocoale mai lungi

### Bec nu pornește complet

**Cauze:**
- `DIM_MIN` prea mic pentru tipul de bec
- Triac nu se aprinde corect

**Soluții:**
- Mărește `DIM_MIN` (ex: 10%)
- Verifică circuitul MOC3021

### ESP-01 nu pornește

**Cauze:**
- GPIO0 sau GPIO2 sunt LOW la boot
- Alimentare insuficientă

**Soluții:**
- Asigură-te că GPIO0/GPIO2 au pull-up
- Folosește sursă care poate furniza 300mA+ la 3.3V

---

## Exemple de Extensii

### Scenă "Film"
```cpp
void activateMovieScene() {
    // Bec 1 la 30%, Bec 2 stins
    softRampTo(0, 30);
    softStop(1);
}
```

### Fade Lent
```cpp
void slowFade(uint8_t bulbIndex, uint8_t target, uint16_t durationSec) {
    int steps = abs(bulb[bulbIndex].currentLevel - target);
    int stepDelay = (durationSec * 1000) / steps;
    
    while (bulb[bulbIndex].currentLevel != target) {
        if (bulb[bulbIndex].currentLevel < target) {
            bulb[bulbIndex].currentLevel++;
        } else {
            bulb[bulbIndex].currentLevel--;
        }
        delay(stepDelay);
        updateDimming();
    }
}
```

### All On / All Off
```cpp
void allOn() {
    for (int i = 0; i < 2; i++) {
        if (!bulb[i].isOn) softStart(i);
    }
}

void allOff() {
    for (int i = 0; i < 2; i++) {
        if (bulb[i].isOn) softStop(i);
    }
}
```

---

## ⚠️ Avertismente de Siguranță

1. **PERICOL DE ELECTROCUTARE** - Lucrează DOAR cu alimentarea deconectată
2. Folosește întotdeauna **izolație galvanică** (optocuploare)
3. Montează **snubber** pe triace pentru sarcini inductive
4. Folosește **carcasă izolată** pentru produs final
5. **NU** lăsa fire expuse sau conexiuni accesibile
6. Respectă normele locale pentru instalații electrice

---

## Licență

MIT License - Liber pentru utilizare personală și comercială.

## Contact

ESP Controller Project - 2025
