#include "doublespinboxsud.h"
#include "brauhelfer.h"
#include "commands/undostack.h"
#include "commands/setmesswertcommand.h"

DoubleSpinBoxSud::DoubleSpinBoxSud(QWidget *parent) :
    DoubleSpinBox(parent),
    mSud(nullptr),
    mCol(-1),
    mWertBeiFokus(0.0),
    mAnzeigeBeiFokus(0.0),
    mSchritt(-1),
    mGroesse(-1)
{
    connect(this, &DoubleSpinBox::valueChanged, this, &DoubleSpinBoxSud::on_valueChanged);
}

void DoubleSpinBoxSud::focusInEvent(QFocusEvent *event)
{
    if (mSud && mCol >= 0)
    {
        mWertBeiFokus = mSud->getValue(mCol).toDouble();
        mAnzeigeBeiFokus = value();
    }
    DoubleSpinBox::focusInEvent(event);
}

void DoubleSpinBoxSud::setColumn(SudObject *sud, int col)
{
    mSud = sud;
    mCol = col;
}

int DoubleSpinBoxSud::column() const
{
    return mCol;
}

void DoubleSpinBoxSud::setMesswert(int schritt, int groesse)
{
    mSchritt = schritt;
    mGroesse = groesse;
    mToolTipBasis = toolTip();
}

void DoubleSpinBoxSud::updateValue()
{
    setValue(mSud->getValue(mCol).toDouble());
    if (mSchritt >= 0)
    {
        const bool gemessen = mSud->bh()->modelMesswerte()->wert(mSud->id(), mSchritt, mGroesse).isValid();
        QFont f = font();
        if (f.italic() == gemessen)
        {
            f.setItalic(!gemessen);
            setFont(f);
        }
        QString hinweis = gemessen ? tr("Gemessen (Brauprotokoll).")
                                   : tr("Nicht gemessen: Sollwert, wird bei Rezeptänderungen nachgeführt. Eine Eingabe gilt als Messwert.");
        setToolTip(mToolTipBasis.isEmpty() ? hinweis : mToolTipBasis + QStringLiteral("\n\n") + hinweis);
    }
}

void DoubleSpinBoxSud::on_valueChanged(double val)
{
    if (hasFocus())
    {
        // Zurueck auf den beim Fokuserhalt angezeigten (gerundeten) Wert -> exakten Wert
        // wiederherstellen, sonst ginge die nicht angezeigte Genauigkeit verloren.
        if (val == mAnzeigeBeiFokus)
            val = mWertBeiFokus;
        if (mSchritt >= 0)
        {
            // Anstellmenge inkl. Speise: gemessen wird die Wuerzemenge ohne Speise
            if (mCol == ModelSud::ColWuerzemengeAnstellenTotal)
                val -= mSud->getValue(ModelSud::ColSpeisemenge).toDouble();
            gUndoStack->push(new SetMesswertCommand(mSud->bh(), mSud->row(), mSchritt, mGroesse, val, ModelMesswerte::MethodeEingabe));
        }
        else
        {
            gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), mCol, val));
        }
    }
}
