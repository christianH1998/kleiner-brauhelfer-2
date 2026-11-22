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
| **Neu:** Benötigte Topfgröße + Hinweis Brauanlage | S10b |
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

**S02 Stammwürze aus Malzmengen (ext-s02)** – *entfällt (Anwender 2026-11-21): Knopf-Zwischenlösung gestrichen, Malz-Modus vollständig in S09*
- `BierCalc`: Umkehrfunktion Schüttung → SW (Fixpunkt, `fachmodelle.md` §2) plus Extrakt aus weiteren Zutaten.
- Virtuelles Feld `SWAusMalzmengen` und Knopf „SW aus Malzmengen übernehmen“ (Muster `on_btnShaAnpassen_clicked`).

**S03 Alkohol vereinheitlichen (ext-s03)** – *umgesetzt mit Abweichung: kein eigenes `AlkoholErwartet`, weil `erg_Alkohol` bereits exakt diese Formel ist (SREIst = Schnellgärprobe/Jungbier, Zucker außer bei Spunden, Verdünnung)*
- Virtuelles Feld `AlkoholJungbier`; `erg_Alkohol` = „fertiges Bier (erwartet)“ (Konzept §7.3).
- Beschriftungen „Rezept (Soll)“ / „Jungbier“ / „fertiges Bier (erwartet)“ in `tababfuellen`, `tabrezept`, `templatetags.cpp` + Templates, Brauübersicht. `erg_Alkohol` bleibt unverändert gespeichert.

**S04 Kennzeichnung und Tooltips (ext-s04)** – *umgesetzt: Kennzeichnung über Palette `paletteBerechnet` für alle schreibgeschützten `DoubleSpinBox`/`SpinBox`; klassische SHA im Rezept bewusst nicht angezeigt (Planungsmodell ohne Verluste Kochende→Anstellen, wäre ≈ Planungswert), stattdessen Begriffe in Rezept/Braudaten/Brauübersicht geschärft*
- Zentrale Kennzeichnung eingegeben/berechnet in `widgets/` (`DoubleSpinBoxSud`/`WidgetDecorator`).
- Tooltip „Sudhausausbeute = effektive Ausbeute …“ und Anzeige der klassischen SHA (berechnet).
- Muster für Modus-Dropdown-Tooltips festlegen (Tooltip pro Eintrag über `setItemData(..., Qt::ToolTipRole)` + Tooltip am Combo).

**S05 Statistik-Reihen + Hefe-Auswertung (ext-s05)** – *umgesetzt: Soll-Reihen + Gärdaten in `mAuswahlListe` (am Ende, Spalten ausgeblendet), Spalte Hefe, Gruppe „Auswertung je Hefe“ (QTableWidget, Doppelklick = Textfilter); virtuelle Felder `Hefe`, `DauerHauptgaerung`, `TemperaturHauptgaerung`*
- Neue Einträge in `DlgBrauUebersicht::mAuswahlListe`: Ausbeute (eff./klassisch), Soll/Ist SW Anstellen, Soll/Ist Menge, Verdampfungsrate.
- Hefe-Statistik (nur lesend): je Hefe aus `Hefegaben` die Anzahl Sude, mittlerer scheinbarer EVG, SRE, Gärtemperatur (vorhandene Felder `SWJungbier`, `TemperaturJungbier`, Schnellgärprobe). Als Tabelle oder Chart in der Brauübersicht.

**S06 Zuckerrechner je Flasche (ext-s06)** – *umgesetzt als Anzeige (g/L exakt aus `ZuckerAnteil`/Süßkraft/Jungbiermenge, ×0,5, ×0,33) in Gruppe Karbonisierung; fester g/L-Wert je Sud folgt in S13*
- In `TabAbfuellen`: Block „Zucker je Flasche“ mit g/L sowie g je 0,5 L und g je 0,33 L.
- Berechnet aus Ziel-CO2 (`Sud.CO2`), Temperatur vor Abfüllung (`TemperaturJungbier`) und Restextrakt (`SWJungbier` gegen erwarteten Endvergärungsgrad), wiederverwendet aus der bestehenden `ZuckerAnteil`-Rechnung bzw. `BierCalc`. Noch ohne festen g/L-Wert je Sud (folgt in S13).

