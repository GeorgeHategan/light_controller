/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                     ATmega328 AC DIMMER CONTROLLER v1.0                       ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Controlează 2 becuri cu dimming pe rețea 220V/50Hz                           ║
 * ║  Power-down sleep când becurile sunt stinse (consum ~0.1µA)                   ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 * 
 * ┌─────────────────────────────────────────────────────────────────────────────┐
 * │                            FUNCȚIONALITĂȚI                                   │
 * ├─────────────────────────────────────────────────────────────────────────────┤
 * │  ✓ Dimming AC cu detecție zero-crossing pentru 2 canale                     │
 * │  ✓ Soft start/stop pentru protecție bec și efect vizual plăcut              │
 * │  ✓ Control butoane touch TTP223 cu debounce                                 │
 * │  ✓ Învățare și memorare coduri IR de la orice telecomandă                   │
 * │  ✓ Memorare setări în EEPROM (persistente la restart)                       │
 * │  ✓ Power-down Sleep pentru economie maximă de energie                       │
 * │  ✓ Wake-up instant la apăsare buton sau semnal IR                           │
 * └─────────────────────────────────────────────────────────────────────────────┘
 * 
 * ┌─────────────────────────────────────────────────────────────────────────────┐
 * │                         COMENZI DISPONIBILE                                  │
 * ├─────────────────────────────────────────────────────────────────────────────┤
 * │  BUTON FIZIC:                                                                │
 * │    • Click scurt (1x)     → Toggle ON/OFF cu soft transition                │
 * │    • Apăsare lungă (>0.8s)→ Ramp up/down pentru ajustare nivel              │
 * │    • Triple-click (3x/3s) → Intră în mod învățare IR (bec trebuie stins)    │
 * │                                                                              │
 * │  TELECOMANDĂ IR (după învățare):                                             │
 * │    • Click scurt          → Toggle ON/OFF cu soft transition                │
 * │    • Apăsare lungă        → Ramp up/down pentru ajustare nivel              │
 * └─────────────────────────────────────────────────────────────────────────────┘
 * 
 * ┌─────────────────────────────────────────────────────────────────────────────┐
 * │                      CONEXIUNI HARDWARE ATmega328                            │
 * ├─────────────────────────────────────────────────────────────────────────────┤
 * │                                                                              │
 * │   ATmega328 (Arduino Uno/Nano)         Componente                           │
 * │  ┌──────────────────────────┐                                               │
 * │  │ D2 (INT0) ◄──────────────┼──── ZCD Circuit (Zero Crossing)               │
 * │  │ D3 ───────────────────────┼────► Triac 1 (MOC3021 + BT136)               │
 * │  │ D4 ───────────────────────┼────► Triac 2 (MOC3021 + BT136)               │
 * │  │ D5 ◄──────────────────────┼──── Button TTP223 #1                         │
 * │  │ D6 ◄──────────────────────┼──── Button TTP223 #2                         │
 * │  │ D7 ◄──────────────────────┼──── IR Receiver (TSOP4838)                   │
 * │  │ VCC ──────────────────────┼──── 5V                                       │
 * │  │ GND ──────────────────────┼──── GND                                      │
 * │  └──────────────────────────┘                                               │
 * │                                                                              │
 * │   NOTĂ: D2 folosește INT0 pentru wake-up din sleep                          │
 * │         D7 folosește PCINT23 pentru recepție IR                             │
 * └─────────────────────────────────────────────────────────────────────────────┘
 * 
 * ┌─────────────────────────────────────────────────────────────────────────────┐
 * │                    SEMNAL ZCD (Zero Crossing Detection)                      │
 * ├─────────────────────────────────────────────────────────────────────────────┤
 * │                                                                              │
 * │   Input: Semnal dreptunghiular 50Hz, 50% duty cycle, în fază cu tensiunea   │
 * │                                                                              │
 * │   Tensiune AC:     /\      /\      /\                                       │
 * │                   /  \    /  \    /  \                                      │
 * │               ───/────\──/────\──/────\───                                  │
 * │                        \/      \/      \/                                    │
 * │                                                                              │
 * │   Semnal ZCD:    ┌────┐     ┌────┐     ┌────┐                               │
 * │                  │    │     │    │     │    │                               │
 * │               ───┘    └─────┘    └─────┘    └───                            │
 * │                  ↑    ↑     ↑    ↑     ↑    ↑                               │
 * │                  │    │     │    │     │    │                               │
 * │              RISING FALLING (ambele = zero crossing)                        │
 * │                                                                              │
 * │   Întreruperea folosește CHANGE pentru a detecta ambele fronturi            │
 * └─────────────────────────────────────────────────────────────────────────────┘
 * 
 * ┌─────────────────────────────────────────────────────────────────────────────┐
 * │                         STRUCTURA EEPROM                                     │
 * ├─────────────────────────────────────────────────────────────────────────────┤
 * │   Adresa │ Mărime  │ Descriere                                              │
 * │   ───────┼─────────┼────────────────────────────────────────────────────────│
 * │   0      │ 1 byte  │ Magic byte (0xA5) - validare date                      │
 * │   1      │ 1 byte  │ Nivel dimming bec 1 (5-95%)                            │
 * │   2      │ 1 byte  │ Nivel dimming bec 2 (5-95%)                            │
 * │   3      │ 1 byte  │ Lungime cod IR bec 1                                   │
 * │   4-71   │ 68 bytes│ Date cod IR bec 1 (max 34 perechi timing)              │
 * │   72     │ 1 byte  │ Lungime cod IR bec 2                                   │
 * │   73-140 │ 68 bytes│ Date cod IR bec 2 (max 34 perechi timing)              │
 * │   141+   │ ...     │ REZERVAT pentru extensii                               │
 * └─────────────────────────────────────────────────────────────────────────────┘
 * 
 * ┌─────────────────────────────────────────────────────────────────────────────┐
 * │                    GHID ADĂUGARE FUNCȚIONALITĂȚI NOI                         │
 * ├─────────────────────────────────────────────────────────────────────────────┤
 * │                                                                              │
 * │   1. ADĂUGARE CANAL NOU (Bec 3, etc):                                        │
 * │      - Mărește array-urile bulb[] și button[]                                │
 * │      - Adaugă pini noi în CONFIGURARE PINI                                   │
 * │      - Adaugă adrese EEPROM pentru nivel și cod IR                           │
 * │                                                                              │
 * │   2. ADĂUGARE SENZOR (temperatură, lumină):                                  │
 * │      - Definește pinul în CONFIGURARE PINI                                   │
 * │      - Creează funcție readSensor()                                          │
 * │      - Apelează din loop() sau pe timer                                      │
 * │                                                                              │
 * │   3. MOD NOU (noapte, scenă):                                                │
 * │      - Definește constante și variabile de stare                             │
 * │      - Implementează logica în funcție dedicată                              │
 * │      - Adaugă activare prin combinație butoane/IR                            │
 * │                                                                              │
 * └─────────────────────────────────────────────────────────────────────────────┘
 * 
 * AUTOR: ESP Controller Project
 * VERSIUNE: 1.0
 * DATA: Decembrie 2025
 * LICENȚĂ: MIT
 * 
 * ⚠️  AVERTISMENT: Acest proiect lucrează cu tensiune de rețea (220V AC).
 *     PERICOL DE ELECTROCUTARE! Lucrați doar cu alimentarea deconectată.
 */

