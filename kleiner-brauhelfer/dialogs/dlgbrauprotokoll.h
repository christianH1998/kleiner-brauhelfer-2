#ifndef DLGBRAUPROTOKOLL_H
#define DLGBRAUPROTOKOLL_H

#include "dlgabstract.h"
#include <QTimer>

namespace Ui {
class DlgBrauprotokoll;
}

class SudObject;
class QTableWidgetItem;

// Brauprotokoll (ext-s12): Checkliste der Brauschritte mit Soll | Ist | Abweichung | Methode | Zeit | Bemerkung.
// Nicht-modal; folgt dem geladenen Sud. Messwerte ueber SetMesswertCommand (Undo).
// Ist und Bemerkung sind editierbare Tabellenzellen (Enter/Tab/Klick daneben uebernimmt).
class DlgBrauprotokoll : public DlgAbstract
{
    Q_OBJECT

public:
    static DlgBrauprotokoll *Dialog;

    explicit DlgBrauprotokoll(QWidget *parent = nullptr);
    virtual ~DlgBrauprotokoll() Q_DECL_OVERRIDE;
    static void restoreView();
    // erzwingen: auch bei gleichem Sud neu aufbauen (z. B. Eingabefelder entsperrt)
    void setSud(SudObject *sud, bool erzwingen = false);

    // Richtwert Aufheizrate [K/min] (Einstellung, Standard 1 K/min)
    static double aufheizrate();

protected:
    void changeEvent(QEvent *event) Q_DECL_OVERRIDE;

private slots:
    void aktualisierenSpaeter();
    void aktualisieren();
    void on_tbAufheizrate_valueChanged(double value);
    void zelleGeaendert(QTableWidgetItem *item);

private:
    enum Hilfe { HilfeKeine, HilfeVolumen, HilfeVolumenSudpfanne, HilfeExtrakt };
    struct Eintrag
    {
        int schritt;
        int groesse;
        QString schrittText;
        QString groesseText;
        QString einheit;
        int dezimalen;
        QVariant soll;
        Hilfe hilfe;
        QString info;       // Tooltip Schritt: was, wann, wie messen
        QString sollInfo;   // Tooltip Soll: Herkunft
        QString feld;       // Feld in den Braudaten/Abfuellen, in das der Messwert uebernommen wird (leer = nur Protokoll)
    };
    QList<Eintrag> eintraege() const;
    void aufbauen();
    void werteSetzen();
    void istGeaendert(int zeile, const QString &text);
    void methodeGeaendert(int zeile);
    void bemerkungGeaendert(int zeile, const QString &text);
    void hilfeGeklickt(int zeile);
    void setzen(int zeile, const QVariant &wert, int methode, const QString &bemerkung);
    QVariant messwert(int zeile) const;
    int messwertZeile(int zeile) const;
    bool gesperrt() const;
    QString zahl(double wert, int dezimalen) const;
    QString istText(double wert, int dezimalen) const;
    void saveSettings() Q_DECL_OVERRIDE;
    void loadSettings() Q_DECL_OVERRIDE;

private:
    Ui::DlgBrauprotokoll *ui;
    SudObject *mSud;
    QList<Eintrag> mEintraege;
    QTimer mTimer;
    bool mWerteSetzen;
};

#endif // DLGBRAUPROTOKOLL_H
