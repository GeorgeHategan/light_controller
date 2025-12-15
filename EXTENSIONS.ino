/*
 * ═══════════════════════════════════════════════════════════════════════════════
 *                    EXEMPLE DE EXTENSII - AC Dimmer Controller
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Acest fișier conține exemple de cod pentru a adăuga funcționalități noi.
 * Copiază și adaptează secțiunile de care ai nevoie în fișierul principal.
 *
 * CUPRINS:
 *   1. Adăugare Canal 3 (al treilea bec)
 *   2. Senzor de Lumină (auto-dimming)
 *   3. Mod Noapte
 *   4. Timer Auto-Off
 *   5. Scene Predefinite
 *   6. Fade Lent
 *   7. Comunicație Serială
 *   8. Salvare Scene în EEPROM
 *   9. Indicator LED Status
 *  10. Buton "All On/Off"
 */

// ═══════════════════════════════════════════════════════════════════════════════
// 1. ADĂUGARE CANAL 3 (AL TREILEA BEC)
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * MODIFICĂRI NECESARE:
 * 
 * a) La începutul fișierului, adaugă definițiile pentru pini:
 */

// #define PIN_TRIAC3      8    // Pin pentru triac-ul 3
// #define PIN_BUTTON3     9    // Pin pentru butonul 3

/*
 * b) Modifică numărul de canale:
 */

// #define NUM_CHANNELS    3    // Era 2, acum 3

/*
 * c) Mărește array-urile:
 */

// BulbState bulb[NUM_CHANNELS];
// ButtonState button[NUM_CHANNELS];
// const uint8_t triacPins[NUM_CHANNELS] = {PIN_TRIAC1, PIN_TRIAC2, PIN_TRIAC3};
// const uint8_t buttonPins[NUM_CHANNELS] = {PIN_BUTTON1, PIN_BUTTON2, PIN_BUTTON3};

/*
 * d) Adaugă adrese EEPROM pentru canalul 3:
 */

// #define EEPROM_DIM3         141
// #define EEPROM_IR3_LEN      142
// #define EEPROM_IR3_DATA     143   // Până la 143 + IR_MAX_LEN*2

/*
 * e) În setup(), inițializează pinii noi:
 */

// pinMode(PIN_TRIAC3, OUTPUT);
// pinMode(PIN_BUTTON3, INPUT);


// ═══════════════════════════════════════════════════════════════════════════════
// 2. SENZOR DE LUMINĂ (AUTO-DIMMING)
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * Ajustează automat luminozitatea în funcție de lumina ambientală.
 * Necesită: Fotorezistor (LDR) sau senzor BH1750
 */

#define PIN_LDR         A0      // Pin analog pentru LDR
#define AUTO_DIM_MIN    20      // Nivel minim auto
#define AUTO_DIM_MAX    90      // Nivel maxim auto
#define LDR_DARK        100     // Valoare ADC pentru întuneric
#define LDR_BRIGHT      900     // Valoare ADC pentru lumină puternică

bool autoDimEnabled = false;

uint16_t readAmbientLight() {
    // Citește de 10 ori și face media pentru stabilitate
    uint32_t total = 0;
    for (int i = 0; i < 10; i++) {
        total += analogRead(PIN_LDR);
        delay(1);
    }
    return total / 10;
}

uint8_t calculateAutoLevel() {
    uint16_t ambient = readAmbientLight();
    
    // Constrain la range-ul așteptat
    ambient = constrain(ambient, LDR_DARK, LDR_BRIGHT);
    
    // Inversare: mai întuneric afară = mai multă lumină de la becuri
    return map(ambient, LDR_DARK, LDR_BRIGHT, AUTO_DIM_MAX, AUTO_DIM_MIN);
}

void applyAutoDim() {
    if (!autoDimEnabled) return;
    
    uint8_t autoLevel = calculateAutoLevel();
    
    for (int i = 0; i < 2; i++) {
        if (bulb[i].isOn) {
            bulb[i].currentLevel = autoLevel;
        }
    }
}

