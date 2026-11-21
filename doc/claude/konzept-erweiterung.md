# Konzept: Rezeptberechnung, Brauprotokoll, Anlagenkalibrierung

Stand 2026-11-21, Phase 2 (nur Konzept, **nicht umgesetzt**). Formeln/Quellen: `fachmodelle.md`. Bestand: `berechnungen.md`, `datenmodell.md`, `gui.md`. Ersetzt die Bewertung in `erweiterung-rezepteingabe.md` (bleibt als Kurzfassung).

## 1 Leitprinzipien
1. **Drei Wertarten strikt trennen**: *Eingabe* (vom Anwender festgelegt), *abgeleitet* (berechnet, nie gespeichert als Eingabe), *gemessen* (Brauprotokoll). Jede Anzeige kennzeichnet die Art (Symbol/Farbe + Tooltip „berechnet aus …“).
2. **Modus 0 = heutiges Verhalten.** Jede neue Option erhält einen Default, der bestehende Rezepte bit-gleich weiterrechnet. Formeländerungen nur als neue, wählbare Variante.
3. **Gebraute Sude einfrieren**: `ModelSud::update()` rechnet Rezeptwerte nur bei Status Rezept [C] – beibehalten. Neue Ist-/Kalibrierlogik darf `erg_*` gebrauter Sude nicht neu schreiben.
4. **Fehlend ≠ 0**: Messwerte nur als vorhandene Datensätze; Abwesenheit = „nicht gemessen“.
5. **Core vor GUI**: neue Rechenlogik in `kleiner-brauhelfer-core` (BierCalc-Funktionen, `ModelSud`-Virtualfelder, neue Modelle), GUI nur bindet.

## 2 Befunde aus dem Bestand, die das Konzept bestimmen (verifiziert)
- Rezeptlogik zielwertgetrieben; Malz-Gesamtmenge `erg_S_Gesamt` wird in jedem `update()` aus SW/Ausbeute/Menge neu gesetzt; `Malzschuettung.erg_Menge` folgt per `onSudDataChanged` aus `Prozent` (`modelmalzschuettung.cpp`).
- Hopfen: `berechnungsArtHopfen` (−1 Keine, 0 Gewicht, 1 IBU) existiert; bei „Keine“ bleiben Mengen fix, Gesamt-IBU wird nicht berechnet.
- Planungs-„Sudhausausbeute“ ist fachlich die effektive Ausbeute auf Anstellwürze (`fachmodelle.md` §1).
- `Malz.Potential` ungenutzt.
- **Ist-Felder werden im Rezeptstatus mit Sollwerten vorbelegt** (Kaskaden `setDataExt_impl`: SW→SWKochbeginn→SWKochende→SWAnstellen→SRE; Menge→Würzemengen) [C]. Eine SW-Änderung überschreibt so bereits eingetragene Messwerte, solange der Sud nicht „gebraut“ ist. Gemessen und vorbelegt sind nicht unterscheidbar.
- Braudaten werden nur im Status Rezept eingegeben; „Sud gebraut“ (`TabBraudaten::on_btnSudGebraut_clicked`) sperrt die Felder [C].
- Live gelesene Anlagenwerte: `KorrekturMenge` (→`MengeSollAnstellen`), `KorrekturWasser`, `KorrekturFarbe`, `Kosten` werden bei jedem Zugriff aus `Ausruestung` gelesen [C]; `Sudhausausbeute`/`Verdampfungsrate` werden beim Setzen der Anlage in den Sud kopiert [C]. Virtuelle Felder (z. B. `MengeSollAnstellen`, `IBUAnteil`, `SRESoll`) sind auch für gebraute Sude live ⇒ Änderungen an Anlage/Formeln verändern deren *Anzeige*, nicht die gespeicherten `erg_*`.
- Kalibrierung existiert rudimentär: `DlgAusruestung::updateDurchschnitt` = Mittel der letzten N gebrauten Sude (`erg_EffektiveAusbeute`, `VerdampfungsrateIst`), Ausschluss per `Sud.AusbeuteIgnorieren`, Übernahme per Knopf (`on_btnAusbeuteMittel_clicked`) [C].
- Alkohol an vier Stellen mit unterschiedlicher Bedeutung (§7.2).
- Diagramme: QCustomPlot (`charts/`), Brauübersicht `DlgBrauUebersicht` mit `Chart3` (bis 3 Reihen, Auswahlliste `mAuswahlListe`) und Mittelwerten über `ProxyModel::mean/min/max` [C].
- kbh-Export/Import arbeitet feldnamenbasiert (`toVariantMap`, `appendDirect(QVariantMap)` ignoriert unbekannte Namen) [C]; neue Spalten in bestehenden Tabellen werden automatisch exportiert, neue Tabellen brauchen eigenen Code. `importKbh` prüft nur `version < 2000` [C].

