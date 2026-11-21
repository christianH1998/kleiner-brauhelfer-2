# Umsetzungsplan Erweiterung kleiner-brauhelfer-2 (Stufen mit GitHub-Versionen)

## Context
Phase 2 (Konzept, `doc/claude/konzept-erweiterung.md`) ist abgeschlossen. Der Anwender hat die offenen Entscheidungen beantwortet und neue Anforderungen ergänzt. Jetzt wird der Umsetzungsplan in kleinen Stufen erstellt. Jede Stufe wird als eigene, wiederherstellbare Version auf GitHub abgelegt (eigener Fork, Updates des Originals sollen weiter einspielbar sein).

### Entscheidungen des Anwenders (verbindlich)
| # | Thema | Entscheidung |
|---|---|---|
| 1 | DB-Migration 2010 | ja; alte DBs müssen weiter eingelesen werden |
| 2 | MVP-1 ohne DB zuerst | ja |
| 3 | Brauprotokoll | eigener Dialog |
| 4 | „Sudhausausbeute“ | Name bleibt, Tooltip „effektive Ausbeute (bezogen auf Anstellwürze)“, klassische SHA zusätzlich als berechneter Wert |
| 5 | Modi | Dropdown je Eingabebereich, **eindeutige Bezeichnungen**, Tooltip-Erklärung wie bei bestehenden Elementen |
| 6 | Whirlpool/Hopfenseihen | ja, als wählbares Modell, das die IBU-Berechnung (und damit im Ziel-IBU-Modus die Mengen/Zeiten) beeinflusst. Malzpotential: keine Aussage, also zurückgestellt |
| 7 | Zucker | Rechengröße Liter; Zuckergabe je Flasche als fester Wert g/L; Rechner Ziel-CO2 + Restextrakt + Temperatur → g/L, g je 0,5 L, g je 0,33 L auf einen Blick; Hefe dokumentiert, Statistik je Hefe |
| 8 | Wasser | Modus wählbar: „Bestand“ (NG folgt Würzemenge) oder „Faktoren HG+NG“ (Würzemenge folgt, Warnung bei Abweichung). Presets: Helles 4,0/3,0 · Dunkles 3,0/4,0 · Benutzerdefiniert 3,5/3,5 L/kg (HG/NG). Faktoren immer editierbar; Kling-Werte im Tooltip, als „vorläufig, Quelle ungeprüft“ markiert, bis der Anwender das Buch liefert |
| 9 | Python | nicht in der App; nur wenn Diagramme/Statistik dadurch klar weniger Aufwand sind. Dev-Hilfsskripte (Regressionsvergleich) erlaubt (`py -3.10`) |

### Globale Invarianten (gelten für jede Stufe)
- **Standardmodus = heutiges Verhalten.** Bestehende Rezepte rechnen bit-gleich weiter (Regressionsvergleich).
- **Moduswechsel verändert keine Werte.** Beim Umschalten werden die aktuell angezeigten Werte als Eingaben des neuen Modus übernommen. Erst eine Eingabe im neuen Modus rechnet das Rezept wie gewohnt nach. Für jeden Modus-Setter gibt es einen Test „umschalten → alle Mengen, Prozent, SW, IBU, Wasser unverändert“.
- Fehlende Messwerte (Brauprotokoll) dürfen nichts kaputt machen: Fehlend ≠ 0, es gibt immer einen Rückfall auf Soll.
- Änderungen möglichst in neuen Dateien bzw. kleinen Blöcken, damit Upstream-Merges konfliktarm bleiben.

### Abgleich mit der ursprünglichen Anfrage
| Ursprünglicher Punkt | Stufe |
|---|---|
| Basis Malzmengen → fehlende Werte über Faktoren | S02, S09 |
| Maischplan als Basis | bleibt Bestand (Rasten). Hauptguss kommt aus Faktor bzw. Preset (S10). Keine Änderung nötig |
| Hopfen g + %α → Bittere berechnen | S01, S08 |
| HG/NG mit verstellbaren Faktoren, hell/dunkel | S10 |
| Modus-Dropdown, umschaltbar, Basisrezept anpassen | S08–S10 (Invariante „Wechsel ohne Wertänderung“) |
| Brauschritte als Checkliste, Biermenge nach Messwert | S12 |
| Zwischenstände auslassbar | S12, S14 (Gültigkeitsregeln) |
| Abfüllart wählbar, Alkohol eindeutig (2 Werte) | S03, S13 |
| Kalibrierung Sudhausausbeute | S14 |
| Weitere Statistiken, ggf. Python | S05, S15 |
| **Neu:** Zucker je Flasche + Rechner, Hefe-Statistik | S06, S13, S05/S15 |
| **Neu:** Whirlpool/Hopfenseihen-Modell | S11 |
| **Neu:** Tooltips/Erklärungen | in jeder UI-Stufe, Basis S04 |