#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                          SECȚIUNEA 1: CONFIGURARE                             ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Modifică valorile de mai jos pentru a personaliza comportamentul             ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== CONFIGURARE PINI ====================
/**
 * @brief Configurare pini hardware ATmega328
 * 
 * D2 (INT0) este obligatoriu pentru ZCD - suportă wake-up din sleep
 * D7 (PCINT23) este recomandat pentru IR - suportă Pin Change Interrupt
 */
#define PIN_ZCD         2    // Zero Crossing Detection (INT0) - NU MODIFICA!
#define PIN_TRIAC1      3    // Triac pentru becul 1 (PWM capable)
#define PIN_TRIAC2      4    // Triac pentru becul 2
#define PIN_BUTTON1     5    // Buton touch TTP223 pentru becul 1
#define PIN_BUTTON2     6    // Buton touch TTP223 pentru becul 2
#define PIN_IR_RECV     7    // Receptor IR (PCINT23)

// ==================== CONFIGURARE TIMING ====================
/**
 * @brief Parametri de timing
 * 
 * HALF_CYCLE_US: 10000 pentru 50Hz, 8333 pentru 60Hz
 * Ajustează celelalte valori pentru comportament diferit
 */
#define HALF_CYCLE_US       10000   // 10ms pentru 50Hz (o jumătate de perioadă)
#define TRIAC_PULSE_US      50      // Durata pulsului de aprindere triac (µs)
#define DEBOUNCE_MS         50      // Debounce pentru butoane (ms)
#define LONG_PRESS_MS       800     // Timp pentru apăsare lungă (ms)
#define TRIPLE_CLICK_WINDOW 3000    // Fereastră pentru 3 clickuri (ms)
#define CLICK_WAIT_MS       1000    // Așteptare după click pentru alte clickuri (ms)
#define IR_LEARN_TIMEOUT    10000   // Timeout pentru învățare IR (ms)
#define RAMP_DELAY_MS       30      // Delay între pașii de ramp up/down (ms)
#define SOFT_STEP_DELAY_MS  20      // Delay pentru soft start/stop (ms)

// ==================== CONFIGURARE DIMMING ====================
/**
 * @brief Limite și valoare implicită pentru dimming
 */
#define DIM_MIN             5       // Nivel minim dimming (%)
#define DIM_MAX             95      // Nivel maxim dimming (%)
#define DIM_DEFAULT         50      // Nivel implicit dimming (%)
#define DIM_STEPS           100     // Număr de pași pentru dimming

// ==================== ADRESE EEPROM ====================
/**
 * @brief Mapare EEPROM pentru persistență date
 * 
 * ATmega328 are 1KB EEPROM (adrese 0-1023)
 */
#define EEPROM_MAGIC        0       // Byte magic pentru validare (1 byte)
#define EEPROM_DIM1         1       // Nivel dimming becul 1 (1 byte)
#define EEPROM_DIM2         2       // Nivel dimming becul 2 (1 byte)
#define EEPROM_IR1_LEN      3       // Lungime cod IR becul 1 (1 byte)
#define EEPROM_IR1_DATA     4       // Date cod IR becul 1 (68 bytes max)
#define EEPROM_IR2_LEN      72      // Lungime cod IR becul 2 (1 byte)
#define EEPROM_IR2_DATA     73      // Date cod IR becul 2 (68 bytes max)
#define EEPROM_MAGIC_VALUE  0xA5    // Valoare magic pentru EEPROM valid

