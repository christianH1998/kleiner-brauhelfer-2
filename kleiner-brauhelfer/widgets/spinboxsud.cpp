#include "spinboxsud.h"
#include "brauhelfer.h"
#include "commands/undostack.h"

SpinBoxSud::SpinBoxSud(QWidget *parent) :
    SpinBox(parent),
    mSud(nullptr),
    mCol(-1),
    mAnzeigeBeiFokus(0)
{
    connect(this, &SpinBox::valueChanged, this, &SpinBoxSud::on_valueChanged);
}

void SpinBoxSud::focusInEvent(QFocusEvent *event)
{
    if (mSud && mCol >= 0)
    {
        mWertBeiFokus = mSud->getValue(mCol);
        mAnzeigeBeiFokus = value();
    }
    SpinBox::focusInEvent(event);
}

void SpinBoxSud::setColumn(SudObject *sud, int col)
{
    mSud = sud;
    mCol = col;
}

int SpinBoxSud::column() const
{
    return mCol;
}

void SpinBoxSud::updateValue()
{
    setValue(mSud->getValue(mCol).toInt());
}

void SpinBoxSud::on_valueChanged(int val)
{
    if (hasFocus())
    {
        // Zurueck auf den beim Fokuserhalt angezeigten (gerundeten) Wert -> exakten Wert
        // wiederherstellen, sonst ginge die nicht angezeigte Genauigkeit verloren.
        if (val == mAnzeigeBeiFokus && mWertBeiFokus.isValid())
            gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), mCol, mWertBeiFokus));
        else
            gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), mCol, val));
    }
}
