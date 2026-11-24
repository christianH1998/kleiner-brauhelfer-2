#ifndef MODELMESSWERTE_H
#define MODELMESSWERTE_H

#include "kleiner-brauhelfer-core_global.h"
#include "sqltablemodel.h"
#include <QDateTime>

class Brauhelfer;

// Brauprotokoll: eine Zeile je Messung (Tabelle ab DB-Version 2010, ext-s07; Bedeutung ext-s12).
// Je Sud, Schritt und Groesse hoechstens ein Messwert. Fehlender Messwert = keine Zeile (nie 0).
// Sollwert = Snapshot zum Erfassungszeitpunkt.
// Messwerte mit zugehoerigem Sud-Feld (z. B. Kochbeginn/Volumen -> Sud.WuerzemengeKochbeginn)
// werden zusaetzlich in das Sud-Feld geschrieben (ModelSud::setzeMesswert) und schuetzen es vor
// der Soll-Vorbelegung im Rezeptstatus.
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

    // Brauschritt (Spalte Schritt); Maischschritt n (0-basiert, Reihenfolge Maischplan) = Maischschritt + n
    enum Schritt
    {
        SchrittAussentemperatur = 1,
        SchrittSchuettung = 2,
        SchrittHauptguss = 3,
        SchrittNachguss = 4,
        SchrittVorderwuerze = 5,
        SchrittKochbeginn = 6,
        SchrittVorHopfenseihen = 7,
        SchrittKochende = 8,
        SchrittAnstellen = 9,
        SchrittAbfuellen = 10,
        SchrittLaeutern = 11,
        SchrittKochen = 12,
        SchrittWhirlpool = 13,
        SchrittKuehlen = 14,
        SchrittMaischschritt = 100
    };
    Q_ENUM(Schritt)

    // Gemessene Groesse (Spalte Groesse)
    enum Groesse
    {
        GroesseVolumen = 1,         // L
        GroesseExtrakt = 2,         // °P
        GroesseTemperatur = 3,      // °C
        GroesseMasse = 4,           // kg
        GroesseDauerAufheizen = 5,  // min
        GroesseDauerRast = 6,       // min
        GroesseDauer = 7            // min (Laeutern, Kochen, Whirlpool, Kuehlen)
    };
    Q_ENUM(Groesse)

    // Messmethode (Spalte Methode)
    enum Methode
    {
        MethodeEingabe = 0,
        MethodeSpindel = 1,
        MethodeRefraktometer = 2,
        MethodeMessstab = 3,
        MethodeWaage = 4,
        MethodeThermometer = 5,
        MethodeUhr = 6,
        MethodeGeschaetzt = 7
    };
    Q_ENUM(Methode)

public:

    ModelMesswerte(Brauhelfer* bh, const QSqlDatabase &db = QSqlDatabase());
    QVariant dataExt(const QModelIndex &index) const Q_DECL_OVERRIDE;
    bool setDataExt(const QModelIndex &index, const QVariant &value) Q_DECL_OVERRIDE;
    void defaultValues(QMap<int, QVariant> &values) const Q_DECL_OVERRIDE;

    // Zeile des Messwerts (nicht geloescht) oder -1
    int zeile(const QVariant &sudId, int schritt, int groesse) const;
    // Messwert oder ungueltiges QVariant, wenn nicht gemessen
    QVariant wert(const QVariant &sudId, int schritt, int groesse) const;
    // Messwert anlegen oder ueberschreiben (Sud-Feld bleibt unberuehrt); liefert die Zeile
    int setze(const QVariant &sudId, int schritt, int groesse, double wert, int methode,
              const QVariant &sollwert, const QString &bemerkung = QString());
    // Messwert entfernen; true, wenn einer vorhanden war
    bool entferne(const QVariant &sudId, int schritt, int groesse);
    // Gibt es fuer dieses Sud-Feld (ModelSud::Col...) einen Messwert?
    bool istGemessen(const QVariant &sudId, int sudSpalte) const;
    // Volumen-/Massewerte eines Suds skalieren (Sud teilen)
    void skaliere(const QVariant &sudId, double faktor);

    // Zuordnung Messwert <-> Sud-Feld; -1 bzw. false, wenn es kein Sud-Feld gibt
    static int sudSpalte(int schritt, int groesse);
    static bool ausSudSpalte(int sudSpalte, int &schritt, int &groesse);
    // Sud-Spalte des Sollwerts (Snapshot) oder -1
    static int sollSpalte(int schritt, int groesse);

private:

    Brauhelfer* bh;
};

#endif // MODELMESSWERTE_H
