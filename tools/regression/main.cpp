// kbh-dump: schreibt alle gespeicherten und virtuellen Werte der Modelle als JSON.
// Zweck: Regressionsvergleich der Berechnungen vor/nach einer Code-Aenderung.
// Die Datenbank wird in ein Temp-Verzeichnis kopiert und nie gespeichert.
//
// Aufruf: kbh-dump <datenbank.sqlite> <ausgabe.json> [--no-recalc] [--save]
//   ohne --no-recalc wird fuer jeden Sud ModelSud::update() ausgefuehrt
//   (wie beim Bearbeiten in der App; wirkt nur auf Sude im Status Rezept).
//   --save: NUR FUER TESTDATEN. Arbeitet direkt auf der angegebenen Datenbank
//   (keine Kopie) und speichert die Neuberechnung (Brauhelfer::save).
//   --rundlauf-hopfen: statt Dump Rundlauftest der Hopfen-Berechnungsarten (Exit 0 = OK).
//   --umkehrtest: Werte aendern und zuruecksetzen, Umkehrbarkeit pruefen (Exit 0 = OK).
//   --rundlauf-malz: Rundlauftest Malz-Berechnungsart Ziel-SW <-> Mengen fest (Exit 0 = OK).
//   --rundlauf-wasser: Rundlauftest Wasserberechnung, Presets, Treberaufnahme (Exit 0 = OK).
//   --rundlauf-nachiso: Rundlauftest Modell "Bittere nach Kochende" (Whirlpool mit Abkuehlkurve, Exit 0 = OK).
//   --rundlauf-protokoll: Rundlauftest Brauprotokoll (Messwerte schuetzen Ist-Felder, Entfernen exakt, Exit 0 = OK).
//   --rundlauf-farbmalz: Rundlauftest Malz-Zugabe im Maischplan (Farbausbeute, Malzanteile, Entfernen exakt, Exit 0 = OK).
//   --migrate: aeltere Datenbank vorher aktualisieren (auf der Kopie; mit --save auf der Datei).

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>
#include <algorithm>
#include <cmath>
#include "brauhelfer.h"
#include "biercalc.h"

static QJsonValue toJson(const QVariant& v)
{
    if (!v.isValid() || v.isNull())
        return QJsonValue();
    switch (v.typeId())
    {
    case QMetaType::Double:
    case QMetaType::Float:
        return QString::number(v.toDouble(), 'g', 12);
    case QMetaType::Bool:
        return v.toBool();
    case QMetaType::Int:
    case QMetaType::LongLong:
    case QMetaType::UInt:
    case QMetaType::ULongLong:
        return QString::number(v.toLongLong());
    default:
        return v.toString();
    }
}

// ---- Rundlauftest Hopfen-Berechnungsart (ext-s01) ----
// Prueft: Moduswechsel zwischen Ziel-IBU (Gewicht/IBU) und "Mengen fest -> IBU berechnet"
// veraendert weder Hopfenmengen noch IBU. Arbeitet nur im Speicher (kein save()).
struct HopfenStand
{
    QList<double> mengen;
    double ibu;
};

static HopfenStand hopfenStand(Brauhelfer& bh, int sudRow)
{
    HopfenStand s;
    const QVariant sudId = bh.modelSud()->data(sudRow, ModelSud::ColID);
    ModelHopfengaben* m = bh.modelHopfengaben();
    const int colDeleted = m->fieldIndex(QStringLiteral("deleted"));
    for (int r = 0; r < m->rowCount(); ++r)
        if (m->data(r, ModelHopfengaben::ColSudID) == sudId && !m->data(r, colDeleted).toBool())
            s.mengen.append(m->data(r, ModelHopfengaben::Colerg_Menge).toDouble());
    s.ibu = bh.modelSud()->data(sudRow, ModelSud::ColIBU).toDouble();
    return s;
}

static double maxRelDiff(const HopfenStand& a, const HopfenStand& b)
{
    auto rel = [](double x, double y) { return std::fabs(x - y) / std::max(1.0, std::max(std::fabs(x), std::fabs(y))); };
    double d = rel(a.ibu, b.ibu);
    if (a.mengen.size() != b.mengen.size())
        return 1.0;
    for (int i = 0; i < a.mengen.size(); ++i)
        d = std::max(d, rel(a.mengen[i], b.mengen[i]));
    return d;
}

static void setArt(Brauhelfer& bh, int sudRow, Brauhelfer::BerechnungsartHopfen art)
{
    bh.modelSud()->setData(sudRow, ModelSud::ColberechnungsArtHopfen, static_cast<int>(art));
}

static int rundlaufHopfen(Brauhelfer& bh, QTextStream& out)
{
    using Art = Brauhelfer::BerechnungsartHopfen;
    const double tol = 1e-9;
    int fehler = 0;
    for (int row = 0; row < bh.modelSud()->rowCount(); ++row)
    {
        const QString name = bh.modelSud()->data(row, ModelSud::ColSudname).toString();
        // Test im Rezeptstatus (nur im Speicher), damit gerechnet wird
        bh.modelSud()->setData(row, ModelSud::ColStatus, static_cast<int>(Brauhelfer::SudStatus::Rezept));
        if (hopfenStand(bh, row).mengen.isEmpty())
            continue;

        // Start: Mengen fest -> IBU berechnet (aus den vorhandenen Mengen)
        setArt(bh, row, Art::IBUBerechnet);
        const HopfenStand basis = hopfenStand(bh, row);
        QStringList zeilen;
        double worst = 0.0;
        for (Art ziel : {Art::Gewicht, Art::IBU})
        {
            setArt(bh, row, ziel);
            const HopfenStand nachZiel = hopfenStand(bh, row);
            // Eingabe im Ziel-Modus ohne Aenderung: IBU auf denselben Wert setzen -> Mengen neu berechnet
            bh.modelSud()->setData(row, ModelSud::ColIBU, nachZiel.ibu + 1.0);
            bh.modelSud()->setData(row, ModelSud::ColIBU, nachZiel.ibu);
            const HopfenStand nachNeurechnung = hopfenStand(bh, row);
            setArt(bh, row, Art::IBUBerechnet);
            const HopfenStand zurueck = hopfenStand(bh, row);
            const double d = std::max({maxRelDiff(basis, nachZiel), maxRelDiff(basis, nachNeurechnung), maxRelDiff(basis, zurueck)});
            worst = std::max(worst, d);
            zeilen << QStringLiteral("%1: %2").arg(ziel == Art::Gewicht ? QStringLiteral("Gewicht") : QStringLiteral("IBU")).arg(d, 0, 'g', 3);
        }
        // Aenderung im Modus "berechnet": Menge +10 % -> IBU muss steigen, Rueckweg wieder exakt
        ModelHopfengaben* m = bh.modelHopfengaben();
        const QVariant sudId = bh.modelSud()->data(row, ModelSud::ColID);
        for (int r = 0; r < m->rowCount(); ++r)
        {
            if (m->data(r, ModelHopfengaben::ColSudID) == sudId)
            {
                m->setData(r, ModelHopfengaben::Colerg_Menge, m->data(r, ModelHopfengaben::Colerg_Menge).toDouble() * 1.1);
                break;
            }
        }
        const HopfenStand geaendert = hopfenStand(bh, row);
        const bool ibuGestiegen = geaendert.ibu > basis.ibu;
        setArt(bh, row, Art::IBU);
        setArt(bh, row, Art::IBUBerechnet);
        worst = std::max(worst, maxRelDiff(geaendert, hopfenStand(bh, row)));

        const bool ok = worst <= tol && ibuGestiegen;
        if (!ok)
            ++fehler;
        out << (ok ? "OK    " : "FEHLER") << " Sud '" << name << "' IBU " << QString::number(basis.ibu, 'f', 2)
            << " | max. rel. Abweichung " << zeilen.join(QStringLiteral(", "))
            << " | Menge+10% -> IBU " << QString::number(geaendert.ibu, 'f', 2) << (ibuGestiegen ? "" : " (nicht gestiegen!)") << "\n";
    }
    return fehler;
}

// ---- Rundlauftest Malz-Berechnungsart (ext-s09) ----
struct MalzStand
{
    QList<double> kg;
    double sw;
    double schuettung;
};

static MalzStand malzStand(Brauhelfer& bh, int sudRow)
{
    MalzStand s;
    const QVariant sudId = bh.modelSud()->data(sudRow, ModelSud::ColID);
    ModelMalzschuettung* m = bh.modelMalzschuettung();
    const int colDeleted = m->fieldIndex(QStringLiteral("deleted"));
    for (int r = 0; r < m->rowCount(); ++r)
        if (m->data(r, ModelMalzschuettung::ColSudID) == sudId && !m->data(r, colDeleted).toBool())
            s.kg.append(m->data(r, ModelMalzschuettung::Colerg_Menge).toDouble());
    s.sw = bh.modelSud()->data(sudRow, ModelSud::ColSW).toDouble();
    s.schuettung = bh.modelSud()->data(sudRow, ModelSud::Colerg_S_Gesamt).toDouble();
    return s;
}

static double malzDiff(const MalzStand& a, const MalzStand& b)
{
    auto rel = [](double x, double y) { return std::fabs(x - y) / std::max(1.0, std::max(std::fabs(x), std::fabs(y))); };
    double d = std::max(rel(a.sw, b.sw), rel(a.schuettung, b.schuettung));
    if (a.kg.size() != b.kg.size())
        return 1.0;
    for (int i = 0; i < a.kg.size(); ++i)
        d = std::max(d, rel(a.kg[i], b.kg[i]));
    return d;
}

