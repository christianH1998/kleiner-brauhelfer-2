# Fachmodelle: Formeln, Quellen, Prüfbeispiele

Stand 2026-11-21. Grundlage für `konzept-erweiterung.md`. Bestehende Code-Formeln: Index in `berechnungen.md`.

Kennzeichnung: **[C]** im Code verifiziert · **[Q]** direkt in Quelle geprüft · **[S]** nur Sekundärquelle/Suchergebnis · **[A]** eigene Ableitung (Rechnung im Scratch-Skript nachvollzogen) · **[U]** ungeklärt/Annahme.

## Quellen
| ID | Quelle | Qualität / Status |
|---|---|---|
| Q1 | Hobbybrauer-Kompendium, „Sudhausausbeute“, http://hobbybrauer-kompendium.de/s/sudhausausbeute/sudhausausbeute.html (Autor/Datum nicht angegeben, abgerufen 2026-11-21) | Hobbybrauer-Referenz, mittel; Formel + Beispiel direkt geprüft |
| Q2 | G. Tinseth, Hop-Utilization-Seite realbeer.com/hops/research.html | Original abgeschaltet (Redirect), Inhalt nur über Sekundärquellen: BeerSmith Blog „Hop Utilization Models for Beer Brewing Compared“ (2021-09-23), Grainfather Help „Calculation IBU“ [S] |
| Q3 | M. Malowicki (Isomerisierungsrate vs. Temperatur), über J.-P. Hosom „mIBU“, https://jphosom.github.io/alchemyoverlord/ | [S]; Konstanten aus dem Code (0,046·e^(0,031·T)) dort **nicht** bestätigt |
| Q4 | K. Kling, „Bier selbst gebraut“, Verlag Die Werkstatt 2017 (ISBN 978-3-89533-550-1; Auflagenangabe 4./5. widersprüchlich) bzw. Ökobuch 2022 | **Nicht eingesehen** [U]. Haupt-/Nachguss-Richtwerte nur aus Anwendervorgabe |
| Q5 | alleantworten.de „Wann Nachguss?“ | schwach [S]: HG 4–5 L/kg hell, 3–4 L/kg dunkel; NG = Zielmenge − Vorderwürze + Koch-/Seihverluste |
| Q6 | Brewer's Friend, Beer Priming Calculator, https://www.brewersfriend.com/beer-priming-calculator | [S]: Rest-CO2 hängt von der höchsten Temperatur nach Gärende ab |
| Q7 | MEBAK K-410.01.002 „Sudhausausbeute – Heißwürzeausbeute“ (2020-10) | maßgeblich, aber nicht abrufbar (HTTP 503) [U] |
| Q8 | Brewfather Issue #1371 (gitlab.com/warpkode/public/brewfather) | schwach [S]: Sudhausausbeute ≈ 80 % der Brewhouse Efficiency (Faustregel) |
| Q9 | J. Palmer, *How to Brew* (Kapitel Abfüllen/Karbonisierung, Tabelle typischer CO2-Gehalte je Bierstil in Volumen) | [S] aus Fachkenntnis übernommen, Ausgabe/Seite nicht eingesehen; Werte decken sich weitgehend mit der bisherigen KBH-Hilfe am Feld CO2 (Lager 4,0–5,5, Weizen 6,5–9,0, Brit. Ales 3,0–4,0, Stout 3,4–4,5, Belg. Ales 3,8–4,8 g/L) |
| — | Code-Kommentare `biercalc.cpp` (realbeer.com/spencer/attenuation.html, brewersfriend.com) | Herkunft der Code-Formeln, nicht separat geprüft |

Empfehlung: Kling (Q4) und eine Fachquelle zur Ausbeute (MEBAK oder Narziß/Back) vor Umsetzung von Wasser- und Kalibriermodell vom Anwender bereitstellen lassen.

