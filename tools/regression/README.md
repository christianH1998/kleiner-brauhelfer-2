# Regressionsvergleich (Entwicklerwerkzeug)

Prüft, ob eine Code-Änderung die Berechnungen bestehender Sude verändert. Gehört nicht zum Programm und nicht zum Installer.

- `kbh-dump` (C++, Core-Lib): Kopiert die Datenbank in ein Temp-Verzeichnis und lädt sie. Dann wird `ModelSud::update()` für jeden Sud ausgeführt (wirkt nur auf den Status Rezept). Alle gespeicherten und virtuellen Felder der wichtigsten Modelle werden als JSON geschrieben. Die Originaldatenbank wird nie verändert, gespeichert wird nichts.
- `compare.py`: Vergleicht zwei Dumps mit relativer Toleranz. Felder, die nur im neuen Dump stehen, gelten als bewusst neu und werden nur aufgelistet.

## Ablauf
```bat
:: 1. App bauen (liefert Qt-DLLs in build-win64\bin)
build_win64.cmd C:\Qt\6.11.0\msvc2022_64\bin
:: 2. Core + kbh-dump bauen (aktueller Quellstand)
tools\regression\build.cmd C:\Qt\6.11.0\msvc2022_64\bin
:: 3. Dump erzeugen
build-win64\bin\kbh-dump.exe Testdatenbank\kb_daten.sqlite build-win64\regression\nachher.json
:: 4. Vergleichen (Exit-Code 0 = identisch)
py -3.10 tools\regression\compare.py build-win64\regression\baseline-ext-s00.json build-win64\regression\nachher.json
```
Optionen:
- `kbh-dump … --no-recalc` (ohne `update()`)
- `kbh-dump … --save`: **nur für Testdaten**. Arbeitet direkt auf der Datenbank ohne Kopie und speichert die Neuberechnung. Damit lässt die App-Logik roh eingefügte Testsude selbst durchrechnen.
- `kbh-dump <db> x --rundlauf-hopfen`: Statt eines Dumps läuft der Rundlauftest der Hopfen-Berechnungsarten (ab ext-s01). Jeder Sud wird dafür im Speicher in den Status Rezept gesetzt. Dann wird geprüft, ob „Mengen fest → IBU berechnet“ ↔ Gewicht/IBU (inkl. Neuberechnung im Zielmodus) Mengen und IBU exakt erhält. Außerdem muss bei Menge +10 % die IBU steigen. Exit-Code 0 bedeutet OK.
- `kbh-dump <db> x --umkehrtest`: Diagnose (ab ext-s01). Jedes wichtige Rezeptfeld wird geändert und exakt zurückgesetzt, danach werden alle Sud- und Zutatenwerte verglichen. Zusätzlich meldet der Test, wo die Anzeige rundet. Jedes Feld wird zweimal geändert: Der erste Durchgang darf angleichen, der zweite muss exakt umkehrbar sein. Die Meldung „BEKANNT (S12)“ steht für das Nachführen nicht gemessener Braudaten durch SW/Menge (seit ext-s12 bleiben Messwerte geschützt, siehe `--rundlauf-protokoll`). Seit ext-s01b ist der Exit-Code 0.
- `kbh-dump <db> x --rundlauf-malz`: Rundlauftest der Malzberechnung (ab ext-s09). Ablauf: Ziel-SW ↔ Mengen fest, Malz +10 % und zurück, SW ändern und zurück. Exit-Code 0 bedeutet OK.
- `kbh-dump <db> x --rundlauf-wasser`: Rundlauftest der Wasserberechnung (ab ext-s10). Ablauf: Bestand → Faktoren (keine Wertänderung), Nachgussfaktor +0,5 und zurück, Voreinstellungen 1–3, zurück auf Bestand, Treberaufnahme der Anlage +0,1 und zurück; zusätzlich „Rezept nachbrauen“ (Umschalten ohne Wertänderung, Nachgussfaktor +0,5, Skalieren auf Menge ×1,5 und zurück, Tippen im Mengenfeld mit Nachbildung des fokussierten GUI-Felds (kein Zurückschreiben, keine Rekursion), zurück auf Planen); ab ext-s11b auch die erwartete Stammwürze (ohne Abweichung = SW, Nachguss +0,5 senkt sie, Nachbrauen = SW). `--rundlauf-malz` prüft bei Suden im Nachbrau-Modus statt „SW steigt“ „Menge steigt“. Exit-Code 0 bedeutet OK.
- `kbh-dump <db> x --rundlauf-protokoll`: Rundlauftest Brauprotokoll (ab ext-s12). Je Sud: Vorbelegung = Sollwerte; Messwerte Kochbeginn (Volumen, SW), Anstellen (Volumen) und eine Maischschritt-Dauer setzen → Sud-Felder übernommen, Sollwert-Snapshot, effektive Menge; SW +0,5, Menge +2 L, Speise +0,3 L → Messwerte bleiben; zurück und entfernen → alle Ist-Felder exakt wie vorher. Exit-Code 0 bedeutet OK.
- `kbh-dump <db> x --rundlauf-nachiso`: Rundlauftest „Whirlpool mit Abkühlkurve“ (ab ext-s11). Erst Physik-Plausibilität von `BierCalc::whirlpoolMinuten` (Temperatur fällt, Deckel/Außentemperatur/Faktor wirken richtig herum, Teilintegrale addieren sich). Dann je Sud mit 30 min Absetzzeit in jeder Hopfen-Berechnungsart Bestand → Whirlpool → Bestand: Mengen unverändert, IBU-Anteil jeder Kochgabe exakt im Verhältnis (1−e^(−0,04·(t+W)))/(1−e^(−0,04·(t+0,549·30))), IBU = Summe der Anteile (außer „Keine“), Außentemperatur +10 K und Deckel erhöhen die IBU, Rückweg exakt. Ab S11 hat die Testdatenbank evtl. noch Version 2010 ⇒ `--migrate` angeben. Exit-Code 0 bedeutet OK.
- `kbh-dump <db> x --rundlauf-farbmalz`: Rundlauftest Malz-Zugabe im Maischplan (ab ext-s12b). Je Sud mit mind. 2 Malzen wird das dunkelste Malz in einer neuen letzten Zeile „Zuschütten“ (8 min) zugegeben: Zeile ohne Zuordnung ändert nichts; mit Zuordnung sinkt die Farbe, Schüttung/SW bleiben, Malzanteil = Prozent des Malzes und Einmaischen um denselben Wert kleiner; Farbminderung exakt ∝ 1 − f(8 min); τ → 0 = volle Farbe; ein Aufheizschritt danach verlängert die Kontaktzeit; Zuordnung/Zeilen entfernen = exakt wie vorher. Exit-Code 0 bedeutet OK.
- `kbh-dump <db> x --rundlauf-abfuellart`: Rundlauftest Abfüllart (ab ext-s13). Je Sud: Ableitung wie bisher (Spunden ⇒ 4, Speise ⇒ 3, sonst 1); Auswahl der abgeleiteten Art ändert nichts (Spunden ohne Schnellgärprobe nur `erg_Alkohol`); Art 1 Zucker = Vorschlag × Menge ohne Speise, Alkohol mit Verdünnung; Art 2 ohne Wert = Vorschlag, 6 g/L ⇒ 6 × Menge, Alkohol ohne Verdünnung, 0 g/L ⇒ 0; Art 5 ohne Zucker/Speise und Zuwachs; Art 4 Alkohol = Rezept bzw. Schnellgärprobe; Art 6 Zucker = fehlendes CO2 (mit/ohne Schnellgärprobe), Alkohol mit Zucker, endvergorenes Jungbier ⇒ Zucker > 0; Rückweg exakt. Exit-Code 0 bedeutet OK.
- `compare.py --tol 1e-9 --ignore Feld1,Feld2`. `Gespeichert` wird immer ignoriert, weil `update()` das Feld bei jedem Lauf auf „jetzt“ setzt.

