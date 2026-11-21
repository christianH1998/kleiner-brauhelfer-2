#ifndef DOUBLESPINBOXSUD_H
#define DOUBLESPINBOXSUD_H

#include "doublespinbox.h"

class SudObject;

class DoubleSpinBoxSud : public DoubleSpinBox
{
    Q_OBJECT

public:
    DoubleSpinBoxSud(QWidget *parent = nullptr);
    void setColumn(SudObject *sud, int col);
    int column() const;
    void updateValue();
protected:
    void focusInEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
private slots:
    void on_valueChanged(double val);
private:
    SudObject *mSud;
    int mCol;
    // exakter Modellwert und gerundeter Anzeigewert beim Fokuserhalt (ext-s01b)
    double mWertBeiFokus;
    double mAnzeigeBeiFokus;
};

#endif // DOUBLESPINBOXSUD_H