## 1 Dichte, Extrakt, Ausbeute
- ρ(P) [kg/L] = 1 + P / (258,6 − 227,1·P/258,2) — `BierCalc::platoToDichte` [C]. ρ(12 °P) = 1,04838; Q1 nennt 1,04837 [Q][A] ✓.
- Extraktmasse [kg] = V[L] · ρ(P) · P/100.
- **Sudhausausbeute (klassisch)** = P_Ausschlag[%] · ρ · V_Ausschlag,heiß[L] · 0,96 / Schüttung[kg] [Q1]. 0,96 = Kontraktion 100→20 °C plus Hopfen-/Eiweißtrub (Q1). Beispiel Q1: 12 · 1,04837 · 23 · 0,96 / 4 = 69,44 % [Q][A ✓].
- kbh `erg_Sudhausausbeute` = SWKochende · ρ · WuerzemengeVorHopfenseihen / erg_S_Gesamt, **ohne 0,96** [C]. `DlgVolumen` kann gemessenes Volumen auf 20 °C umrechnen; ρ_Wasser(100)/ρ_Wasser(20) = 0,9601 [A, `BierCalc::dichteWasser`] ⇒ äquivalent zur 0,96-Korrektur (ohne Trubanteil). Wird heißes Volumen unkorrigiert eingetragen: +4 % (Bsp.: 72,34 % statt 69,44 %) [A].
- kbh `erg_EffektiveAusbeute` = (SWAnstellen − WZ-Extrakt − Starter) · ρ(SWAnstellen) · WuerzemengeAnstellenTotal / erg_S_Gesamt [C] = Ausbeute bezogen auf **Anstellwürze** (inkl. aller Verluste nach Kochende).
- **Planungsparameter `Sud.Sudhausausbeute` / `Ausruestung.Sudhausausbeute`** wird in `ModelSud::update()` mit `MengeSollAnstellen` verrechnet [C] ⇒ fachlich eine *effektive Ausbeute (Anstellwürze)*, nicht die klassische SHA [A]. `DlgAusruestung::updateDurchschnitt` mittelt konsequent `erg_EffektiveAusbeute` [C]. Die Beschriftung „Sudhausausbeute“ ist daher irreführend; externe Rezepte (MMuM übernimmt `Sudhausausbeute` direkt, `importexport.cpp`) geben i. d. R. die klassische SHA an ⇒ Schüttung wird bei Import tendenziell zu niedrig [A, Größenordnung = Verlustanteil Kochende→Anstellen].
- Weitere Begriffe [S/U, Literatur nicht eingesehen]: Laborausbeute (Extrakt der Malzanalyse, Kongressmaische, lufttrocken/wasserfrei) ↔ `Malz.Potential`; Sudhauswirkungsgrad = SHA / Laborausbeute; „Maische-/Vorderwürzeausbeute“ ohne einheitliche Definition in den gefundenen Quellen.

## 2 Stammwürze ↔ Malzmengen
- **Variante B (bestehend)**: S = P_Malz · ρ(P_ges) · V_Anstell / A; m_i = Prozent_i · S [C, `BierCalc::schuettung`, `ModelMalzschuettung`].
- **Variante A (neu)**: gegeben m_i, A, V_Anstell. Löse P·ρ(P) = A·S/V (S = Σm_i). Fixpunkt P_{n+1} = c / ρ(P_n), c = A·S/V [A]. Konvergenz < 5 Iterationen. Beispiel S = 4,5 kg, V = 20 L, A = 65 % ⇒ P = 13,847 °P; Rückprobe `schuettung()` = 4,500 kg [A ✓]. Mit Weiteren Zutaten analog Code: ρ mit Gesamt-SW (Malz + WZ Maischen/Kochen), Malzanteil separat.
- **Malzpotential**: `Malz.Potential`/`Malzschuettung.Potential` wird gespeichert und propagiert, aber **in keiner Berechnung verwendet** [C]; Vorlage `data/Rohstoffe/Malz.csv` hat keine Potential-Spalte [C]. Optionale Gewichtung [A]: S_eff = Σ m_i·(Pot_i / Pot_ref) (Pot_ref = mittleres Potential der Kalibriersude); nur aktiv, wenn alle Malze Potential > 0, sonst Gleichgewichtung + sichtbarer Hinweis. Einfluss: Röst-/Karamellmalze haben geringere Laborausbeute (typ. 65–75 % vs. ~80 % Pilsner) [U, nicht belegt].