## Hinweise
- `deployment\win\64bit\deploy.cmd` löscht `kleiner-brauhelfer-core.lib` aus `build-win64\bin`. Deshalb baut `build.cmd` den Core selbst in `build-win64\tools-regression`.
- Die Testdatenbank und die Dumps enthalten persönliche Daten und werden nicht committet (`Testdatenbank/`, `build*/` stehen in `.gitignore`).
- Die Datenbankversion muss zur Programmversion passen, sonst bricht `kbh-dump` ab. Ablauf bei einer Migration (z. B. 2010):
  1. Dump mit dem alten Code und der alten Datenbank erstellen.
  2. Eine Kopie der Datenbank in der neuen App migrieren.
  3. Dump mit dem neuen Code und der migrierten Kopie erstellen.
  4. Beide Dumps vergleichen. Neue Spalten erscheinen dabei als „neue Felder“.
  5. Die migrierte Kopie dient danach als neue Referenz.
- Inhalt der Testdatenbank (ergänzt mit `Testdatenbank/testdaten_ergaenzen.py`, Stand vorher: `Testdatenbank/kb_daten_vor_testdaten.sqlite`):

  | ID | Sud | Status | Besonderheit |
  |---|---|---|---|
  | 3 | Weinachtsbier Klaus Kling | Abgefüllt | weitere Zutaten (Honig, Gewürze), Teilschüttung, Gärverläufe, Bewertung |
  | 6 | helles Kellerbier | Rezept | große Menge (45,6 L) |
  | 7 | Tittingdorfer Spezial | Rezept | kleine Menge (8,2 L), dunkel |
  | 8 | Export hell (Kling) | Abgefüllt | untergärig, Schnellgärprobe, Nachgärung, Bewertung |
  | 9 | Hefeweizen (Kling) | Gebraut | obergärig, Gärung läuft, Nachisomerisierung 10 min |
  | 10 | Hefeweizen (Kling) Nachbrau | Abgefüllt | obergärig (für Hefe-Auswertung S05, Phase 4 des Skripts) |

  Aktuelle Referenz ab S07 (DB 2010): `build-win64/regression/baseline-ext-s07.json` (6 Sude, migrierte Kopie). Ältere Referenzen: `baseline-ext-s05.json` (DB 2009, 6 Sude), `baseline-ext-s00.json` (5 Sude).
  Ab S07 kbh-dump mit `--migrate` aufrufen, solange die Testdatenbank noch Version 2009 hat. Migriert wird nur die Kopie, die Datei bleibt unverändert.

  Hefen: Braupartners Trockenhefe og und ug, dazu Hefegaben für alle Sude. Hopfen überall im Modus „Keine“, also Mengen fest.
