# Brauberechnungen

Quelle: `kleiner-brauhelfer-core/modelsud.cpp`, `biercalc.cpp`, Gaben-Modelle. Prinzip: **zielwertgetrieben** – Nutzer gibt Sollwerte (Menge, SW, IBU, SHA …) und Anteile (%) vor, Mengen werden berechnet.

## Rechenkette `ModelSud::update(row)` (nur bei Status Rezept, außer Alkohol/Preis)
Ausgelöst durch: `rowChanged` im Sud, `rowChanged` in Malz-/Hopfen-/Hefe-/WZ-Gaben (`onOtherModelRowChanged`), Ausrüstungsänderung (`onAnlageRowChanged`). Reentrance-Schutz `mUpdating`.
1. `updateSwWeitereZutaten` → Extraktanteile WZ je Zeitpunkt (`swWz*Recipe`, `swWzUnvergaerbarRecipe`).
2. `erg_S_Gesamt = BierCalc::schuettung(SWAnteilMalz, SWAnteilMalz+WZ Maischen+Kochen, MengeSollAnstellen, Sudhausausbeute)`.
3. `updateWasser`: Hauptguss = Schüttung × FaktorHauptguss (Sonderformeln Grainfather G30, BrauheldPro30); Nachguss = MengeSollKochbeginn − HG + Schüttung×0.96 + KorrekturWasser; Gesamt inkl. HGF-Wasser.
4. `updateFarbe`: EBC = (Σ m·EBC / Σ m) × SW/10 + 1.5 × Kochdauer/60 + KorrekturFarbe (Malz + WZ mit Farbe).
5. `erg_Sudhausausbeute`, `erg_EffektiveAusbeute` aus Ist-Werten (`BierCalc::sudhausausbeute`).
6. (Status ≤ Gebraut) `erg_Alkohol` (`BierCalc::alkohol`, inkl. Zucker/Verschneidung), `updatePreis` (Lagerpreise × Mengen + Anlage + KostenWasserStrom) / MengeIst.
7. `Gespeichert` = jetzt.

Kindmodelle reagieren auf `ModelSud::dataChanged`:
- Malzschuettung: bei `erg_S_Gesamt` → `erg_Menge` = Prozent × Schüttung.
- Hopfengaben: bei Menge, SW, IBU, Berechnungsart, HGF, Kochdauer, Nachiso → Mengen bzw. Prozent neu.
- WeitereZutatenGaben: bei Menge → `erg_Menge` = Menge/L × MengeSollAnstellen; bei Status → Zugabestatus.
- Maischplan: `updateSud` (Wasser-/Malzmengen der Rasten).

## Setter-Kaskaden `ModelSud::setDataExt_impl` (nur Status Rezept)
- `Menge` → `WuerzemengeKochbeginn` (=MengeSollKochbeginn) → `WuerzemengeVorHopfenseihen` → `WuerzemengeKochende` → `WuerzemengeAnstellen` → `JungbiermengeAbfuellen`.
- `SW` → `SWKochbeginn` (=SWSollKochbeginn) → `SWKochende` → `SWAnstellen` → `SWSchnellgaerprobe`, `SWJungbier` (= SRE aus Vergärungsgrad).
- `Anlage` → übernimmt `Sudhausausbeute`, `Verdampfungsrate` der Ausrüstung.
- `PhMaischeSoll` (virtuell, schreibbar) → `RestalkalitaetSoll`.
→ Ist-Felder werden im Rezeptstatus mit Sollwerten vorbelegt.

## Virtuelle Sud-Felder (`ModelSud::dataExt`)
- Mengen: `MengeSollAnstellen` = Menge + KorrekturMenge(Anlage); `MengeSollHgf`; `MengeSollKochende` = Anstellen − HGF − Hefestarter; `MengeSollVerdampfung` = Rate × Kochdauer/60; `MengeSollKochbeginn`.
- Extrakt: `SWSollAnstellen/Kochende/Kochbeginn(MitWz)`, `SWAnteilMalz` = SW − WZ-Anteile − Hefestarter, `SWAnteilZusatz{Maischen,Kochen,Gaerung}`, `SWAnteilHefestarter`, `SWAnteilZutaten`.
- Gärung: `SRESoll` (aus Vergärungsgrad), `AlkoholSoll`, `SWIst`, `SREErwartet`, `SREIst`, `sEVG`, `tEVG`.
- Karbonisierung: `Spundungsdruck`, `Gruenschlauchzeitpunkt`, `SpeiseNoetig`, `SpeiseAnteil`, `ZuckerAnteil`, `CO2Ist`.
- Sonstiges: `MengeIst`, `IbuIst`, `FarbeIst` (statusabhängig skaliert), `VerdampfungsrateIst`, `Woche`, `ReifezeitDelta`, `AbfuellenBereitZutaten`, `WuerzemengeAnstellenTotal` (schreibbar), Wasser: `RestalkalitaetWasser/Ist`, `PhMalz`, `PhMaische`, `PhMaischeSoll`, `BewertungMittel`.

