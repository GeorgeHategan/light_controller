# AC Dimmer Controller - ATmega328

Controller pentru 2 becuri cu dimming pe rețea 220V/50Hz.

## Funcționalități

- 2 canale dimming independent
- Butoane touch TTP223
- Control IR cu învățare coduri
- Soft start/stop
- Memorare nivel în EEPROM
- Sleep mode când becurile sunt stinse

## Conexiuni

```
D2 ← ZCD (semnal 50Hz, 50% duty cycle, CHANGE interrupt)
D3 → TRIAC1 (HIGH = ON)
D4 → TRIAC2 (HIGH = ON)
D5 ← Button 1
D6 ← Button 2
D7 ← IR Receiver
```

## Comenzi

| Acțiune | Buton | IR |
|---------|-------|-----|
| Toggle ON/OFF | Click scurt | Click scurt |
| Ajustare nivel | Ține apăsat | Ține apăsat (NEC repeat) |
| Învățare IR | 3x click (bec OFF) | - |

## Upload

```bash
cd atmega328_dimmer
pio run -t upload
```

## Notă IR

Telecomanda trebuie să trimită NEC repeat codes la apăsare lungă.
Pattern repeat code: ~9000µs + ~2250µs (detectat automat).