// Apelează în loop() la fiecare 1 secundă
// static unsigned long lastAutoDim = 0;
// if (millis() - lastAutoDim > 1000) {
//     lastAutoDim = millis();
//     applyAutoDim();
// }


// ═══════════════════════════════════════════════════════════════════════════════
// 3. MOD NOAPTE
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * Reduce toate becurile la un nivel confortabil pentru noapte.
 * Activare: Ambele butoane apăsate simultan.
 */

#define NIGHT_MODE_LEVEL    15      // Nivel redus pentru noapte (15%)
#define BOTH_BUTTONS_MS     500     // Timp minim ambele butoane apăsate

bool nightModeActive = false;
uint8_t savedLevels[2];             // Salvează nivelurile anterioare
unsigned long bothButtonsStart = 0;

void checkNightModeToggle() {
    bool bothPressed = (digitalRead(PIN_BUTTON1) == HIGH) && 
                       (digitalRead(PIN_BUTTON2) == HIGH);
    
    if (bothPressed) {
        if (bothButtonsStart == 0) {
            bothButtonsStart = millis();
        } else if (millis() - bothButtonsStart > BOTH_BUTTONS_MS) {
            toggleNightMode();
            bothButtonsStart = 0;
            delay(500);  // Debounce
        }
    } else {
        bothButtonsStart = 0;
    }
}

void toggleNightMode() {
    nightModeActive = !nightModeActive;
    
    if (nightModeActive) {
        // Salvează nivelurile curente și aplică modul noapte
        for (int i = 0; i < 2; i++) {
            savedLevels[i] = bulb[i].currentLevel;
            if (bulb[i].isOn) {
                bulb[i].currentLevel = NIGHT_MODE_LEVEL;
            }
        }
        // Feedback: blink rapid
        blinkConfirmation(3);
    } else {
        // Restaurează nivelurile salvate
        for (int i = 0; i < 2; i++) {
            if (bulb[i].isOn) {
                bulb[i].currentLevel = savedLevels[i];
            }
        }
        // Feedback: blink lung
        blinkConfirmation(1);
    }
}

void blinkConfirmation(uint8_t times) {
    for (int t = 0; t < times; t++) {
        for (int i = 0; i < 2; i++) {
            if (bulb[i].isOn) {
                bulb[i].currentLevel = DIM_MAX;
            }
        }
        delay(100);
        for (int i = 0; i < 2; i++) {
            if (bulb[i].isOn) {
                bulb[i].currentLevel = nightModeActive ? NIGHT_MODE_LEVEL : savedLevels[i];
            }
        }
        delay(100);
    }
}


// ═══════════════════════════════════════════════════════════════════════════════
// 4. TIMER AUTO-OFF
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * Stinge automat becurile după un interval prestabilit.
 * Util pentru: economie energie, copii care uită lumina aprinsă
 */

#define AUTO_OFF_ENABLED    true
#define AUTO_OFF_MINUTES    60          // Timp în minute
#define AUTO_OFF_MS         (AUTO_OFF_MINUTES * 60000UL)

unsigned long bulbOnTimestamp[2] = {0, 0};

void resetAutoOffTimer(uint8_t bulbIndex) {
    bulbOnTimestamp[bulbIndex] = millis();
}

void checkAutoOff() {
    if (!AUTO_OFF_ENABLED) return;
    
    unsigned long now = millis();
    
    for (int i = 0; i < 2; i++) {
        if (bulb[i].isOn && bulbOnTimestamp[i] != 0) {
            if (now - bulbOnTimestamp[i] > AUTO_OFF_MS) {
                Serial.print("Auto-off bec ");
                Serial.println(i + 1);
                softStop(i);
                bulbOnTimestamp[i] = 0;
            }
        }
    }
}

// În softStart(), adaugă:
// resetAutoOffTimer(bulbIndex);

// În loop(), adaugă:
// checkAutoOff();


// ═══════════════════════════════════════════════════════════════════════════════
// 5. SCENE PREDEFINITE
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * Scene = combinații predefinite de niveluri pentru ambele becuri.
 * Activare prin secvențe speciale de butoane.
 */