## Versionierung / GitHub
- Der Anwender legt einen Fork in seinem GitHub an. Danach: `git remote rename origin upstream`, `git remote add origin <fork-url>`.
- Arbeitsbranch `feature-rezept-eingabe`. **Pro Stufe ein Commit (bei Bedarf wenige) + annotierter Tag `ext-sNN`.** Commit, Tag und Push jeweils erst nach ausdrücklicher Freigabe durch den Anwender.
- Upstream-Updates: `git fetch upstream` → `git merge upstream/master` in einen eigenen Merge-Branch → Build + Regressionsvergleich → Merge in den Arbeitsbranch.
- **Risiko DB-Version:** Upstream könnte selbst eine Version 2010 veröffentlichen. Gegenmaßnahmen: Vor S07 Upstream prüfen. Die Migration wird idempotent geschrieben (Spalte bzw. Tabelle nur anlegen, wenn sie fehlt, per `PRAGMA table_info`), damit sie bei einem Konflikt auf eine andere Nummer verschoben werden kann, ohne Daten zu verlieren.
- Nicht committen: `aqtinstall.log`, `doc/Textdokument (neu).txt` (vorher mit dem Anwender klären), `doc/Commands/`, `doc/claudTerminal/` (vorher ansehen).

## Arbeitsablauf je Stufe
1. Relevante Quellstellen lesen. 2. Umsetzen (Core zuerst, dann GUI). 3. Build mit qmake (Qt 6.11 msvc2022_64 ist unter `C:\Qt` installiert). 4. Regressionsvergleich gegen die Referenz-DB. 5. Manueller Test mit einer DB-Kopie (Checkliste der Stufe). 6. `tr()`-Texte setzen, Doku `doc/claude/*` aktualisieren. 7. Freigabe → Commit + Tag (+ Push).

## Stufen

### S00 Grundlage (ext-s00)
- Fork bzw. Remotes einrichten (siehe oben).
- Prüfen, ob ein Build mit `build_win64.cmd C:\Qt\6.11.0\msvc2022_64\bin` möglich ist (VS 2022 / MSVC vorhanden? Bisher war nur VS 14 bekannt; das muss geklärt werden).
- Regressionsbasis: Referenz-DB (Kopie der Anwender-DB + Template-DB). Dev-Skript `tools/regression/` (Python, außerhalb der App) öffnet jeden Sud über die App bzw. exportiert alle `erg_*`- und Virtualfelder. Einfachste Variante: verdeckte Kommandozeilenoption `--dump-sude <db> <json>` in `main.cpp`, die `ModelSud::update` durchläuft und die Werte schreibt. Das Python-Skript vergleicht vorher und nachher.
- Commit: `CLAUDE.md`, `doc/claude/*`, Regressionswerkzeug.

### MVP-1 – ohne Datenbankänderung
**S01 Bittere aus Hopfenmengen (ext-s01)** – *geändert auf Wunsch des Anwenders (2026-11-21): ein einziges IBU-Feld, kein zweites Anzeigefeld; Hopfen-Teil von S08 vorgezogen*
- Neue Berechnungsart `BerechnungsartHopfen::IBUBerechnet = 2` in der bestehenden Spalte (keine DB-Änderung): Mengen fest, `Sud.IBU` = Σ `IBUAnteil` (`ModelSud::updateIbuBerechnet`).
- Dropdown mit eindeutigen Namen und Tooltips je Eintrag; IBU-Feld im neuen Modus schreibgeschützt.
- Rundlauf Ziel-IBU ↔ berechnet ist exakt (`kbh-dump --rundlauf-hopfen`).

**S01b Bugfix Umkehrbarkeit (ext-s01b)** – *neu nach Anwendertest S01*
- Hopfen-Prozent nur bei Benutzereingabe auf 0–100 % begrenzen, nicht bei internen Neuberechnungen (`ModelHopfengaben::setDataExt` ColProzent).
- Anzeige-Rundung: Bittere mit 1 Nachkommastelle; `DoubleSpinBoxSud`/`SpinBoxSud` merken beim Fokuserhalt den exakten Wert und schreiben ihn zurück, wenn der Anzeigewert vom Fokusbeginn wieder erreicht wird (statt Tooltip-Lösung).
- Ist-Feld-Überschreibung durch SW/Menge-Kaskade ⇒ bleibt S12.

**S02 Stammwürze aus Malzmengen (ext-s02)**
- `BierCalc`: Umkehrfunktion Schüttung → SW (Fixpunkt, `fachmodelle.md` §2) plus Extrakt aus weiteren Zutaten.
- Virtuelles Feld `SWAusMalzmengen` und Knopf „SW aus Malzmengen übernehmen“ (Muster `on_btnShaAnpassen_clicked`).