static int rundlaufMalz(Brauhelfer& bh, QTextStream& out)
{
    const double tol = 1e-9;
    const int fest = static_cast<int>(Brauhelfer::BerechnungsartMalz::MengenFest);
    const int ziel = static_cast<int>(Brauhelfer::BerechnungsartMalz::ZielSW);
    int fehler = 0;
    ModelSud* sud = bh.modelSud();
    for (int row = 0; row < sud->rowCount(); ++row)
    {
        const QString name = sud->data(row, ModelSud::ColSudname).toString();
        sud->setData(row, ModelSud::ColStatus, static_cast<int>(Brauhelfer::SudStatus::Rezept));
        sud->setData(row, ModelSud::ColberechnungsArtMalz, ziel);
        if (malzStand(bh, row).kg.isEmpty())
            continue;
        // Ausgangslage stimmig machen (Prozentsumme 100 %), wie "Ausgleichen" in der App
        ModelMalzschuettung* m = bh.modelMalzschuettung();
        const QVariant sudId = sud->data(row, ModelSud::ColID);
        {
            double summe = 0.0;
            for (int r = 0; r < m->rowCount(); ++r)
                if (m->data(r, ModelMalzschuettung::ColSudID) == sudId)
                    summe += m->data(r, ModelMalzschuettung::ColProzent).toDouble();
            if (std::fabs(summe - 100.0) > 1e-9 && summe > 0.0)
                for (int r = 0; r < m->rowCount(); ++r)
                    if (m->data(r, ModelMalzschuettung::ColSudID) == sudId)
                        m->setData(r, ModelMalzschuettung::ColProzent, m->data(r, ModelMalzschuettung::ColProzent).toDouble() * 100.0 / summe);
        }
        const MalzStand a = malzStand(bh, row);

        sud->setData(row, ModelSud::ColberechnungsArtMalz, fest);           // Ziel -> fest
        const MalzStand b = malzStand(bh, row);
        int r0 = -1;
        for (int r = 0; r < m->rowCount() && r0 < 0; ++r)
            if (m->data(r, ModelMalzschuettung::ColSudID) == sudId)
                r0 = r;
        const double kg0 = m->data(r0, ModelMalzschuettung::Colerg_Menge).toDouble();
        // ext-s10: Im Modus "Rezept nachbrauen" waechst mit dem Malz die Menge (SW darf sinken)
        const bool nachbrauen = sud->istNachbrauen(row);
        const double mengeVor = sud->data(row, ModelSud::ColMenge).toDouble();
        m->setData(r0, ModelMalzschuettung::Colerg_Menge, kg0 * 1.1);      // Menge +10 %
        const MalzStand mehr = malzStand(bh, row);
        const bool mengeGestiegen = sud->data(row, ModelSud::ColMenge).toDouble() > mengeVor;
        m->setData(r0, ModelMalzschuettung::Colerg_Menge, kg0);            // zurueck
        const MalzStand c = malzStand(bh, row);
        sud->setData(row, ModelSud::ColberechnungsArtMalz, ziel);           // fest -> Ziel
        const MalzStand d = malzStand(bh, row);
        const double sw0 = sud->data(row, ModelSud::ColSW).toDouble();
        sud->setData(row, ModelSud::ColSW, sw0 + 0.5);                      // Ziel-Modus: SW aendern
        sud->setData(row, ModelSud::ColSW, sw0);                            // und zurueck
        const MalzStand e = malzStand(bh, row);
        sud->setData(row, ModelSud::ColberechnungsArtMalz, fest);           // nochmal Ziel -> fest
        const MalzStand f = malzStand(bh, row);

        const double dWechsel = std::max({malzDiff(a, b), malzDiff(b, d), malzDiff(e, f)});
        const double dZurueck = std::max(malzDiff(b, c), malzDiff(d, e));
        const bool swGestiegen = nachbrauen ? mengeGestiegen : mehr.sw > b.sw;
        const bool ok = dWechsel <= tol && dZurueck <= tol && swGestiegen;
        if (!ok)
            ++fehler;
        out << (ok ? "OK    " : "FEHLER") << " Sud '" << name << "' SW " << QString::number(a.sw, 'f', 2)
            << " Schuettung " << QString::number(a.schuettung, 'f', 3) << " kg"
            << " | Moduswechsel max. rel. " << QString::number(dWechsel, 'g', 3)
            << " | Aendern+Zurueck " << QString::number(dZurueck, 'g', 3)
            << " | Malz+10% -> SW " << QString::number(mehr.sw, 'f', 2) << (nachbrauen ? " (Nachbrauen: Menge gestiegen)" : "")
            << (swGestiegen ? "" : " (nicht gestiegen!)") << "\n";
    }
    out << (fehler == 0 ? QStringLiteral("Rundlauftest Malz: alle OK\n") : QStringLiteral("Rundlauftest Malz: %1 Fehler\n").arg(fehler));
    return fehler;
}

// ---- Umkehrtest: Wert aendern und exakt zuruecksetzen -> alles wieder wie vorher? ----
static QMap<QString, QVariant> sudSnapshot(Brauhelfer& bh, int sudRow)
{
    QMap<QString, QVariant> s;
    ModelSud* sud = bh.modelSud();
    for (int c = 0; c < sud->columnCount(); ++c)
    {
        const QString n = sud->fieldName(c);
        if (n != QStringLiteral("Gespeichert") && n != QStringLiteral("deleted"))
            s.insert(QStringLiteral("Sud.") + n, sud->data(sudRow, c));
    }
    const QVariant sudId = sud->data(sudRow, ModelSud::ColID);
    const QList<QPair<QString, SqlTableModel*>> kinder = {
        {QStringLiteral("Malz"), bh.modelMalzschuettung()},
        {QStringLiteral("Hopfen"), bh.modelHopfengaben()},
        {QStringLiteral("WZ"), bh.modelWeitereZutatenGaben()},
        {QStringLiteral("Maischplan"), bh.modelMaischplan()}};
    for (const auto& k : kinder)
    {
        SqlTableModel* m = k.second;
        const int colSud = m->fieldIndex(QStringLiteral("SudID"));
        const int colDel = m->fieldIndex(QStringLiteral("deleted"));
        int i = 0;
        for (int r = 0; r < m->rowCount(); ++r)
        {
            if (m->data(r, colSud) != sudId || m->data(r, colDel).toBool())
                continue;
            for (int c = 0; c < m->columnCount(); ++c)
                s.insert(QStringLiteral("%1[%2].%3").arg(k.first).arg(i).arg(m->fieldName(c)), m->data(r, c));
            ++i;
        }
    }
    return s;
}

static QStringList snapshotDiff(const QMap<QString, QVariant>& a, const QMap<QString, QVariant>& b)
{
    QStringList d;
    for (auto it = a.constBegin(); it != a.constEnd(); ++it)
    {
        const QVariant vb = b.value(it.key());
        bool okA, okB;
        const double x = it.value().toDouble(&okA), y = vb.toDouble(&okB);
        bool gleich;
        if (okA && okB && (!std::isfinite(x) || !std::isfinite(y)))
            gleich = (std::isnan(x) && std::isnan(y)) || x == y;
        else if (okA && okB)
            gleich = std::fabs(x - y) <= 1e-9 * std::max(1.0, std::max(std::fabs(x), std::fabs(y)));
        else
            gleich = it.value() == vb;
        if (!gleich)
            d << QStringLiteral("%1: %2 -> %3").arg(it.key(), it.value().toString(), vb.toString());
    }
    return d;
}

// ---- Rundlauftest Wasserberechnung (ext-s10) ----
static QStringList diffOhne(const QMap<QString, QVariant>& a, const QMap<QString, QVariant>& b, const QStringList& ohne)
{
    QMap<QString, QVariant> x = a, y = b;
    for (const QString& k : ohne)
    {
        x.remove(k);
        y.remove(k);
    }
    return snapshotDiff(x, y);
}

