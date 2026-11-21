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
        m->setData(r0, ModelMalzschuettung::Colerg_Menge, kg0 * 1.1);      // Menge +10 %
        const MalzStand mehr = malzStand(bh, row);
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
        const bool swGestiegen = mehr.sw > b.sw;
        const bool ok = dWechsel <= tol && dZurueck <= tol && swGestiegen;
        if (!ok)
            ++fehler;
        out << (ok ? "OK    " : "FEHLER") << " Sud '" << name << "' SW " << QString::number(a.sw, 'f', 2)
            << " Schuettung " << QString::number(a.schuettung, 'f', 3) << " kg"
            << " | Moduswechsel max. rel. " << QString::number(dWechsel, 'g', 3)
            << " | Aendern+Zurueck " << QString::number(dZurueck, 'g', 3)
            << " | Malz+10% -> SW " << QString::number(mehr.sw, 'f', 2) << (swGestiegen ? "" : " (nicht gestiegen!)") << "\n";
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