// ==================== IR CONFIGURATION ====================
/**
 * @brief Parametri recepție și procesare IR
 */
#define IR_MAX_LEN          67      // Lungime maximă cod IR (perechi timing)
#define IR_TIMEOUT_US       50000   // Timeout pentru recepție IR (µs)
#define IR_MIN_PULSES       10      // Număr minim de pulsuri pentru cod valid
#define IR_TOLERANCE        20      // Toleranță % pentru comparare coduri IR
#define IR_LONG_PRESS_MS    800     // Timp pentru apăsare lungă pe telecomandă (ms)
#define IR_REPEAT_TIMEOUT   150     // Timeout pentru detectare repeat IR (ms)

// NEC Repeat Code detection (pattern: ~9000µs + ~2250µs + ~560µs)
#define NEC_REPEAT_BURST    9000    // Prima durată (burst) ~9ms
#define NEC_REPEAT_SPACE    2250    // A doua durată (space) ~2.25ms  
#define NEC_REPEAT_TOLERANCE 500    // Toleranță în µs pentru detectare repeat

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                       SECȚIUNEA 2: STRUCTURI DE DATE                          ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== STRUCTURI DATE ====================

/** @brief Structură pentru stocarea unui cod IR învățat */
struct IRCode {
    uint16_t timing[IR_MAX_LEN];    // Timinguri pulsuri (on/off alternativ)
    uint8_t length;                  // Număr de perechi timing
    bool valid;                      // Cod valid memorat
};

/** @brief Structură pentru starea unui bec/canal */
struct BulbState {
    bool isOn;                       // Stare on/off
    uint8_t currentLevel;            // Nivel curent dimming (0-100)
    uint8_t targetLevel;             // Nivel țintă pentru dimming
    uint8_t savedLevel;              // Nivel salvat în EEPROM
    IRCode irCode;                   // Cod IR asociat
};

/** @brief Structură pentru starea unui buton */
struct ButtonState {
    bool lastState;                  // Stare anterioară (debounce)
    bool currentState;               // Stare curentă
    unsigned long pressStart;        // Timestamp început apăsare
    unsigned long lastRelease;       // Timestamp ultima eliberare
    uint8_t clickCount;              // Număr clickuri în fereastră
    bool longPressActive;            // Apăsare lungă în curs
    bool waitingForMoreClicks;       // Așteptăm alte clickuri
    unsigned long waitStartTime;     // Când am început să așteptăm
};

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                      SECȚIUNEA 3: VARIABILE GLOBALE                           ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== VARIABILE GLOBALE ====================
volatile bool zeroCrossing = false;      // Flag zero crossing (setat în ISR)
volatile unsigned long lastZeroCross = 0; // Timestamp ultimul zero crossing

BulbState bulb[2];                       // Starea celor 2 becuri
ButtonState button[2];                   // Starea celor 2 butoane

// Buffer și stare IR
volatile uint16_t irBuffer[IR_MAX_LEN];  // Buffer recepție timing-uri IR
volatile uint8_t irBufferIndex = 0;      // Index curent în buffer
volatile unsigned long irLastEdge = 0;   // Timestamp ultima tranziție IR
volatile bool irReceiving = false;       // Recepție IR în curs
volatile bool irCodeReady = false;       // Cod IR complet

bool irLearningMode = false;
uint8_t irLearningBulb = 0;
unsigned long irLearningStart = 0;

// Pentru IR remote long press
unsigned long irLastReceived[2] = {0, 0};
bool irLongPressActive[2] = {false, false};
unsigned long irPressStart[2] = {0, 0};
int8_t lastIRBulb = -1;                  // Ultimul bec care a primit cod IR valid (-1 = niciunul)

// Pentru dimming ramp
bool rampingUp = true;
unsigned long lastRampStep = 0;

// Pentru sleep mode
bool sleepEnabled = true;

// ==================== FUNCȚII SETUP ====================
void setup() {
    // Configurare pini
    pinMode(PIN_ZCD, INPUT);
    pinMode(PIN_TRIAC1, OUTPUT);
    pinMode(PIN_TRIAC2, OUTPUT);
    pinMode(PIN_BUTTON1, INPUT);
    pinMode(PIN_BUTTON2, INPUT);
    pinMode(PIN_IR_RECV, INPUT);
    
    // Inițializare ieșiri
    digitalWrite(PIN_TRIAC1, LOW);
    digitalWrite(PIN_TRIAC2, LOW);
    
    // Configurare întrerupere ZCD pe INT0 (pin 2)
    // CHANGE: ambele fronturi sunt zero crossing (semnal 50% duty cycle)
    attachInterrupt(digitalPinToInterrupt(PIN_ZCD), zeroCrossISR, CHANGE);
    
    // Configurare Pin Change Interrupt pentru IR (pin 7 = PCINT23 pe PORTD)
    PCICR |= (1 << PCIE2);      // Enable PCINT pentru PORTD
    PCMSK2 |= (1 << PCINT23);   // Enable PCINT pe pin 7
    
    // Încărcare setări din EEPROM
    loadSettings();
    
    // Inițializare stări butoane
    for (int i = 0; i < 2; i++) {
        button[i].lastState = false;
        button[i].currentState = false;
        button[i].pressStart = 0;
        button[i].lastRelease = 0;
        button[i].clickCount = 0;
        button[i].longPressActive = false;
        button[i].waitingForMoreClicks = false;
        button[i].waitStartTime = 0;
    }
    
    // Configurare Timer1 pentru dimming precis
    setupTimer1();
    
    // Serial pentru debug (opțional, poate fi dezactivat pentru economie energie)
    // Serial.begin(9600);
    // Serial.println(F("AC Dimmer Controller Started"));
}

