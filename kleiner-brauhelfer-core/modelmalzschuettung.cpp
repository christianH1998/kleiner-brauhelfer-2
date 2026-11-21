// clazy:excludeall=skipped-base-method
#include "modelmalzschuettung.h"
#include "brauhelfer.h"
#include "modelsud.h"
#include "proxymodelsud.h"

ModelMalzschuettung::ModelMalzschuettung(Brauhelfer* bh, const QSqlDatabase &db) :
    SqlTableModel(bh, db),
    bh(bh)
{
    mVirtualField.append(QStringLiteral("Extrakt"));
    mVirtualField.append(QStringLiteral("ExtraktProzent"));
    connect(bh->modelSud(), &SqlTableModel::dataChanged, this, &ModelMalzschuettung::onSudDataChanged);
}

QVariant ModelMalzschuettung::dataExt(const QModelIndex &idx) const
{
    switch(idx.column())
    {
    case ColExtrakt:
    {
        double swMalz = bh->modelSud()->dataSud(data(idx.row(), ColSudID).toInt(), ModelSud::ColSWAnteilMalz).toDouble();
        double p = data(idx.row(), ColProzent).toDouble() / 100;
        return swMalz * p;
    }
    case ColExtraktProzent:
    {
        double sw = bh->modelSud()->dataSud(data(idx.row(), ColSudID).toInt(), ModelSud::ColSWAnteilZutaten).toDouble();
        double extrakt = data(idx.row(), ColExtrakt).toDouble();
        return extrakt / sw * 100;
    }
    default:
        return QVariant();
    }
}

bool ModelMalzschuettung::setDataExt(const QModelIndex &idx, const QVariant &value)
{
    switch(idx.column())
    {
    case ColName:
    {
        if (QSqlTableModel::setData(idx, value))
        {
            int rowMalz = bh->modelMalz()->getRowWithValue(ModelMalz::ColName, value);
            if (rowMalz >= 0)
            {
                QSqlTableModel::setData(index(idx.row(), ColPotential), bh->modelMalz()->data(rowMalz, ModelMalz::ColPotential));
                QSqlTableModel::setData(index(idx.row(), ColFarbe), bh->modelMalz()->data(rowMalz, ModelMalz::ColFarbe));
                QSqlTableModel::setData(index(idx.row(), ColpH), bh->modelMalz()->data(rowMalz, ModelMalz::ColpH));
            }
            return true;
        }
        return false;
    }
    case ColProzent:
    {
        double fVal = value.toDouble();
        if (fVal < 0.0)
            fVal = 0.0;
        if (QSqlTableModel::setData(idx, fVal))
        {
            double total = bh->modelSud()->dataSud(data(idx.row(), ColSudID).toInt(), ModelSud::Colerg_S_Gesamt).toDouble();
            QSqlTableModel::setData(index(idx.row(), Colerg_Menge), fVal / 100 * total);
            return true;
        }
        return false;
    }
    case Colerg_Menge:
    {
        QVariant sudId = data(idx.row(), ColSudID);
        if (mengenFest(sudId))
        {
            // ext-s09: kg ist fuehrend; Prozent aller Gaben des Suds aus der neuen Summe
            // (verhindert Drift durch Rechnen gegen die alte Gesamtschuettung)
            double fVal = value.toDouble();
            if (fVal < 0.0)
                fVal = 0.0;
            if (!QSqlTableModel::setData(idx, fVal))
                return false;
            prozentAusMengen(sudId);
            return true;
        }
        double total = bh->modelSud()->dataSud(sudId.toInt(), ModelSud::Colerg_S_Gesamt).toDouble();
        return setData(idx.row(), ColProzent, value.toDouble() * 100 / total);
    }
    case ColExtrakt:
    {
        double swMalz = bh->modelSud()->dataSud(data(idx.row(), ColSudID).toInt(), ModelSud::ColSWAnteilMalz).toDouble();
        return setDataExt(index(idx.row(), ColProzent), value.toDouble() / swMalz * 100);
    }
    case ColExtraktProzent:
    {
        double sw = bh->modelSud()->dataSud(data(idx.row(), ColSudID).toInt(), ModelSud::ColSWAnteilZutaten).toDouble();
        return setDataExt(index(idx.row(), ColExtrakt), value.toDouble() * sw / 100);
    }
    default:
        return false;
    }
}

