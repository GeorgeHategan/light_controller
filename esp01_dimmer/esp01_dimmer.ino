/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                      ESP-01 AC DIMMER CONTROLLER v1.0                         ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Controlează 2 becuri cu dimming pe rețea 220V/50Hz                           ║
 * ║  WiFi dezactivat pentru consum redus de energie                               ║
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
 * │  ✓ Light Sleep pentru economie de energie când becurile sunt stinse         │
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
 * │                      CONEXIUNI HARDWARE ESP-01                               │
 * ├─────────────────────────────────────────────────────────────────────────────┤
 * │                                                                              │
 * │   ESP-01 Module          PCF8574 I2C Expander        Componente              │
 * │  ┌──────────────┐       ┌──────────────────┐                                │
 * │  │ VCC ─────────┼───────┤ VCC              │                                │
 * │  │ GND ─────────┼───────┤ GND              │                                │
 * │  │ GPIO0 (SDA) ─┼───────┤ SDA              │                                │
 * │  │ GPIO2 (SCL) ─┼───────┤ SCL              │                                │
 * │  │ GPIO3 (RX) ──┼───────┼──────────────────┼────► ZCD Circuit               │
 * │  │ GPIO1 (TX) ──┼───────┼──────────────────┼────► IR Receiver (TSOP4838)    │
 * │  └──────────────┘       │ P0 ──────────────┼────► TRIAC 1 (MOC3021+BT136)   │
 * │                         │ P1 ──────────────┼────► TRIAC 2 (MOC3021+BT136)   │
 * │                         │ P2 ◄─────────────┼──── Button TTP223 #1           │
 * │                         │ P3 ◄─────────────┼──── Button TTP223 #2           │
 * │                         │ P4-P7 ───────────┼────► (disponibili extensii)    │
 * │                         └──────────────────┘                                │
 * │                                                                              │
 * │   NOTĂ: Adresa I2C implicită PCF8574: 0x20 (A0=A1=A2=GND)                   │
 * │         Pentru altă adresă, modifică PCF8574_ADDR mai jos                    │
 * └─────────────────────────────────────────────────────────────────────────────┘
 * 
 * ┌─────────────────────────────────────────────────────────────────────────────┐
 * │                    CIRCUIT ZCD (Zero Crossing Detector)                      │
 * ├─────────────────────────────────────────────────────────────────────────────┤
 * │                                                                              │
 * │   220V AC ──┬──[R1 100kΩ]──┬──[R2 100kΩ]──┬─── GND                         │
 * │             │              │              │                                  │
 * │             │         ┌────┴────┐         │                                  │
 * │             │         │ 4N35    │         │                                  │
 * │             │    AC──►│ ●     ● │──► VCC (3.3V prin R 10kΩ)                 │
 * │             │         │ ●     ● │──► GPIO3 (RX) - Output ZCD                │
 * │             │    AC──►│ ●     ● │──► GND                                    │
 * │             │         └─────────┘                                            │
 * │             │                                                                │
 * │   220V AC ──┘                                                                │
 * │                                                                              │
 * │   Output: Semnal dreptunghiular 50Hz, RISING edge = zero crossing           │
 * │                                                                              │
 * └─────────────────────────────────────────────────────────────────────────────┘
 * 
 * ┌─────────────────────────────────────────────────────────────────────────────┐
 * │                        CIRCUIT TRIAC (per canal)                             │
 * ├─────────────────────────────────────────────────────────────────────────────┤
 * │                                                                              │
 * │   GPIO ──[R 330Ω]──► MOC3021 ──► BT136 ──► LOAD (Bec) ──► 220V AC          │
 * │                         │                      │                             │
 * │                        GND              [Snubber 100Ω + 100nF/400V]          │
 * │                                                                              │
 * │   ATENȚIE: Folosiți întotdeauna snubber pentru sarcini inductive!           │
 * │                                                                              │
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
 * │   141-255│ 115 b   │ REZERVAT pentru extensii viitoare                      │
 * └─────────────────────────────────────────────────────────────────────────────┘
 * 
 * ┌─────────────────────────────────────────────────────────────────────────────┐
 * │                    GHID ADĂUGARE FUNCȚIONALITĂȚI NOI                         │
 * ├─────────────────────────────────────────────────────────────────────────────┤
 * │                                                                              │
 * │   1. ADĂUGARE CANAL NOU (Bec 3, 4, etc):                                     │
 * │      - Mărește array-urile bulb[] și button[] în secțiunea variabile         │
 * │      - Adaugă pini noi în secțiunea CONFIGURARE PINI                         │
 * │      - Adaugă adrese EEPROM noi pentru nivel și cod IR                       │
 * │      - Modifică loop-urile care iterează prin canale                         │
 * │                                                                              │
 * │   2. ADĂUGARE SENZOR NOU (temperatură, lumină, etc):                         │
 * │      - Definește pinul în secțiunea CONFIGURARE PINI                         │
 * │      - Creează struct pentru starea senzorului                               │
 * │      - Adaugă funcție readSensor() în secțiunea corespunzătoare              │
 * │      - Apelează din loop() sau pe timer                                      │
 * │                                                                              │
 * │   3. ADĂUGARE MOD NOU (ex: mod noapte, scenă, etc):                          │
 * │      - Definește constante pentru modul nou                                  │
 * │      - Adaugă variabilă de stare pentru modul activ                          │
 * │      - Implementează logica în funcția dedicată                              │
 * │      - Adaugă activare prin combinație de butoane sau IR                     │
 * │                                                                              │
 * │   4. ADĂUGARE COMUNICAȚIE (UART, WiFi temporar):                             │
 * │      - WiFi: Reactivează cu WiFi.mode(WIFI_STA) în funcție dedicată         │
 * │      - UART: GPIO1(TX) și GPIO3(RX) - atenție la conflicte cu IR/ZCD        │
 * │      - Adaugă #define pentru activare/dezactivare                            │
 * │                                                                              │
 * │   5. MODIFICARE TIMPI/PARAMETRI:                                             │
 * │      - Toți parametrii sunt în secțiunea CONFIGURARE                         │
 * │      - Modifică #define-urile corespunzătoare                                │
 * │      - Recompilează și încarcă                                               │
 * │                                                                              │
 * └─────────────────────────────────────────────────────────────────────────────┘
 * 
 * ┌─────────────────────────────────────────────────────────────────────────────┐
 * │                            TROUBLESHOOTING                                   │
 * ├─────────────────────────────────────────────────────────────────────────────┤
 * │   Problemă                  │ Soluție                                       │
 * │   ──────────────────────────┼───────────────────────────────────────────────│
 * │   Dimming tremură/flicker   │ Verifică sincronizare ZCD, ajustează timing   │
 * │   IR nu funcționează        │ Verifică polaritate receptor, testează cu LED │
 * │   I2C nu răspunde           │ Verifică pull-up 4.7kΩ, adresa PCF8574       │
 * │   ESP nu pornește           │ GPIO0 și GPIO2 trebuie HIGH la boot           │
 * │   Bec nu se aprinde         │ Verifică circuit triac, optocuplor            │
 * │   Setări pierdute la restart│ Verifică EEPROM.commit() după scriere         │
 * └─────────────────────────────────────────────────────────────────────────────┘
 * 
 * AUTOR: ESP Controller Project
 * VERSIUNE: 1.0
 * DATA: Decembrie 2025
 * LICENȚĂ: MIT
 * 
 * ⚠️  AVERTISMENT: Acest proiect lucrează cu tensiune de rețea (220V AC).
 *     PERICOL DE ELECTROCUTARE! Lucrați doar cu alimentarea deconectată.
 *     Respectați toate normele de siguranță electrică.
 */

#include <ESP8266WiFi.h>
#include <EEPROM.h>

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                          SECȚIUNEA 1: CONFIGURARE                             ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Modifică valorile de mai jos pentru a personaliza comportamentul             ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== DEZACTIVARE WIFI ====================
/**
 * @brief Dezactivează complet modulul WiFi pentru economie de energie
 * 
 * Consumul scade de la ~70mA (WiFi activ) la ~15mA (WiFi oprit)
 * Apelată automat în setup()
 * 
 * Pentru a reactiva WiFi temporar (ex: OTA update):
 *   WiFi.forceSleepWake();
 *   WiFi.mode(WIFI_STA);
 *   WiFi.begin("SSID", "password");
 */
