#include "doublespinboxsud.h"
#include "brauhelfer.h"
#include "commands/undostack.h"

DoubleSpinBoxSud::DoubleSpinBoxSud(QWidget *parent) :
    DoubleSpinBox(parent),
    mSud(nullptr),
    mCol(-1),
    mWertBeiFokus(0.0),
    mAnzeigeBeiFokus(0.0)
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

void DoubleSpinBoxSud::updateValue()
{
    setValue(mSud->getValue(mCol).toDouble());
}

void DoubleSpinBoxSud::on_valueChanged(double val)
{
    if (hasFocus())
    {
        // Zurueck auf den beim Fokuserhalt angezeigten (gerundeten) Wert -> exakten Wert
        // wiederherstellen, sonst ginge die nicht angezeigte Genauigkeit verloren.
        if (val == mAnzeigeBeiFokus)
            val = mWertBeiFokus;
        gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), mCol, val));
    }
}