static int rundlaufWasser(Brauhelfer& bh, QTextStream& out)
{
    const int bestand = static_cast<int>(Brauhelfer::BerechnungsartWasser::NachgussFolgtMenge);
    const int faktoren = static_cast<int>(Brauhelfer::BerechnungsartWasser::Faktoren);
    const QStringList modusFelder = {QStringLiteral("Sud.berechnungsArtWasser"), QStringLiteral("Sud.FaktorNachguss"), QStringLiteral("Sud.WasserPreset")};
    int fehler = 0;
    ModelSud* sud = bh.modelSud();
    for (int row = 0; row < sud->rowCount(); ++row)
    {
        const QString name = sud->data(row, ModelSud::ColSudname).toString();
        sud->setData(row, ModelSud::ColStatus, static_cast<int>(Brauhelfer::SudStatus::Rezept));
        sud->setData(row, ModelSud::ColberechnungsArtWasser, bestand);
        const double schuet = sud->data(row, ModelSud::Colerg_S_Gesamt).toDouble();
        if (schuet <= 0.0)
            continue;
        Brauhelfer::AnlageTyp typ = static_cast<Brauhelfer::AnlageTyp>(sud->dataAnlage(row, ModelAusruestung::ColTyp).toInt());
        const bool sonder = typ == Brauhelfer::AnlageTyp::GrainfatherG30 || typ == Brauhelfer::AnlageTyp::BrauheldPro30;
        QStringList probleme;
        auto pruefe = [&](bool bedingung, const QString& text) { if (!bedingung) probleme << text; };
        auto nahe = [](double x, double y) { return std::fabs(x - y) <= 1e-9 * std::max(1.0, std::max(std::fabs(x), std::fabs(y))); };

        // Bestand: erwartete Wuerzemenge = Sollmenge (sofern NG nicht auf 0 gekappt)
        const QMap<QString, QVariant> a = sudSnapshot(bh, row);
        const double ng0 = sud->data(row, ModelSud::Colerg_WNachguss).toDouble();
        if (ng0 > 0.0)
            pruefe(nahe(sud->data(row, ModelSud::ColWuerzemengeKochbeginnErwartet).toDouble(), sud->data(row, ModelSud::ColMengeSollKochbeginn).toDouble()),
                   QStringLiteral("Bestand: erwartete Menge != Soll"));

        // 1. Wechsel Bestand -> Faktoren: keine Wertaenderung
        sud->setData(row, ModelSud::ColberechnungsArtWasser, faktoren);
        const QMap<QString, QVariant> b = sudSnapshot(bh, row);
        QStringList d = diffOhne(a, b, modusFelder);
        pruefe(d.isEmpty(), QStringLiteral("Wechsel ->Faktoren aendert: ") + d.mid(0, 3).join(QStringLiteral("; ")));
        // ext-s11b: ohne Abweichung der Wuerzemenge ist die erwartete SW = SW des Rezepts
        if (!sonder && ng0 > 0.0)
            pruefe(std::fabs(sud->data(row, ModelSud::ColSWErwartet).toDouble() - sud->data(row, ModelSud::ColSW).toDouble()) < 1e-6,
                   QStringLiteral("Faktoren ohne Abweichung: erwartete SW %1 != SW %2").arg(sud->data(row, ModelSud::ColSWErwartet).toDouble()).arg(sud->data(row, ModelSud::ColSW).toDouble()));

        if (!sonder)
        {
            // 2. Nachgussfaktor +0,5 -> NG und erwartete Menge steigen um 0,5*S, Sollmenge bleibt; zurueck exakt
            const double fng = sud->data(row, ModelSud::ColFaktorNachguss).toDouble();
            const double erw0 = sud->data(row, ModelSud::ColWuerzemengeKochbeginnErwartet).toDouble();
            const double soll0 = sud->data(row, ModelSud::ColMengeSollKochbeginn).toDouble();
            sud->setData(row, ModelSud::ColFaktorNachguss, fng + 0.5);
            pruefe(std::fabs(sud->data(row, ModelSud::ColWuerzemengeKochbeginnErwartet).toDouble() - (erw0 + 0.5 * schuet)) < 1e-6,
                   QStringLiteral("NG-Faktor +0,5 wirkt nicht auf erwartete Menge"));
            // ext-s11b: mehr Nachguss -> erwartete SW sinkt, Rezept-SW bleibt
            pruefe(sud->data(row, ModelSud::ColSWErwartet).toDouble() < sud->data(row, ModelSud::ColSW).toDouble() - 1e-6,
                   QStringLiteral("NG-Faktor +0,5: erwartete SW nicht gesunken"));
            pruefe(nahe(sud->data(row, ModelSud::ColMengeSollKochbeginn).toDouble(), soll0), QStringLiteral("Sollmenge veraendert"));
            sud->setData(row, ModelSud::ColFaktorNachguss, fng);
            d = diffOhne(b, sudSnapshot(bh, row), {});
            pruefe(d.isEmpty(), QStringLiteral("NG-Faktor zurueck nicht exakt: ") + d.mid(0, 3).join(QStringLiteral("; ")));

            // 3. Presets setzen die Faktoren
            const double fhg = sud->data(row, ModelSud::ColFaktorHauptguss).toDouble();
            const QList<QPair<int, QPair<double, double>>> presets = {{1, {4.0, 3.0}}, {2, {3.0, 4.0}}, {3, {3.5, 3.5}}};
            for (const auto& p : presets)
            {
                sud->setData(row, ModelSud::ColWasserPreset, p.first);
                pruefe(sud->data(row, ModelSud::ColFaktorHauptguss).toDouble() == p.second.first && sud->data(row, ModelSud::ColFaktorNachguss).toDouble() == p.second.second,
                       QStringLiteral("Preset %1 setzt Faktoren falsch").arg(p.first));
                pruefe(nahe(sud->data(row, ModelSud::Colerg_WHauptguss).toDouble(), schuet * p.second.first)
                       && nahe(sud->data(row, ModelSud::Colerg_WNachguss).toDouble(), schuet * p.second.second),
                       QStringLiteral("Preset %1: HG/NG nicht S*Faktor").arg(p.first));
            }
            // Preset "keins" laesst Faktoren stehen
            sud->setData(row, ModelSud::ColWasserPreset, 0);
            pruefe(sud->data(row, ModelSud::ColFaktorHauptguss).toDouble() == 3.5, QStringLiteral("Preset keins aendert Faktoren"));
            sud->setData(row, ModelSud::ColFaktorHauptguss, fhg);
            sud->setData(row, ModelSud::ColFaktorNachguss, fng);
            d = diffOhne(b, sudSnapshot(bh, row), {QStringLiteral("Sud.WasserPreset")});
            pruefe(d.isEmpty(), QStringLiteral("nach Presets nicht zurueck: ") + d.mid(0, 3).join(QStringLiteral("; ")));
        }

        // 4. Wechsel Faktoren -> Bestand: wieder Ausgangszustand
        sud->setData(row, ModelSud::ColberechnungsArtWasser, bestand);
        d = diffOhne(a, sudSnapshot(bh, row), modusFelder);
        pruefe(d.isEmpty(), QStringLiteral("Wechsel ->Bestand aendert: ") + d.mid(0, 3).join(QStringLiteral("; ")));

        // 5. Treberaufnahme der Anlage +0,1 -> NG steigt um 0,1*S (nur Standardanlage); zurueck exakt
        if (!sonder && ng0 > 0.0)
        {
            const QVariant t0 = sud->dataAnlage(row, ModelAusruestung::ColTreberaufnahme);
            sud->setDataAnlage(row, ModelAusruestung::ColTreberaufnahme, sud->treberaufnahme(row) + 0.1);
            pruefe(std::fabs(sud->data(row, ModelSud::Colerg_WNachguss).toDouble() - (ng0 + 0.1 * schuet)) < 1e-6, QStringLiteral("Treberaufnahme wirkt nicht auf NG"));
            sud->setDataAnlage(row, ModelAusruestung::ColTreberaufnahme, t0);
            d = diffOhne(a, sudSnapshot(bh, row), modusFelder);
            pruefe(d.isEmpty(), QStringLiteral("Treberaufnahme zurueck nicht exakt: ") + d.mid(0, 3).join(QStringLiteral("; ")));
        }

        // 6. Rezept nachbrauen (Malz fest + Hopfen fest + Wasser Nachbrauen): Umschalten ohne
        //    Wertaenderung, Menge folgt dem Wasser, Skalieren ueber die Menge, zurueck auf Planen
        double mengeSkaliert = 0.0;
        if (!sonder)
        {
            ModelMalzschuettung* m = bh.modelMalzschuettung();
            const QVariant sudId = sud->data(row, ModelSud::ColID);
            const int colDel = m->fieldIndex(QStringLiteral("deleted"));
            // Ausgangslage stimmig machen (Prozentsumme 100 %), wie in --rundlauf-malz
            double summe = 0.0;
            for (int r = 0; r < m->rowCount(); ++r)
                if (m->data(r, ModelMalzschuettung::ColSudID) == sudId && !m->data(r, colDel).toBool())
                    summe += m->data(r, ModelMalzschuettung::ColProzent).toDouble();
            if (summe > 0.0 && std::fabs(summe - 100.0) > 1e-9)
                for (int r = 0; r < m->rowCount(); ++r)
                    if (m->data(r, ModelMalzschuettung::ColSudID) == sudId && !m->data(r, colDel).toBool())
                        m->setData(r, ModelMalzschuettung::ColProzent, m->data(r, ModelMalzschuettung::ColProzent).toDouble() * 100.0 / summe);
            auto kern = [&]() {
                QMap<QString, QVariant> k;
                for (int c : {ModelSud::ColMenge, ModelSud::ColSW, ModelSud::ColIBU, ModelSud::Colerg_S_Gesamt, ModelSud::Colerg_WHauptguss,
                              ModelSud::Colerg_WNachguss, ModelSud::Colerg_W_Gesamt, ModelSud::Colerg_Farbe, ModelSud::ColFaktorHauptguss})
                    k.insert(sud->fieldName(c), sud->data(row, c));
                const QList<QPair<QString, SqlTableModel*>> kinder = {{QStringLiteral("Malz"), bh.modelMalzschuettung()}, {QStringLiteral("Hopfen"), bh.modelHopfengaben()}};
                for (const auto& kd : kinder)
                {
                    const int cs = kd.second->fieldIndex(QStringLiteral("SudID")), cd = kd.second->fieldIndex(QStringLiteral("deleted")), cm = kd.second->fieldIndex(QStringLiteral("erg_Menge"));
                    int i = 0;
                    for (int r = 0; r < kd.second->rowCount(); ++r)
                        if (kd.second->data(r, cs) == sudId && !kd.second->data(r, cd).toBool())
                            k.insert(QStringLiteral("%1[%2]").arg(kd.first).arg(i++), kd.second->data(r, cm));
                }
                return k;
            };
            auto summeMalz = [&]() {
                double s = 0.0;
                for (int r = 0; r < m->rowCount(); ++r)
                    if (m->data(r, ModelMalzschuettung::ColSudID) == sudId && !m->data(r, colDel).toBool())
                        s += m->data(r, ModelMalzschuettung::Colerg_Menge).toDouble();
                return s;
            };
            // Hopfen "Keine" (Testdaten) hat keine berechnete IBU -> vorher auf Ziel-IBU stellen (wertneutral)
            if (sud->data(row, ModelSud::ColberechnungsArtHopfen).toInt() < 0)
            {
                sud->setData(row, ModelSud::ColberechnungsArtHopfen, static_cast<int>(Brauhelfer::BerechnungsartHopfen::IBUBerechnet));
                sud->setData(row, ModelSud::ColberechnungsArtHopfen, static_cast<int>(Brauhelfer::BerechnungsartHopfen::IBU));
            }
            const QMap<QString, QVariant> p0 = kern();
            sud->setData(row, ModelSud::ColberechnungsArtMalz, static_cast<int>(Brauhelfer::BerechnungsartMalz::MengenFest));
            sud->setData(row, ModelSud::ColberechnungsArtHopfen, static_cast<int>(Brauhelfer::BerechnungsartHopfen::IBUBerechnet));
            sud->setData(row, ModelSud::ColberechnungsArtWasser, static_cast<int>(Brauhelfer::BerechnungsartWasser::Nachbrauen));
            pruefe(sud->istNachbrauen(row), QStringLiteral("Nachbrauen nicht aktiv"));
            const QMap<QString, QVariant> n0 = kern();
            d = snapshotDiff(p0, n0);
            pruefe(d.isEmpty(), QStringLiteral("Wechsel ->Nachbrauen aendert: ") + d.mid(0, 3).join(QStringLiteral("; ")));

            // Nachgussfaktor +0,5 -> Menge steigt, SW sinkt; zurueck exakt
            const double fng = sud->data(row, ModelSud::ColFaktorNachguss).toDouble();
            const double menge0 = sud->data(row, ModelSud::ColMenge).toDouble(), sw0 = sud->data(row, ModelSud::ColSW).toDouble();
            sud->setData(row, ModelSud::ColFaktorNachguss, fng + 0.5);
            const double mengeNG = sud->data(row, ModelSud::ColMenge).toDouble();
            pruefe(mengeNG > menge0 && sud->data(row, ModelSud::ColSW).toDouble() < sw0, QStringLiteral("NG-Faktor +0,5: Menge/SW falsch"));
            pruefe(std::fabs(sud->data(row, ModelSud::ColWuerzemengeKochbeginnErwartet).toDouble() - sud->data(row, ModelSud::ColMengeSollKochbeginn).toDouble()) < 1e-6,
                   QStringLiteral("Nachbrauen: erwartete Menge != Soll"));
            pruefe(std::fabs(sud->data(row, ModelSud::ColSWErwartet).toDouble() - sud->data(row, ModelSud::ColSW).toDouble()) < 1e-6,
                   QStringLiteral("Nachbrauen: erwartete SW != SW"));
            sud->setData(row, ModelSud::ColFaktorNachguss, fng);
            d = snapshotDiff(n0, kern());
            pruefe(d.isEmpty(), QStringLiteral("NG-Faktor zurueck (Nachbrauen) nicht exakt: ") + d.mid(0, 3).join(QStringLiteral("; ")));

            // Rezept skalieren: Menge x1,5 -> Menge exakt wie eingegeben, Faktoren unveraendert; zurueck
            sud->setData(row, ModelSud::ColMenge, menge0 * 1.5);
            mengeSkaliert = sud->data(row, ModelSud::ColMenge).toDouble();
            pruefe(std::fabs(mengeSkaliert - menge0 * 1.5) < 1e-6, QStringLiteral("Skalieren: Menge %1 statt %2").arg(mengeSkaliert).arg(menge0 * 1.5));
            pruefe(sud->data(row, ModelSud::ColFaktorNachguss).toDouble() == fng, QStringLiteral("Skalieren aendert Faktor"));
            pruefe(sud->data(row, ModelSud::Colerg_S_Gesamt).toDouble() > n0.value(QStringLiteral("erg_S_Gesamt")).toDouble(), QStringLiteral("Skalieren: Schuettung nicht gestiegen"));
            sud->setData(row, ModelSud::ColMenge, menge0);
            d = snapshotDiff(n0, kern());
            pruefe(d.isEmpty(), QStringLiteral("Skalieren zurueck nicht exakt: ") + d.mid(0, 3).join(QStringLiteral("; ")));

            // Tippen im Mengenfeld (jede Taste schreibt): Zwischenwerte duerfen das Ziel nicht verfaelschen.
            // Nachbildung der GUI: TabRezept::updateValues setzt bei jedem "modified" auch das
            // fokussierte Mengenfeld; weicht der Modellwert sichtbar ab (Spinbox rundet), schreibt das Feld ihn zurueck.
            // Ausgabe der Abweichung mit KBH_TRACE.
            double anzeige = 0.0;
            int zurueckgeschrieben = 0;
            const QMetaObject::Connection verb = QObject::connect(&bh, &Brauhelfer::modified, [&]() {
                const double mm = sud->data(row, ModelSud::ColMenge).toDouble();
                if (anzeige > 0.0 && std::fabs(mm - anzeige) > 1e-6 && zurueckgeschrieben < 20)
                {
                    if (std::getenv("KBH_TRACE"))
                        out << "  zurueck: Anzeige " << anzeige << " Modell " << QString::number(mm, 'f', 9) << "\n";
                    ++zurueckgeschrieben;
                    anzeige = mm;
                    sud->setData(row, ModelSud::ColMenge, mm);
                }
            });
            for (double v : {3.0, 30.0, 0.0, 3.0, 30.0, 300.0, 30.0, 30.5})
            {
                anzeige = v;
                sud->setData(row, ModelSud::ColMenge, v);
                if (std::getenv("KBH_TRACE"))
                    out << "  tippe " << v << " -> Menge " << QString::number(sud->data(row, ModelSud::ColMenge).toDouble(), 'f', 6)
                        << " S " << QString::number(summeMalz(), 'f', 6) << "\n";
            }
            pruefe(std::fabs(sud->data(row, ModelSud::ColMenge).toDouble() - 30.5) < 1e-6,
                   QStringLiteral("Tippen 30,5: Menge %1").arg(sud->data(row, ModelSud::ColMenge).toDouble()));
            QObject::disconnect(verb);
            anzeige = 0.0;
            pruefe(zurueckgeschrieben == 0, QStringLiteral("Tippen: Mengenfeld schreibt %1x zurueck (Rekursion)").arg(zurueckgeschrieben));
            sud->setData(row, ModelSud::ColMenge, menge0);
            d = snapshotDiff(n0, kern());
            pruefe(d.isEmpty(), QStringLiteral("Tippen zurueck nicht exakt: ") + d.mid(0, 3).join(QStringLiteral("; ")));

            // zurueck auf Planen
            sud->setData(row, ModelSud::ColberechnungsArtWasser, bestand);
            sud->setData(row, ModelSud::ColberechnungsArtMalz, static_cast<int>(Brauhelfer::BerechnungsartMalz::ZielSW));
            sud->setData(row, ModelSud::ColberechnungsArtHopfen, static_cast<int>(Brauhelfer::BerechnungsartHopfen::IBU));
            d = snapshotDiff(n0, kern());
            pruefe(d.isEmpty(), QStringLiteral("Wechsel ->Planen aendert: ") + d.mid(0, 3).join(QStringLiteral("; ")));
        }

        const bool ok = probleme.isEmpty();
        if (!ok)
            ++fehler;
        if (mengeSkaliert > 0.0)
            out << "[Nachbrauen ok, skaliert auf " << QString::number(mengeSkaliert, 'f', 2) << " L] ";
        out << (ok ? "OK    " : "FEHLER") << " Sud '" << name << "'" << (sonder ? " (Sonderanlage)" : "")
            << " S " << QString::number(schuet, 'f', 3) << " kg HG " << QString::number(sud->data(row, ModelSud::Colerg_WHauptguss).toDouble(), 'f', 2)
            << " NG " << QString::number(sud->data(row, ModelSud::Colerg_WNachguss).toDouble(), 'f', 2)
            << " -> NG-Faktor " << QString::number(b.value(QStringLiteral("Sud.FaktorNachguss")).toDouble(), 'f', 3);
        if (!ok)
            out << "\n      " << probleme.join(QStringLiteral("\n      "));
        out << "\n";
    }
    out << (fehler == 0 ? QStringLiteral("Rundlauftest Wasser: alle OK\n") : QStringLiteral("Rundlauftest Wasser: %1 Fehler\n").arg(fehler));
    return fehler;
}

