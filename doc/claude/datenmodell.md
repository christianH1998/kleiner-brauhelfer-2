# Datenmodell & Datenbank

Verifiziert gegen `kleiner-brauhelfer/data/kb_daten.sqlite` (Template-DB, `Global.db_Version = 2009`) und `kleiner-brauhelfer-core/`.

## Tabellen → Modelle
| Tabelle | Modell (`kleiner-brauhelfer-core/`) | Schlüssel/Bezug |
|---|---|---|
| Sud | `ModelSud` | ID; Rezept + Sud-Doku in **einer** Zeile |
| Malzschuettung | `ModelMalzschuettung` | SudID, Name→Malz.Name |
| Hopfengaben | `ModelHopfengaben` | SudID, Name→Hopfen.Name |
| Hefegaben | `ModelHefegaben` | SudID, Name→Hefe.Name |
| WeitereZutatenGaben | `ModelWeitereZutatenGaben` | SudID, Name→WeitereZutaten.Name **oder** Hopfen.Name (Typ=100 Stopfhopfen) |
| Maischplan | `ModelMaischplan` | SudID (Rasten, Typ = `Brauhelfer::RastTyp`) |
| Wasseraufbereitung | `ModelWasseraufbereitung` | SudID |
| Schnell-/Haupt-/Nachgaerverlauf | `ModelSchnell-/Haupt-/Nachgaerverlauf` | SudID |
| Bewertungen, Anhang, Etiketten, Tags | jeweilige Modelle | SudID |
| Malz, Hopfen, Hefe, WeitereZutaten | `ModelMalz/Hopfen/Hefe/WeitereZutaten` | Name UNIQUE (Lager/Rohstoffe) |
| Ausruestung, Geraete | `ModelAusruestung/Geraete` | Ausruestung.Name ← Sud.Anlage |
| Wasser | `ModelWasser` | Name ← Sud.Wasserprofil |
| Kategorien | `ModelKategorien` | Name ← Sud.Kategorie |
| Global | – | `db_Version` |

- **Keine Foreign Keys.** Beziehungen rein logisch über `SudID` bzw. Namen (Text).
- Gaben **kopieren** Stoffwerte beim Setzen des Namens (z. B. Malz Farbe/Potential/pH, Hopfen Alpha/Pellets) → Rezept bleibt stabil, auch wenn Rohstoff später geändert/gelöscht wird. `Model*Gaben::import(row)` legt fehlenden Rohstoff aus Gabe an.
- Umbenennen eines Rohstoffs propagiert nur in Sude mit Status Rezept (`Model*Gaben::update(name,col,value)` mit `ProxyModelSud::Rezept`).
- Sud-Status `Brauhelfer::SudStatus`: 0 Rezept, 1 Gebraut, 2 Abgefuellt, 3 Verbraucht. Ein „Rezept“ ist ein Sud mit Status 0.

## Wichtige Sud-Spalten (Auswahl, vollständige Liste: `modelsud.h` Enum)
- Rezept-Sollwerte: `Menge` [L Anstellwürze], `SW` [°P], `IBU`, `CO2` [g/L], `Kochdauer` [min], `Nachisomerisierungszeit`, `Vergaerungsgrad` [%], `Sudhausausbeute` [%], `Verdampfungsrate` [L/h], `FaktorHauptguss`, `highGravityFaktor` [%], `berechnungsArtHopfen` (-1 Keine, 0 Gewicht, 1 IBU), `RestalkalitaetSoll`, `MengeHefestarter`, `SWHefestarter`, `Reifezeit` [Wochen].
- Berechnete, **persistierte** Ergebnisse `erg_*`: `erg_S_Gesamt` [kg], `erg_W_Gesamt/WHauptguss/WNachguss` [L], `erg_Farbe` [EBC], `erg_Sudhausausbeute`, `erg_EffektiveAusbeute`, `erg_Preis` [€/L], `erg_Alkohol` [%vol], `erg_AbgefuellteBiermenge`.
- Brau-/Abfüll-Istwerte: `SWKochbeginn/SWKochende/SWAnstellen`, `WuerzemengeKochbeginn/VorHopfenseihen/Kochende/Anstellen`, `SWSchnellgaerprobe`, `SWJungbier`, `Speisemenge`, `JungbiermengeAbfuellen`, Datumsfelder (ISO-String in DB).
- Bemerkungen (HTML): `Kommentar`, `Bemerkung*`.
- Virtuelle Spalten (nicht in DB) ab `ColDeleted`: siehe `berechnungen.md`.