### MVP-2 – Migration 2010
**S07 Migration 2010, nur Core (ext-s07)** – *umgesetzt; Abweichung: Kodierung 0/NULL = bisheriges Verhalten (`WasserPreset` 0 keins…, `Abfuellart` 0 abgeleitet, `FaktorNachguss` 0 = nicht gesetzt), weil neue ungespeicherte Zeilen NULL lesen. `AusruestungHistorie` nur Tabelle (Modell in S14). Offener Hinweis: Update-Prüfung zeigt Releases des Originals (ggf. später auf Fork umstellen).*
Eine gebündelte, idempotente Migration in `database_update.cpp` mit `ALTER TABLE ADD COLUMN` + `CREATE TABLE`, alle Defaults = Bestandverhalten:
- `Sud`: `berechnungsArtMalz` (0), `berechnungsArtWasser` (0 = Bestand), `WasserPreset` (−1 = keins), `FaktorNachguss` (3,5), `NachisoModell` (0 = Bestand), `Abfuellart` (−1 = abgeleitet), `ZuckerProLiter` (NULL = berechnet)
- `Ausruestung`: `Treberaufnahme` (0,96)
- Tabelle `Messwerte` (Konzept §6), Tabelle `AusruestungHistorie`
- Nachziehen: Spalten-Enums, `defaultValues`, `copyValues`, `sudKopieren`/`sudTeilen`, `removeRows`, `createTables/setTables`, `SudObject`-Proxy, Export/Import kbh (neue Tabelle), Template-DB `data/kb_daten.sqlite`, Versionsnummer (`kleiner-brauhelfer-core.pro`, `CMakeLists.txt`).
- Test: alte DB (2009) öffnen → Backup + Migration, Regressionsvergleich identisch; kbh-Export in die Altversion importierbar.

**S08 Hopfen-Modi mit eindeutigen Namen (ext-s08)** – *in S01 erledigt; S08 entfällt bzw. nur Nacharbeiten*
- Dropdown (Bestandswerte −1/0/1 + neu 2): „Ziel-IBU → Mengen nach Gewichtsanteil“, „Ziel-IBU → Mengen nach IBU-Anteil“, „Mengen fest, Bittere nicht berechnen“, **„Mengen fest → Bittere berechnen“ (neu)**, jeweils mit Tooltip.
- Prüfen, ob der heutige Wechsel des Modus (`modelhopfengaben.cpp` `onSudDataChanged`, `colUpdate`) Werte verändert. Falls ja, das Verhalten an die Invariante anpassen.

**S09 Malz-Modus „Mengen fest → SW berechnen“ (ext-s09)** – *umgesetzt: `BierCalc::swAusSchuettung` (Fixpunkt), `ModelSud::updateSwAusMalzmengen` (SW nur bei Δ>1e-9 setzen), `ModelMalzschuettung` kg führend + `prozentAusMengen`; Rundlauftest `--rundlauf-malz`*
- Dropdown „Ziel-SW → Schüttung berechnen“ (Bestand) / „Malzmengen fest → SW berechnen“.
- Drift-Fix in `modelmalzschuettung.cpp` (`setDataExt`, `onSudDataChanged`) und `ModelSud::update` (Konzept §3).

**S10 Wasser-Modus + Presets (ext-s10)** – *umgesetzt: Modus 1 ohne Korrektur im NG (Korrektur nur in der erwarteten Würzemenge, virtuelles Feld `WuerzemengeKochbeginnErwartet`), Wechsel 0→1 übernimmt NG als `FaktorNachguss`, Warnung ab 0,1 L Abweichung, Preset/NG-Faktor nur im Modus 1 sichtbar, Wasser-Auswahl bei Grainfather/Brauheld ausgeblendet; **Erweiterung (Anwender 2026-11-22):** Auswahl „Berechnung: Planen / Rezept nachbrauen“ links im Rezept, Faktoren HG/NG links (keine Liter-Eingabe), `berechnungsArtWasser = 2` Nachbrauen ⇒ Menge aus Malzmengen + Faktoren (`ModelSud::mengeAusWasser`, lineare Rückwärtskette), SW/IBU berechnet; Menge-Eingabe skaliert Malz (exakt auf die Menge) und Hopfen (Mengenverhältnis); Treberaufnahme (Fallback 0,96) mit Eingabefeld in `DlgAusruestung`; Rundlauftest `--rundlauf-wasser`*
- `ModelSud::updateWasser`: Modus 1 „Faktoren HG+NG“ ⇒ HG = S·f_HG, NG = S·f_NG; erwartete Würzemenge rückgerechnet (Treberaufnahme aus Anlage, Korrektur) + Warnung bei Abweichung von der Sollmenge. Modus 0 bleibt exakt wie heute. Sondertypen Grainfather/Brauheld bleiben unverändert.
- UI `TabRezept`: Dropdown Modus, Dropdown Preset (Helles 4,0/3,0, Dunkles 3,0/4,0, Benutzerdefiniert 3,5/3,5), beide Faktoren editierbar. Ein Preset setzt nur die Faktoren; manuelles Ändern behält das Preset bei. Tooltip mit Richtwerten (Kling, ungeprüft).

