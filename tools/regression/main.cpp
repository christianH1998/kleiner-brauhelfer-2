// kbh-dump: schreibt alle gespeicherten und virtuellen Werte der Modelle als JSON.
// Zweck: Regressionsvergleich der Berechnungen vor/nach einer Code-Aenderung.
// Die Datenbank wird in ein Temp-Verzeichnis kopiert und nie gespeichert.
//
// Aufruf: kbh-dump <datenbank.sqlite> <ausgabe.json> [--no-recalc] [--save]
//   ohne --no-recalc wird fuer jeden Sud ModelSud::update() ausgefuehrt
//   (wie beim Bearbeiten in der App; wirkt nur auf Sude im Status Rezept).
//   --save: NUR FUER TESTDATEN. Arbeitet direkt auf der angegebenen Datenbank
//   (keine Kopie) und speichert die Neuberechnung (Brauhelfer::save).

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>
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
        if (bh.databaseVersion() != bh.databaseVersionSupported())
        {
            err << "Datenbankversion " << bh.databaseVersion() << " != unterstuetzt "
                << bh.databaseVersionSupported() << " (zuerst in der App aktualisieren).\n";
            return 1;
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