**S03 Alkohol vereinheitlichen (ext-s03)**
- Virtuelle Felder `AlkoholJungbier` und `AlkoholErwartet` (Konzept §7.3).
- Beschriftungen „Rezept (Soll)“ / „Jungbier“ / „fertiges Bier (erwartet)“ in `tababfuellen`, `tabrezept`, `templatetags.cpp` + Templates, Brauübersicht. `erg_Alkohol` bleibt unverändert gespeichert.

**S04 Kennzeichnung und Tooltips (ext-s04)**
- Zentrale Kennzeichnung eingegeben/berechnet in `widgets/` (`DoubleSpinBoxSud`/`WidgetDecorator`).
- Tooltip „Sudhausausbeute = effektive Ausbeute …“ und Anzeige der klassischen SHA (berechnet).
- Muster für Modus-Dropdown-Tooltips festlegen (Tooltip pro Eintrag über `setItemData(..., Qt::ToolTipRole)` + Tooltip am Combo).

**S05 Statistik-Reihen + Hefe-Auswertung (ext-s05)**
- Neue Einträge in `DlgBrauUebersicht::mAuswahlListe`: Ausbeute (eff./klassisch), Soll/Ist SW Anstellen, Soll/Ist Menge, Verdampfungsrate.
- Hefe-Statistik (nur lesend): je Hefe aus `Hefegaben` die Anzahl Sude, mittlerer scheinbarer EVG, SRE, Gärtemperatur (vorhandene Felder `SWJungbier`, `TemperaturJungbier`, Schnellgärprobe). Als Tabelle oder Chart in der Brauübersicht.

**S06 Zuckerrechner je Flasche (ext-s06)**
- In `TabAbfuellen`: Block „Zucker je Flasche“ mit g/L sowie g je 0,5 L und g je 0,33 L.
- Berechnet aus Ziel-CO2 (`Sud.CO2`), Temperatur vor Abfüllung (`TemperaturJungbier`) und Restextrakt (`SWJungbier` gegen erwarteten Endvergärungsgrad), wiederverwendet aus der bestehenden `ZuckerAnteil`-Rechnung bzw. `BierCalc`. Noch ohne festen g/L-Wert je Sud (folgt in S13).

### MVP-2 – Migration 2010
**S07 Migration 2010, nur Core (ext-s07)**
Eine gebündelte, idempotente Migration in `database_update.cpp` mit `ALTER TABLE ADD COLUMN` + `CREATE TABLE`, alle Defaults = Bestandverhalten:
- `Sud`: `berechnungsArtMalz` (0), `berechnungsArtWasser` (0 = Bestand), `WasserPreset` (−1 = keins), `FaktorNachguss` (3,5), `NachisoModell` (0 = Bestand), `Abfuellart` (−1 = abgeleitet), `ZuckerProLiter` (NULL = berechnet)
- `Ausruestung`: `Treberaufnahme` (0,96)
- Tabelle `Messwerte` (Konzept §6), Tabelle `AusruestungHistorie`
- Nachziehen: Spalten-Enums, `defaultValues`, `copyValues`, `sudKopieren`/`sudTeilen`, `removeRows`, `createTables/setTables`, `SudObject`-Proxy, Export/Import kbh (neue Tabelle), Template-DB `data/kb_daten.sqlite`, Versionsnummer (`kleiner-brauhelfer-core.pro`, `CMakeLists.txt`).
- Test: alte DB (2009) öffnen → Backup + Migration, Regressionsvergleich identisch; kbh-Export in die Altversion importierbar.

**S08 Hopfen-Modi mit eindeutigen Namen (ext-s08)** – *in S01 erledigt; S08 entfällt bzw. nur Nacharbeiten*
- Dropdown (Bestandswerte −1/0/1 + neu 2): „Ziel-IBU → Mengen nach Gewichtsanteil“, „Ziel-IBU → Mengen nach IBU-Anteil“, „Mengen fest, Bittere nicht berechnen“, **„Mengen fest → Bittere berechnen“ (neu)**, jeweils mit Tooltip.
- Prüfen, ob der heutige Wechsel des Modus (`modelhopfengaben.cpp` `onSudDataChanged`, `colUpdate`) Werte verändert. Falls ja, das Verhalten an die Invariante anpassen.

**S09 Malz-Modus „Mengen fest → SW berechnen“ (ext-s09)**
- Dropdown „Ziel-SW → Schüttung berechnen“ (Bestand) / „Malzmengen fest → SW berechnen“.
- Drift-Fix in `modelmalzschuettung.cpp` (`setDataExt`, `onSudDataChanged`) und `ModelSud::update` (Konzept §3).

