# ESP-01 AC Dimmer Controller

Controller pentru 2 becuri cu dimming pe rețea 220V/50Hz.
**WiFi dezactivat** pentru consum redus.

## Limitări ESP-01

ESP-01 are doar 4 GPIO disponibili:
- GPIO0 (boot mode)
- GPIO2 (boot mode)  
- GPIO1 (TX)
- GPIO3 (RX)

### Soluție: PCF8574 I2C Expander

Pentru funcționalitate completă (2 becuri + 2 butoane), se folosește un expansor I2C PCF8574.

```
ESP-01          PCF8574         Componente
─────────────────────────────────────────────
GPIO0 (SDA) ───► SDA
GPIO2 (SCL) ───► SCL
GPIO3 (RX)  ────────────────► ZCD Input
GPIO1 (TX)  ────────────────► IR Receiver

                P0 ──────────► Triac 1
                P1 ──────────► Triac 2
                P2 ◄────────── Button 1
                P3 ◄────────── Button 2
                P4-P7 ────────► (disponibili)
```

## Hardware necesar

| Component | Conexiune | Funcție |
|-----------|-----------|---------|
| PCF8574 | I2C (addr 0x20) | Expansor I/O |
| ZCD | GPIO3 (RX) | Zero Crossing Detection |
| IR Receiver | GPIO1 (TX) | Recepție telecomandă |
| TRIAC 1 | PCF8574 P0 | Control bec 1 |
| TRIAC 2 | PCF8574 P1 | Control bec 2 |
| TTP223 #1 | PCF8574 P2 | Buton bec 1 |
| TTP223 #2 | PCF8574 P3 | Buton bec 2 |

## Funcționare

Identică cu versiunea ATmega328:

### Nivel implicit: **50%**

### Apăsare scurtă
- Așteaptă 1s pentru alte clickuri
- Toggle on/off cu soft start/stop

### Apăsare lungă (>800ms)
- Ramp up/down continuu
- Salvare nivel la eliberare

### Triple-click (3x în 3s, bec stins)
- Mod învățare cod IR
- Confirmare vizuală

### Telecomandă IR
- Click scurt: toggle on/off
- Click lung: ramp dimming

## Dezactivare WiFi

WiFi-ul este dezactivat în `setup()`:

```cpp
void disableWiFi() {
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);
}
```

Acest lucru reduce semnificativ consumul de energie.

## Light Sleep

Când ambele becuri sunt stinse, ESP-01 intră în Light Sleep cu wake-up pe:
- Semnal ZCD
- Semnal IR
- Apăsare buton (prin PCF8574 interrupt - opțional)

## Configurare I2C

Modifică adresa PCF8574 dacă este diferită:

```cpp
#define PCF8574_ADDR    0x20    // Adresa I2C
```

Adrese posibile: 0x20-0x27 (A0-A2 la GND sau VCC)

## Schema PCF8574

```
         ┌────────────────┐
   A0 ───┤1            16├─── VCC (3.3V)
   A1 ───┤2            15├─── SDA ←── GPIO0 ESP
   A2 ───┤3            14├─── SCL ←── GPIO2 ESP
   P0 ───┤4  PCF8574   13├─── INT (opțional)
   P1 ───┤5            12├─── P7
   P2 ───┤6            11├─── P6
   P3 ───┤7            10├─── P5
  GND ───┤8             9├─── P4
         └────────────────┘

A0-A2: La GND pentru adresa 0x20
P0: Triac 1 (OUTPUT)
P1: Triac 2 (OUTPUT)
P2: Button 1 (INPUT cu pull-up)
P3: Button 2 (INPUT cu pull-up)
P4-P7: Disponibili pentru extinderi
```

## Compilare

### Cu Arduino IDE

1. Adaugă URL pentru ESP8266:
   `http://arduino.esp8266.com/stable/package_esp8266com_index.json`

2. Instalează "ESP8266 Boards"

3. Selectează Board: "Generic ESP8266 Module"

4. Setări:
   - Flash Mode: DOUT
   - Flash Size: 1MB (FS: 64KB)
   - Crystal Frequency: 26 MHz
   - Upload Speed: 115200

### Cu PlatformIO

```bash
pio run -e esp01 -t upload
```

## platformio.ini

```ini
[env:esp01]
platform = espressif8266
board = esp01
framework = arduino
board_build.f_cpu = 80000000L
upload_speed = 115200
monitor_speed = 115200

build_flags = 
    -DDEBUG=0
```

## Consum de energie

| Mod | Consum aproximativ |
|-----|-------------------|
| Activ (WiFi OFF) | ~15-20 mA |
| Light Sleep | ~0.5-1 mA |
| Deep Sleep | ~20 µA (dar pierde stare) |

## Troubleshooting

### ESP-01 nu pornește
- Verifică GPIO0 și GPIO2 să fie HIGH la boot
- Verifică alimentarea (minim 200mA disponibil)

### I2C nu funcționează
- Verifică rezistențe pull-up 4.7kΩ pe SDA/SCL
- Verifică adresa PCF8574

### Dimming tremură
- Verifică semnalul ZCD să fie stabil
- Verifică sincronizarea cu rețeaua

## Atenție

⚠️ **PERICOL**: Acest proiect lucrează cu tensiune de rețea (220V AC).
Ia toate măsurile de siguranță necesare!