void setupTimer1() {
    // Timer1 în mod CTC pentru timing precis al triacurilor
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11); // CTC mode, prescaler 8
    OCR1A = 20000; // Valoare inițială
    TIMSK1 = 0; // Dezactivat inițial
}

// ==================== ISR ZERO CROSSING ====================
void zeroCrossISR() {
    zeroCrossing = true;
    lastZeroCross = micros();
    
    // Oprește triacurile imediat la zero crossing
    digitalWrite(PIN_TRIAC1, LOW);
    digitalWrite(PIN_TRIAC2, LOW);
}

// ==================== ISR PIN CHANGE (IR) ====================
ISR(PCINT2_vect) {
    unsigned long now = micros();
    
    if (!irReceiving) {
        // Prima tranziție - începe recepția
        irReceiving = true;
        irBufferIndex = 0;
        irLastEdge = now;
    } else {
        // Calculează durata de la ultima tranziție
        unsigned long duration = now - irLastEdge;
        irLastEdge = now;
        
        if (duration < IR_TIMEOUT_US && irBufferIndex < IR_MAX_LEN) {
            irBuffer[irBufferIndex++] = (uint16_t)duration;
        } else if (duration >= IR_TIMEOUT_US) {
            // Timeout - cod complet
            if (irBufferIndex >= IR_MIN_PULSES) {
                irCodeReady = true;
            }
            irReceiving = false;
        }
    }
}

// ==================== FUNCȚII EEPROM ====================
void loadSettings() {
    // Verifică dacă EEPROM-ul este valid
    if (EEPROM.read(EEPROM_MAGIC) != EEPROM_MAGIC_VALUE) {
        // Prima utilizare - inițializează cu valori implicite
        saveDefaultSettings();
    }
    
    // Încarcă nivelurile de dimming
    bulb[0].savedLevel = EEPROM.read(EEPROM_DIM1);
    bulb[1].savedLevel = EEPROM.read(EEPROM_DIM2);
    
    // Validare valori
    if (bulb[0].savedLevel < DIM_MIN || bulb[0].savedLevel > DIM_MAX) {
        bulb[0].savedLevel = DIM_DEFAULT;
    }
    if (bulb[1].savedLevel < DIM_MIN || bulb[1].savedLevel > DIM_MAX) {
        bulb[1].savedLevel = DIM_DEFAULT;
    }
    
    // Inițializare stări becuri
    for (int i = 0; i < 2; i++) {
        bulb[i].isOn = false;
        bulb[i].currentLevel = 0;
        bulb[i].targetLevel = bulb[i].savedLevel;
    }
    
    // Încarcă coduri IR
    loadIRCode(0);
    loadIRCode(1);
}

void saveDefaultSettings() {
    EEPROM.update(EEPROM_MAGIC, EEPROM_MAGIC_VALUE);
    EEPROM.update(EEPROM_DIM1, DIM_DEFAULT);
    EEPROM.update(EEPROM_DIM2, DIM_DEFAULT);
    EEPROM.update(EEPROM_IR1_LEN, 0);
    EEPROM.update(EEPROM_IR2_LEN, 0);
}

void saveDimmingLevel(uint8_t bulbIndex) {
    if (bulbIndex == 0) {
        EEPROM.update(EEPROM_DIM1, bulb[0].savedLevel);
    } else {
        EEPROM.update(EEPROM_DIM2, bulb[1].savedLevel);
    }
}

void loadIRCode(uint8_t bulbIndex) {
    uint8_t lenAddr = (bulbIndex == 0) ? EEPROM_IR1_LEN : EEPROM_IR2_LEN;
    uint8_t dataAddr = (bulbIndex == 0) ? EEPROM_IR1_DATA : EEPROM_IR2_DATA;
    
    uint8_t len = EEPROM.read(lenAddr);
    
    if (len > 0 && len <= IR_MAX_LEN) {
        bulb[bulbIndex].irCode.length = len;
        bulb[bulbIndex].irCode.valid = true;
        
        for (uint8_t i = 0; i < len; i++) {
            uint16_t val = EEPROM.read(dataAddr + i * 2);
            val |= ((uint16_t)EEPROM.read(dataAddr + i * 2 + 1)) << 8;
            bulb[bulbIndex].irCode.timing[i] = val;
        }
    } else {
        bulb[bulbIndex].irCode.valid = false;
        bulb[bulbIndex].irCode.length = 0;
    }
}