void disableWiFi() {
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);
}

// ==================== CONFIGURARE PINI ESP-01 ====================
/**
 * @brief Configurare pini hardware
 * 
 * ESP-01 are doar 4 GPIO disponibili:
 *   - GPIO0: Boot mode (trebuie HIGH la pornire), folosit pentru I2C SDA
 *   - GPIO2: Boot mode (trebuie HIGH la pornire), folosit pentru I2C SCL
 *   - GPIO1 (TX): Poate fi GPIO dacă nu folosești Serial
 *   - GPIO3 (RX): Poate fi GPIO dacă nu folosești Serial
 * 
 * Setează USE_I2C_EXPANDER pe true pentru 2 canale complete
 * Setează USE_I2C_EXPANDER pe false pentru doar 1 canal (pini limitați)
 */
#define USE_I2C_EXPANDER    true   // true = 2 canale cu PCF8574, false = 1 canal direct

#if USE_I2C_EXPANDER
    // ═══════════════════════════════════════════════════════════════
    // CONFIGURARE CU PCF8574 I2C EXPANDER (recomandat pentru 2 canale)
    // ═══════════════════════════════════════════════════════════════
    #define PIN_SDA         0      // GPIO0 - I2C Data (conectat la SDA pe PCF8574)
    #define PIN_SCL         2      // GPIO2 - I2C Clock (conectat la SCL pe PCF8574)
    #define PIN_ZCD         3      // GPIO3 (RX) - Zero Crossing Detection input
    #define PIN_IR_RECV     1      // GPIO1 (TX) - IR Receiver input (TSOP4838)
    
    // Pini pe PCF8574 (P0-P7)
    // Modifică dacă ai conectat diferit pe expansor
    #define EXP_TRIAC1      0      // P0 - Output pentru Triac canal 1
    #define EXP_TRIAC2      1      // P1 - Output pentru Triac canal 2
    #define EXP_BUTTON1     2      // P2 - Input de la buton TTP223 canal 1
    #define EXP_BUTTON2     3      // P3 - Input de la buton TTP223 canal 2
    // P4-P7 disponibili pentru extensii (senzori, LED status, etc.)
    
#else
    // ═══════════════════════════════════════════════════════════════
    // CONFIGURARE FĂRĂ EXPANDER (doar 1 canal, funcționalitate limitată)
    // ═══════════════════════════════════════════════════════════════
    #define PIN_ZCD         3      // GPIO3 (RX) - Zero Crossing Detection
    #define PIN_TRIAC1      0      // GPIO0 - Triac canal 1 (atenție la boot!)
    #define PIN_BUTTON1     2      // GPIO2 - Buton canal 1 (atenție la boot!)
    #define PIN_IR_RECV     1      // GPIO1 (TX) - IR Receiver
    // NOTĂ: În acest mod, canalul 2 nu este disponibil!
#endif

// ==================== CONFIGURARE TIMING ====================
/**
 * @brief Parametri de timing - ajustează pentru comportament diferit
 * 
 * TIMING REȚEA (nu modifica decât dacă frecvența rețelei e diferită):
 *   HALF_CYCLE_US: 10000 pentru 50Hz, 8333 pentru 60Hz
 *   TRIAC_PULSE_US: Durata minimă pentru aprindere triac sigură
 * 
 * TIMING BUTOANE:
 *   DEBOUNCE_MS: Timp ignorare bounce buton (mărește dacă ai false triggers)
 *   LONG_PRESS_MS: Pragul pentru apăsare lungă
 *   CLICK_WAIT_MS: Cât așteaptă după click pentru alte clickuri
 *   TRIPLE_CLICK_WINDOW: Fereastră pentru 3 clickuri consecutive
 * 
 * TIMING DIMMING:
 *   RAMP_DELAY_MS: Viteză ramp up/down în mod manual
 *   SOFT_STEP_DELAY_MS: Viteză soft start/stop
 */
#define HALF_CYCLE_US       10000  // Jumătate perioadă AC: 10ms@50Hz, 8.33ms@60Hz
#define TRIAC_PULSE_US      50     // Durata puls gate triac (50-100µs tipic)

#define DEBOUNCE_MS         50     // Debounce butoane touch (ms)
#define LONG_PRESS_MS       800    // Prag apăsare lungă (ms)
#define TRIPLE_CLICK_WINDOW 3000   // Fereastră pentru triple-click (ms)
#define CLICK_WAIT_MS       1000   // Așteptare după click pentru alte clickuri (ms)
#define IR_LEARN_TIMEOUT    10000  // Timeout mod învățare IR (ms)
#define RAMP_DELAY_MS       30     // Delay între pași ramp manual (ms)
#define SOFT_STEP_DELAY_MS  20     // Delay între pași soft start/stop (ms)

// ==================== CONFIGURARE DIMMING ====================
/**
 * @brief Limite și valoare implicită pentru dimming
 * 
 * DIM_MIN: Sub această valoare becul poate să nu pornească stabil
 * DIM_MAX: Peste această valoare diferența e imperceptibilă
 * DIM_DEFAULT: Valoare la prima utilizare sau după reset EEPROM
 * 
 * Valori: 0-100 (procent din putere maximă)
 */
#define DIM_MIN             5      // Nivel minim dimming (%)
#define DIM_MAX             95     // Nivel maxim dimming (%)
#define DIM_DEFAULT         50     // Nivel implicit la prima utilizare (%)

// ==================== ADRESE EEPROM ====================
/**
 * @brief Mapare EEPROM pentru persistență date
 * 
 * IMPORTANT: Nu modifica adresele existente după ce ai date salvate!
 *            Poți adăuga noi adrese după EEPROM_IR2_DATA + 68
 * 
 * Spațiu disponibil pentru extensii: adresele 141-255 (115 bytes)
 */
#define EEPROM_SIZE         256    // Dimensiune totală EEPROM folosită
#define EEPROM_MAGIC        0      // Adresa byte magic validare (1 byte)
#define EEPROM_DIM1         1      // Adresa nivel dimming canal 1 (1 byte)
#define EEPROM_DIM2         2      // Adresa nivel dimming canal 2 (1 byte)
#define EEPROM_IR1_LEN      3      // Adresa lungime cod IR canal 1 (1 byte)
#define EEPROM_IR1_DATA     4      // Adresa început date IR canal 1 (68 bytes)
#define EEPROM_IR2_LEN      72     // Adresa lungime cod IR canal 2 (1 byte)
#define EEPROM_IR2_DATA     73     // Adresa început date IR canal 2 (68 bytes)
#define EEPROM_MAGIC_VALUE  0xA5   // Valoare magic pentru EEPROM valid

// Adrese disponibile pentru extensii (exemple):
// #define EEPROM_SCENE1     141   // Scenă predefinită 1
// #define EEPROM_NIGHT_MODE 142   // Setări mod noapte
// #define EEPROM_SCHEDULE   143   // Programare orară (necesită RTC)

// ==================== IR CONFIGURATION ====================
/**
 * @brief Parametri recepție și procesare IR
 * 
 * IR_MAX_LEN: Număr maxim de tranziții memorate (mărește pentru protocoale lungi)
 * IR_MIN_PULSES: Minim tranziții pentru cod valid (reduce zgomot)
 * IR_TOLERANCE: Toleranță comparare (%, mărește dacă telecomanda nu e recunoscută)
 * IR_LONG_PRESS_MS: Prag pentru apăsare lungă pe telecomandă
 * IR_REPEAT_TIMEOUT: Timeout între repeat codes (ms)
 */
#define IR_MAX_LEN          67     // Lungime max buffer IR (perechi timing)
#define IR_TIMEOUT_US       50000  // Timeout recepție IR (µs)
#define IR_MIN_PULSES       10     // Minim pulsuri pentru cod valid
#define IR_TOLERANCE        20     // Toleranță comparare coduri (%)
#define IR_LONG_PRESS_MS    800    // Prag apăsare lungă telecomandă (ms)
#define IR_REPEAT_TIMEOUT   150    // Timeout detectare repeat IR (ms)

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                       SECȚIUNEA 2: STRUCTURI DE DATE                          ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Definirea tipurilor de date folosite în program                              ║
 * ║  Pentru extensii, adaugă noi structuri aici                                   ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== STRUCTURI ====================

