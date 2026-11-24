// clazy:excludeall=skipped-base-method
#include "modelmesswerte.h"
#include "brauhelfer.h"
#include "modelsud.h"

ModelMesswerte::ModelMesswerte(Brauhelfer* bh, const QSqlDatabase &db) :
    SqlTableModel(bh, db),
    bh(bh)
{
}

QVariant ModelMesswerte::dataExt(const QModelIndex &idx) const
{
    switch(idx.column())
    {
    case ColZeitstempel:
    {
        return QDateTime::fromString(QSqlTableModel::data(idx).toString(), Qt::ISODate);
    }
    default:
        return QVariant();
    }
}

bool ModelMesswerte::setDataExt(const QModelIndex &idx, const QVariant &value)
{
    switch(idx.column())
    {
    case ColZeitstempel:
    {
        return QSqlTableModel::setData(idx, value.toDateTime().toString(Qt::ISODate));
    }
    default:
        return false;
    }
}

void ModelMesswerte::defaultValues(QMap<int, QVariant> &values) const
{
    if (!values.contains(ColZeitstempel))
        values.insert(ColZeitstempel, QDateTime::currentDateTime());
    if (!values.contains(ColMethode))
        values.insert(ColMethode, 0);
}

int ModelMesswerte::zeile(const QVariant &sudId, int schritt, int groesse) const
{
    const int id = sudId.toInt();
    for (int r = 0; r < rowCount(); ++r)
    {
        if (data(r, ColSudID).toInt() == id &&
            data(r, ColSchritt).toInt() == schritt &&
            data(r, ColGroesse).toInt() == groesse &&
            !data(r, ColDeleted).toBool())
            return r;
    }
    return -1;
}

QVariant ModelMesswerte::wert(const QVariant &sudId, int schritt, int groesse) const
{
    int r = zeile(sudId, schritt, groesse);
    if (r < 0)
        return QVariant();
    QVariant v = data(r, ColWert);
    return v.isNull() ? QVariant() : QVariant(v.toDouble());
}

int ModelMesswerte::setze(const QVariant &sudId, int schritt, int groesse, double wert, int methode,
                          const QVariant &sollwert, const QString &bemerkung)
{
    int r = zeile(sudId, schritt, groesse);
    if (r < 0)
    {
        QMap<int, QVariant> values({{ColSudID, sudId},
                                    {ColSchritt, schritt},
                                    {ColGroesse, groesse},
                                    {ColWert, wert},
                                    {ColMethode, methode},
                                    {ColSollwert, sollwert},
                                    {ColBemerkung, bemerkung}});
        return append(values);
    }
    setData(r, ColWert, wert);
    setData(r, ColMethode, methode);
    setData(r, ColSollwert, sollwert);
    setData(r, ColBemerkung, bemerkung);
    setData(r, ColZeitstempel, QDateTime::currentDateTime());
    return r;
}

bool ModelMesswerte::entferne(const QVariant &sudId, int schritt, int groesse)
{
    int r = zeile(sudId, schritt, groesse);
    if (r < 0)
        return false;
    return removeRow(r);
}

bool ModelMesswerte::istGemessen(const QVariant &sudId, int sudSpalte) const
{
    int schritt, groesse;
    if (!ausSudSpalte(sudSpalte, schritt, groesse))
        return false;
    return zeile(sudId, schritt, groesse) >= 0;
}

void ModelMesswerte::skaliere(const QVariant &sudId, double faktor)
{
    const int id = sudId.toInt();
    for (int r = 0; r < rowCount(); ++r)
    {
        if (data(r, ColSudID).toInt() != id || data(r, ColDeleted).toBool())
            continue;
        int groesse = data(r, ColGroesse).toInt();
        if (groesse != GroesseVolumen && groesse != GroesseMasse)
            continue;
        QVariant w = data(r, ColWert);
        if (!w.isNull())
            setData(r, ColWert, w.toDouble() * faktor);
        QVariant s = data(r, ColSollwert);
        if (!s.isNull())
            setData(r, ColSollwert, s.toDouble() * faktor);
    }
}