## 3 Wasser
- **Bestand [C]** (`ModelSud::updateWasser`): HG = S · FaktorHauptguss; NG = MengeSollKochbeginn − HG + 0,96·S + Ausruestung.KorrekturWasser; Gesamt = HG + NG + HGF-Wasser. Sonderfälle Grainfather G30 (HG = 2,7·S + 3,5) und BrauheldPro30. Treberaufnahme 0,96 L/kg fest im Code. `MengeSollKochende = MengeSollAnstellen − HGF − Hefestarter` ⇒ **keine eigene Verlustgröße Kochende→Anstellen**; Verluste nur über `Ausruestung.KorrekturMenge` (erhöht MengeSollAnstellen) [C]. Empfehlung HG-Faktor `BierCalc::hauptgussFaktor(ebc)` = 4,0 − 0,02·EBC (EBC < 50), sonst 3,0 [C] (nur Anzeige).
- Default-Inkonsistenz: DB-Default FaktorHauptguss 3,5, `ModelSud::defaultValues` 3 [C].
- **Vorschlag Verlustkette** [A] (alle Volumina bei 20 °C):
  - V_Kochende = V_Anstellen + V_Verlust,Trub/Hopfen (Anlage, L; optional + k_Hopfen · m_Hopfen)
  - V_Kochbeginn = V_Kochende + Verdampfungsrate · t_Koch/60
  - W_ges = V_Kochbeginn + k_Treber · S + V_Totraum,Läuter (Anlage)
  - HG = F_HG · S (F_HG aus Biertyp-Preset, EBC-Empfehlung oder Anlage) ; NG = W_ges − HG (Warnung, wenn < 0)
  - Heißvolumen-Anzeige über `BierCalc::volumenWasser(20, T, V)` [C].
- Zuordnung zu bestehenden Feldern: `KorrekturWasser` ≈ Totraum/Zusatzwasser, `KorrekturMenge` ≈ Verlust nach Kochende, 0,96 ≈ k_Treber [A]. Typische k_Treber 0,8–1,0 L/kg [S, nicht belegt → U].
- Richtwerte F_HG / NG je kg (Anwendervorgabe, Kling nicht eingesehen [U]): Hell 4 / 3–3,5; Dunkel 3–3,5 / 4; Weizen 2,5–2,8 / 3,5–4. Q5: Hell 4–5, Dunkel 3–4 [S]. Code-Empfehlung liegt im selben Bereich (EBC 8 → 3,84; EBC 50 → 3,0) [A]. NG pro kg ist fachlich **keine** unabhängige Größe, sondern Ergebnis von W_ges − HG [A] ⇒ NG-Richtwerte nur als Plausibilitätsprüfung verwenden.
- Nur mit Annahmen bestimmbar: Treberaufnahme, Totraum, Trubverluste, Verdampfung ⇒ als Anlagenparameter mit Herkunft „Standard/kalibriert/manuell“ anzeigen.