typedef struct {
    const char* name;
    uint8_t level1;
    uint8_t level2;
} Scene;

// Definire scene
const Scene scenes[] = {
    {"Off",         0,   0},     // Scenă 0: Totul stins
    {"Film",        25,  0},     // Scenă 1: Lumină ambientală pentru film
    {"Lectură",     80,  30},    // Scenă 2: Lumină focalizată + ambient
    {"Romantic",    30,  30},    // Scenă 3: Lumină caldă redusă
    {"Full",        90,  90},    // Scenă 4: Lumină maximă
    {"Noapte",      10,  10},    // Scenă 5: Lumină minimă
};

#define NUM_SCENES  (sizeof(scenes) / sizeof(Scene))

uint8_t currentScene = 0;

void applyScene(uint8_t sceneIndex) {
    if (sceneIndex >= NUM_SCENES) return;
    
    currentScene = sceneIndex;
    const Scene* s = &scenes[sceneIndex];
    
    Serial.print("Scenă: ");
    Serial.println(s->name);
    
    // Bec 1
    if (s->level1 > 0) {
        bulb[0].isOn = true;
        bulb[0].targetLevel = s->level1;
        softRampTo(0, s->level1);
    } else {
        softStop(0);
    }
    
    // Bec 2
    if (s->level2 > 0) {
        bulb[1].isOn = true;
        bulb[1].targetLevel = s->level2;
        softRampTo(1, s->level2);
    } else {
        softStop(1);
    }
}

void nextScene() {
    currentScene = (currentScene + 1) % NUM_SCENES;
    applyScene(currentScene);
}

// Funcție helper pentru soft ramp
void softRampTo(uint8_t bulbIndex, uint8_t target) {
    while (bulb[bulbIndex].currentLevel != target) {
        if (bulb[bulbIndex].currentLevel < target) {
            bulb[bulbIndex].currentLevel++;
        } else {
            bulb[bulbIndex].currentLevel--;
        }
        updateDimming();
        delay(SOFT_STEP_DELAY_MS);
    }
}


// ═══════════════════════════════════════════════════════════════════════════════
// 6. FADE LENT (SUNRISE/SUNSET SIMULATION)
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * Simulează răsăritul/apusul soarelui cu un fade foarte lent.
 * Ideal pentru trezire naturală sau adormire.
 */

#define SUNRISE_DURATION_MIN    15      // Durata în minute
#define SUNSET_DURATION_MIN     10

void sunriseFade(uint8_t bulbIndex) {
    uint32_t totalMs = SUNRISE_DURATION_MIN * 60000UL;
    uint8_t steps = DIM_MAX - DIM_MIN;
    uint32_t stepDelay = totalMs / steps;
    
    Serial.println("Începe sunrise fade...");
    
    bulb[bulbIndex].isOn = true;
    bulb[bulbIndex].currentLevel = DIM_MIN;
    
    for (uint8_t level = DIM_MIN; level <= DIM_MAX; level++) {
        bulb[bulbIndex].currentLevel = level;
        updateDimming();
        
        // Delay lung cu verificare întrerupere
        unsigned long startStep = millis();
        while (millis() - startStep < stepDelay) {
            // Verifică dacă s-a apăsat butonul pentru anulare
            if (digitalRead(buttonPins[bulbIndex]) == HIGH) {
                Serial.println("Sunrise anulat!");
                return;
            }
            delay(100);
        }
    }
    
    Serial.println("Sunrise complet!");
}

void sunsetFade(uint8_t bulbIndex) {
    uint32_t totalMs = SUNSET_DURATION_MIN * 60000UL;
    uint8_t steps = bulb[bulbIndex].currentLevel - DIM_MIN;
    if (steps == 0) return;
    
    uint32_t stepDelay = totalMs / steps;
    
    Serial.println("Începe sunset fade...");
    
    for (int level = bulb[bulbIndex].currentLevel; level >= DIM_MIN; level--) {
        bulb[bulbIndex].currentLevel = level;
        updateDimming();
        
        unsigned long startStep = millis();
        while (millis() - startStep < stepDelay) {
            if (digitalRead(buttonPins[bulbIndex]) == HIGH) {
                Serial.println("Sunset anulat!");
                return;
            }
            delay(100);
        }
    }
    
    softStop(bulbIndex);
    Serial.println("Sunset complet - bec stins!");
}


