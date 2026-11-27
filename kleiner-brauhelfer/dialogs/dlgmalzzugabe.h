#ifndef DLGMALZZUGABE_H
#define DLGMALZZUGABE_H

#include "dlgabstract.h"

class SudObject;
class QTableWidget;
class QDoubleSpinBox;
class QLabel;

// Auswahl der Malze, die in einer Maischplan-Zeile (Art "Zuschuetten") zugegeben werden (ext-s12b)
class DlgMalzZugabe : public DlgAbstract
{
    Q_OBJECT

public:
    explicit DlgMalzZugabe(SudObject *sud, int rowMaischplan, QWidget *parent = nullptr);
    static void restoreView();
    QStringList malze() const;
    double tau() const;

private slots:
    void updateAusbeute();

private:
    SudObject *mSud;
    double mKontaktzeit;
    QTableWidget *mTable;
    QDoubleSpinBox *mTau;
    QLabel *mLblKontakt;
};

#endif // DLGMALZZUGABE_H
