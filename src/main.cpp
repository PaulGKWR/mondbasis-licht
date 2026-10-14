/*
 * Mondbasis-Beleuchtung – Seeed Studio XIAO ESP32-S3
 * KreativWerkstatt Ried
 *
 * - Schalter geschlossen (D1 gegen GND): Automatik aktiv
 * - Schalter offen: beide LEDs aus
 * - Automatik: Es wird dunkel -> LEDs sanft an, es wird hell -> LEDs sanft aus
 * - Hysterese verhindert Flackern an der Helligkeitsgrenze
 * - Keine delay()-Aufrufe, alles läuft zeitgesteuert über millis()
 *
 * Verdrahtung des LDR: 3V3 -> LDR -> D0 -> 10 kΩ -> GND
 * => Je DUNKLER es ist, desto NIEDRIGER ist der ADC-Messwert.
 */

#include <Arduino.h>

// ============================================================
//  EINSTELLUNGEN (hier anpassen)
// ============================================================

// --- Pins (GPIO-Nummern) ---
constexpr uint8_t PIN_LED_INNEN  = 3;  // D2
constexpr uint8_t PIN_LED_AUSSEN = 4;  // D3
constexpr uint8_t PIN_LDR        = 1;  // D0 / A0
constexpr uint8_t PIN_SCHALTER   = 2;  // D1 (gegen GND, INPUT_PULLUP)

// --- Helligkeits-Schwellen (ADC-Wert 0..4095) ---
// Kalibrieren: Serial Monitor beobachten, Raum abdunkeln bzw. beleuchten,
// dann die beiden Werte passend dazwischen legen.
// Es muss gelten: SCHWELLE_DUNKEL < SCHWELLE_HELL
constexpr int SCHWELLE_DUNKEL = 1200;  // darunter  -> es ist dunkel -> LEDs AN
constexpr int SCHWELLE_HELL   = 1800;  // darüber   -> es ist hell   -> LEDs AUS
// Dazwischen (Hysterese-Bereich) bleibt der letzte Zustand erhalten.

// --- Messung ---
constexpr uint32_t LDR_MESSINTERVALL_MS = 20;  // Abtastung alle 20 ms
constexpr float    LDR_GLAETTUNG        = 0.05f; // 0..1, kleiner = träger/ruhiger

// --- Schalter ---
constexpr uint32_t ENTPRELL_MS = 20;  // Schalter muss 20 ms stabil sein

// --- Sanftes Ein-/Ausblenden ---
constexpr uint32_t FADE_DAUER_MS = 1000;  // Zeit von 0 % bis 100 % (bzw. zurück)
constexpr float    GAMMA         = 2.0f;  // 1.0 = linear; 2.0 wirkt für das Auge gleichmäßiger

// --- Maximale Helligkeit je LED (0.0 .. 1.0) ---
constexpr float MAX_HELLIGKEIT_INNEN  = 1.0f;
constexpr float MAX_HELLIGKEIT_AUSSEN = 1.0f;

// --- PWM ---
constexpr uint32_t PWM_FREQUENZ  = 5000;  // Hz (flimmerfrei)
constexpr uint8_t  PWM_AUFLOESUNG = 10;   // Bit -> Werte 0..1023
constexpr uint32_t PWM_MAX       = (1UL << PWM_AUFLOESUNG) - 1;

// --- Debug-Ausgabe ---
constexpr uint32_t DEBUG_INTERVALL_MS = 1000;

// ============================================================
//  PWM-Abstraktion (ESP32-Arduino-Core 2.x und 3.x)
// ============================================================

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // Core 3.x: Pin direkt anbinden, Kanal wird automatisch vergeben
  void pwmInit(uint8_t pin, uint8_t /*kanal*/) {
    ledcAttach(pin, PWM_FREQUENZ, PWM_AUFLOESUNG);
  }
  void pwmWrite(uint8_t pin, uint8_t /*kanal*/, uint32_t duty) {
    ledcWrite(pin, duty);
  }
#else
  // Core 2.x: Kanal einrichten und mit Pin verbinden
  void pwmInit(uint8_t pin, uint8_t kanal) {
    ledcSetup(kanal, PWM_FREQUENZ, PWM_AUFLOESUNG);
    ledcAttachPin(pin, kanal);
  }
  void pwmWrite(uint8_t /*pin*/, uint8_t kanal, uint32_t duty) {
    ledcWrite(kanal, duty);
  }
#endif

// ============================================================
//  Programmzustand
// ============================================================

constexpr uint8_t KANAL_INNEN  = 0;
constexpr uint8_t KANAL_AUSSEN = 1;

// Fortschritt der Ein-/Ausblendung: 0.0 = aus, 1.0 = voll an
float fadeFortschritt = 0.0f;

// Automatik-Entscheidung (mit Hysterese): true = es ist dunkel
bool istDunkel = false;

// Gleitender Mittelwert des LDR
float ldrGeglaettet = 0.0f;
int   ldrRoh        = 0;

// Entprellter Schalterzustand: true = geschlossen = Automatik aktiv
bool schalterGeschlossen = false;
bool schalterRohLetzt    = false;
uint32_t schalterAenderungMs = 0;

