# Mondbasis-Beleuchtung (XIAO ESP32-S3)

Automatische, sanft ein-/ausblendende LED-Beleuchtung mit LDR und Schalter.

## Verdrahtung

| Bauteil | Pin | GPIO |
|---|---|---|
| LED 1 Innen (Ã¼ber 220â€“330 Î© gegen GND) | D2 | 3 |
| LED 2 AuÃŸen (Ã¼ber 220â€“330 Î© gegen GND) | D3 | 4 |
| LDR: 3V3 â†’ LDR â†’ D0 â†’ 10 kÎ© â†’ GND | D0 | 1 |
| Schalter: D1 â†’ Schalter â†’ GND | D1 | 2 |

LED: Anode (+) am Widerstand, Kathode (â€“) an GND (kurzes Beinchen).

## Flashen

1. **VS Code** und die Erweiterung **PlatformIO IDE** installieren.
2. Ordner `mondbasis-licht` in VS Code Ã¶ffnen (*Datei â†’ Ordner Ã¶ffnen*). PlatformIO lÃ¤dt beim ersten Mal Plattform und Toolchain automatisch (Internet nÃ¶tig, dauert einige Minuten).
3. XIAO per USB-C direkt mit dem PC verbinden.
4. Falls das Board nicht erkannt wird: **BOOT**-Taste gedrÃ¼ckt halten, **RESET** kurz drÃ¼cken (oder USB einstecken), BOOT loslassen â†’ Bootloader-Modus.
5. In der blauen Statusleiste unten auf **â†’ (Upload)** klicken, oder im Terminal:
   ```
   pio run -t upload
   ```
6. Seriellen Monitor Ã¶ffnen (Steckdosen-Symbol in der Statusleiste oder `pio device monitor`). Nach dem Upload ggf. einmal RESET drÃ¼cken.

## Kalibrieren

Im Monitor erscheint einmal pro Sekunde z. B.:

```
LDR roh: 2310 | LDR geglaettet: 2305 | Dunkel<1200 Hell>1800 | Schalter: geschlossen | LEDs:   0% | Zustand: AUS (hell)
```

Der LDR ist so verdrahtet, dass **dunkel = kleiner Wert** ist.

1. Werte im normal hellen Raum ablesen (z. B. 2300).
2. LDR mit der Hand oder dem Mondbasis-GehÃ¤use abdunkeln, Wert ablesen (z. B. 600).
3. In `src/main.cpp` eintragen: `SCHWELLE_DUNKEL` etwas Ã¼ber dem dunklen Wert, `SCHWELLE_HELL` etwas unter dem hellen Wert (mit Abstand dazwischen â€“ das ist die Hysterese).
4. Neu hochladen.

Weitere Einstellungen (Fade-Zeit, maximale Helligkeit je LED, GlÃ¤ttung) stehen als Konstanten am Anfang von `main.cpp`.

## Verhalten

- Schalter **geschlossen**: Automatik. Dunkel â†’ LEDs blenden in ca. 1 s ein, hell â†’ aus.
- Schalter **offen**: LEDs blenden aus und bleiben aus.

Hinweis: Das Projekt lÃ¤uft sowohl mit dem ESP32-Arduino-Core 2.x (Standard der PlatformIO-Plattform `espressif32`) als auch mit 3.x; die passende LEDC-API wird automatisch gewÃ¤hlt.

## Web-App

- Live: https://paulgkwr.github.io/mondbasis-licht/ (Chrome/Edge, XIAO per USB am selben Rechner, serieller Monitor vorher schließen)
- Lokal: `python -m http.server 8000 --directory docs`