**S10b Benötigte Topfgröße + Hinweis Brauanlage (ext-s10b)** – *neu, Anwender 2026-11-22; umgesetzt: Felder `tbMaxFluessigkeit`, `tbTopfgroesse` (= max/0,95), `lblWarnungAnlage` in `groupAnlage`; Markierung „Benötigtes Volumen“ jetzt ab 95 %, kein Hinweis bei Volumen 0; **geändert in S11 (Anwender 2026-11-22): keine feste 5-%-Reserve, Grenze = nutzbares Volumen (max. Füllhöhe), Topfgröße = Flüssigkeit × Gesamt-/Nutzvolumen des Gefäßes***
- **Anforderung:** Im Reiter Rezept rechts (Übersicht, Gruppe „Anlage“) direkt sichtbar die voraussichtlich benötigte Topfgröße anzeigen:
  - **Maischen:** Hauptguss + Volumen des Malzes (Malzverdrängung `BierCalc::MalzVerdraengung` × Schüttung) – heute schon als „Volumen Maische“ berechnet.
  - **Kochbeginn:** Würzemenge zu Kochbeginn aus Hauptguss + Nachguss (abzüglich Treberaufnahme bzw. die Soll-Würzemenge zu Kochbeginn, heiß auf 100 °C umgerechnet wie heute `tbVolumenKochen`).
  - **Angezeigt wird die erwartete maximale Flüssigkeitsmenge** (Entscheidung Anwender 2026-11-22): je Gefäß der größte zu erwartende Wert (Maischebottich: Maische; Sudpfanne: Würze Kochbeginn heiß), bei Ein-Topf-Anlagen der größere der beiden.
- **Hinweis**, wenn die maximale Flüssigkeitsmenge mehr als **95 % des maximalen Füllvolumens** der Brauanlage beträgt (**5 % Luft** eingeplant, Entscheidung Anwender 2026-11-22; `Ausruestung.Maischebottich_MaxFuellvolumen` / `Sudpfanne_MaxFuellvolumen`): klarer Hinweistext (rot) mit Überschreitung in Litern und Abhilfe (Menge/Faktoren reduzieren, andere Anlage) – heute nur rote Markierung des Zahlenfelds bei > 100 %.
- Gilt in allen Modi; besonders relevant beim Nachbrauen/Skalieren (S10), wo die Menge aus Malz und Faktoren entsteht. Reine Anzeige: keine DB-Änderung, keine Wertänderung (Invariante).
- Kochbeginn: im Faktoren-Modus der größere Wert aus Soll (`MengeSollKochbeginn`) und erwarteter Würzemenge (`WuerzemengeKochbeginnErwartet`), sonst Soll.