uint32_t letzteMessungMs = 0;
uint32_t letzterFadeMs   = 0;
uint32_t letzteDebugMs   = 0;

// ============================================================
//  Funktionen
// ============================================================

// Schalter einlesen und entprellen (ohne delay)
void schalterAktualisieren(uint32_t jetzt) {
  bool roh = (digitalRead(PIN_SCHALTER) == LOW);  // LOW = geschlossen
  if (roh != schalterRohLetzt) {
    schalterRohLetzt = roh;
    schalterAenderungMs = jetzt;
  }
  if ((jetzt - schalterAenderungMs) >= ENTPRELL_MS) {
    schalterGeschlossen = roh;
  }
}

// LDR messen, glätten, Hysterese auswerten
void ldrAktualisieren(uint32_t jetzt) {
  if ((jetzt - letzteMessungMs) < LDR_MESSINTERVALL_MS) return;
  letzteMessungMs = jetzt;

  ldrRoh = analogRead(PIN_LDR);
  ldrGeglaettet += LDR_GLAETTUNG * (ldrRoh - ldrGeglaettet);

  if (ldrGeglaettet < SCHWELLE_DUNKEL)      istDunkel = true;
  else if (ldrGeglaettet > SCHWELLE_HELL)   istDunkel = false;
  // dazwischen: Zustand beibehalten (Hysterese)
}

// Soll die Beleuchtung gerade leuchten?
bool beleuchtungSoll() {
  return schalterGeschlossen && istDunkel;
}

// Fortschritt in Richtung Ziel bewegen und PWM ausgeben
void fadeAktualisieren(uint32_t jetzt) {
  uint32_t dt = jetzt - letzterFadeMs;
  letzterFadeMs = jetzt;

  float schritt = (float)dt / (float)FADE_DAUER_MS;
  if (beleuchtungSoll()) {
    fadeFortschritt = min(1.0f, fadeFortschritt + schritt);
  } else {
    fadeFortschritt = max(0.0f, fadeFortschritt - schritt);
  }

  float hell = powf(fadeFortschritt, GAMMA);  // Gamma-Korrektur
  pwmWrite(PIN_LED_INNEN,  KANAL_INNEN,  (uint32_t)(hell * MAX_HELLIGKEIT_INNEN  * PWM_MAX));
  pwmWrite(PIN_LED_AUSSEN, KANAL_AUSSEN, (uint32_t)(hell * MAX_HELLIGKEIT_AUSSEN * PWM_MAX));
}

const char* betriebszustand() {
  if (!schalterGeschlossen)      return "AUS (Schalter offen)";
  if (beleuchtungSoll()) return (fadeFortschritt >= 1.0f) ? "AN (dunkel)" : "BLENDET EIN";
  return (fadeFortschritt <= 0.0f) ? "AUS (hell)" : "BLENDET AUS";
}

// Messwerte einmal pro Sekunde ausgeben
void debugAusgeben(uint32_t jetzt) {
  if ((jetzt - letzteDebugMs) < DEBUG_INTERVALL_MS) return;
  letzteDebugMs = jetzt;

  Serial.printf("LDR roh: %4d | LDR geglaettet: %4d | Dunkel<%d Hell>%d | Schalter: %s | LEDs: %3d%% | Zustand: %s\n",
                ldrRoh, (int)ldrGeglaettet, SCHWELLE_DUNKEL, SCHWELLE_HELL,
                schalterGeschlossen ? "geschlossen" : "offen",
                (int)(fadeFortschritt * 100.0f), betriebszustand());
}

// ============================================================
//  Arduino-Einstieg
// ============================================================

void setup() {
  Serial.begin(115200);  // USB-CDC: blockiert nicht, wenn kein Monitor offen ist

  pinMode(PIN_SCHALTER, INPUT_PULLUP);
  pinMode(PIN_LDR, INPUT);
  analogReadResolution(12);  // 0..4095

  pwmInit(PIN_LED_INNEN,  KANAL_INNEN);
  pwmInit(PIN_LED_AUSSEN, KANAL_AUSSEN);
  pwmWrite(PIN_LED_INNEN,  KANAL_INNEN,  0);
  pwmWrite(PIN_LED_AUSSEN, KANAL_AUSSEN, 0);

  // Startwerte, damit nicht erst eingeschwungen werden muss
  ldrRoh = analogRead(PIN_LDR);
  ldrGeglaettet = ldrRoh;
  istDunkel = (ldrRoh < (SCHWELLE_DUNKEL + SCHWELLE_HELL) / 2);
  schalterRohLetzt = schalterGeschlossen = (digitalRead(PIN_SCHALTER) == LOW);

  uint32_t jetzt = millis();
  letzteMessungMs = letzterFadeMs = letzteDebugMs = jetzt;

  Serial.println("Mondbasis-Beleuchtung gestartet");
}

void loop() {
  uint32_t jetzt = millis();

  schalterAktualisieren(jetzt);
  ldrAktualisieren(jetzt);
  fadeAktualisieren(jetzt);
  debugAusgeben(jetzt);
}
