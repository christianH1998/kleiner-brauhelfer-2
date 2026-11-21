# Bewertung: manuelle Eingabe eines fertigen Rezepts

> Phase-1-Kurzbewertung. Fortgeschrieben und ersetzt durch `konzept-erweiterung.md` (Phase 2); bei Widersprüchen gilt das Konzept.

Stand: Analyse, **keine Planung/Umsetzung**. Ziel: bereits ausgearbeitetes Rezept (feste Mengen) eingeben, Werte berechnen und anzeigen.

## Kernbefund
- Das Programm ist **zielwertgetrieben**: Eingabe SW/IBU/Menge/SHA + Prozentanteile → Mengen werden berechnet (`berechnungen.md`).
- Fertige Rezepte liefern meist **feste Mengen** (kg Malz, g Hopfen mit Kochzeit, Hefe, Zusätze) + evtl. Soll-SW/IBU/EBC.
- Bestehende Brücken zwischen beiden Welten:
  - Malz: `setData(Colerg_Menge)` → Prozent; danach `TabRezept::on_btnShaAnpassen_clicked` rechnet SHA so, dass Mengen bei gegebenem SW passen.
  - Hopfen: `berechnungsArtHopfen = Keine` + `Colerg_Menge` → Mengen bleiben fix (so machen es alle Importer).
  - WZ: `Colerg_Menge` → `Menge` pro Liter.
  - Importer `ImportExport::importMaischeMalzundMehr/importBeerXml` (`importexport.cpp`) legen komplette Sude programmatisch an (Muster: `modelSud()->append(values)` → `update(row)` → Gaben per `append` → `update(row)`).

## Wiederverwendbare Komponenten
- Anlegen: `ModelSud::append` (+`defaultValues`), `Model*Gaben::append`, `Brauhelfer::sudKopierenModel`.
- Rohstoffzuordnung: `DlgRohstoffAuswahl`, `Model*Gaben::import()` (fehlende Rohstoffe ins Lager), `TabRezept::checkRohstoffe()`.
- Berechnung/Anzeige: alle `ModelSud`-Virtualfelder (Alkohol, SRE, Farbe, Wasser, Schüttung, Hopfen-`IBUAnteil`), `BierCalc`.
- Darstellung: bestehender Rezept-Tab, Charts, Mustache-Templates (Spickzettel/Zusammenfassung) – zeigen jeden Sud automatisch an.

## Lücken (verifiziert)
- Keine Gesamt-IBU aus festen Hopfenmengen (nur je Gabe `IBUAnteil`); bei `Keine` bleibt `Sud.IBU` = Eingabewert.
- Keine SW-Berechnung aus fester Schüttung + SHA (nur Gegenrichtung bzw. SHA-Anpassung).
- `erg_S_Gesamt` wird in jedem `update()` aus SW/SHA/Menge neu berechnet → feste kg bleiben nur erhalten, wenn SHA passend gesetzt ist; spätere SW-/Mengenänderung skaliert Malz. Importer (MMuM, BeerXML) rechnen kg in `Prozent` um und übernehmen SHA (MMuM) bzw. Default 60 % (BeerXML) → absolute Malzmengen des Originalrezepts gehen dabei ggf. verloren.

## Mögliche Erweiterungspunkte (isoliert, nach Invasivität)
1. **Neuer Dialog** (z. B. `dialogs/dlgrezepteingabe.*`) in der App, der Eingaben sammelt und einen Sud mit Status Rezept anlegt (analog Importer), danach normaler Rezept-Tab. Einstieg: zusätzliche Action im „Anlegen“-Menü (`TabSudAuswahl::setupToolbar`). Keine DB-Änderung, keine Änderung bestehender Rechenlogik.
2. **Neue Core-Funktion** im Stil von `ImportExport::import*` (z. B. Rezeptstruktur → Sud), GUI-unabhängig, wiederverwendbar.
3. **Reine Berechnungshelfer** (IBU-Summe, SW aus Schüttung) als neue statische Funktionen in `BierCalc` bzw. neue `ModelSud`-Virtualfelder → Virtualfeld bedeutet Enum-Erweiterung nach `ColBewertungMittel` + `mVirtualField.append` (Reihenfolge!), keine DB-Änderung.
4. Änderungen am Rezept-Tab (`tabrezept.ui` ~4000 Zeilen) – hohes Konfliktrisiko, nur wenn nötig.

## Risiken
- Kaskaden in `setDataExt`/`update()`: Reihenfolge des Setzens beeinflusst Ergebnis (z. B. erst Menge/SW/SHA, dann Gaben, dann `update`). Importer als Referenz nehmen.
- `append()` vs `appendDirect()`: `appendDirect` überspringt Logik (z. B. Stoffwerte aus Lager, erg_Menge-Berechnung).
- Rohstoffnamen müssen exakt zum Lager passen, sonst keine Stoffwerte (Farbe/Alpha/Potential) → falsche Farbe/IBU.
- Hopfen-IBU nutzt Tinseth mit Zuschlägen (Pellets, Kochzeit) – Abweichung zu externen Rezeptangaben möglich.
- Farbe: einfache Formel (`updateFarbe`) – kann von Rezeptangabe (Morey o. ä.) abweichen.
- Undo: programmatische Anlage ist nicht rückgängig machbar (wie Import); Verwerfen nur über `discard`.
- Schemaänderungen vermeiden (Kompatibilität bestehender DBs, Export/Import, ältere Versionen).
- Keine Tests → Vergleich mit bekannten Rezepten/Importergebnissen manuell nötig.

## Offene Fragen für die Planung
- Eingabeformat: Formular in App vs. strukturierter Text/Datei?
- Was ist führend: feste Mengen (Rezept exakt nachbauen) oder Zielwerte (auf eigene Anlage skalieren)?
- Sollen Abweichungen (berechnet vs. Rezeptangabe für SW/IBU/EBC/Alkohol) angezeigt werden?
- Umgang mit Rohstoffen, die nicht im Lager sind.