**S11 Whirlpool/Hopfenseihen-Modell (ext-s11)** – *umgesetzt, nach Anwendertest umgebaut (2026-11-22): Ablauf beim Anwender = alle Gaben während des Kochens, danach Whirlpool und ~30 min Absetzen bis zum Ablassen ⇒ Modell „Whirlpool mit Abkühlkurve“ (`NachisoModell = 1`): Temperaturverlauf aus Wärmebilanz (Verdunstung offene Oberfläche, Konvektion, Strahlung; Topfdurchmesser Sudpfanne aus Ausrüstung, Würzemenge, Außentemperatur, Deckel), Bittere = Integral der Isomerisierungsrate (`BierCalc::whirlpoolMinuten`) für alle Gaben; Kalibrierung über gemessene Endtemperatur (`Ausruestung.WhirlpoolFaktor`, Knopf „Kalibrieren…“). **DB-Migration 2011** (Anwender 2026-11-22): `Sud.Aussentemperatur` (später auch S12), `Sud.WhirlpoolDeckel`, `Ausruestung.WhirlpoolFaktor`. Anwender bestätigt: Mengen bleiben, IBU ändert sich (auch in Ziel-IBU-Modi); Rundlauftest `--rundlauf-nachiso`*
- Dropdown im Hopfenbereich: „Nachisomerisierung Bestand“ / „Whirlpool (Wartezeit vor Abschwächung)“, umgesetzt als `BierCalc`-Variante (Befund `fachmodelle.md` §4). Wirkt auf `IBUAnteil` und im Ziel-IBU-Modus auf die Mengen.
- Ein Wechsel des Modells ist ein Modus-Wechsel im Sinne der Invariante: Die Mengen bleiben, die angezeigte IBU ändert sich. Das wird im Tooltip erklärt. Hier muss der Anwender bestätigen, ob die IBU-Anzeige sich ändern darf.

**S12 Brauprotokoll (ext-s12)**
- `ModelMesswerte` (Core) + Dialog `dialogs/dlgbrauprotokoll.*` (Checkliste Soll | Ist | Abweichung | Zeit | Methode, Volumen- und Extrakthilfen).
- Schutz der Ist-Felder in `ModelSud::setDataExt_impl`. Effektive Biermenge = letzter gemessener Volumenwert, sonst Soll mit Kennzeichnung „geschätzt“.
- **Anforderung Anwender (2026-11-22):** Reale Zeiten je Maischschritt dokumentieren: Aufheizdauer und Rastdauer, vorbelegt mit den Rezeptwerten aus dem Maischplan, im Protokoll änderbar. Die Aufheizzeit hängt u. a. von der Außentemperatur ab, deshalb die Außentemperatur mit erfassen (Spalte `Sud.Aussentemperatur` existiert ab DB 2011/S11) (Grundlage für spätere Auswertung/Kalibrierung der Aufheizrate in S14/S15).

**S13 Abfüllart + Zucker fest in g/L (ext-s13)**
- Dropdown Abfüllart (Konzept §7.1 inkl. Zwangskarbonisierung).
- Bei „Zucker je Flasche“: fester Wert `ZuckerProLiter` (vorbelegt aus dem Rechner aus S06) → g je 0,5 L / 0,33 L.
- Zwei Alkoholwerte je Abfüllart (Jungbier / nach Nachgärung). *(Anzeige bereits in S03 vereinheitlicht.)*
- **Anforderung Anwender (2026-11-21, nach Test S03, erneut bestätigt bei S06):** Auswahl „Karbonisierungsart“/Abfüllart im Reiter Abfüllen; **je Variante nur die relevanten Felder sichtbar** (heute sind alle Eingabefelder gleichzeitig sichtbar). Entwurf:
  - Flasche – Zucker in den ganzen Sud: Temperatur, Süßkraft, Wasser Zuckerlösung → Zucker gesamt + je Flasche
  - Flasche – Zucker je Flasche: Ziel-CO2, Restextrakt, Temperatur → g/L fest (Rechner S06 als Vorschlag) → g je 0,5 L / 0,33 L
  - Flasche – Speise: verfügbare Speise → Speise gesamt/je Flasche, ggf. Restzucker
  - Spunden: Temperatur → Spundungsdruck, Grünschlauchzeitpunkt
  - Fass – Zwangskarbonisierung: Temperatur → Druck für Ziel-CO2, kein Zucker
  - Altsude: Abfüllart aus `Spunden`/`Speisemenge` ableiten (−1 = abgeleitet), Rechnung unverändert; Wechsel blendet nur um, ändert keine Werte. Offene Rückfrage an Anwender in S13: weitere Varianten (Fass mit Zucker, Speise + Zucker)?