## Gaben-Logik
- **Malz** (`modelmalzschuettung.cpp`): `setData(Prozent)` → `erg_Menge`; `setData(erg_Menge)` → Prozent = Menge/erg_S_Gesamt (Gesamtschüttung bleibt aus SW/SHA berechnet!). Virtuell: `Extrakt`, `ExtraktProzent`.
- **Hopfen** (`modelhopfengaben.cpp`): `Ausbeute` (virtuell) = 100 × `BierCalc::tinseth(t + Nachiso-Anteil, SW Mitte Kochen)` × 1.1 Pellets × 1.1 wenn Kochzeit−t > 15. `IBUAnteil` = Menge·Alpha·Ausbeute / (10·MengeSollAnstellen).
  - Berechnungsart `IBU`: Prozent = Anteil an Soll-IBU → Menge.
  - `Gewicht`: Prozent = Gewichtsanteil, Gesamtmenge so, dass Soll-IBU erreicht.
  - `Keine`: Mengen fix, keine Rückrechnung. **Es gibt keine Summenbildung der IBU aus Hopfenmengen** (nur `IBUAnteil` je Gabe, angezeigt in `WdgHopfenGabe`). Importe setzen `Keine` + `erg_Menge`.
- **WeitereZutaten**: `Extrakt` = Menge/L[g→kg] × Ausbeute (Stk ohne /1000); fließt über `updateSwWeitereZutaten` in SW-Aufteilung.
- **Hefe**: nur Menge, Zugabestatus; keine Berechnung.
- UI-Hilfsfunktionen in `tabrezept.cpp`: `on_btnMalzAusgleichen_clicked` (Prozente auf 100 normieren), `on_btnShaAnpassen_clicked` (SHA aus vorhandenen Malzmengen rückrechnen), `on_btnHopfenAusgleichen_clicked`, `vergaerungsgradUebernehmen` (EVG aus Hefe).

## `BierCalc` (statisch, `biercalc.h`) – Index
- Umrechnung: `brixToPlato`, `platoToBrix` (Faktor `faktorPlatoToBrix`, aus Settings), `brixToDichte` (Terrill/Linear/Standard/Novotny), `dichteToPlato`, `platoToDichte`, `spindelKorrektur`.
- Extrakt/Alkohol: `toTRE`, `toSRE` (Balling), `vergaerungsgrad`, `sreAusVergaerungsgrad`, `alkohol(sw,sre,alcZusatz)`, `alkoholVonZucker`.
- CO2: `co2(p,T)`, `p(co2,T)`, `gruenschlauchzeitpunkt`, `spundungsdruck`, `co2Noetig`, `co2Vergaerung`, `co2Zucker`, `speise`, `zucker`.
- Volumen/Ausbeute: `dichteWasser`, `volumenWasser`, `verdampfungsziffer`, `verdampfungsrate`, `sudhausausbeute(sw, sw_dichte, V, schuettung)`, `schuettung(sw, sw_dichte, V, sha)`, `verschneidung`.
- Hopfen: `tinseth(t, sw)`.
- Maische/Wasser: `mischungstemperaturTm/T2/M2`, `cMaische`, `phMalz/phMalzCarafa/phMalzRoest`, `hauptgussFaktor(ebc)`; Restalkalität: `ModelWasser::Restalkalitaet`, `ModelWasseraufbereitung::restalkalitaet(sudId)`.
- Farbe: `ebcToColor` (RGB).
- Konstanten: `MalzVerdraengung 0.75`, `Balling 2.0665`, `cWasser 4.2`, `cMalz 1.7`.
- Alternative Formeln über `#if 0/#else` im Code auskommentiert – nicht ändern ohne Absprache.

## Weitere verifizierte Befunde (Phase 2)
- `Malz.Potential` / `Malzschuettung.Potential` wird gespeichert, aber in keiner Berechnung verwendet.
- Planungs-`Sudhausausbeute` wirkt auf `MengeSollAnstellen` ⇒ fachlich effektive Ausbeute (Anstellwürze); Details `fachmodelle.md` §1.
- `update()` schreibt `erg_S_Gesamt`, Wasser, `erg_Farbe`, `erg_Sudhausausbeute`, `erg_EffektiveAusbeute` nur bei Status Rezept; `erg_Alkohol`, `erg_Preis` bis Status Gebraut. Virtuelle Felder sind immer live.
- Live aus Ausrüstung gelesen: `KorrekturMenge`, `KorrekturWasser`, `KorrekturFarbe`, `Kosten`, `Typ`; kopiert beim Anlagenwechsel: `Sudhausausbeute`, `Verdampfungsrate`.
- Ist-Felder (Würzemengen, SW Kochbeginn/-ende/Anstellen, SRE) werden im Status Rezept per Kaskade mit Sollwerten überschrieben.
- Alkohol an 4 Stellen mit unterschiedlicher Bedeutung; `SREIst` hängt von `SchnellgaerprobeAktiv` ab (`konzept-erweiterung.md` §7.2).
- `VerschneidungAbfuellen` = „Wasser für Zuckerlösung“ (`TabAbfuellen::tbWassserZuckerloesung`); verdünnt Alkohol nur bei Zucker > 0.
- `ZuckerFaktor`, `FlaschenGroesse` global in INI-Gruppe `TabAbfuellen`.
- Whirlpool-Gaben (Zeit < 0): t_eff = 0,549·t_Nachiso + Zeit (`modelhopfengaben.cpp` `ColAusbeute`) – siehe `fachmodelle.md` §4.
- Bestehende Kalibrierung: `DlgAusruestung::updateDurchschnitt` (Mittel der letzten N Sude, `AusbeuteIgnorieren`).

## Fehlende Umkehrrechnungen (relevant für Eingabe fertiger Rezepte)
- SW aus Schüttung [kg] + SHA + Volumen: nicht vorhanden (Umkehr von `schuettung`, `sw_dichte` hängt von SW ab → iterativ oder Näherung).
- Gesamt-IBU aus Hopfenmengen: nicht vorhanden (Σ `IBUAnteil` wäre Ansatz).
- Farbe aus Mengen: vorhanden (`updateFarbe` nutzt `erg_Menge`).
