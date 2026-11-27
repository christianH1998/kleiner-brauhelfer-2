# CLAUDE.md – kleiner-brauhelfer-2

Desktop-App für Hobbybrauer (Rezepte, Sude, Rohstofflager, Gärverlauf, Bewertung). C++/Qt 6 Widgets, SQLite. Version 2.6.5, DB-Version **2012** (Fork: 2010 ab ext-s07, 2011 ab ext-s11, 2012 ab ext-s12b; Original: 2009 – Original-Programm öffnet die neueren DBs nicht). Sprache in Code/UI: Deutsch (Bezeichner, `tr()`-Quelltexte).

## Architektur (Kurzfassung)
- `kleiner-brauhelfer-core/` – statische Lib, **ohne GUI** (Qt Core/Sql/Xml): DB, Modelle, Berechnungen, Import/Export.
  - `Brauhelfer` (`brauhelfer.h`): Fassade, besitzt alle Modelle, `save()/discard()`, `sudKopieren/sudTeilen/rohstoffAbziehen`.
  - `Database` (`database.cpp`, `database_update.cpp`): Verbindung `"kbh"`, Transaktion beim Speichern, Migrationen.
  - `SqlTableModel` → je Tabelle ein `Model*` mit Spalten-Enum `Col*`, virtuellen Feldern (`dataExt/setDataExt`).
  - `ModelSud` = Rechenzentrum (`update()`), `BierCalc` = reine Formeln (statisch).
  - `SudObject`: aktuell geladener Sud + gefilterte `ProxyModel`s der Kindtabellen.
- `kleiner-brauhelfer/` – GUI-App: `MainWindow`, Tabs (`tab*.cpp/.ui`), `dialogs/`, `widgets/`, `model/` (Delegates), `charts/` (QCustomPlot), `commands/` (Undo), `templatetags.cpp` + `data/Webview/*.html` (Mustache-Ansichten).
- Globale Objekte: `Brauhelfer* bh` (main.cpp), `Settings* gSettings`, `UndoStack* gUndoStack` (mainwindow.cpp).
- Datenfluss: Widget → `gUndoStack->push(SetModelDataCommand)` → `SqlTableModel::setData` → `setDataExt` → Signale `rowChanged`/`dataChanged` → `ModelSud::update()` + Kindmodelle rechnen nach → GUI `updateValues()`.
- Alle Änderungen bleiben **im Speicher** (`OnManualSubmit`) bis `bh->save()`.

## Ergänzende Dokumentation (`doc/claude/`)
- `datenmodell.md` – Tabellen, Beziehungen, Modelle, Einheiten, DB-Migration, Speichersemantik.
- `berechnungen.md` – Rechenkette `ModelSud::update()`, virtuelle Felder, Gaben-Logik, `BierCalc`-Index.
- `gui.md` – Tabs, Widgets, SudObject-Bindung, Undo, Dialoge, Übersetzung, Build-Registrierung.
- `erweiterung-rezepteingabe.md` – Phase-1-Kurzbewertung Rezepteingabe (ersetzt durch Konzept).
- `konzept-erweiterung.md` – Phase-2-Konzept: Berechnungsmodi, Brauprotokoll, Kalibrierung, Abfüllung/Alkohol, Statistik, DB-Migration 2010, Priorisierung, offene Entscheidungen.
- `fachmodelle.md` – Formeln mit Einheiten, Quellen (mit Kennzeichnung geprüft/sekundär/abgeleitet/ungeklärt), Rechenbeispiele, Prüfszenarien.
- `../Umsetzung_Aenderungsprotokoll.txt` – Anwender-Protokoll je Stufe (Änderungen, neue Features, Bedienung); **nach jeder Stufe ergänzen**.
- `umsetzungsplan.md` – freigegebener Stufenplan S00–S15 (Entscheidungen des Anwenders, Invarianten, Git-Tags `ext-sNN`).

Nur bei Bedarf lesen; Detailwissen immer im Quellcode verifizieren.

