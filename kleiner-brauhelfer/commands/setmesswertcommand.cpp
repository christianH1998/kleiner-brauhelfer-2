#include "setmesswertcommand.h"
#include "brauhelfer.h"

SetMesswertCommand::SetMesswertCommand(Brauhelfer *bh, int sudRow, int schritt, int groesse, const QVariant &wert, int methode,
                                       const QString &bemerkung, const QVariant &sollwert, QUndoCommand *parent) :
    QUndoCommand(parent),
    bh(bh),
    sudRow(sudRow),
    schritt(schritt),
    groesse(groesse)
{
    neu = {wert, methode, bemerkung, sollwert};
    ModelMesswerte *mw = bh->modelMesswerte();
    const QVariant sudId = bh->modelSud()->data(sudRow, ModelSud::ColID);
    int r = mw->zeile(sudId, schritt, groesse);
    if (r >= 0)
        alt = {mw->wert(sudId, schritt, groesse), mw->data(r, ModelMesswerte::ColMethode).toInt(),
               mw->data(r, ModelMesswerte::ColBemerkung).toString(), mw->data(r, ModelMesswerte::ColSollwert)};
    else
        alt = {QVariant(), 0, QString(), QVariant()};
    int sudCol = ModelMesswerte::sudSpalte(schritt, groesse);
    if (sudCol >= 0)
        sudWertAlt = bh->modelSud()->data(sudRow, sudCol);
}

void SetMesswertCommand::anwenden(const Stand &stand)
{
    bh->modelSud()->setzeMesswert(sudRow, schritt, groesse, stand.wert, stand.methode, stand.bemerkung, stand.sollwert);
}

void SetMesswertCommand::redo()
{
    anwenden(neu);
}

void SetMesswertCommand::undo()
{
    anwenden(alt);
    int sudCol = ModelMesswerte::sudSpalte(schritt, groesse);
    if (sudCol >= 0 && sudWertAlt.isValid())
        bh->modelSud()->setData(sudRow, sudCol, sudWertAlt);
}

int SetMesswertCommand::id() const
{
    return 2;
}

bool SetMesswertCommand::mergeWith(const QUndoCommand *other)
{
    // Tippen im selben Feld zu einem Schritt zusammenfassen
    if (other->id() != id())
        return false;
    const SetMesswertCommand *o = static_cast<const SetMesswertCommand*>(other);
    if (o->bh != bh || o->sudRow != sudRow || o->schritt != schritt || o->groesse != groesse)
        return false;
    if (!o->neu.wert.isValid() || !neu.wert.isValid())
        return false;
    neu = o->neu;
    return true;
}