## 3 Berechnungsmodi (Bedienkonzept)
**Empfehlung: getrennte Modi je Bereich + optionale Schnellwahl.**
- Malz, Hopfen, Wasser sind fachlich unabhängig (SW ↔ Schüttung, IBU ↔ Hopfen, Wasser folgt Schüttung + Volumen). Ein globaler Modus würde Kombinationen wie „Malz fix, IBU-Ziel“ verhindern; Hopfen hat bereits einen eigenen Modus.
- Schnellwahl im Rezept-Kopf: „Rezept nachbauen (Mengen fix)“ / „Rezept planen (Zielwerte)“ setzt nur die Einzelmodi.

| Bereich | Modus | Eingabe | Abgeleitet | Speicherung |
|---|---|---|---|---|
| Malz | Ziel-SW (Bestand) | SW, Menge, Ausbeute, Prozent | Schüttung, kg je Malz | – |
| Malz | Mengen fix (neu) | kg je Malz, Menge, Ausbeute | SW, Prozent | neue Spalte `Sud.berechnungsArtMalz` (Default 0) |
| Hopfen | IBU / Gewicht (Bestand) | Soll-IBU, Anteile | g je Gabe | `berechnungsArtHopfen` 1 / 0 |
| Hopfen | Keine (Bestand) | g je Gabe | – (IBU-Feld frei) | −1 |
| Hopfen | Mengen fix, IBU berechnet (neu) | g je Gabe | `Sud.IBU` = Σ IBUAnteil | neuer Wert 2 in bestehender Spalte – **keine Schemaänderung** |
| Wasser | Faktor (Bestand) | FaktorHauptguss | HG, NG | – |
| Wasser | Biertyp-Preset (neu) | Preset → Faktor | wie Bestand | Preset nur als Vorschlag, Faktor bleibt gespeichert |

Regeln:
- Moduswechsel konvertiert **verlustfrei**: aktuelle abgeleitete Werte werden zu Eingaben (z. B. Malz Ziel-SW → fix: kg aus aktuellem `erg_Menge`; fix → Ziel-SW: SW = aktueller berechneter Wert, Prozent bleiben). Keine Werte werden gelöscht.
- Abgeleitete Felder sind schreibgeschützt; Eingabefelder des aktiven Modus normal. Widerspruch (z. B. extern vorgegebene SW ≠ berechnete SW im Modus „fix“) ⇒ Rezept-Zielwert als Referenz separat anzeigen, maßgeblich ist immer der Modus.
- Hopfen-Modus 2 bei Altversion: Combobox-Index ungültig, Setter-Switch ohne Fall ⇒ verhält sich wie „Keine“ [A]; akzeptabel, dokumentieren.

### Technische Umsetzung Malz „Mengen fix“
- In `ModelSud::update()` (Status Rezept): Σ `Malzschuettung.erg_Menge` ⇒ `erg_S_Gesamt`; SW aus `BierCalc` (neue Funktion, Fixpunkt, `fachmodelle.md` §2) **plus** WZ-Extrakt ⇒ `Sud.SW` setzen (Kaskaden laufen wie heute).
- **Risiko Drift**: `setDataExt(Colerg_Menge)` rechnet Prozent gegen die *alte* Gesamtschüttung; danach skaliert `onSudDataChanged(erg_S_Gesamt)` alle `erg_Menge` neu [C]. Im Modus fix muss das Setzen einer Menge alle Prozent des Suds gegen die neue Summe neu berechnen und `onSudDataChanged` darf `erg_Menge` nicht umskalieren. Betroffen: `modelmalzschuettung.cpp` (`setDataExt`, `onSudDataChanged`), `modelsud.cpp` (`update`). Regressionstest Modus 0 zwingend.
- Ohne Schemaänderung möglich (MVP-Vorstufe): virtuelles Feld `SWAusMalzmengen` + Knopf „SW aus Malzmengen übernehmen“ (analog `on_btnShaAnpassen_clicked`).