**S13b Gesamtübersicht fertiges Bier (ext-s13b)** – *neu, Anwender 2026-11-21*
- Kompakte Übersicht „Was ist wirklich in der Flasche“: abgefüllte Menge (gemessen) und Anzahl Flaschen, Alkohol fertiges Bier, Stammwürze Ist, Restextrakt und Vergärungsgrad, Bittere und Farbe Ist, CO2 (Ziel/Ist), Abfüllart mit Zucker bzw. Speise je Flasche, Hefe, Brau-, Abfüll- und Reifedatum, Bewertung. Dazu je Wert die Herkunft (gemessen/berechnet) und der Sollwert zum Vergleich.
- Leicht erreichbar: Knopf im Reiter Abfüllen, in der Toolbar bzw. Sudauswahl (ausgewählter Sud) und im Menü. Als Dialog mit Drucken/PDF über die vorhandene Mustache-Vorlagentechnik (`templatetags.cpp`, `data/Webview/`). Vorhandene `zusammenfassung.html` als Ausgangspunkt prüfen.
- Platz nach S13, weil die Übersicht Abfüllart (S13) und gemessene Mengen (S12) braucht. Eine erste Version mit vorhandenen Daten wäre auch früher möglich.

### Stufe 2+
- **S14 Kalibrierung (ext-s14)**: `DlgAusruestung` mit Gültigkeitsregeln, Median, Ausreißern, Vorschau und Historie, ergänzt um die Treberaufnahme aus Messwerten.
- **S15 Diagramme/Statistik erweitert (ext-s15)**: Verluste je Brauschritt, Hefe-Vergleich, Gärverlauf-Vergleich, CSV-Export. Python nur, wenn ein konkretes Diagramm in QCustomPlot unverhältnismäßig aufwendig wäre (vorher abstimmen).
- **S16 (optional) Export ins alte Datenbankformat 2009 (ext-s16)** – *Anwender 2026-11-21*: Kopie der aktuellen Datenbank als Version 2009 speichern, damit das Original-Programm (2.6.5) sie öffnen kann. Vorher Warnhinweis mit Liste der dabei verlorenen Daten (neue Sud-Spalten: Malz-/Wasser-/Whirlpool-Modus, Wasser-Preset, Nachguss-Faktor, Abfüllart, Zucker g/L; Ausrüstung.Treberaufnahme; ab 2011 Sud.Aussentemperatur, Sud.WhirlpoolDeckel, Ausrüstung.WhirlpoolFaktor; Tabellen Messwerte und AusruestungHistorie). Hopfen-Modus 2 wird dabei auf „Keine“ (−1) gesetzt. Zugang unauffällig im Einstellungsdialog, Bereich Datenbank. Umsetzung: Kopie anlegen, neue Spalten/Tabellen per Tabellenumbau entfernen, `Global.db_Version = 2009`.
- **Zurückgestellt**: Malzpotential, Zuckerarten-Liste, Flaschengröße je Sud (aktuell feste Anzeige 0,5/0,33 L).

## Kritische Dateien
`kleiner-brauhelfer-core/modelsud.{h,cpp}` (dataExt, setDataExt_impl, update, updateWasser) · `modelhopfengaben.cpp` · `modelmalzschuettung.cpp` · `biercalc.{h,cpp}` · `database.cpp`, `database_update.cpp` · `brauhelfer.{h,cpp}` · `importexport.cpp` · `sudobject.*` · `kleiner-brauhelfer/tabrezept.{cpp,ui}`, `tababfuellen.{cpp,ui}`, `dialogs/dlgbrauuebersicht.*`, `dialogs/dlgausruestung.*`, `widgets/` · `templatetags.cpp` · `.pro`-Dateien.

## Verifikation
- Jede Stufe: Build mit qmake fehlerfrei, App startet.
- Regressionsvergleich: Alle Sude der Referenz-DB haben vor und nach der Stufe identische `erg_*`- und Virtualwerte (Ausnahme: bewusst neue Felder).
- Moduswechsel-Test: In jedem neuen Dropdown hin und zurück schalten, dabei bleiben alle Werte unverändert. Danach eine Eingabe machen, dann wird wie gewohnt nachgerechnet.
- Ab S07: Alte 2009-DB wird migriert, Backup ist vorhanden, Daten sind vollständig, ein kbh-Export lässt sich in v2.6.5 importieren.
- Manuelle Testcheckliste je Stufe im Commit bzw. in der Doku festhalten.