// ═══════════════════════════════════════════════════════════════════════════════
// 7. COMUNICAȚIE SERIALĂ (COMANDĂ DE PE PC)
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * Permite controlul prin comenzi seriale de pe PC.
 * Format: COMANDĂ PARAMETRU
 */

void processSerialCommand() {
    if (!Serial.available()) return;
    
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();
    
    Serial.print("CMD: ");
    Serial.println(cmd);
    
    // Parsare comandă
    int spaceIdx = cmd.indexOf(' ');
    String command = (spaceIdx > 0) ? cmd.substring(0, spaceIdx) : cmd;
    String param = (spaceIdx > 0) ? cmd.substring(spaceIdx + 1) : "";
    
    if (command == "ON") {
        // ON 1 sau ON 2 sau ON (ambele)
        if (param == "1") softStart(0);
        else if (param == "2") softStart(1);
        else { softStart(0); softStart(1); }
    }
    else if (command == "OFF") {
        // OFF 1 sau OFF 2 sau OFF (ambele)
        if (param == "1") softStop(0);
        else if (param == "2") softStop(1);
        else { softStop(0); softStop(1); }
    }
    else if (command == "DIM") {
        // DIM 1 50 sau DIM 2 75
        int bulbIdx = param.charAt(0) - '1';
        int level = param.substring(2).toInt();
        if (bulbIdx >= 0 && bulbIdx < 2 && level >= DIM_MIN && level <= DIM_MAX) {
            bulb[bulbIdx].currentLevel = level;
            bulb[bulbIdx].targetLevel = level;
            Serial.print("Bec ");
            Serial.print(bulbIdx + 1);
            Serial.print(" la ");
            Serial.print(level);
            Serial.println("%");
        }
    }
    else if (command == "SCENE") {
        // SCENE 0-5
        int sceneIdx = param.toInt();
        applyScene(sceneIdx);
    }
    else if (command == "STATUS") {
        printStatus();
    }
    else if (command == "HELP") {
        Serial.println("Comenzi disponibile:");
        Serial.println("  ON [1|2]      - Pornește bec");
        Serial.println("  OFF [1|2]     - Oprește bec");
        Serial.println("  DIM n level   - Setează nivel (ex: DIM 1 50)");
        Serial.println("  SCENE n       - Activează scenă (0-5)");
        Serial.println("  STATUS        - Afișează status");
        Serial.println("  NIGHT         - Toggle mod noapte");
        Serial.println("  SUNRISE n     - Pornește sunrise pe bec n");
        Serial.println("  SUNSET n      - Pornește sunset pe bec n");
    }
    else if (command == "NIGHT") {
        toggleNightMode();
    }
    else if (command == "SUNRISE") {
        int bulbIdx = param.toInt() - 1;
        if (bulbIdx >= 0 && bulbIdx < 2) {
            sunriseFade(bulbIdx);
        }
    }
    else if (command == "SUNSET") {
        int bulbIdx = param.toInt() - 1;
        if (bulbIdx >= 0 && bulbIdx < 2) {
            sunsetFade(bulbIdx);
        }
    }
    else {
        Serial.println("Comandă necunoscută. Tastează HELP.");
    }
}

