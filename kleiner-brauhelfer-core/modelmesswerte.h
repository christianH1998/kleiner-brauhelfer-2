#ifndef MODELMESSWERTE_H
#define MODELMESSWERTE_H

#include "kleiner-brauhelfer-core_global.h"
#include "sqltablemodel.h"
#include <QDateTime>

class Brauhelfer;

// Brauprotokoll: eine Zeile je Messung (ab DB-Version 2010, ext-s07).
// Fehlender Messwert = keine Zeile (nie 0). Sollwert = Snapshot zum Erfassungszeitpunkt.
// Bedeutung von Schritt/Groesse/Methode wird in ext-s12 (Brauprotokoll) festgelegt.
class LIB_EXPORT ModelMesswerte : public SqlTableModel
{
    Q_OBJECT

public:

    enum Column
    {
        ColID,
        ColSudID,
        ColSchritt,
        ColGroesse,
        ColWert,
        ColSollwert,
        ColTemperatur,
        ColMethode,
        ColZeitstempel,
        ColBemerkung,
        // virtual
        ColDeleted,
        // number of columns
        NumCols
    };
    Q_ENUM(Column)

public:

    ModelMesswerte(Brauhelfer* bh, const QSqlDatabase &db = QSqlDatabase());
    QVariant dataExt(const QModelIndex &index) const Q_DECL_OVERRIDE;
    bool setDataExt(const QModelIndex &index, const QVariant &value) Q_DECL_OVERRIDE;
    void defaultValues(QMap<int, QVariant> &values) const Q_DECL_OVERRIDE;

private:

    Brauhelfer* bh;
};

#endif // MODELMESSWERTE_H