// Rundlauftest Modell "Bittere nach Kochende" (ext-s11): Whirlpool mit Abkuehlkurve.
// Physik-Plausibilitaet von BierCalc::whirlpoolMinuten, dann je Sud (Absetzzeit 30 min) in
// jeder Hopfen-Berechnungsart Bestand -> Whirlpool -> Bestand: Mengen unveraendert, IBU-Anteil
// jeder Kochgabe genau im Verhaeltnis (1-e^(-0,04*(t+W))) / (1-e^(-0,04*(t+0,549*t_iso))),
// IBU = Summe Anteile, Rueckweg exakt. Aussentemperatur +10 K und Deckel erhoehen die IBU.
static int rundlaufNachiso(Brauhelfer& bh, QTextStream& out)
{
    int fehler = 0;
    // Physik: 20 L, Topf 36 cm, 20 °C, offen, 30 min
    {
        QStringList p;
        double t30 = 0.0, t10 = 0.0, tDeckel = 0.0, tWarm = 0.0, tF2 = 0.0;
        const double w30 = BierCalc::whirlpoolMinuten(20, 36, 20, false, 1.0, 0, 30, &t30);
        BierCalc::whirlpoolMinuten(20, 36, 20, false, 1.0, 0, 10, &t10);
        const double wDeckel = BierCalc::whirlpoolMinuten(20, 36, 20, true, 1.0, 0, 30, &tDeckel);
        BierCalc::whirlpoolMinuten(20, 36, 30, false, 1.0, 0, 30, &tWarm);
        BierCalc::whirlpoolMinuten(20, 36, 20, false, 2.0, 0, 30, &tF2);
        const double wTeil = BierCalc::whirlpoolMinuten(20, 36, 20, false, 1.0, 10, 30);
        const double w10 = BierCalc::whirlpoolMinuten(20, 36, 20, false, 1.0, 0, 10);
        if (!(t30 > 20.0 && t30 < t10 && t10 < 100.0)) p << QStringLiteral("Temperatur faellt nicht plausibel");
        if (!(tDeckel > t30)) p << QStringLiteral("Deckel kuehlt nicht langsamer");
        if (!(tWarm > t30)) p << QStringLiteral("Aussentemperatur ohne Wirkung");
        if (!(tF2 < t30)) p << QStringLiteral("Faktor 2 kuehlt nicht schneller");
        if (!(w30 > 0.0 && w30 <= 30.0 * BierCalc::isomerisierungsrate(100.0) + 1e-9)) p << QStringLiteral("wirksame Minuten ausserhalb");
        if (!(wDeckel > w30)) p << QStringLiteral("Deckel: wirksame Minuten nicht hoeher");
        if (std::fabs(w10 + wTeil - w30) > 1e-9) p << QStringLiteral("Teilintegrale addieren sich nicht");
        out << (p.isEmpty() ? "OK    " : "FEHLER") << " Physik 20 L / 36 cm / 20 °C offen: nach 10 min " << QString::number(t10, 'f', 1)
            << " °C, nach 30 min " << QString::number(t30, 'f', 1) << " °C (Deckel " << QString::number(tDeckel, 'f', 1)
            << " °C), wirksam " << QString::number(w30, 'f', 1) << " min (bisher 0,549*30 = "
            << QString::number(30 * BierCalc::isomerisierungsrate(80.0), 'f', 1) << ")\n";
        if (!p.isEmpty())
        {
            ++fehler;
            out << "      " << p.join(QStringLiteral("\n      ")) << "\n";
        }
    }
    ModelSud* sud = bh.modelSud();
    ModelHopfengaben* m = bh.modelHopfengaben();
    const int colDel = m->fieldIndex(QStringLiteral("deleted"));
    const int bestand = static_cast<int>(Brauhelfer::NachisoModell::Bestand);
    const int whirlpool = static_cast<int>(Brauhelfer::NachisoModell::Whirlpool);
    for (int row = 0; row < sud->rowCount(); ++row)
    {
        const QString name = sud->data(row, ModelSud::ColSudname).toString();
        sud->setData(row, ModelSud::ColStatus, static_cast<int>(Brauhelfer::SudStatus::Rezept));
        const QVariant sudId = sud->data(row, ModelSud::ColID);
        QList<int> gaben;
        for (int r = 0; r < m->rowCount(); ++r)
            if (m->data(r, ModelHopfengaben::ColSudID) == sudId && !m->data(r, colDel).toBool())
                gaben.append(r);
        if (gaben.isEmpty())
        {
            out << "--     Sud '" << name << "' ohne Hopfengaben\n";
            continue;
        }
        QStringList probleme;
        auto pruefe = [&](bool bedingung, const QString& text) { if (!bedingung) probleme << text; };
        sud->setData(row, ModelSud::ColNachisoModell, bestand);
        setArt(bh, row, Brauhelfer::BerechnungsartHopfen::IBUBerechnet);
        sud->setData(row, ModelSud::ColNachisomerisierungszeit, 30);
        const double tIso = 30.0;
        double ibuB = 0.0, ibuW = 0.0, wMin = 0.0, tEnde = 0.0;
        for (Brauhelfer::BerechnungsartHopfen art : {Brauhelfer::BerechnungsartHopfen::Gewicht, Brauhelfer::BerechnungsartHopfen::IBU,
                                                     Brauhelfer::BerechnungsartHopfen::IBUBerechnet, Brauhelfer::BerechnungsartHopfen::Keine})
        {
            setArt(bh, row, art);
            const QString a = QString::number(static_cast<int>(art));
            const HopfenStand s0 = hopfenStand(bh, row);
            QList<double> anteil0;
            for (int r : gaben)
                anteil0.append(m->data(r, ModelHopfengaben::ColIBUAnteil).toDouble());
            sud->setData(row, ModelSud::ColNachisoModell, whirlpool);
            const HopfenStand s1 = hopfenStand(bh, row);
            wMin = sud->data(row, ModelSud::ColWhirlpoolWirksameZeit).toDouble();
            tEnde = sud->data(row, ModelSud::ColWhirlpoolEndtemperatur).toDouble();
            HopfenStand n0 = s0, n1 = s1;
            n0.ibu = n1.ibu = 0.0;
            pruefe(maxRelDiff(n0, n1) <= 1e-9, QStringLiteral("Art %1: Mengen veraendert").arg(a));
            double summe = 0.0;
            for (int i = 0; i < gaben.size(); ++i)
            {
                const double anteil1 = m->data(gaben[i], ModelHopfengaben::ColIBUAnteil).toDouble();
                summe += anteil1;
                const double t = m->data(gaben[i], ModelHopfengaben::ColZeit).toDouble();
                if (t >= 0.0 && anteil0[i] > 0.0)
                {
                    const double verh = (1 - exp(-0.04 * (t + wMin))) / (1 - exp(-0.04 * (t + tIso * BierCalc::isomerisierungsrate(80.0))));
                    pruefe(std::fabs(anteil1 / anteil0[i] - verh) <= 1e-9,
                           QStringLiteral("Art %1: Gabe %2 Verhaeltnis %3 statt %4").arg(a).arg(i).arg(anteil1 / anteil0[i]).arg(verh));
                }
            }
            if (art != Brauhelfer::BerechnungsartHopfen::Keine)
                pruefe(std::fabs(s1.ibu - summe) <= 1e-9 * std::max(1.0, summe), QStringLiteral("Art %1: IBU %2 != Summe Anteile %3").arg(a).arg(s1.ibu).arg(summe));
            else
                pruefe(s1.ibu == s0.ibu, QStringLiteral("Art Keine: IBU veraendert"));
            if (art == Brauhelfer::BerechnungsartHopfen::IBUBerechnet)
            {
                ibuB = s0.ibu;
                ibuW = s1.ibu;
                // Aussentemperatur und Deckel wirken; zurueck exakt
                const double ta = sud->data(row, ModelSud::ColAussentemperatur).toDouble();
                sud->setData(row, ModelSud::ColAussentemperatur, ta + 10.0);
                pruefe(sud->data(row, ModelSud::ColIBU).toDouble() > s1.ibu, QStringLiteral("Aussentemperatur +10: IBU nicht gestiegen"));
                sud->setData(row, ModelSud::ColAussentemperatur, ta);
                sud->setData(row, ModelSud::ColWhirlpoolDeckel, 1);
                pruefe(sud->data(row, ModelSud::ColIBU).toDouble() > s1.ibu, QStringLiteral("Deckel: IBU nicht gestiegen"));
                sud->setData(row, ModelSud::ColWhirlpoolDeckel, 0);
                pruefe(maxRelDiff(s1, hopfenStand(bh, row)) <= 1e-9, QStringLiteral("Aussentemperatur/Deckel zurueck nicht exakt"));
            }
            sud->setData(row, ModelSud::ColNachisoModell, bestand);
            pruefe(maxRelDiff(s0, hopfenStand(bh, row)) <= 1e-9, QStringLiteral("Art %1: Rueckweg nicht exakt").arg(a));
        }
        const bool ok = probleme.isEmpty();
        if (!ok)
            ++fehler;
        out << (ok ? "OK    " : "FEHLER") << " Sud '" << name << "' Absetzzeit 30 min: Ende " << QString::number(tEnde, 'f', 1)
            << " °C, wirksam " << QString::number(wMin, 'f', 1) << " min; IBU bisher " << QString::number(ibuB, 'f', 2)
            << " -> Whirlpool " << QString::number(ibuW, 'f', 2) << "\n";
        if (!ok)
            out << "      " << probleme.join(QStringLiteral("\n      ")) << "\n";
    }
    out << (fehler == 0 ? QStringLiteral("Rundlauftest Nachiso: alle OK\n") : QStringLiteral("Rundlauftest Nachiso: %1 Fehler\n").arg(fehler));
    return fehler;
}