## 4 Wasser
- MVP: Biertyp-Presets als Vorschlag für `FaktorHauptguss` (Werte editierbar, Quelle gekennzeichnet), Anzeige Gesamt/HG/NG + NG je kg als Plausibilitätswert, Warnung bei NG < 0 bzw. Maischvolumen > Bottich (existiert in `TabRezept::updateValues`).
- Stufe 2: explizite Verlustkette (`fachmodelle.md` §3) mit Anlagenparametern; bestehende Felder `KorrekturWasser`/`KorrekturMenge` werden umbenannt angezeigt (Totraum/Zusatz bzw. Verlust nach Kochende), neue Spalte nur für Treberaufnahme (Default 0,96 ⇒ identisches Ergebnis).
- Unterscheiden und anzeigen: Gesamtwasser, HG, NG (Soll) ↔ gemessene Vorderwürze/Pfannevoll/Ausschlag (Ist, Brauprotokoll).

## 5 Hopfen
- MVP ohne DB: virtuelles Feld `ColIbuBerechnet` (Σ `IBUAnteil` der Hopfengaben des Suds) überall neben Soll-IBU anzeigen; im Modus „Keine“ Abweichungshinweis. Danach Modus 2.
- Gabenarten: Kochgabe/Vorderwürze/Kochbeginn/Kochende/Ausschlagen bestehen (`Brauhelfer::HopfenZeitpunkt`), Kalthopfung über Weitere Zutaten (Typ Hopfen). Whirlpool-Formel nur als optionale Variante (Befund `fachmodelle.md` §4).

## 6 Brauprotokoll
### Datenmodell (empfohlen: neue Tabelle, Langformat)
`Messwerte` (Migration 2010):
`ID INTEGER PK, SudID INTEGER NOT NULL, Schritt INTEGER NOT NULL, Groesse INTEGER NOT NULL, Wert REAL, Sollwert REAL, Temperatur REAL, Methode INTEGER, Zeitstempel DATETIME, Bemerkung TEXT`
- `Schritt` (Enum in `Brauhelfer`): Schüttung, Einmaischen, Rast n, Abmaischen, Läutern/Vorderwürze, Pfannevoll (Kochbeginn), Kochende/Ausschlagen, nach Kühlung, Anstellen/Gärbehälter, Gärung, Abfüllen.
- `Groesse`: Volumen [L, 20 °C normiert], Volumen roh + Temperatur, Extrakt [°P], Masse [kg/g], Temperatur [°C], Dauer [min], Druck [bar].
- `Methode`: Spindel, Refraktometer (inkl. Umrechnung über bestehende `DlgRestextrakt`/`BierCalc::brixToPlato`), Messstab/Skala, Waage, geschätzt.
- `Sollwert` = Snapshot zum Erfassungszeitpunkt ⇒ Historie unabhängig von späteren Formel-/Anlagenänderungen.
- Vorteile ggü. neuen Sud-Spalten: fehlend = keine Zeile, beliebig viele Messungen je Schritt, erweiterbar ohne weitere Migration, Korrekturen nachvollziehbar (neue Zeile/Zeitstempel). Nachteil: eigenes Modell, eigener Export/Import-Code.
- Kindtabelle per `SudID` ⇒ in `ModelSud::removeRows`, `Brauhelfer::sudKopieren` (nicht kopieren, außer `teilen`), `sudTeilen`, `ImportExport::exportKbh/importKbh`, `Database::createTables/setTables`, `Brauhelfer` (Getter, `modified`-Connect) und `SudObject` (ProxyModel) eintragen.