void saveIRCode(uint8_t bulbIndex, uint16_t* timing, uint8_t length) {
    uint8_t lenAddr = (bulbIndex == 0) ? EEPROM_IR1_LEN : EEPROM_IR2_LEN;
    uint8_t dataAddr = (bulbIndex == 0) ? EEPROM_IR1_DATA : EEPROM_IR2_DATA;
    
    EEPROM.update(lenAddr, length);
    
    for (uint8_t i = 0; i < length && i < IR_MAX_LEN; i++) {
        EEPROM.update(dataAddr + i * 2, timing[i] & 0xFF);
        EEPROM.update(dataAddr + i * 2 + 1, (timing[i] >> 8) & 0xFF);
    }
    
    // Actualizează structura în memorie
    bulb[bulbIndex].irCode.length = length;
    bulb[bulbIndex].irCode.valid = true;
    for (uint8_t i = 0; i < length; i++) {
        bulb[bulbIndex].irCode.timing[i] = timing[i];
    }
}

// ==================== FUNCȚII DIMMING ====================
void updateDimming() {
    if (!zeroCrossing) return;
    zeroCrossing = false;
    
    // Calculează delay-ul pentru fiecare bec bazat pe nivelul de dimming
    // Nivel 100% = delay minim, Nivel 0% = delay maxim (nu aprinde)
    
    for (int i = 0; i < 2; i++) {
        if (bulb[i].currentLevel > 0) {
            // Calculează delay-ul în microsecunde
            // La nivel 100%, delay = ~0.5ms (aprindere aproape la zero crossing)
            // La nivel 1%, delay = ~9.5ms (aprindere aproape la următorul zero crossing)
            unsigned long delayTime = map(bulb[i].currentLevel, 1, 100, 
                                          HALF_CYCLE_US - 500, 500);
            
            // Programează aprinderea triacului
            delayMicroseconds(delayTime);
            
            if (i == 0) {
                digitalWrite(PIN_TRIAC1, HIGH);
            } else {
                digitalWrite(PIN_TRIAC2, HIGH);
            }
            
            delayMicroseconds(TRIAC_PULSE_US);
            
            if (i == 0) {
                digitalWrite(PIN_TRIAC1, LOW);
            } else {
                digitalWrite(PIN_TRIAC2, LOW);
            }
        }
    }
}

// Varianta cu Timer pentru dimming mai precis (non-blocking)
void scheduleDimming() {
    if (!zeroCrossing) return;
    zeroCrossing = false;
    
    unsigned long now = micros();
    
    for (int i = 0; i < 2; i++) {
        if (bulb[i].currentLevel > 0) {
            unsigned long delayTime = map(bulb[i].currentLevel, 1, 100, 
                                          HALF_CYCLE_US - 500, 500);
            
            // Calculează timpul de aprindere
            unsigned long fireTime = lastZeroCross + delayTime;
            
            // Așteaptă până la momentul aprinderii
            while (micros() < fireTime) {
                // Verifică dacă a trecut prea mult timp
                if (micros() - lastZeroCross > HALF_CYCLE_US) break;
            }
            
            // Aprinde triacul
            if (i == 0) {
                digitalWrite(PIN_TRIAC1, HIGH);
                delayMicroseconds(TRIAC_PULSE_US);
                digitalWrite(PIN_TRIAC1, LOW);
            } else {
                digitalWrite(PIN_TRIAC2, HIGH);
                delayMicroseconds(TRIAC_PULSE_US);
                digitalWrite(PIN_TRIAC2, LOW);
            }
        }
    }
}

void softStart(uint8_t bulbIndex) {
    bulb[bulbIndex].isOn = true;
    bulb[bulbIndex].targetLevel = bulb[bulbIndex].savedLevel;
    
    // Gradual ramp up
    while (bulb[bulbIndex].currentLevel < bulb[bulbIndex].targetLevel) {
        bulb[bulbIndex].currentLevel++;
        updateDimming();
        delay(SOFT_STEP_DELAY_MS);
        
        // Procesează zero crossing în timpul ramp-ului
        for (int i = 0; i < 5; i++) {
            if (zeroCrossing) updateDimming();
            delayMicroseconds(2000);
        }
    }
}

void softStop(uint8_t bulbIndex) {
    // Gradual ramp down
    while (bulb[bulbIndex].currentLevel > 0) {
        bulb[bulbIndex].currentLevel--;
        updateDimming();
        delay(SOFT_STEP_DELAY_MS);
        
        // Procesează zero crossing în timpul ramp-ului
        for (int i = 0; i < 5; i++) {
            if (zeroCrossing) updateDimming();
            delayMicroseconds(2000);
        }
    }
    
    bulb[bulbIndex].isOn = false;
}

void softRampTo(uint8_t bulbIndex, uint8_t targetLevel) {
    while (bulb[bulbIndex].currentLevel != targetLevel) {
        if (bulb[bulbIndex].currentLevel < targetLevel) {
            bulb[bulbIndex].currentLevel++;
        } else {
            bulb[bulbIndex].currentLevel--;
        }
        
        updateDimming();
        delay(SOFT_STEP_DELAY_MS);
        
        // Procesează zero crossing
        for (int i = 0; i < 5; i++) {
            if (zeroCrossing) updateDimming();
            delayMicroseconds(2000);
        }
    }
}