static int rundlaufProtokoll(Brauhelfer& bh, QTextStream& out)
{
    // Brauprotokoll (ext-s12): Messwerte schuetzen Ist-Felder vor der Soll-Vorbelegung,
    // Entfernen faellt exakt auf die Vorbelegung zurueck.
    ModelSud* sud = bh.modelSud();
    ModelMesswerte* mw = bh.modelMesswerte();
    const QList<int> istFelder = {ModelSud::ColWuerzemengeKochbeginn, ModelSud::ColWuerzemengeVorHopfenseihen, ModelSud::ColWuerzemengeKochende,
                                  ModelSud::ColWuerzemengeAnstellen, ModelSud::ColJungbiermengeAbfuellen, ModelSud::ColSWKochbeginn,
                                  ModelSud::ColSWKochende, ModelSud::ColSWAnstellen, ModelSud::ColSWJungbier};
    auto istStand = [&](int row) { QList<double> l; for (int c : istFelder) l.append(sud->data(row, c).toDouble()); return l; };
    int fehler = 0;
    for (int row = 0; row < sud->rowCount(); ++row)
    {
        const QString name = sud->data(row, ModelSud::ColSudname).toString();
        const QVariant sudId = sud->data(row, ModelSud::ColID);
        QStringList probleme;
        auto pruefe = [&](bool bedingung, const QString& text) { if (!bedingung) probleme << text; };
        sud->setData(row, ModelSud::ColStatus, static_cast<int>(Brauhelfer::SudStatus::Rezept));
        const double sw = sud->data(row, ModelSud::ColSW).toDouble();
        const double menge = sud->data(row, ModelSud::ColMenge).toDouble();
        // vorhandene Messwerte des Suds (z. B. aus einem App-Test) im Speicher entfernen
        for (int r = mw->rowCount() - 1; r >= 0; --r)
            if (mw->data(r, ModelMesswerte::ColSudID) == sudId && !mw->data(r, ModelMesswerte::ColDeleted).toBool())
                mw->removeRow(r);
        // Ausgangslage stimmig machen: Vorbelegung aller Ist-Felder aus den Sollwerten
        sud->setData(row, ModelSud::ColSW, sw);
        sud->setData(row, ModelSud::ColMenge, menge);
        const QList<double> s0 = istStand(row);
        for (int i = 0; i < istFelder.size(); ++i)
            pruefe(std::fabs(s0[i] - sud->istVorbelegung(row, istFelder[i]).toDouble()) <= 1e-9,
                   QStringLiteral("Vorbelegung Feld %1: %2 statt %3").arg(sud->fieldName(istFelder[i])).arg(s0[i]).arg(sud->istVorbelegung(row, istFelder[i]).toDouble()));
        pruefe(sud->data(row, ModelSud::ColMengeEffektivQuelle).toInt() == static_cast<int>(ModelSud::MengeQuelle::Soll), QStringLiteral("Quelle ohne Messwert nicht Soll"));

        // Messen
        const double vKb = s0[0] + 1.5, swKb = s0[5] - 0.7, vAn = s0[3] - 1.2;
        const double sollKb = sud->data(row, ModelSud::ColMengeSollKochbeginn).toDouble();
        sud->setzeMesswert(row, ModelMesswerte::SchrittKochbeginn, ModelMesswerte::GroesseVolumen, vKb, ModelMesswerte::MethodeMessstab);
        sud->setzeMesswert(row, ModelMesswerte::SchrittKochbeginn, ModelMesswerte::GroesseExtrakt, swKb, ModelMesswerte::MethodeRefraktometer);
        sud->setzeMesswert(row, ModelMesswerte::SchrittAnstellen, ModelMesswerte::GroesseVolumen, vAn);
        sud->setzeMesswert(row, ModelMesswerte::SchrittMaischschritt + 1, ModelMesswerte::GroesseDauerRast, 47.0, ModelMesswerte::MethodeUhr, QString(), 45.0);
        pruefe(sud->data(row, ModelSud::ColWuerzemengeKochbeginn).toDouble() == vKb, QStringLiteral("Sud-Feld Kochbeginn != Messwert"));
        pruefe(sud->data(row, ModelSud::ColSWKochbeginn).toDouble() == swKb, QStringLiteral("Sud-Feld SW Kochbeginn != Messwert"));
        pruefe(sud->data(row, ModelSud::ColWuerzemengeAnstellen).toDouble() == vAn, QStringLiteral("Sud-Feld Anstellen != Messwert"));
        pruefe(std::fabs(sud->data(row, ModelSud::ColWuerzemengeKochende).toDouble() - vKb) <= 1e-9, QStringLiteral("Kochende folgt Messwert Kochbeginn nicht"));
        pruefe(mw->data(mw->zeile(sudId, ModelMesswerte::SchrittKochbeginn, ModelMesswerte::GroesseVolumen), ModelMesswerte::ColSollwert).toDouble() == sollKb,
               QStringLiteral("Sollwert-Snapshot Kochbeginn falsch"));
        pruefe(mw->wert(sudId, ModelMesswerte::SchrittMaischschritt + 1, ModelMesswerte::GroesseDauerRast).toDouble() == 47.0, QStringLiteral("Maischschritt-Dauer nicht gespeichert"));
        pruefe(sud->istGemessen(row, ModelSud::ColWuerzemengeKochbeginn) && !sud->istGemessen(row, ModelSud::ColWuerzemengeKochende), QStringLiteral("istGemessen falsch"));
        pruefe(sud->data(row, ModelSud::ColMengeEffektiv).toDouble() == vAn
               && sud->data(row, ModelSud::ColMengeEffektivQuelle).toInt() == static_cast<int>(ModelSud::MengeQuelle::Gemessen), QStringLiteral("effektive Menge nicht Messwert Anstellen"));

        // Rezept aendern: Messwerte bleiben
        sud->setData(row, ModelSud::ColSW, sw + 0.5);
        sud->setData(row, ModelSud::ColMenge, menge + 2.0);
        sud->setData(row, ModelSud::ColSpeisemenge, sud->data(row, ModelSud::ColSpeisemenge).toDouble() + 0.3);
        pruefe(sud->data(row, ModelSud::ColWuerzemengeKochbeginn).toDouble() == vKb, QStringLiteral("Rezeptaenderung ueberschreibt Messwert Kochbeginn"));
        pruefe(sud->data(row, ModelSud::ColSWKochbeginn).toDouble() == swKb, QStringLiteral("Rezeptaenderung ueberschreibt Messwert SW Kochbeginn"));
        pruefe(sud->data(row, ModelSud::ColWuerzemengeAnstellen).toDouble() == vAn, QStringLiteral("Rezeptaenderung ueberschreibt Messwert Anstellen"));
        sud->setData(row, ModelSud::ColSpeisemenge, sud->data(row, ModelSud::ColSpeisemenge).toDouble() - 0.3);
        sud->setData(row, ModelSud::ColSW, sw);
        sud->setData(row, ModelSud::ColMenge, menge);

        // Entfernen: zurueck auf die Vorbelegung
        sud->setzeMesswert(row, ModelMesswerte::SchrittKochbeginn, ModelMesswerte::GroesseVolumen, QVariant());
        sud->setzeMesswert(row, ModelMesswerte::SchrittKochbeginn, ModelMesswerte::GroesseExtrakt, QVariant());
        sud->setzeMesswert(row, ModelMesswerte::SchrittAnstellen, ModelMesswerte::GroesseVolumen, QVariant());
        sud->setzeMesswert(row, ModelMesswerte::SchrittMaischschritt + 1, ModelMesswerte::GroesseDauerRast, QVariant());
        const QList<double> s1 = istStand(row);
        for (int i = 0; i < istFelder.size(); ++i)
            pruefe(std::fabs(s1[i] - s0[i]) <= 1e-9 * std::max(1.0, std::fabs(s0[i])),
                   QStringLiteral("Rueckweg Feld %1: %2 statt %3").arg(sud->fieldName(istFelder[i])).arg(s1[i]).arg(s0[i]));
        pruefe(mw->zeile(sudId, ModelMesswerte::SchrittKochbeginn, ModelMesswerte::GroesseVolumen) < 0
               && !mw->wert(sudId, ModelMesswerte::SchrittMaischschritt + 1, ModelMesswerte::GroesseDauerRast).isValid(), QStringLiteral("Messwerte nicht entfernt"));
        pruefe(sud->data(row, ModelSud::ColMengeEffektivQuelle).toInt() == static_cast<int>(ModelSud::MengeQuelle::Soll), QStringLiteral("Quelle nach Entfernen nicht Soll"));

        const bool ok = probleme.isEmpty();
        if (!ok)
            ++fehler;
        out << (ok ? "OK    " : "FEHLER") << " Sud '" << name << "'\n";
        if (!ok)
            out << "      " << probleme.join(QStringLiteral("\n      ")) << "\n";
    }
    out << (fehler == 0 ? QStringLiteral("Rundlauftest Protokoll: alle OK\n") : QStringLiteral("Rundlauftest Protokoll: %1 Fehler\n").arg(fehler));
    return fehler;
}