## Besonders relevante Quellcode-Stellen
- `kleiner-brauhelfer-core/modelsud.cpp` – `dataExt` (virtuelle Werte), `setDataExt_impl` (Kaskaden), `update()`, `updateWasser/Farbe/Preis`.
- `kleiner-brauhelfer-core/model{malzschuettung,hopfengaben,hefegaben,weiterezutatengaben}.cpp` – Mengen ↔ Prozent.
- `kleiner-brauhelfer-core/biercalc.cpp` – Formeln.
- `kleiner-brauhelfer-core/importexport.cpp` – Import (kbh/MMuM/BeerXML) = Vorbild für programmatisches Anlegen eines Rezepts.
- `kleiner-brauhelfer/tabrezept.cpp/.ui` – Rezept-Editor (größte UI, ~4000 Zeilen .ui).
- `kleiner-brauhelfer/tabsudauswahl.cpp` – Sud anlegen/kopieren/importieren (Toolbar-Menü „Anlegen“).

## Build / Test
- Kanonisch: **qmake** (`kleiner-brauhelfer-2.pro`, CI `.github/workflows/*.yml`, `build_win64.cmd <qt-bin>`). Alternativ CMake (Glob).
- Qt ≥ 6.5 (CI: 6.11) inkl. WebEngine, WebChannel, Positioning, Svg, PrintSupport, Network.
- Neue Dateien: in `.pro` (`SOURCES/HEADERS/FORMS`) **eintragen**; CMake findet sie per Glob automatisch.
- **Keine automatisierten Unit-Tests.** Verifikation = Build + Regressionsvergleich (`tools/regression/`, siehe README dort: `kbh-dump` + `compare.py`, Referenz `build-win64/regression/baseline-*.json`) + manueller Test in der App mit einer **Kopie** der Datenbank (`Testdatenbank/`, nicht committen).
- Lokale Umgebung (verifiziert 2026-11-21): Qt 6.11 `C:\Qt\6.11.0\msvc2022_64` + VS 2022 Build Tools; `build_win64.cmd C:\Qt\6.11.0\msvc2022_64\bin` baut erfolgreich. Das Skript meldet immer Exit 0 ⇒ Log auf `error` prüfen. Laufende App sperrt die exe (LNK1104).
- Versionierung: Fork `origin` (christianH1998), Original `upstream`. Je Umsetzungsstufe ein Commit + Tag `ext-sNN` auf `feature-rezept-eingabe` (Plan: `doc/claude/umsetzungsplan.md`).

## Regeln für zukünftige Arbeit
- Das bestehende Projekt erweitern, nicht neu entwickeln.
- Bestehende Architektur und Designentscheidungen respektieren (Core ohne GUI, Modelle über `SqlTableModel`, UI über `SudObject`/Undo-Stack).
- Bestehende Funktionen und Berechnungen bevorzugt wiederverwenden (`BierCalc`, `ModelSud`-Virtualfelder, `Brauhelfer::sudKopieren*`, Import-Muster).
- Keine unnötigen Refactorings.
- Vor Änderungen die relevanten Quellcode-Stellen überprüfen.
- Größere Änderungen zunächst planen und mit mir abstimmen.
- Änderungen möglichst isoliert und in kleinen Schritten durchführen.
- Datenbankänderungen besonders sorgfältig prüfen (Migration in `database_update.cpp`, Versionsnummer, Spalten-Enums, Template-DB, Export/Import).
- Bestehende Datenbankstrukturen für Rohstoffe, Rezepte, Sud-Dokumentation usw. müssen weiter funktionieren; bestehende Datenbanken müssen ohne Datenverlust weiter nutzbar bleiben.
- Build und geeignete Tests nach Änderungen durchführen.
- Dokumentation (`CLAUDE.md`, `doc/claude/*`) bei relevanten Architekturänderungen aktualisieren.
- Keine Commits ohne meine ausdrückliche Zustimmung.

## Technische Fallstricke (Kurzliste)
- Spalten-Enums `Col*` müssen exakt der DB-Spaltenreihenfolge entsprechen, virtuelle Felder folgen danach (`Q_ASSERT` in `Database::setTables`).
- Rohstoffe werden in Gaben **per Name** referenziert (kein FK); Kindtabellen per `SudID` (kein FK, kein Cascade in SQLite).
- Rechenlogik greift nur bei `Status == Rezept`; gebraute Sude werden nicht mehr neu berechnet.
- `setData` löst Kaskaden aus; `append()` vs. `appendDirect()` (ohne Abhängigkeitsberechnung) bewusst wählen.
- Neue UI-Texte mit `tr()`; Übersetzungen (`translations/kbh_*.ts`) separat pflegen.