## Einheiten der Gaben (verifiziert)
- Malzschuettung: `Prozent` [% der Schüttung] = führende Größe; `erg_Menge` [kg] = Prozent × `Sud.erg_S_Gesamt`.
- Hopfengaben: `Prozent` (Anteil IBU bzw. Gewicht je Berechnungsart); `erg_Menge` [g]; `Zeit` [min Kochzeit, negativ = Nachisomerisierung]; DB-Spalte `Vorderwuerze` ↔ Enum `ColZeitpunkt` (`Brauhelfer::HopfenZeitpunkt`).
- Hefegaben: `Menge` [Integer, Packungen/Einheiten].
- WeitereZutatenGaben: `erg_Menge` in Basiseinheit g / ml / Stk; `Menge` = erg_Menge / `MengeSollAnstellen` (pro Liter); `Einheit` = Anzeige-/Lagereinheit (`Brauhelfer::Einheit`); `Ausbeute` [%] → Extraktbeitrag; `Zeitpunkt` 0 Gärung/1 Kochen/2 Maischen; `ZugabeNach` [Tage], `Zugabedauer` [min].
- **Unsicher:** `importBeerXml` setzt bei Nicht-Malz-Fermentables `Menge` = kg/L mit `Einheit=Kg`, während sonst g-Basis gilt → mögliche Inkonsistenz (Faktor 1000), nicht verifiziert.

## SqlTableModel-Mechanik (`sqltablemodel.cpp`)
- `EditStrategy::OnManualSubmit`; `Database::save()` = eine Transaktion über alle Modelle (`submitAll`), Rollback bei Fehler.
- `data()/setData()` versuchen zuerst `dataExt/setDataExt` (Modell-Logik), sonst Rohwert.
- `setData` emittiert `rowChanged` nur auf äußerster Ebene (`mSetDataCnt`), `modified` sofern nicht `mSignalModifiedBlocked`.
- `append(values)`: `defaultValues()` + Rohwerte + danach `setDataExt` für alle Werte (Kaskaden laufen, Signale blockiert). `appendDirect(values)`: nur Rohwerte, keine Logik.
- IDs: `getNextId()` = max(ID)+1 im Modell (nicht SQLite-Autoincrement).
- Löschen: Zeile wird bis zum Speichern als gelöscht markiert (`ColDeleted`, Header „!“), `ProxyModel` filtert sie aus.
- `ModelSud::removeRows` löscht Kindzeilen in Maischplan, Malz-/Hopfen-/WZ-/Hefegaben, Gärverläufen, Bewertungen, Anhang, Etiketten, Tags. **Wasseraufbereitung fehlt in der Liste** (verifiziert; mögliche Waisen-Zeilen, bestehendes Verhalten).

## DB-Versionierung & Migration
- Unterstützte Version = `VER_MAJ*1000 + VER_MIN` aus `kleiner-brauhelfer-core.pro` (`VER_MIN = 9` → 2009) bzw. `CMakeLists.txt` (`VERSION 2.9.12`). Beide synchron halten.
- Minimal unterstützt: 21 (`Brauhelfer::supportedDatabaseVersionMinimal`).
- `Database::update()` (`database_update.cpp`): Kette `if (version == N) {++version; transaction; ALTER/CREATE...; UPDATE Global; commit}`. Tabellen-Umbau via `RENAME TO TempTable` → `CREATE` → `INSERT ... SELECT` → `DROP`.
- Ablauf beim Start (`main.cpp: connectDatabase()`): ältere DB → Nutzerabfrage, Backup-Kopie, Update auf Kopie `*_update.sqlite`, Rückkopie. Neuere DB als unterstützt → Abbruch.
- Template-DB für Neuinstallation: `kleiner-brauhelfer/data/kb_daten.sqlite` (Ressource `:/data/kb_daten.sqlite`).
- **Checkliste DB-Änderung:** Migration anfügen; Versionsnummer in `.pro` **und** CMake erhöhen; `Col*`-Enum in DB-Spaltenreihenfolge ergänzen (neue Spalten hinten, vor `// virtual`); `defaultValues`/`copyValues` prüfen; `sudKopieren`/`sudTeilen`/`removeRows` prüfen; Import/Export (`importexport.cpp`, kbh-JSON nutzt Feldnamen) prüfen; Template-DB aktualisieren; ältere Programmversionen können neue DB nicht öffnen.
- Bevorzugt: neue Funktionen **ohne** Schemaänderung (vorhandene Spalten, `Tags`-Tabelle Key/Value pro Sud, Bemerkungsfelder).
