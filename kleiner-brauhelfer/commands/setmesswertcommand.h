#ifndef SETMESSWERTCOMMAND_H
#define SETMESSWERTCOMMAND_H

#include <QUndoCommand>
#include <QVariant>

class Brauhelfer;

// Brauprotokoll (ext-s12): Messwert setzen bzw. entfernen (wert ungueltig) inkl. zugehoerigem
// Sud-Feld (ModelSud::setzeMesswert). Undo stellt den vorherigen Messwert und das Sud-Feld her.
class SetMesswertCommand : public QUndoCommand
{
public:
    SetMesswertCommand(Brauhelfer *bh, int sudRow, int schritt, int groesse, const QVariant &wert, int methode,
                       const QString &bemerkung = QString(), const QVariant &sollwert = QVariant(), QUndoCommand *parent = nullptr);
    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;
    int id() const Q_DECL_OVERRIDE;
    bool mergeWith(const QUndoCommand *other) Q_DECL_OVERRIDE;

private:
    struct Stand
    {
        QVariant wert;  // ungueltig = nicht gemessen
        int methode;
        QString bemerkung;
        QVariant sollwert;
    };
    void anwenden(const Stand &stand);

    Brauhelfer *bh;
    int sudRow;
    int schritt;
    int groesse;
    Stand neu;
    Stand alt;
    QVariant sudWertAlt;
};

#endif // SETMESSWERTCOMMAND_H