### Kopplung an bestehende Ist-Felder
- Messwerte für Schritte mit vorhandenem Sud-Feld (`SWKochbeginn`, `WuerzemengeKochbeginn`, `WuerzemengeVorHopfenseihen`, `SWKochende`, `WuerzemengeKochende`, `SWAnstellen`, `WuerzemengeAnstellen`, `SWJungbier`, `JungbiermengeAbfuellen`, `erg_AbgefuellteBiermenge` …) werden zusätzlich in das Sud-Feld geschrieben ⇒ bestehende Berechnungen, Charts, Templates, Statistik funktionieren unverändert.
- **Schutz**: Kaskaden in `ModelSud::setDataExt_impl` überschreiben ein Ist-Feld nicht, wenn für dieses Feld ein Messwert existiert (Abfrage über neues Modell). Ohne Messwert bleibt heutiges Verhalten (Sollwert-Vorbelegung) und UI zeigt „nicht gemessen – Sollwert“.
- Ist-Menge „effektiv“: neuer virtueller Wert = letzter vorhandener gemessener Volumenwert entlang der Kette (Abfüllen > Anstellen > nach Kühlung > Kochende), sonst Soll mit Kennzeichnung „geschätzt“. Bestehendes `MengeIst` bleibt unverändert.
- Unterscheidung Rezept / Brauverlauf / fertiges Bier: weiterhin eine Sud-Zeile (Status 0–3); Rezeptwerte = Sud-Eingaben, Brauverlauf = `Messwerte`, fertiges Bier = Abfüll-/Gärwerte + Bewertungen. Option für später: Sud-Kopie als „Rezeptvorlage“ (bereits über `sudKopieren` möglich).

### Bedienung
- Neuer, nicht-modaler Dialog `dialogs/dlgbrauprotokoll.*` (Muster `DlgAbstract::showDialog`), Action in Toolbar; Checkliste der Schritte mit Spalten Soll | Ist | Abweichung | Zeit | Methode, Schnellerfassung, Volumenhilfe (`DlgVolumen`), Extrakthilfe (`DlgRestextrakt`). Tab `TabBraudaten` bleibt unverändert nutzbar; später Verlinkung.

## 7 Abfüllung und Alkohol
### 7.1 Abfüllart (neue Spalte `Sud.Abfuellart`, Default −1 = „abgeleitet wie bisher“)
| Wert | Art | Rechnung |
|---|---|---|
| 0 | Flasche, Zucker gesamt (Batch-Priming) | bestehend: `ZuckerAnteil`, Wasser für Zuckerlösung = `VerschneidungAbfuellen` |
| 1 | Flasche, Zucker je Flasche | bestehend `tbZuckerFlasche` = Zucker gesamt · V_Flasche / V_Jungbier; keine Verdünnung |
| 2 | Speise | bestehend `SpeiseNoetig/SpeiseAnteil` (Modul Speise) |
| 3 | Spunden (Grünschlauchen) | bestehend `Spunden`, `Spundungsdruck`, `Gruenschlauchzeitpunkt` |
| 4 | Fass, Zwangskarbonisierung | neu: Druck = `BierCalc::p(CO2_soll, T)`, kein Zucker, kein Alkoholzuwachs |
- Legacy-Ableitung bei −1: `Spunden`=1 ⇒ 3; `Speisemenge`>0 ⇒ 2; sonst 0.
- Zuckerart: heute globaler `ZuckerFaktor` (INI). Vorschlag: Liste Zuckerarten mit CO2-/Alkoholausbeute (`fachmodelle.md` §6) und Speicherung je Sud (Spalte `Zuckerart` oder Tag) – Konstanten vorher fachlich bestätigen.
- Flaschengröße heute global (INI) ⇒ je Sud nur, wenn gewünscht (offene Frage).

### 7.2 Alkohol – Bestand [C]
| Anzeige | Formel | Ort |
|---|---|---|
| `AlkoholSoll` (Rezept) | alkohol(SW, SRE aus Vergärungsgrad) | TabRezept, TabAbfuellen „Rezept“, Template `Rezept.Alkohol`, MMuM-Export |
| `tbAlkoholGaerung` (nur UI) | alkohol(SWIst, SREIst) | TabAbfuellen |
| `erg_Alkohol` (gespeichert) | alkohol(SWIst, SREIst, Zucker)/(1 + Verdünnung/Menge); nur Status ≤ Gebraut berechnet | TabAbfuellen, Template `Sud.Alkohol`, Brauübersicht-Mittel |
| Gärverlauf `Alc` | alkohol(SWIst, SRE Messung) je Messzeile | Schnell-/Hauptgärverlauf |
- `SREIst` = Schnellgärprobe, falls aktiv, sonst `SWJungbier` ⇒ „Alkohol Gärung“ bedeutet je nach Einstellung *Endvergärung* oder *Stand bei Abfüllung*. Ursache der widersprüchlichen Werte.
- Verdünnung durch Zuckerlösung wirkt nur, wenn Zucker > 0 (Feld `VerschneidungAbfuellen` = „Wasser Zuckerlösung“ in `TabAbfuellen`).