## 4 Hopfen / IBU
- Tinseth [S Q2, C]: U = 1,65 · 0,000125^(SG−1) · (1 − e^(−0,04·t)) / 4,15; IBU = 10 · U · α[%] · m[g] / V[L]. Code: SG−1 ≈ 0,004·P (Näherung; bei 12 °P 0,048 statt 0,0484, < 1 % Abweichung) [C][A]; V = MengeSollAnstellen; SW = Mittel aus SWSollKochbeginn/-ende über Gabendauer [C].
- kbh-Zuschläge [C, Herkunft U]: ×1,1 Pellets; ×1,1 wenn Kochdauer − t > 15 min (spätere Gaben); Kommentar verweist auf MMuM-IBU-Rechner.
- Nachisomerisierung [C]: t_eff = t + 0,549 · t_Nachiso (0,046·e^(0,031·80)); entspricht etwa relativer Rate 80 °C vs. 100 °C (e^(−0,031·20) = 0,538) [A]; Konstanten in Q3 nicht bestätigt [U].
- **Befund Whirlpool** [C][A]: Gabe x min nach Kochende (`Zeit = −x`, `HopfenZeitpunkt::Ausschlagen`): Code t_eff = 0,549·t_iso − x; plausibler wäre 0,549·(t_iso − x). Prüfbeispiel (20 L, 12 °P, 30 g, 12 % α, Pellets, Kochzeit 60, t_iso 20):

  | Zeit [min] | U·100 (Code) | IBU (Code) |
  |---|---|---|
  | 60 | 27,33 | 49,2 |
  | 30 | 25,46 | 45,8 |
  | 10 | 17,82 | 32,1 |
  | 0 | 11,11 | 20,0 |
  | −5 | 6,65 | 12,0 (alternativ 8,77 → 15,8) |
  | −10 | 1,21 | 2,2 (alternativ 6,16 → 11,1) |
  | −20 | 0 | 0 |
  Änderung nur als neue, gekennzeichnete Option, nie stillschweigend (bestehende Rezepte im IBU-Modus würden andere Mengen erhalten). **Umgesetzt in ext-s11** als `Sud.NachisoModell = 1` „Whirlpool mit Abkühlkurve“ (Anwenderablauf: alle Gaben im Kochen, danach Whirlpool + ~30 min Absetzen, dann Ablassen); Wechsel lässt die Mengen und passt die IBU an (Anwenderentscheidung 2026-11-22). Modell:
  - Bittere in der Absetzzeit: t_eff,extra = ∫₀^t_abs r(T(t)) dt, r(T) = 0,046·e^(0,031·T) (= bisherige Konstante, 100 °C → 1,02, 80 °C → 0,549), unter 80 °C 0 [C für Konstanten, A für Integral; Konstanten U, Q3 nicht bestätigt].
  - Wärmebilanz runder Topf (Durchmesser Sudpfanne, Füllhöhe aus Volumen), c = 4,19 kJ/(L·K): Verdunstung offene Oberfläche q = 0,089 kW/(m²·kPa)·(p_s(T) − 0,5·p_s(T_a)) (ASHRAE-Beckenformel für ruhende Luft, für Pools gedacht, hier bis 100 °C extrapoliert [S/U]); Konvektion h = 1,31·ΔT^(1/3) W/(m²·K) [S, vereinfachte Luft-Korrelation]; Strahlung Wasseroberfläche ε 0,95, Topf/Deckel ε 0,3 [U]; Wand nur benetzt, Boden wie Wand [A]; Deckel: keine Verdunstung nach außen [A]; Dampfdruck Antoine (1–100 °C) [Q-Standard].
  - Prüfbeispiel [A, Code]: 20 L, Ø 36 cm, 20 °C, offen: 92,2 °C nach 10 min, 80,9 °C nach 30 min (Deckel 94,3 °C), wirksam 22,4 min statt 16,5 (0,549·30). Erfahrungswerte Hobbybrauer: einige K bis ~10 K in 30 min [U] ⇒ Modell kühlt eher schneller; daher Kalibrierung: `Ausruestung.WhirlpoolFaktor` skaliert den Wärmeverlust, per Bisektion aus gemessener Endtemperatur.
- Kalthopfung = `WeitereZutatenGaben` Typ 100, Zeitpunkt Gärung ⇒ 0 IBU [C]; fachlich vertretbar (keine Isomerisierung) [A].
- **Gesamt-IBU aus festen Mengen** = Σ IBUAnteil der Hopfengaben [A]; existiert nicht [C].
- Variante B (bestehend): Modus IBU (Anteile der Soll-IBU) bzw. Gewicht (Gewichtsanteile, Gesamtmenge auf Soll-IBU skaliert) [C] ⇒ Verhältnis der Gaben bleibt erhalten.