void printStatus() {
    Serial.println("=== STATUS ===");
    for (int i = 0; i < 2; i++) {
        Serial.print("Bec ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(bulb[i].isOn ? "ON" : "OFF");
        Serial.print(" | Nivel: ");
        Serial.print(bulb[i].currentLevel);
        Serial.println("%");
    }
    Serial.print("Mod noapte: ");
    Serial.println(nightModeActive ? "ACTIV" : "inactiv");
    Serial.print("Scenă curentă: ");
    Serial.println(currentScene);
    Serial.println("==============");
}


// ═══════════════════════════════════════════════════════════════════════════════
// 8. SALVARE SCENE ÎN EEPROM
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * Permite utilizatorului să-și creeze scene personalizate salvate în EEPROM.
 */

#define EEPROM_SCENES_START     200     // Adresă de start pentru scene
#define MAX_USER_SCENES         4       // Număr maxim scene utilizator

typedef struct {
    uint8_t level1;
    uint8_t level2;
    bool valid;         // Flag pentru a ști dacă e o scenă validă
} UserScene;

UserScene userScenes[MAX_USER_SCENES];

void loadUserScenes() {
    for (int i = 0; i < MAX_USER_SCENES; i++) {
        int addr = EEPROM_SCENES_START + (i * 3);
        userScenes[i].level1 = EEPROM.read(addr);
        userScenes[i].level2 = EEPROM.read(addr + 1);
        userScenes[i].valid = (EEPROM.read(addr + 2) == 0xAA);
    }
}

void saveUserScene(uint8_t sceneIndex) {
    if (sceneIndex >= MAX_USER_SCENES) return;
    
    userScenes[sceneIndex].level1 = bulb[0].currentLevel;
    userScenes[sceneIndex].level2 = bulb[1].currentLevel;
    userScenes[sceneIndex].valid = true;
    
    int addr = EEPROM_SCENES_START + (sceneIndex * 3);
    EEPROM.write(addr, userScenes[sceneIndex].level1);
    EEPROM.write(addr + 1, userScenes[sceneIndex].level2);
    EEPROM.write(addr + 2, 0xAA);   // Marker valid
    
    // Pentru ESP8266:
    // EEPROM.commit();
    
    Serial.print("Scenă ");
    Serial.print(sceneIndex);
    Serial.println(" salvată!");
}

void applyUserScene(uint8_t sceneIndex) {
    if (sceneIndex >= MAX_USER_SCENES) return;
    if (!userScenes[sceneIndex].valid) {
        Serial.println("Scenă invalidă!");
        return;
    }
    
    softRampTo(0, userScenes[sceneIndex].level1);
    softRampTo(1, userScenes[sceneIndex].level2);
}


// ═══════════════════════════════════════════════════════════════════════════════
// 9. INDICATOR LED STATUS
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * LED extern pentru indicare status sistem.
 * Moduri: OFF, ON solid, Blink lent, Blink rapid
 */

#define PIN_STATUS_LED      13      // LED-ul built-in sau extern

typedef enum {
    LED_OFF,
    LED_ON,
    LED_BLINK_SLOW,     // 1 Hz
    LED_BLINK_FAST,     // 5 Hz
    LED_BLINK_PATTERN   // Pattern personalizat
} LedMode;

LedMode statusLedMode = LED_OFF;
unsigned long lastLedToggle = 0;
bool ledState = false;

void updateStatusLed() {
    unsigned long now = millis();
    
    switch (statusLedMode) {
        case LED_OFF:
            digitalWrite(PIN_STATUS_LED, LOW);
            break;
            
        case LED_ON:
            digitalWrite(PIN_STATUS_LED, HIGH);
            break;
            
        case LED_BLINK_SLOW:
            if (now - lastLedToggle > 500) {
                ledState = !ledState;
                digitalWrite(PIN_STATUS_LED, ledState);
                lastLedToggle = now;
            }
            break;
            
        case LED_BLINK_FAST:
            if (now - lastLedToggle > 100) {
                ledState = !ledState;
                digitalWrite(PIN_STATUS_LED, ledState);
                lastLedToggle = now;
            }
            break;
            
        case LED_BLINK_PATTERN:
            // Exemplu: 2 blink-uri scurte, pauză lungă
            // Implementează pattern custom aici
            break;
    }
}

void setStatusLed(LedMode mode) {
    statusLedMode = mode;
    lastLedToggle = millis();
}

// Exemplu utilizare:
// - La pornire: setStatusLed(LED_BLINK_FAST);
// - Sistem idle: setStatusLed(LED_BLINK_SLOW);
// - Învățare IR: setStatusLed(LED_ON);
// - Sleep: setStatusLed(LED_OFF);


// ═══════════════════════════════════════════════════════════════════════════════
// 10. BUTON "ALL ON/OFF"
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * Un singur buton suplimentar care controlează toate becurile simultan.
 */

#define PIN_BUTTON_ALL      10      // Pin pentru butonul "all"

bool allLightsOn = false;

void checkAllButton() {
    static bool lastState = false;
    static unsigned long lastChange = 0;
    
    bool currentState = digitalRead(PIN_BUTTON_ALL);
    
    // Debounce
    if (currentState != lastState && millis() - lastChange > 50) {
        lastChange = millis();
        lastState = currentState;
        
        if (currentState == HIGH) {  // Apăsat
            toggleAllLights();
        }
    }
}

void toggleAllLights() {
    allLightsOn = !allLightsOn;
    
    if (allLightsOn) {
        // Pornește toate becurile
        for (int i = 0; i < 2; i++) {
            if (!bulb[i].isOn) {
                softStart(i);
            }
        }
    } else {
        // Oprește toate becurile
        for (int i = 0; i < 2; i++) {
            if (bulb[i].isOn) {
                softStop(i);
            }
        }
    }
}

void allOn() {
    for (int i = 0; i < 2; i++) {
        if (!bulb[i].isOn) {
            softStart(i);
        }
    }
    allLightsOn = true;
}

void allOff() {
    for (int i = 0; i < 2; i++) {
        if (bulb[i].isOn) {
            softStop(i);
        }
    }
    allLightsOn = false;
}


// ═══════════════════════════════════════════════════════════════════════════════
// CUM SĂ INTEGREZI EXTENSIILE
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * PAȘI PENTRU INTEGRARE:
 *
 * 1. Copiază funcțiile și variabilele necesare în fișierul principal
 * 
 * 2. Adaugă pinMode() în setup() pentru pini noi
 * 
 * 3. Adaugă apeluri în loop():
 *    
 *    void loop() {
 *        // ... cod existent ...
 *        
 *        // Extensii:
 *        processSerialCommand();    // Comenzi seriale
 *        checkNightModeToggle();    // Mod noapte
 *        checkAutoOff();            // Timer auto-off
 *        updateStatusLed();         // LED status
 *        checkAllButton();          // Buton all on/off
 *        
 *        // La fiecare secundă:
 *        static unsigned long lastSec = 0;
 *        if (millis() - lastSec > 1000) {
 *            lastSec = millis();
 *            applyAutoDim();         // Auto-dimming
 *        }
 *    }
 *
 * 4. Adaugă în setup():
 *
 *    void setup() {
 *        // ... cod existent ...
 *        
 *        loadUserScenes();           // Încarcă scene utilizator
 *        setStatusLed(LED_BLINK_SLOW);  // LED status
 *    }
 *
 * 5. Modifică funcțiile existente pentru a reseta timere:
 *
 *    În softStart():
 *        resetAutoOffTimer(bulbIndex);
 *
 *    În handleLongPressRelease():
 *        resetAutoOffTimer(bulbIndex);
 *
 * 6. Compilează și testează!
 */


// ═══════════════════════════════════════════════════════════════════════════════
// COMPATIBILITATE
// ═══════════════════════════════════════════════════════════════════════════════

/*
 * Toate extensiile de mai sus sunt compatibile cu:
 *   ✅ ATmega328 (Arduino Nano/Uno)
 *   ✅ ESP-01 (ESP8266) - cu mici modificări pentru EEPROM.commit()
 *
 * Pentru ESP-01 cu PCF8574, pini suplimentari pot fi adăugați prin:
 *   - Folosirea pinilor liberi de pe PCF8574 (P4-P7)
 *   - Adăugarea unui al doilea PCF8574 la adresă diferită (0x21)
 *
 * Atenție la memorie:
 *   - ATmega328: 2KB RAM - atenție la string-uri și array-uri mari
 *   - ESP-01: 80KB RAM - mult mai generos
 */

// End of file