static int umkehrTest(Brauhelfer& bh, QTextStream& out)
{
    struct Feld { int col; const char* name; double delta; int hopfenArt; int malzArt = 0; };
    const QList<Feld> felder = {
        {ModelSud::ColMenge, "Menge (Malz fest)", 2.0, -2, 1},
        {ModelSud::ColSudhausausbeute, "Sudhausausbeute (Malz fest)", 2.0, -2, 1},
        {ModelSud::ColIBU, "IBU (Bittereanteil)", 1.0, 1},
        {ModelSud::ColIBU, "IBU (Gewichtsanteil)", 1.0, 0},
        {ModelSud::ColSW, "SW", 0.5, -2},
        {ModelSud::ColMenge, "Menge", 2.0, -2},
        {ModelSud::ColSudhausausbeute, "Sudhausausbeute", 2.0, -2},
        {ModelSud::ColVerdampfungsrate, "Verdampfungsrate", 1.0, -2},
        {ModelSud::ColKochdauer, "Kochdauer", 10.0, -2},
        {ModelSud::ColNachisomerisierungszeit, "Nachisomerisierungszeit", 5.0, -2},
        {ModelSud::ColFaktorHauptguss, "FaktorHauptguss", 0.5, -2},
        {ModelSud::ColhighGravityFaktor, "highGravityFaktor", 10.0, -2},
        {ModelSud::ColVergaerungsgrad, "Vergaerungsgrad", 2.0, -2},
        {ModelSud::ColCO2, "CO2", 0.5, -2}};
    int fehler = 0;
    for (int row = 0; row < bh.modelSud()->rowCount(); ++row)
    {
        ModelSud* sud = bh.modelSud();
        const QString name = sud->data(row, ModelSud::ColSudname).toString();
        sud->setData(row, ModelSud::ColStatus, static_cast<int>(Brauhelfer::SudStatus::Rezept));
        for (const Feld& f : felder)
        {
            if (f.hopfenArt != -2)
                sud->setData(row, ModelSud::ColberechnungsArtHopfen, f.hopfenArt);
            sud->setData(row, ModelSud::ColberechnungsArtMalz, f.malzArt);
            const double v0 = sud->data(row, f.col).toDouble();
            // 1. Durchgang: "erste Eingabe gleicht an" (inkonsistente Vorlage wird stimmig) - erwartet
            const QMap<QString, QVariant> start = sudSnapshot(bh, row);
            sud->setData(row, f.col, v0 + f.delta);
            sud->setData(row, f.col, v0);
            const QMap<QString, QVariant> vorher = sudSnapshot(bh, row);
            const int angeglichen = snapshotDiff(start, vorher).size();
            // 2. Durchgang: jetzt muss das Aendern/Zuruecksetzen exakt umkehrbar sein
            sud->setData(row, f.col, v0 + f.delta);
            sud->setData(row, f.col, v0);
            const QStringList exakt = snapshotDiff(vorher, sudSnapshot(bh, row));
            if (angeglichen > 0 && exakt.isEmpty())
            {
                if (f.col == ModelSud::ColSW || f.col == ModelSud::ColMenge)
                    out << "BEKANNT (S12): Braudaten mit Sollwerten ueberschrieben  ";
                else
                    out << "erste Eingabe glich an (erwartet), danach umkehrbar  ";
                out << name << " / " << f.name << ": " << angeglichen << " Feld(er)\n";
            }
            // wie in der GUI: Ruecksetzen auf den angezeigten (gerundeten) Wert
            const int dez = (f.col == ModelSud::ColIBU || f.col == ModelSud::ColKochdauer || f.col == ModelSud::ColNachisomerisierungszeit
                             || f.col == ModelSud::ColhighGravityFaktor || f.col == ModelSud::ColVergaerungsgrad) ? 0 : 1;
            const double angezeigt = std::round(v0 * std::pow(10, dez)) / std::pow(10, dez);
            QString hinweis;
            if (angezeigt != v0)
                hinweis = QStringLiteral(" (gespeichert %1, angezeigt %2)").arg(v0, 0, 'g', 12).arg(angezeigt);
            if (!exakt.isEmpty())
            {
                ++fehler;
                out << "NICHT UMKEHRBAR  " << name << " / " << f.name << hinweis << ": " << exakt.size() << " Feld(er)\n";
                for (const QString& z : exakt.mid(0, 8))
                    out << "      " << z << "\n";
            }
            else if (!hinweis.isEmpty())
            {
                out << "ok, aber Anzeige gerundet  " << name << " / " << f.name << hinweis << "\n";
            }
        }
    }
    out << (fehler == 0 ? QStringLiteral("Umkehrtest: alle exakt umkehrbar\n") : QStringLiteral("Umkehrtest: %1 nicht umkehrbar\n").arg(fehler));
    return fehler;
}

