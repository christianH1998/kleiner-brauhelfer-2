#ifndef SPINBOXSUD_H
#define SPINBOXSUD_H

#include "spinbox.h"

class SudObject;


class SpinBoxSud : public SpinBox
{
    Q_OBJECT

public:
    SpinBoxSud(QWidget *parent = nullptr);
    void setColumn(SudObject *sud, int col);
    int column() const;
    void updateValue();
protected:
    void focusInEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
private slots:
    void on_valueChanged(int val);
private:
    SudObject *mSud;
    int mCol;
    // exakter Modellwert und gerundeter Anzeigewert beim Fokuserhalt (ext-s01b)
    QVariant mWertBeiFokus;
    int mAnzeigeBeiFokus;
};

#endif // SPINBOXSUD_H
