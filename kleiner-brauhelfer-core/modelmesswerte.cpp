// clazy:excludeall=skipped-base-method
#include "modelmesswerte.h"
#include "brauhelfer.h"

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