### 7.3 Vorschlag
- Zentrale virtuelle Felder in `ModelSud` (keine DB-Änderung): `AlkoholJungbier` = alkohol(SWIst, SWJungbier); `AlkoholErwartet` = alkohol(SWIst, SRE_End, Zucker je Abfüllart)/(1 + Verdünnung), SRE_End = Schnellgärprobe falls vorhanden, sonst `SREErwartet` (Kennzeichnung „geschätzt“). Zwangskarbonisierung/Spunden: Zuckerbeitrag 0.
- Alle Anzeigen (Tabs, Templates, Brauübersicht) auf diese Felder umstellen und eindeutig beschriften („Rezept (Soll)“, „Jungbier“, „fertiges Bier (erwartet)“). `erg_Alkohol` bleibt gespeichert (Kompatibilität, Historie) und wird nicht rückwirkend neu berechnet.
- Abfüllmenge: Zucker skaliert mit `JungbiermengeAbfuellen` (Ist), Flaschenzahl mit `erg_AbgefuellteBiermenge` [C] – beibehalten.

## 8 Kalibrierung
- Kenngrößen je gebrautem Sud (alle bestehend oder aus Messwerten): klassische SHA (Ausschlag), effektive Ausbeute (Anstellwürze, = Planungsparameter), Verdampfungsrate, Verluste Kochende→Anstellen, Treberaufnahme = (W_ges,ist − V_Pfannevoll)/S_ist (nur mit Messwerten).
- Messpunkte für zuverlässige Ausbeute: Schüttung Ist (Waage), SW und Volumen Kochende (20 °C-normiert, Temperatur dokumentiert), SW und Volumen Anstellen. Ohne Ist-Schüttung wird Soll-Schüttung verwendet ⇒ Qualität „mittel“.
- Gültigkeit: alle Pflichtmessungen vorhanden, Methode ≠ „geschätzt“, Wert im Plausibilitätsbereich (z. B. Ausbeute 40–95 %, Verdampfung 0–30 %/h), nicht `AusbeuteIgnorieren`. Altsude ohne Messwerte: „Altbestand, ungeprüft“, standardmäßig einbezogen wie heute, abschaltbar.
- Methode: Median bzw. gleitendes Mittel der letzten N gültigen Sude (heutiger Slider), Ausreißer per Median ± k·MAD markieren; später optional Regression Ausbeute über SW (Ausbeute sinkt mit steigender SW [U, fachlich zu belegen]).
- Verwaltung: Anlagenwert bleibt manuell editierbar; Übernahme nur per Knopf mit Vorschau. Historie optional in neuer Tabelle `AusruestungHistorie` (AnlageName, Datum, Parameter, alt, neu, AnzahlSude, Methode). Da Sude Ausbeute/Verdampfung beim Anlagenwechsel kopieren, ändert eine Kalibrierung keine bestehenden Rezepte [C]; Live-Felder `Korrektur*` dagegen wirken auf alle Rezept-Sude und auf Anzeigen gebrauter Sude ⇒ Änderungen daran mit Hinweis.

## 9 Statistiken
| Auswertung | Nutzen | Aufwand | Daten |
|---|---|---|---|
| Ausbeute (eff./klassisch) über Zeit je Anlage | hoch | gering: Einträge in `DlgBrauUebersicht::mAuswahlListe` | vorhanden |
| Soll/Ist SW Anstellen | hoch | gering (neues Virtualfeld Δ) | vorhanden (Qualität unklar, Vorbelegung) |
| Soll/Ist Menge Anstellen/Abfüllen | hoch | gering | vorhanden |
| Verdampfungsrate Ist über Zeit | mittel | gering | vorhanden |
| EVG, Alkohol, Kosten über Zeit | mittel | bereits teilweise (Mittelwerte) | vorhanden |
| Verluste je Brauschritt (Wasserfall) | hoch | mittel (neuer Chart) | Messwerte nötig |
| Treberaufnahme, Totraum kalibriert | mittel | mittel | Messwerte nötig |
| Ausbeute vs. SW / Schüttung (Streudiagramm) | mittel | mittel (`Chart3` erweitern oder neuer Chart) | vorhanden |
| Entwicklung Anlagenparameter | mittel | gering | Historie-Tabelle nötig |
| Gärverlauf-Vergleich mehrerer Sude | mittel | mittel | vorhanden (Gärverlauf-Tabellen) |
| Rezeptvergleich (Kennzahlen nebeneinander) | gering | mittel | vorhanden |