**S10 Wasser-Modus + Presets (ext-s10)**
- `ModelSud::updateWasser`: Modus 1 „Faktoren HG+NG“ ⇒ HG = S·f_HG, NG = S·f_NG; erwartete Würzemenge rückgerechnet (Treberaufnahme aus Anlage, Korrektur) + Warnung bei Abweichung von der Sollmenge. Modus 0 bleibt exakt wie heute. Sondertypen Grainfather/Brauheld bleiben unverändert.
- UI `TabRezept`: Dropdown Modus, Dropdown Preset (Helles 4,0/3,0, Dunkles 3,0/4,0, Benutzerdefiniert 3,5/3,5), beide Faktoren editierbar. Ein Preset setzt nur die Faktoren; manuelles Ändern behält das Preset bei. Tooltip mit Richtwerten (Kling, ungeprüft).

**S11 Whirlpool/Hopfenseihen-Modell (ext-s11)**
- Dropdown im Hopfenbereich: „Nachisomerisierung Bestand“ / „Whirlpool (Wartezeit vor Abschwächung)“, umgesetzt als `BierCalc`-Variante (Befund `fachmodelle.md` §4). Wirkt auf `IBUAnteil` und im Ziel-IBU-Modus auf die Mengen.
- Ein Wechsel des Modells ist ein Modus-Wechsel im Sinne der Invariante: Die Mengen bleiben, die angezeigte IBU ändert sich. Das wird im Tooltip erklärt. Hier muss der Anwender bestätigen, ob die IBU-Anzeige sich ändern darf.

**S12 Brauprotokoll (ext-s12)**
- `ModelMesswerte` (Core) + Dialog `dialogs/dlgbrauprotokoll.*` (Checkliste Soll | Ist | Abweichung | Zeit | Methode, Volumen- und Extrakthilfen).
- Schutz der Ist-Felder in `ModelSud::setDataExt_impl`. Effektive Biermenge = letzter gemessener Volumenwert, sonst Soll mit Kennzeichnung „geschätzt“.

**S13 Abfüllart + Zucker fest in g/L (ext-s13)**
- Dropdown Abfüllart (Konzept §7.1 inkl. Zwangskarbonisierung).
- Bei „Zucker je Flasche“: fester Wert `ZuckerProLiter` (vorbelegt aus dem Rechner aus S06) → g je 0,5 L / 0,33 L.
- Zwei Alkoholwerte je Abfüllart (Jungbier / nach Nachgärung).

### Stufe 2+
- **S14 Kalibrierung (ext-s14)**: `DlgAusruestung` mit Gültigkeitsregeln, Median, Ausreißern, Vorschau und Historie, ergänzt um die Treberaufnahme aus Messwerten.
- **S15 Diagramme/Statistik erweitert (ext-s15)**: Verluste je Brauschritt, Hefe-Vergleich, Gärverlauf-Vergleich, CSV-Export. Python nur, wenn ein konkretes Diagramm in QCustomPlot unverhältnismäßig aufwendig wäre (vorher abstimmen).
- **Zurückgestellt**: Malzpotential, Zuckerarten-Liste, Flaschengröße je Sud (aktuell feste Anzeige 0,5/0,33 L).

## Kritische Dateien
`kleiner-brauhelfer-core/modelsud.{h,cpp}` (dataExt, setDataExt_impl, update, updateWasser) · `modelhopfengaben.cpp` · `modelmalzschuettung.cpp` · `biercalc.{h,cpp}` · `database.cpp`, `database_update.cpp` · `brauhelfer.{h,cpp}` · `importexport.cpp` · `sudobject.*` · `kleiner-brauhelfer/tabrezept.{cpp,ui}`, `tababfuellen.{cpp,ui}`, `dialogs/dlgbrauuebersicht.*`, `dialogs/dlgausruestung.*`, `widgets/` · `templatetags.cpp` · `.pro`-Dateien.

## Verifikation
- Jede Stufe: Build mit qmake fehlerfrei, App startet.
- Regressionsvergleich: Alle Sude der Referenz-DB haben vor und nach der Stufe identische `erg_*`- und Virtualwerte (Ausnahme: bewusst neue Felder).
- Moduswechsel-Test: In jedem neuen Dropdown hin und zurück schalten, dabei bleiben alle Werte unverändert. Danach eine Eingabe machen, dann wird wie gewohnt nachgerechnet.
- Ab S07: Alte 2009-DB wird migriert, Backup ist vorhanden, Daten sind vollständig, ein kbh-Export lässt sich in v2.6.5 importieren.
- Manuelle Testcheckliste je Stufe im Commit bzw. in der Doku festhalten.
