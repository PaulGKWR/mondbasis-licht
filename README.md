# Mondbasis-Beleuchtung (XIAO ESP32-S3)

Automatische, sanft ein-/ausblendende LED-Beleuchtung mit LDR und Schalter.

## Verdrahtung

| Bauteil | Pin | GPIO |
|---|---|---|
| LED 1 Innen (über 220–330 Ω gegen GND) | D2 | 3 |
| LED 2 Außen (über 220–330 Ω gegen GND) | D3 | 4 |
| LDR: 3V3 → LDR → D0 → 10 kΩ → GND | D0 | 1 |
| Schalter: D1 → Schalter → GND | D1 | 2 |

LED: Anode (+) am Widerstand, Kathode (–) an GND (kurzes Beinchen).

## Flashen

1. **VS Code** und die Erweiterung **PlatformIO IDE** installieren.
2. Ordner `mondbasis-licht` in VS Code öffnen (*Datei → Ordner öffnen*). PlatformIO lädt beim ersten Mal Plattform und Toolchain automatisch (Internet nötig, dauert einige Minuten).
3. XIAO per USB-C direkt mit dem PC verbinden.
4. Falls das Board nicht erkannt wird: **BOOT**-Taste gedrückt halten, **RESET** kurz drücken (oder USB einstecken), BOOT loslassen → Bootloader-Modus.
5. In der blauen Statusleiste unten auf **→ (Upload)** klicken, oder im Terminal:
   ```
   pio run -t upload
   ```
6. Seriellen Monitor öffnen (Steckdosen-Symbol in der Statusleiste oder `pio device monitor`). Nach dem Upload ggf. einmal RESET drücken.

## Kalibrieren

Im Monitor erscheint einmal pro Sekunde z. B.:

```
LDR roh: 2310 | LDR geglaettet: 2305 | Dunkel<1200 Hell>1800 | Schalter: geschlossen | LEDs:   0% | Zustand: AUS (hell)
```

Der LDR ist so verdrahtet, dass **dunkel = kleiner Wert** ist.

1. Werte im normal hellen Raum ablesen (z. B. 2300).
2. LDR mit der Hand oder dem Mondbasis-Gehäuse abdunkeln, Wert ablesen (z. B. 600).
3. In `src/main.cpp` eintragen: `SCHWELLE_DUNKEL` etwas über dem dunklen Wert, `SCHWELLE_HELL` etwas unter dem hellen Wert (mit Abstand dazwischen – das ist die Hysterese).
4. Neu hochladen.

Weitere Einstellungen (Fade-Zeit, maximale Helligkeit je LED, Glättung) stehen als Konstanten am Anfang von `main.cpp`.

## Verhalten

- Schalter **geschlossen**: Automatik. Dunkel → LEDs blenden in ca. 1 s ein, hell → aus.
- Schalter **offen**: LEDs blenden aus und bleiben aus.

Hinweis: Das Projekt läuft sowohl mit dem ESP32-Arduino-Core 2.x (Standard der PlatformIO-Plattform `espressif32`) als auch mit 3.x; die passende LEDC-API wird automatisch gewählt.

## Web-App

- Live: https://paulgkwr.github.io/mondbasis-licht/ (Chrome oder Edge, XIAO per USB am selben Rechner, serieller Monitor vorher schließen)
- Lokal: `python -m http.server 8000 --directory docs`