## 5 Farbe
- Code [C]: EBC = (Σ m·EBC / Σ m) · P/10 + 1,5 · t_Koch/60 + KorrekturFarbe. Eigenes kbh-Modell; Rezeptangaben anderer Programme (z. B. Morey) können abweichen [U, Morey hier nicht geprüft]. Empfehlung: Modell beibehalten, als „kbh-Farbmodell“ kennzeichnen, Abgleich über `KorrekturFarbe`.
- Späte Malzzugabe (ext-s12b) [A, ungeprüft]: Farbausbeute f(t) = 1 − e^(−t/τ), t = Kontaktzeit ab Zugabe bis Ende Maischplan [min], τ = 10 min Richtwert (einstellbar). Malz geht mit EBC·f(t) in die Mischungsformel ein, Masse und Extrakt voll. Beispiel: 8 min → f = 0,551; 2,5 % Farbmalz 1150 EBC im Testsud „Tittingdorfer Spezial“: 86,2 → 67,4 EBC. Prüfszenario: `kbh-dump --rundlauf-farbmalz` (τ→0 = volle Farbe exakt, Farbminderung ∝ 1 − f). Extraktfehler bei kleinen Farbmalzmengen ~0,2 °P (Anwender 2026-11-24 akzeptiert, Ausgleich über Kalibrierung S14).

## 6 Gärung, Alkohol, Karbonisierung
- TRE = 0,1808·P + 0,8192·SRE; Alkohol_w = (P − TRE)/(2,0665 − 0,010665·P); Alkohol_v = (Alkohol_w + Zusatz)·ρ(TRE)/0,794 [C]. Beispiel 12 °P / SRE 2,5 ⇒ 5,14 %vol [A].
- Zucker-Alkohol [C]: `alkoholVonZucker(g/L)` = g·0,488/10 (Ergebnis w/v, wird zu w/w addiert ⇒ Einheitenmischung, Fehler ~1 % relativ [A]). +6 g/L ⇒ 5,51 %vol [A].
- CO2-Löslichkeit [C]: c(p,T) = (1,013 + p)·e^(−10,73797 + 2617,25/(T+273,15))·10 [g/L]. Rest-CO2 (p=0): 20 °C 1,66 g/L, 12 °C 2,13 g/L [A]; plausibel ggü. Q6 (~0,86 vol bei 20 °C ≈ 1,7 g/L; 1 vol ≈ 1,96 g/L) [S][A].
- Zucker [C]: (CO2_soll − CO2_rest(T_Jungbier) − CO2 aus Restvergärung) / 0,468. Konstanten 0,468 g CO2/g und 0,488 g Alkohol/g: Herkunft [U]. Stöchiometrie [A]: Saccharose 0,515 g CO2/g · 0,538 g EtOH/g; Glukose 0,489 · 0,511; Glukose-Monohydrat 0,444 · 0,465 (theoretisch, ohne Hefebiomasse). `ZuckerFaktor` und `FlaschenGroesse` liegen **global in der INI** (`TabAbfuellen`), nicht pro Sud [C].
- Richtwerte Ziel-CO2 je Bierart (ext-s13, Tooltip Ziel-Karbonisierung) [S] Q9, Vol. × 1,96 g/L: Brit. Ales 1,5–2,0 Vol. (2,9–3,9 g/L), Porter/Stout 1,7–2,3 (3,3–4,5), Belg. Ales 1,9–2,4 (3,7–4,7), Amer. Ales/Lager 2,2–2,7 (4,3–5,3), Frucht-Lambic 3,0–4,5 (5,9–8,8), Weizen 3,3–4,5 (6,5–8,8).
- Schlauchzeitpunkt beim Spunden [C]: `gruenschlauchzeitpunkt(CO2_soll, SW, SRE_Ende, T_Jungbier)` = SRE bei TRE_Ende + (CO2_soll − CO2_rest(T))/(0,468·10). SRE_Ende = Schnellgärprobe, sonst (ext-s13, nur Anzeige) `SREErwartet` aus dem Vergärungsgrad der Hefe [A] – Schätzung, Abweichung des realen Endvergärungsgrads um 1 °P ändert den Schlauchpunkt um ≈ 1 °P bzw. das CO2 um ≈ 4,7 g/L ⇒ ohne Schnellgärprobe unzuverlässig.
- Zwangskarbonisierung [A]: Gleichgewichtsdruck p = `BierCalc::p(CO2_soll, T_Lager)` (= `spundungsdruck`) [C]; kein Alkoholzuwachs.