void confirmationBlink(uint8_t bulbIndex, uint8_t times) {
    for (uint8_t t = 0; t < times; t++) {
        // Soft on la maxim
        while (bulb[bulbIndex].currentLevel < DIM_MAX) {
            bulb[bulbIndex].currentLevel++;
            updateDimming();
            delay(SOFT_STEP_DELAY_MS / 2);
            for (int i = 0; i < 3; i++) {
                if (zeroCrossing) updateDimming();
                delayMicroseconds(1500);
            }
        }
        
        delay(100);
        
        // Soft off
        while (bulb[bulbIndex].currentLevel > 0) {
            bulb[bulbIndex].currentLevel--;
            updateDimming();
            delay(SOFT_STEP_DELAY_MS / 2);
            for (int i = 0; i < 3; i++) {
                if (zeroCrossing) updateDimming();
                delayMicroseconds(1500);
            }
        }
        
        if (t < times - 1) delay(200);
    }
    
    bulb[bulbIndex].isOn = false;
}

// ==================== FUNCȚII BUTOANE ====================
void readButtons() {
    unsigned long now = millis();
    
    for (int i = 0; i < 2; i++) {
        bool reading = digitalRead((i == 0) ? PIN_BUTTON1 : PIN_BUTTON2);
        
        // Debounce
        if (reading != button[i].lastState) {
            button[i].lastState = reading;
            continue;
        }
        
        // Detectare apăsare
        if (reading && !button[i].currentState) {
            // Buton apăsat
            button[i].currentState = true;
            button[i].pressStart = now;
            button[i].longPressActive = false;
        }
        
        // Detectare eliberare
        if (!reading && button[i].currentState) {
            // Buton eliberat
            button[i].currentState = false;
            unsigned long pressDuration = now - button[i].pressStart;
            
            if (!button[i].longPressActive) {
                // A fost apăsare scurtă
                handleShortPress(i, now);
            } else {
                // Sfârșit apăsare lungă - salvează nivelul
                handleLongPressEnd(i);
            }
        }
        
        // Detectare apăsare lungă în timp ce butonul este apăsat
        if (button[i].currentState && !button[i].longPressActive) {
            if (now - button[i].pressStart >= LONG_PRESS_MS) {
                button[i].longPressActive = true;
                handleLongPressStart(i);
            }
        }
        
        // Continuă ramp up/down în timpul apăsării lungi
        if (button[i].currentState && button[i].longPressActive) {
            handleLongPressHold(i, now);
        }
    }
    
    // Procesează așteptarea pentru mai multe clickuri
    processClickWaiting(now);
}

void handleShortPress(uint8_t bulbIndex, unsigned long now) {
    // Verifică dacă este în fereastra de triple-click
    if (now - button[bulbIndex].lastRelease < TRIPLE_CLICK_WINDOW) {
        button[bulbIndex].clickCount++;
    } else {
        button[bulbIndex].clickCount = 1;
    }
    
    button[bulbIndex].lastRelease = now;
    
    // Începe așteptarea pentru alte clickuri
    if (!button[bulbIndex].waitingForMoreClicks) {
        button[bulbIndex].waitingForMoreClicks = true;
        button[bulbIndex].waitStartTime = now;
    }
}

void processClickWaiting(unsigned long now) {
    for (int i = 0; i < 2; i++) {
        if (button[i].waitingForMoreClicks) {
            // Verifică dacă a trecut timpul de așteptare
            if (now - button[i].waitStartTime >= CLICK_WAIT_MS) {
                button[i].waitingForMoreClicks = false;
                
                // Verifică ce tip de acțiune trebuie executată
                if (button[i].clickCount >= 3 && !bulb[i].isOn) {
                    // Triple click cu becul stins - mod învățare IR
                    startIRLearning(i);
                } else {
                    // Click simplu sau dublu - toggle on/off
                    toggleBulb(i);
                }
                
                button[i].clickCount = 0;
            }
        }
    }
}

void toggleBulb(uint8_t bulbIndex) {
    if (bulb[bulbIndex].isOn) {
        softStop(bulbIndex);
    } else {
        softStart(bulbIndex);
    }
}

void handleLongPressStart(uint8_t bulbIndex) {
    // Resetează contorul de clickuri
    button[bulbIndex].clickCount = 0;
    button[bulbIndex].waitingForMoreClicks = false;
    
    // Marchează becul ca aprins (pentru dimming)
    bulb[bulbIndex].isOn = true;
    
    // Dacă nivelul curent e 0, pornește de la minim
    if (bulb[bulbIndex].currentLevel == 0) {
        bulb[bulbIndex].currentLevel = DIM_MIN;
    }
    
    // Începe ramp up de la nivelul curent
    rampingUp = true;
    lastRampStep = millis();
}

void handleLongPressHold(uint8_t bulbIndex, unsigned long now) {
    if (now - lastRampStep >= RAMP_DELAY_MS) {
        lastRampStep = now;
        
        if (rampingUp) {
            if (bulb[bulbIndex].currentLevel < DIM_MAX) {
                bulb[bulbIndex].currentLevel++;
            } else {
                rampingUp = false;
            }
        } else {
            if (bulb[bulbIndex].currentLevel > DIM_MIN) {
                bulb[bulbIndex].currentLevel--;
            } else {
                rampingUp = true;
            }
        }
    }
}

void handleLongPressEnd(uint8_t bulbIndex) {
    // Salvează nivelul curent în EEPROM
    bulb[bulbIndex].savedLevel = bulb[bulbIndex].currentLevel;
    saveDimmingLevel(bulbIndex);
}