namespace
{
struct Zuordnung
{
    int schritt;
    int groesse;
    int sudSpalte;
    int sollSpalte;
};

// Messwerte mit Sud-Feld bzw. Sollwert. Ohne Eintrag: nur im Protokoll gespeichert, kein Soll aus dem Sud.
const Zuordnung zuordnungen[] = {
    {ModelMesswerte::SchrittAussentemperatur, ModelMesswerte::GroesseTemperatur, -1, ModelSud::ColAussentemperatur},
    {ModelMesswerte::SchrittSchuettung, ModelMesswerte::GroesseMasse, -1, ModelSud::Colerg_S_Gesamt},
    {ModelMesswerte::SchrittHauptguss, ModelMesswerte::GroesseVolumen, -1, ModelSud::Colerg_WHauptguss},
    {ModelMesswerte::SchrittNachguss, ModelMesswerte::GroesseVolumen, -1, ModelSud::Colerg_WNachguss},
    {ModelMesswerte::SchrittKochbeginn, ModelMesswerte::GroesseVolumen, ModelSud::ColWuerzemengeKochbeginn, ModelSud::ColMengeSollKochbeginn},
    {ModelMesswerte::SchrittKochbeginn, ModelMesswerte::GroesseExtrakt, ModelSud::ColSWKochbeginn, ModelSud::ColSWSollKochbeginn},
    {ModelMesswerte::SchrittVorHopfenseihen, ModelMesswerte::GroesseVolumen, ModelSud::ColWuerzemengeVorHopfenseihen, ModelSud::ColMengeSollKochende},
    {ModelMesswerte::SchrittKochende, ModelMesswerte::GroesseVolumen, ModelSud::ColWuerzemengeKochende, ModelSud::ColMengeSollKochende},
    {ModelMesswerte::SchrittKochende, ModelMesswerte::GroesseExtrakt, ModelSud::ColSWKochende, ModelSud::ColSWSollKochende},
    {ModelMesswerte::SchrittAnstellen, ModelMesswerte::GroesseVolumen, ModelSud::ColWuerzemengeAnstellen, ModelSud::ColMengeSollAnstellen},
    {ModelMesswerte::SchrittAnstellen, ModelMesswerte::GroesseExtrakt, ModelSud::ColSWAnstellen, ModelSud::ColSWSollAnstellen},
    {ModelMesswerte::SchrittAbfuellen, ModelMesswerte::GroesseVolumen, ModelSud::ColJungbiermengeAbfuellen, -1},
    {ModelMesswerte::SchrittAbfuellen, ModelMesswerte::GroesseExtrakt, ModelSud::ColSWJungbier, ModelSud::ColSREErwartet},
    {ModelMesswerte::SchrittKochen, ModelMesswerte::GroesseDauer, -1, ModelSud::ColKochdauer},
    {ModelMesswerte::SchrittWhirlpool, ModelMesswerte::GroesseDauer, -1, ModelSud::ColNachisomerisierungszeit},
};
}

int ModelMesswerte::sudSpalte(int schritt, int groesse)
{
    for (const Zuordnung &z : zuordnungen)
        if (z.schritt == schritt && z.groesse == groesse)
            return z.sudSpalte;
    return -1;
}

bool ModelMesswerte::ausSudSpalte(int sudSpalte, int &schritt, int &groesse)
{
    if (sudSpalte < 0)
        return false;
    for (const Zuordnung &z : zuordnungen)
    {
        if (z.sudSpalte == sudSpalte)
        {
            schritt = z.schritt;
            groesse = z.groesse;
            return true;
        }
    }
    return false;
}

int ModelMesswerte::sollSpalte(int schritt, int groesse)
{
    for (const Zuordnung &z : zuordnungen)
        if (z.schritt == schritt && z.groesse == groesse)
            return z.sollSpalte;
    return -1;
}