## 8 Prüfliste: nicht verifizierte Berechnungen und Richtwerte (Abschlussprüfung)
Prüfung durch den Anwender bei Projektabschluss. Spalte „Status“ zunächst überall **nicht verifiziert**; nach Prüfung durch Quellenangabe (Werk, Auflage, Seite/Tabelle) ersetzen und den Wert ggf. im Code korrigieren (Ort in Spalte „Code“). Bei Korrektur: Regressionsvergleich, betroffene Rundlauftests und Änderungsprotokoll nachziehen. Neue ungeprüfte Annahmen in späteren Stufen hier ergänzen.

| Nr. | Thema | Wert / Formel (heute) | Code / Ort | Herkunft bisher | Status |
|---|---|---|---|---|---|
| P1 | Bittere Tinseth | U = 1,65·0,000125^(SG−1)·(1−e^(−0,04·t))/4,15; SG−1 ≈ 0,004·P | `BierCalc::tinseth`, `ModelHopfengaben` | Q2 nur sekundär | nicht verifiziert |
| P2 | Zuschläge Bittere | ×1,1 Pellets; ×1,1 bei Gaben > 15 min vor Kochende | `ModelHopfengaben` | Code-Kommentar MMuM, Herkunft unklar | nicht verifiziert |
| P3 | Isomerisierungsrate | r(T) = 0,046·e^(0,031·T); 80 °C ⇒ 0,549; unter 80 °C 0 | `BierCalc::isomerisierungsrate`, Nachiso | Q3 sekundär, Konstanten dort nicht bestätigt | nicht verifiziert |
| P4 | Whirlpool-Abkühlung | Verdunstung 0,089 kW/(m²·kPa) (ASHRAE, Pool), Konvektion 1,31·ΔT^(1/3) W/(m²·K), Strahlung ε 0,95; Abkühlung 20 L/36 cm ≈ 19 K in 30 min | `BierCalc::whirlpoolMinuten` | Formeln für andere Anwendungen, extrapoliert; über Endtemperatur kalibrierbar | nicht verifiziert |
| P5 | Farbmodell | EBC = (Σ m·EBC/Σ m)·P/10 + 1,5·t_Koch/60 + Korrektur | `ModelSud::updateFarbe` | eigenes kbh-Modell (Original) | nicht verifiziert |
| P6 | Farbmalz späte Zugabe | f(t) = 1 − e^(−t/τ), τ = 10 min | `BierCalc::farbausbeute`, Einstellung τ | Annahme (S12b) | nicht verifiziert |
| P7 | Aufheizrate Maische | 1 K/min Richtwert | `Brauhelfer::aufheizrate`, Einstellung | Annahme (S12) | nicht verifiziert |
| P8 | Wasser-Voreinstellungen | HG/NG je kg: Helles 4,0/3,0, Dunkles 3,0/4,0, Benutzerdefiniert 3,5/3,5; Weizen 2,5–2,8/3,5–4 | `ModelSud` Presets, TabRezept-Tooltip | Anwendervorgabe nach Kling (Q4 nicht eingesehen), Q5 schwach | nicht verifiziert |
| P9 | Treberaufnahme | 0,96 L/kg Standard (typisch 0,8–1,0) | `Ausruestung.Treberaufnahme`, `ModelSud::updateWasser` | Code-Konstante Original | nicht verifiziert |
| P10 | Malzverdrängung | 0,75 L/kg (Volumen Maische, Topfgröße) | `BierCalc::MalzVerdraengung` | Code-Konstante Original | nicht verifiziert |
| P11 | Sudhausausbeute-Bezug | klassisch mit Faktor 0,96 (Q1); MEBAK; Faustregel SHA ≈ 80 % der Brewhouse Efficiency | `fachmodelle` §1, Hinweistexte | Q1 geprüft, Q7 nicht abrufbar, Q8 schwach | nicht verifiziert |
| P12 | Alkoholformel | Balling: Alk_w = (P − TRE)/(2,0665 − 0,010665·P), TRE = 0,1808·P + 0,8192·SRE, Alk_v = Alk_w·ρ/0,794 | `BierCalc::alkohol`, `toTRE` | Code, Quelle nicht zitiert | nicht verifiziert |
| P13 | Zucker → CO2 / Alkohol | 0,468 g CO2/g Zucker; 0,488 g Alkohol/g (w/v zu w/w addiert); Stöchiometrie Saccharose 0,515/0,538 | `BierCalc::co2Zucker`, `alkoholVonZucker` | Herkunft unklar | nicht verifiziert |
| P14 | Süßkraft-Faktoren | Haushaltszucker 1,00, Traubenzucker 0,69, Fruchtzucker 1,14 (Tooltip); Kern rechnet festen g/L-Wert mit Süßkraft 1 | `TabAbfuellen` (INI `ZuckerFaktor`) | Original-Tooltip; „Süßkraft“ fachlich ≠ CO2-Ausbeute | nicht verifiziert |
| P15 | CO2-Löslichkeit | c(p,T) = (1,013 + p)·e^(−10,73797 + 2617,25/(T+273,15))·10 g/L | `BierCalc::co2`, `p` | plausibel ggü. Q6 | nicht verifiziert |
| P16 | Rest-CO2-Temperatur | höchste Temperatur nach Gärende (Feld Temperatur Karbonisierung); beim Spunden Temperatur Jungbier | `ZuckerAnteil`, `gruenschlauchzeitpunkt` | Q6 sekundär | nicht verifiziert |
| P17 | Ziel-CO2 nach Bierart | Brit. Ales 2,9–3,9; Porter/Stout 3,3–4,5; Belg. Ales 3,7–4,7; Amer. Ales/Lager/Pils 4,3–5,3; Frucht-Lambic 5,9–8,8; Weizen 6,5–8,8 g/L (1 Vol. ≈ 1,96 g/L) | `TabAbfuellen::co2RichtwerteHtml` | Q9 Palmer, nicht eingesehen | nicht verifiziert |
| P18 | Flaschenfestigkeit | normale Flaschen bis ca. 6 g/L | `TabAbfuellen::co2RichtwerteHtml` | Faustregel | nicht verifiziert |
| P19 | Speise / Restextrakt → CO2 | CO2 = 0,468·10·(TRE_Anfang − TRE_Ende) g/L | `BierCalc::co2Vergaerung`, `speise` | wie P13 | nicht verifiziert |
| P20 | Schlauchzeitpunkt ohne Schnellgärprobe | Endvergärung aus Vergärungsgrad der Hefe (`SREErwartet`); 1 °P Fehler ≈ 4,7 g/L CO2 | `TabAbfuellen` (Anzeige), Abfüllart 6 | eigene Ableitung, Hefewerte = Herstellerangaben | nicht verifiziert |
| P21 | Zuckerausgleich beim Spunden | Zucker = (CO2_soll − Rest-CO2(T_Jungbier) − CO2 aus Restextrakt)/0,468, ohne Verdünnung | `ModelSud` `ZuckerAnteil` Art 6 | eigene Ableitung aus P13/P15/P19 | nicht verifiziert |
| P22 | Zwangskarbonisierung | Druck = Gleichgewicht `p(CO2_soll, T_Jungbier)` | `BierCalc::spundungsdruck` | eigene Ableitung aus P15 | nicht verifiziert |

## 7 Prüfszenarien (für spätere Tests)
1. ρ(12) = 1,04838; SHA Q1-Beispiel = 69,44 % (mit 0,96) bzw. 72,34 % (kbh ohne Umrechnung).
2. Variante A ↔ B Rundlauf: Mengen → SW (A) → `schuettung()` (B) liefert identische Mengen (Abweichung < 1 g).
3. Hopfen: Tabelle oben reproduzieren; Σ IBUAnteil = Soll-IBU im IBU-Modus nach „Ausgleichen“.
4. Alkohol: 12/2,5 ⇒ 5,14 %vol; Zwangskarbonisierung ⇒ Alkohol erwartet = Alkohol Jungbier (bei Endvergärung).
5. Fehlender Messwert: Sud mit nur SW Anstellen + Volumen Anstellen ⇒ effektive Ausbeute berechenbar, klassische SHA „nicht verfügbar“ (nicht 0).
6. Altbestand: Sud aus Template-DB unverändert öffnen/speichern ⇒ alle `erg_*` identisch (Regressionstest).