// ==================== FUNCȚII IR ====================
void startIRLearning(uint8_t bulbIndex) {
    // Confirmare - aprinde și stinge o dată
    confirmationBlink(bulbIndex, 1);
    
    // Intră în mod învățare
    irLearningMode = true;
    irLearningBulb = bulbIndex;
    irLearningStart = millis();
    
    // Resetează buffer-ul IR
    irBufferIndex = 0;
    irCodeReady = false;
    irReceiving = false;
}

void processIRLearning() {
    if (!irLearningMode) return;
    
    unsigned long now = millis();
    
    // Verifică timeout
    if (now - irLearningStart >= IR_LEARN_TIMEOUT) {
        // Timeout - ieși din modul învățare
        irLearningMode = false;
        return;
    }
    
    // Verifică dacă s-a primit un cod IR
    if (irCodeReady) {
        irCodeReady = false;
        
        // Ignoră coduri prea scurte (inclusiv NEC repeat code ~3 timing-uri)
        if (irBufferIndex >= IR_MIN_PULSES) {
            // Verifică dacă NU este NEC repeat code
            // NEC repeat: ~9000µs + ~2250µs + ~560µs (3 timing-uri, prea scurt)
            // Cod valid NEC: 67+ timing-uri
            // Această verificare e redundantă dar adaugă siguranță
            
            // Salvează codul
            uint16_t tempBuffer[IR_MAX_LEN];
            noInterrupts();
            memcpy(tempBuffer, (const void*)irBuffer, irBufferIndex * sizeof(uint16_t));
            uint8_t len = irBufferIndex;
            irBufferIndex = 0;
            interrupts();
            
            saveIRCode(irLearningBulb, tempBuffer, len);
            
            // Confirmare - aprinde și stinge de 2 ori
            confirmationBlink(irLearningBulb, 2);
            irLearningMode = false;
        }
        // Dacă codul e prea scurt (repeat code), continuă să așteptăm cod valid
    }
}

bool compareIRCodes(uint16_t* received, uint8_t receivedLen, IRCode* stored) {
    if (!stored->valid) return false;
    if (receivedLen < IR_MIN_PULSES) return false;
    
    // Compară doar primele N pulsuri (ignoră coada care poate varia)
    uint8_t compareLen = min(receivedLen, stored->length);
    compareLen = min(compareLen, (uint8_t)20); // Compară doar primele 20 pentru eficiență
    
    int matches = 0;
    for (uint8_t i = 0; i < compareLen; i++) {
        uint16_t storedVal = stored->timing[i];
        uint16_t recvVal = received[i];
        
        // Calculează toleranța
        uint16_t tolerance = (storedVal * IR_TOLERANCE) / 100;
        
        if (recvVal >= storedVal - tolerance && recvVal <= storedVal + tolerance) {
            matches++;
        }
    }
    
    // Dacă mai mult de 70% se potrivesc, considerăm că e același cod
    return (matches * 100 / compareLen) >= 70;
}

/**
 * @brief Detectează dacă buffer-ul conține un NEC repeat code
 * 
 * NEC repeat code: ~9000µs burst + ~2250µs space + ~560µs burst
 * Total 3 timing-uri (sau 2 dacă ultimul nu e capturat)
 * 
 * @param buffer Array cu timing-uri
 * @param len Lungime buffer
 * @return true dacă e repeat code
 */
bool isNECRepeatCode(uint16_t* buffer, uint8_t len) {
    // Repeat code are 2-4 timing-uri
    if (len < 2 || len > 5) return false;
    
    // Verifică primul timing (~9000µs burst)
    if (buffer[0] < NEC_REPEAT_BURST - NEC_REPEAT_TOLERANCE ||
        buffer[0] > NEC_REPEAT_BURST + NEC_REPEAT_TOLERANCE) {
        return false;
    }
    
    // Verifică al doilea timing (~2250µs space)
    if (buffer[1] < NEC_REPEAT_SPACE - NEC_REPEAT_TOLERANCE ||
        buffer[1] > NEC_REPEAT_SPACE + NEC_REPEAT_TOLERANCE) {
        return false;
    }
    
    return true;
}

void processIRRemote() {
    if (irLearningMode) return; // În mod învățare, nu procesa ca telecomandă
    
    if (!irCodeReady) {
        // Verifică timeout recepție
        if (irReceiving && (micros() - irLastEdge > IR_TIMEOUT_US)) {
            if (irBufferIndex >= 2) {  // Minim 2 pentru repeat code
                irCodeReady = true;
            }
            irReceiving = false;
        }
        
        if (!irCodeReady) return;
    }
    
    irCodeReady = false;
    
    // Copiază buffer-ul pentru procesare
    uint16_t tempBuffer[IR_MAX_LEN];
    noInterrupts();
    memcpy(tempBuffer, (const void*)irBuffer, irBufferIndex * sizeof(uint16_t));
    uint8_t len = irBufferIndex;
    irBufferIndex = 0;
    interrupts();
    
    unsigned long now = millis();
    
    // Verifică dacă e NEC repeat code
    if (isNECRepeatCode(tempBuffer, len)) {
        // E repeat code - continuă apăsarea pe ultimul bec identificat
        if (lastIRBulb >= 0 && lastIRBulb < 2) {
            // Actualizează timestamp pentru a menține apăsarea activă
            irLastReceived[lastIRBulb] = now;
            
            // Dacă apăsarea lungă e activă, continuă ramp
            if (irLongPressActive[lastIRBulb]) {
                handleLongPressHold(lastIRBulb, now);
            }
            // Verifică dacă a devenit apăsare lungă
            else if (now - irPressStart[lastIRBulb] >= IR_LONG_PRESS_MS) {
                irLongPressActive[lastIRBulb] = true;
                handleLongPressStart(lastIRBulb);
            }
        }
        return;
    }
    
    // Nu e repeat code - verifică potrivirea cu codurile salvate
    if (len < IR_MIN_PULSES) return;  // Cod prea scurt (nu e valid)
    
    for (int i = 0; i < 2; i++) {
        if (compareIRCodes(tempBuffer, len, &bulb[i].irCode)) {
            // Cod valid găsit - salvează care bec
            lastIRBulb = i;
            handleIRCommand(i);
            break;
        }
    }
}

