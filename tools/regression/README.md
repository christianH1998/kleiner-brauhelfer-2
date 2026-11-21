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
- `kbh-dump <db> x --umkehrtest`: Diagnose (ab ext-s01). Jedes wichtige Rezeptfeld wird geändert und exakt zurückgesetzt, danach werden alle Sud- und Zutatenwerte verglichen. Zusätzlich meldet der Test, wo die Anzeige rundet. Jedes Feld wird zweimal geändert: Der erste Durchgang darf angleichen, der zweite muss exakt umkehrbar sein. Die Meldung „BEKANNT (S12)“ steht für das Überschreiben von Braudaten durch SW/Menge. Seit ext-s01b ist der Exit-Code 0.
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

  Hefen: Braupartners Trockenhefe og und ug, dazu Hefegaben für alle Sude. Hopfen überall im Modus „Keine“, also Mengen fest.