/**
 * @brief Structură pentru stocarea unui cod IR învățat
 * 
 * Stochează timing-urile (duratele) între tranzițiile semnalului IR.
 * Primul timing este HIGH, al doilea LOW, alternativ.
 * 
 * Exemplu extindere pentru mai multe coduri per bec:
 *   struct IRCodeSet {
 *       IRCode onCode;      // Cod pentru ON
 *       IRCode offCode;     // Cod pentru OFF
 *       IRCode dimUpCode;   // Cod pentru dim up
 *       IRCode dimDownCode; // Cod pentru dim down
 *   };
 */
struct IRCode {
    uint16_t timing[IR_MAX_LEN];   // Array cu duratele pulsurilor (µs)
    uint8_t length;                 // Număr de elemente valide în timing[]
    bool valid;                     // true dacă codul a fost învățat
};

/**
 * @brief Structură pentru starea unui bec/canal
 * 
 * Conține toate informațiile despre starea curentă și setările unui canal.
 * 
 * @member isOn          Starea curentă (aprins/stins)
 * @member currentLevel  Nivelul actual de dimming (0-100)
 * @member targetLevel   Nivelul țintă pentru tranziții (soft start/stop)
 * @member savedLevel    Nivelul salvat în EEPROM (persistent)
 * @member irCode        Codul IR asociat acestui canal
 * 
 * EXTENSII POSIBILE:
 *   bool autoFadeEnabled;     // Fade automată activă
 *   uint8_t fadeDirection;    // Direcția fade-ului
 *   uint8_t sceneLevel;       // Nivel pentru scenă predefinită
 *   uint32_t lastOnTime;      // Timestamp ultima aprindere
 *   uint32_t totalOnTime;     // Timp total funcționare (minute)
 */
struct BulbState {
    bool isOn;                      // true = becul este aprins
    uint8_t currentLevel;           // Nivel actual dimming 0-100%
    uint8_t targetLevel;            // Nivel țintă pentru tranziții
    uint8_t savedLevel;             // Nivel memorat în EEPROM
    IRCode irCode;                  // Cod IR asociat canalului
};

/**
 * @brief Structură pentru starea unui buton
 * 
 * Gestionează debounce, detectare apăsare scurtă/lungă și multi-click.
 * 
 * @member lastState             Starea anterioară (pentru debounce)
 * @member currentState          Starea procesată curentă
 * @member pressStart            Timestamp începere apăsare
 * @member lastRelease           Timestamp ultima eliberare
 * @member clickCount            Contor clickuri în fereastră
 * @member longPressActive       Flag apăsare lungă în curs
 * @member waitingForMoreClicks  Flag așteptare multi-click
 * @member waitStartTime         Timestamp început așteptare
 * 
 * EXTENSII POSIBILE:
 *   uint8_t pressPattern;      // Pattern apăsări (ex: SOS)
 *   bool doubleClickEnabled;   // Activare funcție double-click
 */
struct ButtonState {
    bool lastState;                 // Stare anterioară (debounce)
    bool currentState;              // Stare curentă procesată
    unsigned long pressStart;       // Când a început apăsarea (ms)
    unsigned long lastRelease;      // Când s-a eliberat ultima dată (ms)
    uint8_t clickCount;             // Număr clickuri în fereastră
    bool longPressActive;           // Apăsare lungă în desfășurare
    bool waitingForMoreClicks;      // Așteptăm alte clickuri
    unsigned long waitStartTime;    // Când am început să așteptăm
};

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                      SECȚIUNEA 3: VARIABILE GLOBALE                           ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Variabile de stare ale sistemului                                            ║
 * ║  'volatile' pentru variabilele modificate în ISR                              ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== VARIABILE GLOBALE ====================

// --- Zero Crossing Detection ---
volatile bool zeroCrossing = false;      // Flag setat în ISR la zero crossing
volatile unsigned long lastZeroCross = 0; // Timestamp ultimul zero crossing (µs)

// --- Stări canale ---
BulbState bulb[2];                       // Starea celor 2 becuri/canale
ButtonState button[2];                   // Starea celor 2 butoane

// --- Buffer și stare IR ---
volatile uint16_t irBuffer[IR_MAX_LEN];  // Buffer recepție timing-uri IR
volatile uint8_t irBufferIndex = 0;      // Index curent în buffer
volatile unsigned long irLastEdge = 0;   // Timestamp ultima tranziție IR (µs)
volatile bool irReceiving = false;       // Recepție IR în curs
volatile bool irCodeReady = false;       // Cod IR complet, gata de procesare

// --- Mod învățare IR ---
bool irLearningMode = false;             // Mod învățare activ
uint8_t irLearningBulb = 0;              // Canalul pentru care învățăm IR
unsigned long irLearningStart = 0;       // Timestamp intrare în mod învățare

// --- Apăsare lungă IR ---
unsigned long irLastReceived[2] = {0, 0}; // Timestamp ultimul cod IR per canal
bool irLongPressActive[2] = {false, false}; // Apăsare lungă IR activă
unsigned long irPressStart[2] = {0, 0};  // Timestamp început apăsare IR

// --- Ramp dimming ---
bool rampingUp = true;                   // Direcția ramp (true=sus, false=jos)
unsigned long lastRampStep = 0;          // Timestamp ultimul pas ramp

// --- I2C Expander ---
#if USE_I2C_EXPANDER
    #include <Wire.h>
    #define PCF8574_ADDR    0x20         // Adresa I2C a PCF8574 (modifică dacă diferită)
    uint8_t expanderState = 0xFF;        // Stare curentă ieșiri PCF8574
#endif

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                     SECȚIUNEA 4: RUTINE DE ÎNTRERUPERE                        ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  ISR-uri (Interrupt Service Routines)                                         ║
 * ║  ATENȚIE: Codul din ISR trebuie să fie minim și rapid!                        ║
 * ║  Folosește IRAM_ATTR pentru funcții ISR pe ESP8266                            ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== ISR ZERO CROSSING ====================
/**
 * @brief ISR pentru detectarea trecerii prin zero a tensiunii AC
 * 
 * Apelată la fiecare front crescător al semnalului ZCD.
 * Setează flag-ul zeroCrossing și salvează timestamp-ul.
 * 
 * TIMING CRITIC: Această funcție trebuie să fie foarte rapidă!
 * Nu adăuga cod suplimentar aici - procesarea se face în loop().
 */
void IRAM_ATTR zeroCrossISR() {
    zeroCrossing = true;
    lastZeroCross = micros();
}

// ==================== ISR IR RECEIVER ====================
/**
 * @brief ISR pentru recepția semnalelor IR
 * 
 * Apelată la fiecare tranziție (HIGH→LOW sau LOW→HIGH) a pinului IR.
 * Măsoară durata între tranziții și le stochează în buffer.
 * 
 * PROTOCOL IR:
 *   - Prima tranziție: început cod, reset buffer
 *   - Tranziții ulterioare: salvare durată în buffer
 *   - Timeout: cod complet, setare flag irCodeReady
 * 
 * TIMING: Duratele sunt în microsecunde (µs)
 */