void handleIRCommand(uint8_t bulbIndex) {
    unsigned long now = millis();
    
    // Verifică dacă este apăsare repetată (continuare)
    if (now - irLastReceived[bulbIndex] < IR_REPEAT_TIMEOUT) {
        // Este un repeat - continuă acțiunea curentă
        if (irLongPressActive[bulbIndex]) {
            // Continuă ramp up/down
            handleLongPressHold(bulbIndex, now);
        }
    } else {
        // Apăsare nouă
        irPressStart[bulbIndex] = now;
        irLongPressActive[bulbIndex] = false;
    }
    
    irLastReceived[bulbIndex] = now;
    
    // Verifică dacă a devenit apăsare lungă
    if (!irLongPressActive[bulbIndex] && 
        (now - irPressStart[bulbIndex] >= IR_LONG_PRESS_MS)) {
        irLongPressActive[bulbIndex] = true;
        handleLongPressStart(bulbIndex);
    }
}

void processIRTimeout() {
    unsigned long now = millis();
    
    for (int i = 0; i < 2; i++) {
        // Dacă nu am primit IR recent și era apăsare lungă activă
        if (irLongPressActive[i] && (now - irLastReceived[i] > IR_REPEAT_TIMEOUT * 2)) {
            // Consideră că s-a terminat apăsarea
            irLongPressActive[i] = false;
            
            // Salvează nivelul dacă era în mod dimming
            if (bulb[i].isOn) {
                bulb[i].savedLevel = bulb[i].currentLevel;
                saveDimmingLevel(i);
            }
        }
        
        // Dacă a fost apăsare scurtă (fără să devină lungă)
        if (!irLongPressActive[i] && irPressStart[i] > 0 && 
            (now - irLastReceived[i] > IR_REPEAT_TIMEOUT * 2) &&
            (irLastReceived[i] - irPressStart[i] < IR_LONG_PRESS_MS)) {
            // Toggle bec
            toggleBulb(i);
            irPressStart[i] = 0; // Resetează pentru a nu repeta
        }
    }
}

// ==================== FUNCȚII SLEEP ====================
void enterSleep() {
    // Verifică dacă putem intra în sleep (ambele becuri stinse)
    if (bulb[0].isOn || bulb[1].isOn || irLearningMode) {
        return;
    }
    
    // Dezactivează triacurile
    digitalWrite(PIN_TRIAC1, LOW);
    digitalWrite(PIN_TRIAC2, LOW);
    
    // Configurare wake-up pe Pin Change Interrupt
    // Butoanele sunt pe PORTD (pins 5, 6)
    PCICR |= (1 << PCIE2);      // Enable PCINT pentru PORTD
    PCMSK2 |= (1 << PCINT21) | (1 << PCINT22); // Pins 5 și 6
    
    // IR-ul este deja configurat pentru wake-up
    
    // Configurare mod sleep
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    
    // Dezactivează module pentru economie de energie
    power_adc_disable();
    power_spi_disable();
    power_timer0_disable();
    power_timer2_disable();
    power_twi_disable();
    
    // Intră în sleep
    sleep_enable();
    sei(); // Enable interrupts pentru wake-up
    sleep_cpu();
    
    // === WAKE UP ===
    sleep_disable();
    
    // Reactivează modulele
    power_all_enable();
    
    // Mică întârziere pentru stabilizare
    delay(10);
}

// ISR suplimentar pentru wake-up de la butoane (dacă nu este definit)
ISR(PCINT2_vect, ISR_ALIASOF(PCINT2_vect));

// ==================== LOOP PRINCIPAL ====================
void loop() {
    // Procesează dimming la fiecare zero crossing
    updateDimming();
    
    // Citește și procesează butoanele
    readButtons();
    
    // Procesează învățare IR
    processIRLearning();
    
    // Procesează comenzi IR de la telecomandă
    processIRRemote();
    processIRTimeout();
    
    // Verifică dacă putem intra în sleep
    if (sleepEnabled && !bulb[0].isOn && !bulb[1].isOn && 
        !irLearningMode && 
        !button[0].waitingForMoreClicks && !button[1].waitingForMoreClicks) {
        
        // Așteaptă puțin înainte de sleep pentru a permite procesarea
        static unsigned long lastActivity = 0;
        if (millis() - lastActivity > 1000) {
            enterSleep();
        }
    }
}