## 10 Diagramme / Python
- Vorhanden: QCustomPlot 2.1.1 eingebunden (`charts/qcustomplot.*`), `ChartBase` + Spezialcharts, `Chart3` (Zeitreihen über Sude), Mittelwerte in `ProxyModel`. Reicht für alle Vorschläge in §9 (Linien, Balken, Streu, Fehlerbalken unterstützt QCustomPlot).
- Python: kein nachweisbarer Mehrwert für die geplanten Auswertungen (Mittel, Median, lineare Regression in C++ trivial). Nachteile: Interpreter + Pakete in Windows-Installer, AppImage, macOS-Bundle; zweite Toolchain in qmake/CMake/CI; Versionskonflikte. **Empfehlung: kein Python in der App.** Optional CSV-Export der Sud-/Messwert-Tabelle (`TableView` kann bereits kopieren/drucken) für externe Analyse.

## 11 Oberflächen-Zuordnung
| Bereich | Ort | Art |
|---|---|---|
| Berechnungsmodi Malz/Hopfen/Wasser | `TabRezept` (vorhandene Gruppen; Hopfen-Combobox existiert) | kleine Erweiterung `.ui` |
| Kennzeichnung eingegeben/berechnet/gemessen | `DoubleSpinBoxSud`/`WidgetDecorator` | zentrale Erweiterung |
| Brauprotokoll/Checkliste | neuer `DlgBrauprotokoll` | isoliert |
| Kalibrierung | `DlgAusruestung` (Mittelwert existiert) | Erweiterung |
| Abfüllart, Alkohol | `TabAbfuellen` | Erweiterung |
| Statistik | `DlgBrauUebersicht` | Erweiterung Auswahlliste/Charts |
Anwenderablauf: Rezept anlegen/öffnen (`TabSudAuswahl`) → Werte eingeben, Modi wählen (`TabRezept`) → speichern → Brauprotokoll öffnen, Messwerte erfassen → „Sud gebraut“ (`TabBraudaten`) → Gärung (`TabGaerverlauf1`) → Abfüllart + Abfüllen (`TabAbfuellen`) → Auswertung (`DlgBrauUebersicht`, `DlgAusruestung`).

## 12 Datenbank & Kompatibilität
- Eine gebündelte Migration **2010** in `database_update.cpp` statt mehrerer: `Sud.berechnungsArtMalz INTEGER DEFAULT 0`, `Sud.Abfuellart INTEGER DEFAULT -1`, (opt.) `Sud.Zuckerart`, `Ausruestung.Treberaufnahme REAL DEFAULT 0.96`, Tabelle `Messwerte`, (opt.) `AusruestungHistorie`. Nur `ALTER TABLE ADD COLUMN` + `CREATE TABLE` (kein Tabellenumbau).
- Versionsnummer in `kleiner-brauhelfer-core.pro` (`VER_MIN = 10`) **und** `CMakeLists.txt` (`2.10.x`), Template-DB `data/kb_daten.sqlite`, Spalten-Enums (neue Spalten vor `// virtual`), `defaultValues`, `copyValues`, `sudKopieren` (Messwerte nicht kopieren), `removeRows`.
- Ältere Programmversionen öffnen eine 2010-DB nicht (Abbruch bei `version > supported`, `main.cpp`) [C]; Update legt Backup an [C]. kbh-Export aus neuer Version in alter Version importierbar (unbekannte Felder/Tabellen ignoriert) [C], neue Informationen gehen dabei verloren.
- Alles mit Default = Bestandverhalten ⇒ bestehende Rezepte rechnen identisch.