void IRAM_ATTR irISR() {
    unsigned long now = micros();
    
    if (!irReceiving) {
        // Prima tranziție - începem recepția
        irReceiving = true;
        irBufferIndex = 0;
        irLastEdge = now;
    } else {
        // Tranziție ulterioară - salvăm durata
        unsigned long duration = now - irLastEdge;
        irLastEdge = now;
        
        if (duration < IR_TIMEOUT_US && irBufferIndex < IR_MAX_LEN) {
            // Durată validă, salvăm în buffer
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

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                   SECȚIUNEA 5: FUNCȚII I2C EXPANDER                           ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Comunicare cu PCF8574 pentru extinderea numărului de pini                    ║
 * ║  Aceste funcții sunt disponibile doar dacă USE_I2C_EXPANDER = true            ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== I2C EXPANDER FUNCTIONS ====================
#if USE_I2C_EXPANDER

/**
 * @brief Scrie o valoare pe un pin al expanderului PCF8574
 * 
 * @param pin   Numărul pinului (0-7)
 * @param value Valoarea de scris (true=HIGH, false=LOW)
 * 
 * NOTĂ: PCF8574 are ieșiri quasi-bidirecționale.
 *       Pentru a citi, pinul trebuie să fie HIGH.
 */
void expanderWrite(uint8_t pin, bool value) {
    if (value) {
        expanderState |= (1 << pin);
    } else {
        expanderState &= ~(1 << pin);
    }
    Wire.beginTransmission(PCF8574_ADDR);
    Wire.write(expanderState);
    Wire.endTransmission();
}

/**
 * @brief Citește valoarea unui pin de pe expanderul PCF8574
 * 
 * @param pin   Numărul pinului (0-7)
 * @return      Valoarea citită (true=HIGH, false=LOW)
 * 
 * NOTĂ: Pentru a citi corect, pinul trebuie să fie setat HIGH
 *       (pull-up intern activ)
 */
bool expanderRead(uint8_t pin) {
    Wire.requestFrom(PCF8574_ADDR, (uint8_t)1);
    if (Wire.available()) {
        uint8_t data = Wire.read();
        return (data >> pin) & 0x01;
    }
    return false;
}

/**
 * @brief Inițializează expanderul PCF8574
 * 
 * Configurează bus-ul I2C și setează starea inițială a pinilor:
 *   - P0, P1: LOW (ieșiri triac)
 *   - P2-P7: HIGH (intrări cu pull-up)
 * 
 * Apelată automat în setup()
 */
void expanderInit() {
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(100000);          // 100kHz I2C clock
    
    // Setează toate ieșirile LOW, intrările HIGH (pull-up intern PCF8574)
    expanderState = 0xFC;           // Bit 0,1 = LOW (triac), Bit 2-7 = HIGH (inputs)
    Wire.beginTransmission(PCF8574_ADDR);
    Wire.write(expanderState);
    Wire.endTransmission();
}
#endif

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                     SECȚIUNEA 6: SETUP ȘI INIȚIALIZARE                        ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Configurare inițială a sistemului                                            ║
 * ║  Apelată o singură dată la pornire/reset                                      ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== SETUP ====================
/**
 * @brief Funcția de inițializare a sistemului
 * 
 * Ordinea operațiilor:
 *   1. Dezactivare WiFi (economie energie)
 *   2. Inițializare EEPROM
 *   3. Configurare I2C expander (dacă este folosit)
 *   4. Configurare pini GPIO
 *   5. Atașare întreruperi (ZCD, IR)
 *   6. Încărcare setări din EEPROM
 *   7. Inițializare structuri de stare
 */
void setup() {
    // ═══════════════════════════════════════════════════════════════
    // PASUL 1: Dezactivare WiFi pentru economie de energie
    // ═══════════════════════════════════════════════════════════════
    disableWiFi();
    
    // ═══════════════════════════════════════════════════════════════
    // PASUL 2: Inițializare EEPROM
    // ═══════════════════════════════════════════════════════════════
    EEPROM.begin(EEPROM_SIZE);
    
    // ═══════════════════════════════════════════════════════════════
    // PASUL 3 & 4: Configurare hardware I/O
    // ═══════════════════════════════════════════════════════════════
    #if USE_I2C_EXPANDER
        expanderInit();
        
        // Configurare ZCD și IR pe GPIO direct
        pinMode(PIN_ZCD, INPUT);
        pinMode(PIN_IR_RECV, INPUT);
    #else
        // Configurare pini direct (mod 1 canal)
        pinMode(PIN_ZCD, INPUT);
        pinMode(PIN_TRIAC1, OUTPUT);
        pinMode(PIN_BUTTON1, INPUT);
        pinMode(PIN_IR_RECV, INPUT);
        
        digitalWrite(PIN_TRIAC1, LOW);
    #endif
    
    // ═══════════════════════════════════════════════════════════════
    // PASUL 5: Atașare întreruperi
    // ═══════════════════════════════════════════════════════════════
    // Întrerupere ZCD
    attachInterrupt(digitalPinToInterrupt(PIN_ZCD), zeroCrossISR, RISING);
    
    // Întrerupere IR
    attachInterrupt(digitalPinToInterrupt(PIN_IR_RECV), irISR, CHANGE);
    
    // ═══════════════════════════════════════════════════════════════
    // PASUL 6: Încărcare setări din EEPROM
    // ═══════════════════════════════════════════════════════════════
    loadSettings();
    
    // ═══════════════════════════════════════════════════════════════
    // PASUL 7: Inițializare structuri de stare butoane
    // ═══════════════════════════════════════════════════════════════
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
}

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                        SECȚIUNEA 7: FUNCȚII EEPROM                            ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Persistență date - niveluri dimming și coduri IR                             ║
 * ║  IMPORTANT: Apelează EEPROM.commit() după scriere!                            ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== EEPROM FUNCTIONS ====================

/**
 * @brief Încarcă toate setările din EEPROM
 * 
 * Verifică byte-ul magic pentru validitate.
 * Dacă EEPROM-ul nu este inițializat, scrie valori implicite.
 * Încarcă nivelurile de dimming și codurile IR pentru ambele canale.
 */
void loadSettings() {
    // Verifică dacă EEPROM-ul conține date valide
    if (EEPROM.read(EEPROM_MAGIC) != EEPROM_MAGIC_VALUE) {
        // Prima utilizare sau date corupte - inițializează cu valori implicite
        saveDefaultSettings();
    }
    
    // Încarcă nivelurile de dimming
    bulb[0].savedLevel = EEPROM.read(EEPROM_DIM1);
    bulb[1].savedLevel = EEPROM.read(EEPROM_DIM2);
    
    // Validare valori (în caz de corupție parțială)
    if (bulb[0].savedLevel < DIM_MIN || bulb[0].savedLevel > DIM_MAX) {
        bulb[0].savedLevel = DIM_DEFAULT;
    }
    if (bulb[1].savedLevel < DIM_MIN || bulb[1].savedLevel > DIM_MAX) {
        bulb[1].savedLevel = DIM_DEFAULT;
    }
    
    // Inițializează stările becurilor
    for (int i = 0; i < 2; i++) {
        bulb[i].isOn = false;
        bulb[i].currentLevel = 0;
        bulb[i].targetLevel = bulb[i].savedLevel;
    }
    
    // Încarcă codurile IR
    loadIRCode(0);
    loadIRCode(1);
}

/**
 * @brief Salvează setările implicite în EEPROM
 * 
 * Apelată la prima utilizare sau când datele sunt corupte.
 * Scrie byte-ul magic și valorile implicite pentru toate setările.
 */
void saveDefaultSettings() {
    EEPROM.write(EEPROM_MAGIC, EEPROM_MAGIC_VALUE);
    EEPROM.write(EEPROM_DIM1, DIM_DEFAULT);
    EEPROM.write(EEPROM_DIM2, DIM_DEFAULT);
    EEPROM.write(EEPROM_IR1_LEN, 0);   // Nici un cod IR salvat
    EEPROM.write(EEPROM_IR2_LEN, 0);
    EEPROM.commit();                    // IMPORTANT: Salvează efectiv în flash!
}

/**
 * @brief Salvează nivelul de dimming pentru un canal
 * 
 * @param bulbIndex Index canal (0 sau 1)
 * 
 * Apelată după ce utilizatorul setează un nivel nou prin apăsare lungă.
 */
void saveDimmingLevel(uint8_t bulbIndex) {
    if (bulbIndex == 0) {
        EEPROM.write(EEPROM_DIM1, bulb[0].savedLevel);
    } else {
        EEPROM.write(EEPROM_DIM2, bulb[1].savedLevel);
    }
    EEPROM.commit();
}

/**
 * @brief Încarcă codul IR pentru un canal din EEPROM
 * 
 * @param bulbIndex Index canal (0 sau 1)
 * 
 * Citește lungimea și timing-urile codului IR salvat.
 * Setează flag-ul valid pe false dacă nu există cod salvat.
 */
void loadIRCode(uint8_t bulbIndex) {
    uint8_t lenAddr = (bulbIndex == 0) ? EEPROM_IR1_LEN : EEPROM_IR2_LEN;
    uint8_t dataAddr = (bulbIndex == 0) ? EEPROM_IR1_DATA : EEPROM_IR2_DATA;
    
    uint8_t len = EEPROM.read(lenAddr);
    
    if (len > 0 && len <= IR_MAX_LEN) {
        bulb[bulbIndex].irCode.length = len;
        bulb[bulbIndex].irCode.valid = true;
        
        // Citește timing-urile (2 bytes per valoare, little-endian)
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

/**
 * @brief Salvează un cod IR învățat în EEPROM
 * 
 * @param bulbIndex Index canal (0 sau 1)
 * @param timing    Array cu timing-urile de salvat
 * @param length    Numărul de elemente din array
 * 
 * Salvează lungimea și timing-urile codului IR.
 * Actualizează și structura din memorie.
 */
void saveIRCode(uint8_t bulbIndex, uint16_t* timing, uint8_t length) {
    uint8_t lenAddr = (bulbIndex == 0) ? EEPROM_IR1_LEN : EEPROM_IR2_LEN;
    uint8_t dataAddr = (bulbIndex == 0) ? EEPROM_IR1_DATA : EEPROM_IR2_DATA;
    
    EEPROM.write(lenAddr, length);
    
    // Salvează timing-urile (2 bytes per valoare, little-endian)
    for (uint8_t i = 0; i < length && i < IR_MAX_LEN; i++) {
        EEPROM.write(dataAddr + i * 2, timing[i] & 0xFF);
        EEPROM.write(dataAddr + i * 2 + 1, (timing[i] >> 8) & 0xFF);
    }
    EEPROM.commit();
    
    // Actualizează și structura din RAM
    bulb[bulbIndex].irCode.length = length;
    bulb[bulbIndex].irCode.valid = true;
    for (uint8_t i = 0; i < length; i++) {
        bulb[bulbIndex].irCode.timing[i] = timing[i];
    }
}

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                     SECȚIUNEA 8: CONTROL TRIAC/DIMMING                        ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Funcții pentru controlul puterii becurilor prin phase-cut dimming            ║
 * ║                                                                               ║
 * ║  PRINCIPIU DE FUNCȚIONARE:                                                    ║
 * ║  ┌────────────────────────────────────────────────────────────┐              ║
 * ║  │ AC Waveform        ZCD         Triac Fire Point            │              ║
 * ║  │                     │                │                     │              ║
 * ║  │     ╱╲             │                │    ╱╲               │              ║
 * ║  │    ╱  ╲            │                │   ╱  ╲              │              ║
 * ║  │───╱────╲───────────┼────────────────┼──╱────╲─────────    │              ║
 * ║  │  ╱      ╲          │◄──── delay ───►│ ╱      ╲            │              ║
 * ║  │ ╱        ╲         │                │╱        ╲           │              ║
 * ║  │                                                            │              ║
 * ║  │  delay mic = putere mare (bec strălucitor)                │              ║
 * ║  │  delay mare = putere mică (bec slab)                      │              ║
 * ║  └────────────────────────────────────────────────────────────┘              ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== TRIAC CONTROL ====================

/**
 * @brief Setează starea unui triac (ON/OFF)
 * 
 * @param triacIndex Index canal (0 sau 1)
 * @param state      true = aprinde, false = stinge
 * 
 * Funcție de abstractizare pentru controlul hardware-ului.
 * Gestionează diferența între modul expander și modul direct.
 */
void setTriac(uint8_t triacIndex, bool state) {
    #if USE_I2C_EXPANDER
        expanderWrite((triacIndex == 0) ? EXP_TRIAC1 : EXP_TRIAC2, state);
    #else
        if (triacIndex == 0) {
            digitalWrite(PIN_TRIAC1, state ? HIGH : LOW);
        }
    #endif
}

// ==================== DIMMING FUNCTIONS ====================
void updateDimming() {
    if (!zeroCrossing) return;
    zeroCrossing = false;
    
    // Oprește triacurile la zero crossing
    setTriac(0, false);
    setTriac(1, false);
    
    for (int i = 0; i < 2; i++) {
        if (bulb[i].currentLevel > 0) {
            unsigned long delayTime = map(bulb[i].currentLevel, 1, 100, 
                                          HALF_CYCLE_US - 500, 500);
            
            unsigned long fireTime = lastZeroCross + delayTime;
            
            while (micros() < fireTime) {
                if (micros() - lastZeroCross > HALF_CYCLE_US) break;
                yield();  // ESP8266 watchdog
            }
            
            setTriac(i, true);
            delayMicroseconds(TRIAC_PULSE_US);
            setTriac(i, false);
        }
    }
}

/**
 * @brief Pornește soft un bec de la 0 la nivelul salvat
 * 
 * @param bulbIndex Index canal (0 sau 1)
 * 
 * Crește gradual luminozitatea pentru efect vizual plăcut
 * și pentru a reduce stresul termic asupra filamentului.
 * 
 * MODIFICARE VITEZĂ: Ajustează SOFT_STEP_DELAY_MS
 */
void softStart(uint8_t bulbIndex) {
    bulb[bulbIndex].isOn = true;
    bulb[bulbIndex].targetLevel = bulb[bulbIndex].savedLevel;
    
    while (bulb[bulbIndex].currentLevel < bulb[bulbIndex].targetLevel) {
        bulb[bulbIndex].currentLevel++;
        updateDimming();
        delay(SOFT_STEP_DELAY_MS);
        
        // Procesează zero crossing în timpul ramp-ului
        for (int i = 0; i < 5; i++) {
            if (zeroCrossing) updateDimming();
            delayMicroseconds(2000);
            yield();  // ESP8266 watchdog
        }
    }
}

/**
 * @brief Oprește soft un bec de la nivelul curent la 0
 * 
 * @param bulbIndex Index canal (0 sau 1)
 * 
 * Scade gradual luminozitatea pentru efect vizual plăcut.
 */
void softStop(uint8_t bulbIndex) {
    while (bulb[bulbIndex].currentLevel > 0) {
        bulb[bulbIndex].currentLevel--;
        updateDimming();
        delay(SOFT_STEP_DELAY_MS);
        
        for (int i = 0; i < 5; i++) {
            if (zeroCrossing) updateDimming();
            delayMicroseconds(2000);
            yield();
        }
    }
    
    bulb[bulbIndex].isOn = false;
}

/**
 * @brief Efectuează clipire de confirmare vizuală
 * 
 * @param bulbIndex Index canal (0 sau 1)
 * @param times     Numărul de clipiri
 * 
 * Folosită pentru feedback vizual:
 *   - 1 clipire: Intrare în mod învățare IR
 *   - 2 clipiri: Cod IR salvat cu succes
 * 
 * EXTENSIE: Poți adăuga pattern-uri diferite pentru alte confirmări:
 *   confirmationPattern(bulbIndex, pattern); // ex: SOS, etc.
 */
void confirmationBlink(uint8_t bulbIndex, uint8_t times) {
    for (uint8_t t = 0; t < times; t++) {
        // Ramp up la maxim
        while (bulb[bulbIndex].currentLevel < DIM_MAX) {
            bulb[bulbIndex].currentLevel++;
            updateDimming();
            delay(SOFT_STEP_DELAY_MS / 2);  // Mai rapid decât soft start normal
            for (int i = 0; i < 3; i++) {
                if (zeroCrossing) updateDimming();
                delayMicroseconds(1500);
                yield();
            }
        }
        
        delay(100);  // Menține aprins 100ms
        
        // Ramp down la zero
        while (bulb[bulbIndex].currentLevel > 0) {
            bulb[bulbIndex].currentLevel--;
            updateDimming();
            delay(SOFT_STEP_DELAY_MS / 2);
            for (int i = 0; i < 3; i++) {
                if (zeroCrossing) updateDimming();
                delayMicroseconds(1500);
                yield();
            }
        }
        
        if (t < times - 1) delay(200);  // Pauză între clipiri
    }
    
    bulb[bulbIndex].isOn = false;
}

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                      SECȚIUNEA 9: FUNCȚII BUTOANE                             ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Citire și procesare butoane touch TTP223                                     ║
 * ║                                                                               ║
 * ║  DIAGRAMA STĂRI BUTON:                                                        ║
 * ║  ┌─────────────────────────────────────────────────────────────────────┐     ║
 * ║  │                                                                     │     ║
 * ║  │   IDLE ──► PRESSED ──► RELEASED ──► WAIT_MORE_CLICKS ──► ACTION   │     ║
 * ║  │     │          │                            │                │     │     ║
 * ║  │     │          │ >800ms                     │ timeout        │     │     ║
 * ║  │     │          ▼                            │                │     │     ║
 * ║  │     │    LONG_PRESS ──► RAMP ──► SAVE      │                │     │     ║
 * ║  │     │                                       │                │     │     ║
 * ║  │     │                                       │ 3 clicks       │     │     ║
 * ║  │     │                                       ▼                │     │     ║
 * ║  │     │                                  IR_LEARN_MODE         │     │     ║
 * ║  │     │                                                        │     │     ║
 * ║  │     └────────────────────────────────────────────────────────┘     │     ║
 * ║  └─────────────────────────────────────────────────────────────────────┘     ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== BUTTON FUNCTIONS ====================

/**
 * @brief Citește starea unui buton
 * 
 * @param buttonIndex Index buton (0 sau 1)
 * @return Starea butonului (true = apăsat)
 * 
 * Abstractizează diferența între modul expander și direct.
 */
bool readButton(uint8_t buttonIndex) {
    #if USE_I2C_EXPANDER
        return expanderRead((buttonIndex == 0) ? EXP_BUTTON1 : EXP_BUTTON2);
    #else
        if (buttonIndex == 0) {
            return digitalRead(PIN_BUTTON1);
        }
        return false;  // Canal 2 nu există în modul direct
    #endif
}

/**
 * @brief Citește și procesează toate butoanele
 * 
 * Apelată în loop() pentru a detecta și procesa:
 *   - Click scurt (cu așteptare pentru multi-click)
 *   - Apăsare lungă (cu ramp dimming)
 *   - Triple-click (intrare mod învățare IR)
 */
void readButtons() {
    unsigned long now = millis();
    
    #if USE_I2C_EXPANDER
        int numButtons = 2;
    #else
        int numButtons = 1;
    #endif
    
    for (int i = 0; i < numButtons; i++) {
        bool reading = readButton(i);
        
        // Debounce simplu - ignoră schimbările prea rapide
        if (reading != button[i].lastState) {
            button[i].lastState = reading;
            continue;
        }
        
        // Detectare apăsare (front crescător)
        if (reading && !button[i].currentState) {
            button[i].currentState = true;
            button[i].pressStart = now;
            button[i].longPressActive = false;
        }
        
        // Detectare eliberare (front descrescător)
        if (!reading && button[i].currentState) {
            button[i].currentState = false;
            
            if (!button[i].longPressActive) {
                handleShortPress(i, now);
            } else {
                handleLongPressEnd(i);
            }
        }
        
        // Verificare apăsare lungă în timp ce butonul e apăsat
        if (button[i].currentState && !button[i].longPressActive) {
            if (now - button[i].pressStart >= LONG_PRESS_MS) {
                button[i].longPressActive = true;
                handleLongPressStart(i);
            }
        }
        
        // Continuare ramp în timpul apăsării lungi
        if (button[i].currentState && button[i].longPressActive) {
            handleLongPressHold(i, now);
        }
    }
    
    processClickWaiting(now);
}

/**
 * @brief Procesează un click scurt pe buton
 * 
 * @param bulbIndex Index buton/canal
 * @param now       Timestamp curent (ms)
 * 
 * Incrementează contorul de clickuri și începe așteptarea.
 * Nu execută acțiunea imediat - așteaptă să vadă dacă urmează alte clickuri.
 */
void handleShortPress(uint8_t bulbIndex, unsigned long now) {
    // Verifică dacă e în fereastra de multi-click
    if (now - button[bulbIndex].lastRelease < TRIPLE_CLICK_WINDOW) {
        button[bulbIndex].clickCount++;
    } else {
        button[bulbIndex].clickCount = 1;  // Primul click din serie nouă
    }
    
    button[bulbIndex].lastRelease = now;
    
    // Începe așteptarea pentru alte clickuri
    if (!button[bulbIndex].waitingForMoreClicks) {
        button[bulbIndex].waitingForMoreClicks = true;
        button[bulbIndex].waitStartTime = now;
    }
}

/**
 * @brief Procesează așteptarea pentru multi-click
 * 
 * @param now Timestamp curent (ms)
 * 
 * După CLICK_WAIT_MS, decide ce acțiune să execute:
 *   - 3+ clickuri cu bec stins → mod învățare IR
 *   - Altfel → toggle on/off
 */
void processClickWaiting(unsigned long now) {
    #if USE_I2C_EXPANDER
        int numButtons = 2;
    #else
        int numButtons = 1;
    #endif
    
    for (int i = 0; i < numButtons; i++) {
        if (button[i].waitingForMoreClicks) {
            if (now - button[i].waitStartTime >= CLICK_WAIT_MS) {
                button[i].waitingForMoreClicks = false;
                
                // Decizie: triple-click sau toggle?
                if (button[i].clickCount >= 3 && !bulb[i].isOn) {
                    // Triple-click cu bec stins → mod învățare IR
                    startIRLearning(i);
                } else {
                    // Click simplu → toggle on/off
                    toggleBulb(i);
                }
                
                button[i].clickCount = 0;
            }
        }
    }
}

/**
 * @brief Toggle on/off pentru un canal
 * 
 * @param bulbIndex Index canal (0 sau 1)
 */
void toggleBulb(uint8_t bulbIndex) {
    if (bulb[bulbIndex].isOn) {
        softStop(bulbIndex);
    } else {
        softStart(bulbIndex);
    }
}

/**
 * @brief Începe procesarea apăsării lungi
 * 
 * @param bulbIndex Index canal (0 sau 1)
 * 
 * Apelată când apăsarea depășește LONG_PRESS_MS.
 * Dacă becul e stins, îl aprinde la nivelul salvat.
 * Inițializează direcția ramp (up).
 */
void handleLongPressStart(uint8_t bulbIndex) {
    // Anulează orice așteptare pentru multi-click
    button[bulbIndex].clickCount = 0;
    button[bulbIndex].waitingForMoreClicks = false;
    
    // Dacă becul e stins, îl aprindem
    if (!bulb[bulbIndex].isOn) {
        bulb[bulbIndex].isOn = true;
        bulb[bulbIndex].currentLevel = bulb[bulbIndex].savedLevel;
    }
    
    // Începe ramp-ul de la nivelul curent
    rampingUp = true;
    lastRampStep = millis();
}

/**
 * @brief Continuă ramp-ul în timpul apăsării lungi
 * 
 * @param bulbIndex Index canal (0 sau 1)
 * @param now       Timestamp curent (ms)
 * 
 * Mișcă nivelul de dimming sus/jos cu RAMP_DELAY_MS între pași.
 * Schimbă direcția când atinge limitele (DIM_MIN, DIM_MAX).
 */
void handleLongPressHold(uint8_t bulbIndex, unsigned long now) {
    if (now - lastRampStep >= RAMP_DELAY_MS) {
        lastRampStep = now;
        
        if (rampingUp) {
            if (bulb[bulbIndex].currentLevel < DIM_MAX) {
                bulb[bulbIndex].currentLevel++;
            } else {
                rampingUp = false;  // Schimbă direcția la maxim
            }
        } else {
            if (bulb[bulbIndex].currentLevel > DIM_MIN) {
                bulb[bulbIndex].currentLevel--;
            } else {
                rampingUp = true;   // Schimbă direcția la minim
            }
        }
    }
}

/**
 * @brief Finalizează apăsarea lungă - salvează nivelul
 * 
 * @param bulbIndex Index canal (0 sau 1)
 * 
 * Apelată la eliberarea butonului după apăsare lungă.
 * Salvează nivelul curent în EEPROM pentru utilizare viitoare.
 */
void handleLongPressEnd(uint8_t bulbIndex) {
    bulb[bulbIndex].savedLevel = bulb[bulbIndex].currentLevel;
    saveDimmingLevel(bulbIndex);
}

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                       SECȚIUNEA 10: FUNCȚII IR                                ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Învățare, comparare și procesare coduri IR                                   ║
 * ║                                                                               ║
 * ║  PROCESUL DE ÎNVĂȚARE IR:                                                     ║
 * ║  ┌──────────────────────────────────────────────────────────────────────┐    ║
 * ║  │                                                                      │    ║
 * ║  │   [Triple-Click] → [Confirmare 1x] → [Așteptare 10s] → [Recepție]   │    ║
 * ║  │                                              │                       │    ║
 * ║  │                                              ▼                       │    ║
 * ║  │                    [Timeout] ←──── sau ────► [Cod primit]           │    ║
 * ║  │                        │                         │                   │    ║
 * ║  │                        ▼                         ▼                   │    ║
 * ║  │                   [Ieșire]              [Salvare EEPROM]             │    ║
 * ║  │                                              │                       │    ║
 * ║  │                                              ▼                       │    ║
 * ║  │                                     [Confirmare 2x]                  │    ║
 * ║  │                                                                      │    ║
 * ║  └──────────────────────────────────────────────────────────────────────┘    ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== IR FUNCTIONS ====================

/**
 * @brief Intră în modul de învățare IR pentru un canal
 * 
 * @param bulbIndex Index canal (0 sau 1)
 * 
 * Efectuează confirmare vizuală (1 clipire) și așteaptă
 * un cod IR timp de IR_LEARN_TIMEOUT ms.
 */
void startIRLearning(uint8_t bulbIndex) {
    // Confirmare vizuală - intrare în mod învățare
    confirmationBlink(bulbIndex, 1);
    
    // Activare mod învățare
    irLearningMode = true;
    irLearningBulb = bulbIndex;
    irLearningStart = millis();
    
    // Resetare buffer IR
    irBufferIndex = 0;
    irCodeReady = false;
    irReceiving = false;
}

/**
 * @brief Procesează modul de învățare IR
 * 
 * Verifică timeout și recepția codului.
 * Dacă primește cod valid, îl salvează și confirmă vizual (2 clipiri).
 * 
 * Apelată în loop() când irLearningMode == true.
 */
void processIRLearning() {
    if (!irLearningMode) return;
    
    unsigned long now = millis();
    
    // Verificare timeout
    if (now - irLearningStart >= IR_LEARN_TIMEOUT) {
        irLearningMode = false;
        return;  // Ieșire fără confirmare (timeout)
    }
    
    // Verificare cod primit
    if (irCodeReady) {
        irCodeReady = false;
        
        if (irBufferIndex >= IR_MIN_PULSES) {
            // Copiază buffer-ul (critical section)
            uint16_t tempBuffer[IR_MAX_LEN];
            noInterrupts();
            memcpy(tempBuffer, (const void*)irBuffer, irBufferIndex * sizeof(uint16_t));
            uint8_t len = irBufferIndex;
            irBufferIndex = 0;
            interrupts();
            
            // Salvează codul în EEPROM
            saveIRCode(irLearningBulb, tempBuffer, len);
            
            // Confirmare vizuală - cod salvat cu succes
            confirmationBlink(irLearningBulb, 2);
        }
        
        irLearningMode = false;
    }
}

/**
 * @brief Compară un cod IR primit cu unul salvat
 * 
 * @param received    Array cu timing-urile primite
 * @param receivedLen Lungimea array-ului primit
 * @param stored      Pointer la codul salvat
 * @return true dacă codurile se potrivesc, false altfel
 * 
 * Folosește comparare cu toleranță (IR_TOLERANCE %).
 * Compară doar primele 20 de timing-uri pentru eficiență.
 * Consideră potrivire dacă >70% din timing-uri corespund.
 * 
 * AJUSTARE SENSIBILITATE:
 *   - Mărește IR_TOLERANCE dacă telecomanda nu e recunoscută
 *   - Micșorează dacă apar false positives
 */
bool compareIRCodes(uint16_t* received, uint8_t receivedLen, IRCode* stored) {
    if (!stored->valid) return false;
    if (receivedLen < IR_MIN_PULSES) return false;
    
    // Compară doar primele N timing-uri
    uint8_t compareLen = min(receivedLen, stored->length);
    compareLen = min(compareLen, (uint8_t)20);  // Max 20 pentru eficiență
    
    int matches = 0;
    for (uint8_t i = 0; i < compareLen; i++) {
        uint16_t storedVal = stored->timing[i];
        uint16_t recvVal = received[i];
        
        // Calculează toleranța ca procent din valoarea salvată
        uint16_t tolerance = (storedVal * IR_TOLERANCE) / 100;
        
        if (recvVal >= storedVal - tolerance && recvVal <= storedVal + tolerance) {
            matches++;
        }
    }
    
    // Potrivire dacă >70% corespund
    return (matches * 100 / compareLen) >= 70;
}

/**
 * @brief Procesează codurile IR de la telecomandă
 * 
 * Verifică dacă codul primit corespunde cu vreun canal
 * și execută acțiunea corespunzătoare (toggle sau ramp).
 * 
 * Nu procesează dacă suntem în mod învățare IR.
 */
void processIRRemote() {
    if (irLearningMode) return;  // Ignoră în mod învățare
    
    // Verifică timeout recepție (cod complet)
    if (!irCodeReady) {
        if (irReceiving && (micros() - irLastEdge > IR_TIMEOUT_US)) {
            if (irBufferIndex >= IR_MIN_PULSES) {
                irCodeReady = true;
            }
            irReceiving = false;
        }
        
        if (!irCodeReady) return;
    }
    
    irCodeReady = false;
    
    // Copiază buffer-ul pentru procesare (critical section)
    uint16_t tempBuffer[IR_MAX_LEN];
    noInterrupts();
    memcpy(tempBuffer, (const void*)irBuffer, irBufferIndex * sizeof(uint16_t));
    uint8_t len = irBufferIndex;
    irBufferIndex = 0;
    interrupts();
    
    // Compară cu codurile salvate pentru fiecare canal
    for (int i = 0; i < 2; i++) {
        if (compareIRCodes(tempBuffer, len, &bulb[i].irCode)) {
            handleIRCommand(i);
            break;  // Execută doar prima potrivire
        }
    }
}

/**
 * @brief Procesează o comandă IR pentru un canal
 * 
 * @param bulbIndex Index canal (0 sau 1)
 * 
 * Gestionează apăsarea scurtă vs lungă pe telecomandă:
 *   - Apăsare scurtă → toggle on/off
 *   - Apăsare lungă → ramp dimming (ca butonul fizic)
 * 
 * Detectează apăsarea lungă prin primirea repetată a codului IR
 * (majoritatea telecomenzilor trimit repeat codes la apăsare lungă).
 */
void handleIRCommand(uint8_t bulbIndex) {
    unsigned long now = millis();
    
    // Verifică dacă este repeat (apăsare continuă)
    if (now - irLastReceived[bulbIndex] < IR_REPEAT_TIMEOUT) {
        if (irLongPressActive[bulbIndex]) {
            // Continuă ramp-ul
            handleLongPressHold(bulbIndex, now);
        }
    } else {
        // Apăsare nouă
        irPressStart[bulbIndex] = now;
        irLongPressActive[bulbIndex] = false;
    }
    
    irLastReceived[bulbIndex] = now;
    
    // Verifică tranziția la apăsare lungă
    if (!irLongPressActive[bulbIndex] && 
        (now - irPressStart[bulbIndex] >= IR_LONG_PRESS_MS)) {
        irLongPressActive[bulbIndex] = true;
        handleLongPressStart(bulbIndex);
    }
}

/**
 * @brief Procesează timeout-ul IR pentru a detecta sfârșitul apăsării
 * 
 * Când nu mai primim coduri IR, considerăm că butonul a fost eliberat.
 * Dacă era apăsare scurtă → toggle
 * Dacă era apăsare lungă → salvează nivelul
 */
void processIRTimeout() {
    unsigned long now = millis();
    
    for (int i = 0; i < 2; i++) {
        // Verifică sfârșitul apăsării lungi
        if (irLongPressActive[i] && (now - irLastReceived[i] > IR_REPEAT_TIMEOUT * 2)) {
            irLongPressActive[i] = false;
            
            // Salvează nivelul setat
            if (bulb[i].isOn) {
                bulb[i].savedLevel = bulb[i].currentLevel;
                saveDimmingLevel(i);
            }
        }
        
        // Verifică apăsare scurtă finalizată (fără să devină lungă)
        if (!irLongPressActive[i] && irPressStart[i] > 0 && 
            (now - irLastReceived[i] > IR_REPEAT_TIMEOUT * 2) &&
            (irLastReceived[i] - irPressStart[i] < IR_LONG_PRESS_MS)) {
            // A fost apăsare scurtă → toggle
            toggleBulb(i);
            irPressStart[i] = 0;  // Resetează pentru a nu repeta
        }
    }
}

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                    SECȚIUNEA 11: ECONOMIE DE ENERGIE                          ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Funcții pentru reducerea consumului când becurile sunt stinse                ║
 * ║                                                                               ║
 * ║  MODURI DE SLEEP ESP8266:                                                     ║
 * ║  ┌──────────────────┬──────────────┬────────────────────────────────────┐    ║
 * ║  │ Mod              │ Consum       │ Wake-up                            │    ║
 * ║  ├──────────────────┼──────────────┼────────────────────────────────────┤    ║
 * ║  │ Modem Sleep      │ ~15mA        │ Oricând                            │    ║
 * ║  │ Light Sleep      │ ~0.5mA       │ GPIO, Timer                        │    ║
 * ║  │ Deep Sleep       │ ~20µA        │ Reset (pierde stare!)              │    ║
 * ║  └──────────────────┴──────────────┴────────────────────────────────────┘    ║
 * ║                                                                               ║
 * ║  Folosim Light Sleep pentru a păstra starea și a permite wake-up rapid.      ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== LIGHT SLEEP ====================

/**
 * @brief Intră în modul Light Sleep pentru economie de energie
 * 
 * Verifică dacă sunt îndeplinite condițiile pentru sleep:
 *   - Ambele becuri stinse
 *   - Nu suntem în mod învățare IR
 *   - Nu așteptăm alte clickuri
 * 
 * Configurează wake-up pe:
 *   - Semnal ZCD (front crescător)
 *   - Semnal IR (orice tranziție)
 * 
 * NOTĂ: Butoanele pe I2C nu pot trezi direct din sleep.
 *       Wake-up-ul pe ZCD permite verificarea periodică a butoanelor.
 */
void enterLightSleep() {
    // Verifică condițiile pentru sleep
    if (bulb[0].isOn || bulb[1].isOn || irLearningMode) {
        return;
    }
    
    for (int i = 0; i < 2; i++) {
        if (button[i].waitingForMoreClicks) return;
    }
    
    // Asigură-te că triacurile sunt oprite
    setTriac(0, false);
    setTriac(1, false);
    
    // Configurare Light Sleep ESP8266
    wifi_set_sleep_type(LIGHT_SLEEP_T);
    
    // Configurare GPIO pentru wake-up
    GPIO_DIS_OUTPUT(PIN_ZCD);
    gpio_pin_wakeup_enable(GPIO_ID_PIN(PIN_ZCD), GPIO_PIN_INTR_POSEDGE);
    gpio_pin_wakeup_enable(GPIO_ID_PIN(PIN_IR_RECV), GPIO_PIN_INTR_ANYEDGE);
    
    // Mică pauză pentru stabilizare
    delay(100);
    
    // La wake-up, execuția continuă de aici
}

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                       SECȚIUNEA 12: LOOP PRINCIPAL                            ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║  Bucla principală - orchestrează toate funcționalitățile                      ║
 * ║                                                                               ║
 * ║  FLUX DE EXECUȚIE:                                                            ║
 * ║  ┌────────────────────────────────────────────────────────────────────────┐  ║
 * ║  │                                                                        │  ║
 * ║  │   ┌──────────────┐                                                     │  ║
 * ║  │   │ updateDimming│◄─── Procesare zero crossing, aprindere triace      │  ║
 * ║  │   └──────┬───────┘                                                     │  ║
 * ║  │          │                                                             │  ║
 * ║  │          ▼                                                             │  ║
 * ║  │   ┌──────────────┐                                                     │  ║
 * ║  │   │ readButtons  │◄─── Citire și procesare butoane touch              │  ║
 * ║  │   └──────┬───────┘                                                     │  ║
 * ║  │          │                                                             │  ║
 * ║  │          ▼                                                             │  ║
 * ║  │   ┌────────────────┐                                                   │  ║
 * ║  │   │processIRLearning│◄─── Procesare mod învățare IR (dacă activ)       │  ║
 * ║  │   └──────┬─────────┘                                                   │  ║
 * ║  │          │                                                             │  ║
 * ║  │          ▼                                                             │  ║
 * ║  │   ┌──────────────┐                                                     │  ║
 * ║  │   │processIRRemote│◄─── Procesare comenzi telecomandă                  │  ║
 * ║  │   └──────┬───────┘                                                     │  ║
 * ║  │          │                                                             │  ║
 * ║  │          ▼                                                             │  ║
 * ║  │   ┌───────────────┐                                                    │  ║
 * ║  │   │processIRTimeout│◄─── Detectare eliberare buton telecomandă        │  ║
 * ║  │   └──────┬────────┘                                                    │  ║
 * ║  │          │                                                             │  ║
 * ║  │          ▼                                                             │  ║
 * ║  │   ┌───────────────┐                                                    │  ║
 * ║  │   │enterLightSleep│◄─── Sleep dacă inactiv (opțional)                 │  ║
 * ║  │   └──────┬────────┘                                                    │  ║
 * ║  │          │                                                             │  ║
 * ║  │          ▼                                                             │  ║
 * ║  │   ┌──────────────┐                                                     │  ║
 * ║  │   │    yield()   │◄─── Feed watchdog ESP8266                          │  ║
 * ║  │   └──────┬───────┘                                                     │  ║
 * ║  │          │                                                             │  ║
 * ║  │          └─────────────────────────────────────────────────────────┐   │  ║
 * ║  │                                                                    │   │  ║
 * ║  └────────────────────────────────────────────────────────────────────┘   │  ║
 * │                                                                            │  ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

// ==================== LOOP PRINCIPAL ====================
/**
 * @brief Bucla principală a programului
 * 
 * Execută continuu toate operațiunile sistemului.
 * IMPORTANT: Nu pune delay-uri lungi aici - afectează dimming-ul!
 */
void loop() {
    // ═══════════════════════════════════════════════════════════════
    // 1. DIMMING - Procesează zero crossing și aprinde triacurile
    // ═══════════════════════════════════════════════════════════════
    updateDimming();
    
    // ═══════════════════════════════════════════════════════════════
    // 2. BUTOANE - Citește și procesează apăsările
    // ═══════════════════════════════════════════════════════════════
    readButtons();
    
    // ═══════════════════════════════════════════════════════════════
    // 3. IR - Procesează învățare și comenzi telecomandă
    // ═══════════════════════════════════════════════════════════════
    processIRLearning();
    processIRRemote();
    processIRTimeout();
    
    // ═══════════════════════════════════════════════════════════════
    // 4. SLEEP - Intră în sleep dacă inactiv
    // ═══════════════════════════════════════════════════════════════
    static unsigned long lastActivity = 0;
    if (!bulb[0].isOn && !bulb[1].isOn && !irLearningMode) {
        if (millis() - lastActivity > 2000) {  // 2 secunde inactivitate
            enterLightSleep();
            lastActivity = millis();
        }
    } else {
        lastActivity = millis();
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 5. WATCHDOG - Previne reset-ul ESP8266
    // ═══════════════════════════════════════════════════════════════
    yield();
}

/*
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                         SFÂRȘITUL CODULUI PRINCIPAL                           ║
 * ╠═══════════════════════════════════════════════════════════════════════════════╣
 * ║                                                                               ║
 * ║  SPAȚIU PENTRU EXTENSII:                                                      ║
 * ║  ─────────────────────────────────────────────────────────────────────────    ║
 * ║                                                                               ║
 * ║  Adaugă funcții noi mai jos. Exemple de extensii posibile:                    ║
 * ║                                                                               ║
 * ║  1. MOD NOAPTE - Reducere automată nivel la ore târzii                        ║
 * ║     void nightModeCheck() { ... }                                             ║
 * ║                                                                               ║
 * ║  2. SCENE PREDEFINITE - Niveluri memorate pentru ambele becuri                ║
 * ║     void activateScene(uint8_t sceneIndex) { ... }                            ║
 * ║                                                                               ║
 * ║  3. FADE AUTOMATĂ - Tranziție lentă între niveluri                            ║
 * ║     void autoFade(uint8_t bulbIndex, uint8_t target, uint16_t duration) {...} ║
 * ║                                                                               ║
 * ║  4. SENZOR LUMINĂ - Ajustare automată în funcție de lumina ambientală         ║
 * ║     void adjustByAmbientLight() { ... }                                       ║
 * ║                                                                               ║
 * ║  5. TIMER - Oprire automată după un timp setat                                ║
 * ║     void checkTimers() { ... }                                                ║
 * ║                                                                               ║
 * ║  6. COMUNICAȚIE SERIAL - Debug sau control extern                             ║
 * ║     void processSerialCommands() { ... }                                      ║
 * ║                                                                               ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */