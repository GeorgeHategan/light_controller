# ATmega328 AC Dimmer Controller

Controller pentru 2 becuri cu dimming pe rețea 220V/50Hz.

## Caracteristici

- **Sleep mode** când ambele becuri sunt stinse (consum minim)
- **Wake-up** la apăsare buton sau semnal IR
- **Dimming smooth** cu soft start/stop
- **Memorare nivel dimming** în EEPROM
- **Învățare cod IR** pentru fiecare bec
- **Control dual** - butoane touch TTP223 + telecomandă IR

## Hardware necesar

| Component | Pin ATmega328 | Funcție |
|-----------|---------------|---------|
| ZCD (Zero Crossing) | D2 (INT0) | Detectare trecere prin zero |
| TRIAC 1 | D3 | Control bec 1 |
| TRIAC 2 | D4 | Control bec 2 |
| Buton TTP223 #1 | D5 | Control bec 1 |
| Buton TTP223 #2 | D6 | Control bec 2 |
| Receptor IR | D7 | Recepție telecomandă |

## Schema de control

```
                    220V AC
                       │
                    ┌──┴──┐
              ┌─────┤ ZCD ├─────┐
              │     └─────┘     │
              │                 │
           ┌──┴──┐           ┌──┴──┐
           │TRIAC│           │TRIAC│
           │  1  │           │  2  │
           └──┬──┘           └──┬──┘
              │                 │
           ┌──┴──┐           ┌──┴──┐
           │BEC 1│           │BEC 2│
           └──┬──┘           └──┬──┘
              │                 │
              └────────┬────────┘
                       │
                      N
```

## Funcționare

### Nivel implicit
- Dimming inițial: **50%**

### Apăsare scurtă pe buton
1. Așteaptă **1 secundă** pentru a vedea dacă urmează alte clickuri
2. Dacă becul era **stins** → **soft start** la nivelul salvat
3. Dacă becul era **aprins** → **soft stop**

### Apăsare lungă pe buton (> 800ms)
1. Dacă becul era stins, îl aprinde la nivelul curent
2. Face **ramp-up** de la nivel curent → maxim (95%)
3. Apoi **ramp-down** de la maxim → minim (5%)
4. Repetă până se eliberează butonul
5. **Salvează** nivelul final în EEPROM

### Triple-click (3 apăsări în 3 secunde) - cu bec stins
1. **Confirmare vizuală**: aprinde soft la max și stinge
2. **Mod învățare IR**: așteaptă max 10 secunde un cod IR
3. Dacă primește cod → îl **memorează în EEPROM**
4. **Confirmare**: aprinde și stinge becul de **2 ori**

### Control telecomandă IR
- **Apăsare scurtă**: toggle on/off soft la nivelul salvat
- **Apăsare lungă**: ramp up/down pentru ajustare dimming (ca butonul fizic)

## Configurare (în cod)

```cpp
#define DIM_MIN             5       // Nivel minim dimming (%)
#define DIM_MAX             95      // Nivel maxim dimming (%)
#define DIM_DEFAULT         50      // Nivel implicit dimming (%)
#define LONG_PRESS_MS       800     // Timp pentru apăsare lungă
#define TRIPLE_CLICK_WINDOW 3000    // Fereastră pentru 3 clickuri
#define CLICK_WAIT_MS       1000    // Așteptare după click
#define IR_LEARN_TIMEOUT    10000   // Timeout învățare IR
```

## Structura EEPROM

| Adresă | Dimensiune | Conținut |
|--------|------------|----------|
| 0 | 1 byte | Magic byte (0xA5) |
| 1 | 1 byte | Nivel dimming bec 1 |
| 2 | 1 byte | Nivel dimming bec 2 |
| 3 | 1 byte | Lungime cod IR bec 1 |
| 4-71 | 68 bytes | Date cod IR bec 1 |
| 72 | 1 byte | Lungime cod IR bec 2 |
| 73-140 | 68 bytes | Date cod IR bec 2 |

## Diagrama de stări

```
    ┌─────────────────────────────────────────────────────┐
    │                     SLEEP MODE                       │
    │            (ambele becuri stinse)                   │
    └───────────────────────┬─────────────────────────────┘
                            │
            Wake-up: Buton sau IR
                            │
                            ▼
    ┌─────────────────────────────────────────────────────┐
    │                    ACTIVE MODE                       │
    ├─────────────────────────────────────────────────────┤
    │                                                      │
    │   ┌─────────┐    Click    ┌──────────────┐          │
    │   │BEC OFF  │────────────▶│  WAIT 1s     │          │
    │   └────┬────┘             └──────┬───────┘          │
    │        │                         │                   │
    │        │ Long press      1 click │ 3+ clicks        │
    │        │                         │                   │
    │        ▼                         ▼                   │
    │   ┌─────────┐             ┌──────────────┐          │
    │   │RAMP MODE│             │IR LEARN MODE │          │
    │   └─────────┘             └──────────────┘          │
    │                                                      │
    │   ┌─────────┐    Click    ┌──────────────┐          │
    │   │BEC ON   │────────────▶│ SOFT STOP    │          │
    │   └────┬────┘             └──────────────┘          │
    │        │                                             │
    │        │ Long press                                  │
    │        ▼                                             │
    │   ┌─────────┐                                        │
    │   │RAMP MODE│                                        │
    │   └─────────┘                                        │
    │                                                      │
    └─────────────────────────────────────────────────────┘
```

## Compilare și upload

### Cu Arduino IDE
1. Deschide `atmega328_dimmer.ino`
2. Selectează Board: "Arduino Uno" sau "Arduino Nano"
3. Selectează portul serial
4. Upload

### Cu PlatformIO
```bash
pio run -t upload
```

## Notă privind circuitul ZCD

Semnalul ZCD trebuie să fie un semnal dreptunghiular de 50Hz, în fază cu tensiunea rețelei. 
Frontul RISING al semnalului trebuie să coincidă cu trecerea tensiunii prin zero.

## Siguranță

⚠️ **ATENȚIE**: Acest proiect lucrează cu tensiunea rețelei (220V AC). 
Lucrările la tensiune înaltă sunt periculoase și pot provoca electrocutare sau incendii.
Asigură-te că ai cunoștințele și echipamentul necesar înainte de a lucra cu acest proiect.