## 13 Priorisierung
| Stufe | Funktion | Nutzen | Aufwand | Risiko | DB | Code |
|---|---|---|---|---|---|---|
| **MVP-1** | `IbuBerechnet` (Σ IBUAnteil) + Anzeige | hoch | gering | gering | nein | `modelsud.*`, `tabrezept.*` |
| MVP-1 | `SWAusMalzmengen` + Knopf „SW übernehmen“; BierCalc-Umkehrfunktion | hoch | gering | gering | nein | `biercalc.*`, `modelsud.*`, `tabrezept.*` |
| MVP-1 | Alkohol vereinheitlichen (Virtualfelder, Beschriftung) | hoch | gering–mittel | gering | nein | `modelsud.*`, `tababfuellen.*`, `templatetags.cpp`, Templates |
| MVP-1 | Kennzeichnung berechnet/eingegeben | mittel | mittel | gering | nein | `widgets/` |
| MVP-1 | Statistik-Reihen ergänzen | mittel | gering | gering | nein | `dlgbrauuebersicht.cpp` |
| **MVP-2** | Migration 2010 + Hopfen-Modus 2 + Malz-Modus fix | hoch | mittel | **mittel–hoch** (Drift, Kaskaden) | ja | `modelsud`, `modelmalzschuettung`, `modelhopfengaben`, `database_update` |
| MVP-2 | Brauprotokoll (Tabelle, Modell, Dialog, Schutz der Ist-Felder) | hoch | hoch | mittel | ja | neues Modell, `brauhelfer`, `database`, `sudobject`, `importexport`, neuer Dialog, `modelsud` |
| Stufe 2 | Kalibrierung mit Gültigkeitsregeln, Historie | hoch | mittel | gering | opt. | `dlgausruestung.*`, core |
| Stufe 2 | Abfüllart inkl. Zwangskarbonisierung, Zuckerarten | mittel | mittel | gering | ja (Spalten) | `tababfuellen.*`, `modelsud` |
| Stufe 2 | Wasser-Verlustkette, Biertyp-Presets | mittel | mittel | mittel | opt. | `modelsud::updateWasser`, `dlgausruestung` |
| Stufe 3 | Verlust-/Streudiagramme, Regression, Malzpotential, Whirlpool-Variante | mittel | mittel | gering | nein | `charts/`, `biercalc` |

Reihenfolge: (1) Build-Umgebung + Regressionsbasis (Referenz-DB, Vergleich `erg_*` vor/nach). (2) MVP-1 (ohne DB). (3) Entscheidung DB 2010. (4) Migration + Messwerte-Modell (Core, ohne UI) + Export/Import. (5) Brauprotokoll-Dialog + Ist-Feld-Schutz. (6) Modi Malz/Hopfen. (7) Abfüllart. (8) Kalibrierung. (9) Statistik/Diagramme.

## 14 Risiken
- Kaskaden/Reentrance (`mUpdating`, `mSkipUpdateOnOtherModelChanged`, `mSignalModifiedBlocked`) – neue Logik in `update()`/`setDataExt` kann Endlosschleifen oder Drift erzeugen.
- Ist-Feld-Vorbelegung: ohne Schutz überschreiben Rezeptänderungen Messwerte.
- Live-Anlagenwerte verändern Anzeigen gebrauter Sude.
- Fehlende automatisierte Tests; lokale Qt-Umgebung fehlt (Phase 1).
- Fachliche Konstanten mit unklarer Herkunft (Hopfenzuschläge, 0,468/0,488, Nachiso-Faktor) – nicht ändern ohne Quelle.
- Begriff „Sudhausausbeute“ (Planung = effektiv) – Umbenennung nur in UI-Texten/Tooltips, nicht in DB.

## 15 Offene Entscheidungen (Zustimmung erforderlich)
1. DB-Migration 2010 ja/nein und Umfang (Messwerte-Tabelle, neue Sud-Spalten).
2. MVP-1 ohne DB zuerst umsetzen?
3. Brauprotokoll als eigener Dialog (empfohlen) oder Integration in `TabBraudaten`?
4. Begriffe: Planungs-Ausbeute als „effektive Ausbeute“ beschriften?
5. Hopfen-Modus 2 als neuer Wert der bestehenden Spalte (empfohlen) akzeptiert?
6. Whirlpool-/Malzpotential-Varianten gewünscht (Stufe 3)?
7. Zuckerart und Flaschengröße je Sud statt global?
8. Bereitstellung Kling (Haupt-/Nachguss-Kapitel) und ggf. MEBAK/Narziß für die fachliche Bestätigung.
9. Kein Python in der App, stattdessen optional CSV-Export – einverstanden?