bool ModelMalzschuettung::mengenFest(const QVariant &sudId) const
{
    return static_cast<Brauhelfer::BerechnungsartMalz>(bh->modelSud()->dataSud(sudId, ModelSud::ColberechnungsArtMalz).toInt())
            == Brauhelfer::BerechnungsartMalz::MengenFest;
}

void ModelMalzschuettung::prozentAusMengen(const QVariant &sudId)
{
    // Prozent = Anteil an der Summe der kg (nur Prozent, kg bleiben unveraendert)
    double summe = 0.0;
    for (int r = 0; r < rowCount(); ++r)
        if (data(r, ColSudID) == sudId && !data(r, ColDeleted).toBool())
            summe += data(r, Colerg_Menge).toDouble();
    if (summe <= 0.0)
        return;
    for (int r = 0; r < rowCount(); ++r)
        if (data(r, ColSudID) == sudId && !data(r, ColDeleted).toBool())
            QSqlTableModel::setData(index(r, ColProzent), data(r, Colerg_Menge).toDouble() * 100 / summe);
}

void ModelMalzschuettung::onSudDataChanged(const QModelIndex &idx)
{
    if (idx.column() == ModelSud::ColberechnungsArtMalz)
    {
        // Wechsel auf "Mengen fest": kg bleiben, Prozent werden nur an die Summe angeglichen
        QVariant sudId = bh->modelSud()->data(idx.row(), ModelSud::ColID);
        if (mengenFest(sudId))
        {
            mSignalModifiedBlocked = true;
            prozentAusMengen(sudId);
            mSignalModifiedBlocked = false;
        }
        return;
    }
    if (idx.column() == ModelSud::Colerg_S_Gesamt)
    {
        QVariant sudId = bh->modelSud()->data(idx.row(), ModelSud::ColID);
        mSignalModifiedBlocked = true;
        for (int r = 0; r < rowCount(); ++r)
        {
            if (data(r, ColSudID) == sudId)
            {
                QModelIndex idx2 = index(r, ColProzent);
                setData(idx2, data(idx2));
            }
        }
        mSignalModifiedBlocked = false;
    }
}

void ModelMalzschuettung::update(const QVariant &name, int col, const QVariant &value)
{
    ProxyModelSud modelSud;
    modelSud.setSourceModel(bh->modelSud());
    modelSud.setFilterStatus(ProxyModelSud::Rezept);
    for (int r = 0; r < modelSud.rowCount(); ++r)
    {
        QVariant sudId = modelSud.data(r, ModelSud::ColID);
        for (int j = 0; j < rowCount(); ++j)
        {
            if (data(j, ColSudID) == sudId && data(j, ColName) == name)
                setData(j, col, value);
        }
    }
}

int ModelMalzschuettung::import(int row)
{
    QMap<int, QVariant> values({{ModelMalz::ColName, data(row, ColName)},
                                {ModelMalz::ColFarbe, data(row, ColFarbe)},
                                {ModelMalz::ColPotential, data(row, ColPotential)},
                                {ModelMalz::ColpH, data(row, ColpH)}});
    return bh->modelMalz()->append(values);
}

void ModelMalzschuettung::defaultValues(QMap<int, QVariant> &values) const
{
    if (!values.contains(ColName))
        values.insert(ColName, bh->modelMalz()->data(0, ModelMalz::ColName));
    if (!values.contains(ColProzent))
        values.insert(ColProzent, 0.0);
}