static QJsonArray dumpModel(SqlTableModel* model)
{
    QJsonArray rows;
    const int cols = model->columnCount();
    for (int r = 0; r < model->rowCount(); ++r)
    {
        QJsonObject obj;
        for (int c = 0; c < cols; ++c)
        {
            const QString name = model->fieldName(c);
            if (name.isEmpty() || name == QStringLiteral("deleted"))
                continue;
            obj.insert(name, toJson(model->data(r, c)));
        }
        rows.append(obj);
    }
    return rows;
}

// ---- Rundlauftest Farbmalz-Zugabe im Maischplan (ext-s12b) ----
// Je Sud mit mind. 2 Malzen: dunkelstes Malz in einer neuen letzten Maischplan-Zeile "Zuschuetten"
// (8 min) zugeben. Pruefungen: Extrakt/Schuettung unveraendert, Malzanteil der Zeile = Prozent des
// Malzes und Einmaischen um denselben Wert kleiner, Farbe sinkt im Verhaeltnis (1 - f(t)),
// tau sehr klein = volle Farbe (exakt), Kontaktzeit mit Aufheizschritt, Entfernen = exakt wie vorher.
static int rundlaufFarbmalz(Brauhelfer& bh, QTextStream& out)
{
    int fehler = 0;
    ModelSud* sud = bh.modelSud();
    ModelMaischplan* mp = bh.modelMaischplan();
    ModelMalzschuettung* ms = bh.modelMalzschuettung();
    const int colDelMp = mp->fieldIndex(QStringLiteral("deleted"));
    const int colDelMs = ms->fieldIndex(QStringLiteral("deleted"));
    const int zuschuetten = static_cast<int>(Brauhelfer::RastTyp::Zuschuetten);
    const int einmaischen = static_cast<int>(Brauhelfer::RastTyp::Einmaischen);
    for (int row = 0; row < sud->rowCount(); ++row)
    {
        const QString name = sud->data(row, ModelSud::ColSudname).toString();
        sud->setData(row, ModelSud::ColStatus, static_cast<int>(Brauhelfer::SudStatus::Rezept));
        const QVariant sudId = sud->data(row, ModelSud::ColID);
        QString malz;
        double farbeMalz = -1.0, prozent = 0.0;
        int anzahl = 0;
        for (int r = 0; r < ms->rowCount(); ++r)
        {
            if (ms->data(r, ModelMalzschuettung::ColSudID) != sudId || ms->data(r, colDelMs).toBool())
                continue;
            ++anzahl;
            double f = ms->data(r, ModelMalzschuettung::ColFarbe).toDouble();
            if (f > farbeMalz)
            {
                farbeMalz = f;
                malz = ms->data(r, ModelMalzschuettung::ColName).toString().trimmed();
                prozent = ms->data(r, ModelMalzschuettung::ColProzent).toDouble();
            }
        }
        int rowEin = -1;
        double lastT = 0.0;
        for (int r = 0; r < mp->rowCount(); ++r)
        {
            if (mp->data(r, ModelMaischplan::ColSudID) != sudId || mp->data(r, colDelMp).toBool())
                continue;
            if (rowEin < 0 && mp->data(r, ModelMaischplan::ColTyp).toInt() == einmaischen)
                rowEin = r;
            lastT = mp->data(r, ModelMaischplan::ColTempRast).toDouble();
        }
        if (mp->hatMalzZugabe(sudId))
        {
            out << "--     Sud '" << name << "' uebersprungen (hat bereits eine Malz-Zugabe im Maischplan)\n";
            continue;
        }
        if (anzahl < 2 || farbeMalz <= 0.0 || rowEin < 0)
        {
            out << "--     Sud '" << name << "' uebersprungen (weniger als 2 Malze oder kein Einmaischen)\n";
            continue;
        }
        sud->update(row);
        const double farbe0 = sud->data(row, ModelSud::Colerg_Farbe).toDouble();
        const double schuet0 = sud->data(row, ModelSud::Colerg_S_Gesamt).toDouble();
        const double sw0 = sud->data(row, ModelSud::ColSW).toDouble();
        const double ein0 = mp->data(rowEin, ModelMaischplan::ColAnteilMalz).toDouble();
        QStringList p;

        // Zugabe-Zeile anhaengen (wie GUI: Zeile anlegen, dann Malze zuordnen)
        QMap<int, QVariant> v;
        v[ModelMaischplan::ColSudID] = sudId;
        v[ModelMaischplan::ColTyp] = zuschuetten;
        v[ModelMaischplan::ColName] = QStringLiteral("Farbmalz-Zugabe");
        v[ModelMaischplan::ColAnteilWasser] = 0;
        v[ModelMaischplan::ColAnteilMalz] = 0;
        v[ModelMaischplan::ColTempMalz] = 21;
        v[ModelMaischplan::ColTempRast] = lastT;
        v[ModelMaischplan::ColDauerRast] = 8;
        const int rowZu = mp->append(v);
        mp->setData(rowZu, ModelMaischplan::ColDauerRast, 8);   // Art setzt Standarddauer
        const double farbeOhne = sud->data(row, ModelSud::Colerg_Farbe).toDouble();
        if (farbeOhne != farbe0)
            p << QStringLiteral("Zeile ohne Zuordnung aendert Farbe (%1 -> %2)").arg(farbe0).arg(farbeOhne);
        mp->setData(rowZu, ModelMaischplan::ColMalze, malz);

        const double kontakt = mp->kontaktzeit(rowZu);
        const double farbeTau10 = sud->data(row, ModelSud::Colerg_Farbe).toDouble();
        if (std::fabs(kontakt - 8.0) > 1e-12)
            p << QStringLiteral("Kontaktzeit %1 statt 8 min").arg(kontakt);
        if (!(farbeTau10 < farbe0))
            p << QStringLiteral("Farbe sinkt nicht (%1 -> %2)").arg(farbe0).arg(farbeTau10);
        if (sud->data(row, ModelSud::Colerg_S_Gesamt).toDouble() != schuet0 || sud->data(row, ModelSud::ColSW).toDouble() != sw0)
            p << QStringLiteral("Schuettung/SW veraendert");
        const double anteilZu = mp->data(rowZu, ModelMaischplan::ColAnteilMalz).toDouble();
        const double anteilEin = mp->data(rowEin, ModelMaischplan::ColAnteilMalz).toDouble();
        if (std::fabs(anteilZu - prozent) > 1e-9)
            p << QStringLiteral("Malzanteil Zugabe %1 statt %2 %").arg(anteilZu).arg(prozent);
        if (std::fabs(anteilEin - (ein0 - prozent)) > 1e-9)
            p << QStringLiteral("Einmaischen %1 statt %2 %").arg(anteilEin).arg(ein0 - prozent);

        // tau sehr klein -> volle Farbe (exakt); tau sehr gross -> Farbe des Malzes ~0; Verhaeltnis (1 - f)
        bh.setFarbmalzTau(1e-3);
        const double farbeVoll = sud->data(row, ModelSud::Colerg_Farbe).toDouble();
        bh.setFarbmalzTau(1e12);
        const double farbeNull = sud->data(row, ModelSud::Colerg_Farbe).toDouble();
        bh.setFarbmalzTau(10.0);
        if (farbeVoll != farbe0)
            p << QStringLiteral("tau->0: Farbe %1 statt %2").arg(farbeVoll, 0, 'g', 15).arg(farbe0, 0, 'g', 15);
        const double erwartet = (1.0 - BierCalc::farbausbeute(8.0, 10.0));
        const double ist = (farbe0 - farbeTau10) / (farbe0 - farbeNull);
        if (std::fabs(ist - erwartet) > 1e-6)
            p << QStringLiteral("Farbminderung %1 statt %2 von voll").arg(ist).arg(erwartet);
        if (sud->data(row, ModelSud::Colerg_Farbe).toDouble() != farbeTau10)
            p << QStringLiteral("tau zurueck: Farbe nicht wie vorher");

        // weiterer Aufheizschritt danach: +6 K, 5 min -> Kontaktzeit 8 + 6/Rate + 5
        QMap<int, QVariant> v2;
        v2[ModelMaischplan::ColSudID] = sudId;
        v2[ModelMaischplan::ColTyp] = static_cast<int>(Brauhelfer::RastTyp::Aufheizen);
        v2[ModelMaischplan::ColName] = QStringLiteral("Test");
        v2[ModelMaischplan::ColTempRast] = lastT + 6;
        v2[ModelMaischplan::ColDauerRast] = 5;
        const int rowAuf = mp->append(v2);
        mp->setData(rowAuf, ModelMaischplan::ColTempRast, lastT + 6);
        mp->setData(rowAuf, ModelMaischplan::ColDauerRast, 5);
        const double kontakt2 = mp->kontaktzeit(rowZu);
        const double kontakt2Soll = 8.0 + qRound(6.0 / bh.aufheizrate()) + 5.0;
        if (std::fabs(kontakt2 - kontakt2Soll) > 1e-12)
            p << QStringLiteral("Kontaktzeit mit Aufheizen %1 statt %2").arg(kontakt2).arg(kontakt2Soll);
        if (!(sud->data(row, ModelSud::Colerg_Farbe).toDouble() > farbeTau10))
            p << QStringLiteral("laengere Kontaktzeit erhoeht Farbe nicht");
        mp->removeRow(rowAuf);
        if (sud->data(row, ModelSud::Colerg_Farbe).toDouble() != farbeTau10)
            p << QStringLiteral("Aufheizschritt entfernt: Farbe nicht wie vorher");

        // Zuordnung entfernen, Zeile entfernen -> Farbe exakt wie vorher
        mp->setData(rowZu, ModelMaischplan::ColMalze, QString());
        if (sud->data(row, ModelSud::Colerg_Farbe).toDouble() != farbe0)
            p << QStringLiteral("Zuordnung entfernt: Farbe %1 statt %2").arg(sud->data(row, ModelSud::Colerg_Farbe).toDouble(), 0, 'g', 15).arg(farbe0, 0, 'g', 15);
        mp->removeRow(rowZu);
        if (sud->data(row, ModelSud::Colerg_Farbe).toDouble() != farbe0)
            p << QStringLiteral("Zeile entfernt: Farbe nicht wie vorher");

        out << (p.isEmpty() ? "OK    " : "FEHLER") << " " << name << ": " << malz << " (" << farbeMalz << " EBC, "
            << QString::number(prozent, 'f', 1) << " %) 8 min -> Farbe " << QString::number(farbe0, 'f', 2) << " -> "
            << QString::number(farbeTau10, 'f', 2) << " EBC (Ausbeute " << QString::number(BierCalc::farbausbeute(8, 10) * 100, 'f', 0) << " %)\n";
        if (!p.isEmpty())
        {
            ++fehler;
            out << "      " << p.join(QStringLiteral("\n      ")) << "\n";
        }
    }
    out << (fehler == 0 ? "Rundlauftest Farbmalz: alle OK\n" : QStringLiteral("Rundlauftest Farbmalz: %1 Fehler\n").arg(fehler));
    return fehler;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream err(stderr);

    const QStringList args = app.arguments();
    if (args.size() < 3)
    {
        err << "Aufruf: kbh-dump <datenbank.sqlite> <ausgabe.json> [--no-recalc] [--save]\n";
        return 2;
    }
    const QString dbIn = args.at(1);
    const QString jsonOut = args.at(2);
    const bool recalc = !args.contains(QStringLiteral("--no-recalc"));
    const bool save = args.contains(QStringLiteral("--save"));

    QTemporaryDir tmp;
    QString dbCopy = dbIn;
    if (!save)
    {
        dbCopy = tmp.filePath(QStringLiteral("kbh-dump.sqlite"));
        if (!tmp.isValid() || !QFile::copy(dbIn, dbCopy))
        {
            err << "Datenbank kann nicht kopiert werden: " << dbIn << "\n";
            return 1;
        }
        QFile::setPermissions(dbCopy, QFile::ReadOwner | QFile::WriteOwner);
    }

    int rc = 0;
    {
        Brauhelfer bh(dbCopy);
        if (!bh.connectDatabase())
        {
            err << "Verbindung fehlgeschlagen.\n";
            return 1;
        }
        if (bh.databaseVersion() < bh.databaseVersionSupported() && args.contains(QStringLiteral("--migrate")))
        {
            // Migration wie in der App (main.cpp), hier auf der Kopie bzw. bei --save auf der Datei selbst
            err << "Migriere Datenbank " << bh.databaseVersion() << " -> " << bh.databaseVersionSupported() << "\n";
            if (!bh.updateDatabase())
            {
                err << "Migration fehlgeschlagen.\n";
                return 1;
            }
        }
        if (bh.databaseVersion() != bh.databaseVersionSupported())
        {
            err << "Datenbankversion " << bh.databaseVersion() << " != unterstuetzt "
                << bh.databaseVersionSupported() << " (--migrate verwenden oder in der App aktualisieren).\n";
            return 1;
        }

        // Spaltenpruefung (Q_ASSERT in Database::setTables greift im Release-Build nicht)
        {
            const QList<QPair<SqlTableModel*, int>> pruefung = {
                {bh.modelSud(), ModelSud::NumCols},
                {bh.modelAusruestung(), ModelAusruestung::NumCols},
                {bh.modelMesswerte(), ModelMesswerte::NumCols}};
            for (const auto& p : pruefung)
            {
                if (p.first->columnCount() != p.second)
                {
                    err << "Spaltenanzahl passt nicht zum Enum: " << p.first->tableName() << " "
                        << p.first->columnCount() << " != " << p.second << "\n";
                    return 1;
                }
            }
        }

        if (args.contains(QStringLiteral("--rundlauf-malz")) && !save)
        {
            QTextStream out(stdout);
            int fehler = rundlaufMalz(bh, out);
            out.flush();
            bh.discard();
            bh.disconnectDatabase();
            return fehler == 0 ? 0 : 3;
        }

        if (args.contains(QStringLiteral("--rundlauf-wasser")) && !save)
        {
            QTextStream out(stdout);
            int fehler = rundlaufWasser(bh, out);
            out.flush();
            bh.discard();
            bh.disconnectDatabase();
            return fehler == 0 ? 0 : 3;
        }

        if (args.contains(QStringLiteral("--rundlauf-nachiso")) && !save)
        {
            QTextStream out(stdout);
            int fehler = rundlaufNachiso(bh, out);
            out.flush();
            bh.discard();
            bh.disconnectDatabase();
            return fehler == 0 ? 0 : 3;
        }

        if (args.contains(QStringLiteral("--rundlauf-farbmalz")) && !save)
        {
            QTextStream out(stdout);
            int fehler = rundlaufFarbmalz(bh, out);
            out.flush();
            bh.discard();
            bh.disconnectDatabase();
            return fehler == 0 ? 0 : 3;
        }

        if (args.contains(QStringLiteral("--rundlauf-protokoll")) && !save)
        {
            QTextStream out(stdout);
            int fehler = rundlaufProtokoll(bh, out);
            out.flush();
            bh.discard();
            bh.disconnectDatabase();
            return fehler == 0 ? 0 : 3;
        }

        if (args.contains(QStringLiteral("--umkehrtest")) && !save)
        {
            QTextStream out(stdout);
            int fehler = umkehrTest(bh, out);
            out.flush();
            bh.discard();
            bh.disconnectDatabase();
            return fehler == 0 ? 0 : 3;
        }

        if (args.contains(QStringLiteral("--rundlauf-hopfen")))
        {
            if (save)
            {
                err << "--rundlauf-hopfen nicht zusammen mit --save.\n";
                return 2;
            }
            QTextStream out(stdout);
            int fehler = rundlaufHopfen(bh, out);
            out << (fehler == 0 ? "Rundlauftest Hopfen: alle OK\n" : QStringLiteral("Rundlauftest Hopfen: %1 Fehler\n").arg(fehler));
            out.flush();
            bh.discard();
            bh.disconnectDatabase();
            return fehler == 0 ? 0 : 3;
        }

        if (recalc)
        {
            for (int row = 0; row < bh.modelSud()->rowCount(); ++row)
                bh.modelSud()->update(row);
        }

        const QList<QPair<QString, SqlTableModel*>> models = {
            {QStringLiteral("Sud"), bh.modelSud()},
            {QStringLiteral("Malzschuettung"), bh.modelMalzschuettung()},
            {QStringLiteral("Hopfengaben"), bh.modelHopfengaben()},
            {QStringLiteral("Hefegaben"), bh.modelHefegaben()},
            {QStringLiteral("WeitereZutatenGaben"), bh.modelWeitereZutatenGaben()},
            {QStringLiteral("Maischplan"), bh.modelMaischplan()},
            {QStringLiteral("Wasseraufbereitung"), bh.modelWasseraufbereitung()},
            {QStringLiteral("Schnellgaerverlauf"), bh.modelSchnellgaerverlauf()},
            {QStringLiteral("Hauptgaerverlauf"), bh.modelHauptgaerverlauf()},
            {QStringLiteral("Nachgaerverlauf"), bh.modelNachgaerverlauf()},
            {QStringLiteral("Ausruestung"), bh.modelAusruestung()},
            {QStringLiteral("Malz"), bh.modelMalz()},
            {QStringLiteral("Hopfen"), bh.modelHopfen()},
            {QStringLiteral("Hefe"), bh.modelHefe()},
            {QStringLiteral("WeitereZutaten"), bh.modelWeitereZutaten()},
            {QStringLiteral("Wasser"), bh.modelWasser()},
            {QStringLiteral("Messwerte"), bh.modelMesswerte()},
        };

        QJsonObject root;
        root.insert(QStringLiteral("_databaseVersion"), bh.databaseVersion());
        root.insert(QStringLiteral("_recalc"), recalc);
        for (const auto& m : models)
            root.insert(m.first, dumpModel(m.second));

        QFile out(jsonOut);
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        }
        else
        {
            err << "Ausgabe kann nicht geschrieben werden: " << jsonOut << "\n";
            rc = 1;
        }

        if (save)
        {
            if (!bh.save() && bh.isDirty())
            {
                err << "Speichern fehlgeschlagen.\n";
                rc = 1;
            }
        }
        else
        {
            bh.discard();
        }
        bh.disconnectDatabase();
    }
    return rc;
}
