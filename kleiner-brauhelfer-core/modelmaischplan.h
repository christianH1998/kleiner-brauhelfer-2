#ifndef MODELMAISCHPLAN_H
#define MODELMAISCHPLAN_H

#include "kleiner-brauhelfer-core_global.h"
#include "sqltablemodel.h"

class Brauhelfer;

class LIB_EXPORT ModelMaischplan : public SqlTableModel
{
    Q_OBJECT

public:

    enum Column
    {
        ColID,
        ColSudID,
        ColTyp,
        ColName,
        ColTempWasser,
        ColAnteilWasser,
        ColTempMalz,
        ColAnteilMalz,
        ColTempRast,
        ColDauerRast,
        ColAnteilMaische,
        ColTempExtra1,
        ColDauerExtra1,
        ColTempExtra2,
        ColDauerExtra2,
        ColMalze,
        // virtual
        ColDeleted,
        ColMengeWasser,
        ColTotalMengeWasser,
        ColMengeMalz,
        ColTotalMengeMalz,
        ColMengeMaische,
        // number of columns
        NumCols
    };
    Q_ENUM(Column)

public:

    ModelMaischplan(Brauhelfer* bh, const QSqlDatabase &db = QSqlDatabase());
    QVariant dataExt(const QModelIndex &index) const Q_DECL_OVERRIDE;
    bool setDataExt(const QModelIndex &index, const QVariant &value) Q_DECL_OVERRIDE;

    // Malz-Zugabe im Maischplan (ext-s12b): Zeile Art "Zuschuetten" mit den Namen der hier
    // zugegebenen Malze (ColMalze, Zeilenumbruch-getrennt); nicht zugeordnete Malze = Einmaischen
    static QStringList malzListe(const QVariant &value);
    static QString malzText(const QStringList &namen);
    // Kontaktzeit [min] ab Zeile row (Quellzeile) bis Ende Maischplan: Rasten, Dekoktionsschritte
    // und Aufheizzeiten nach Richtwert Brauhelfer::aufheizrate()
    double kontaktzeit(int row) const;
    // Malzname -> Kontaktzeit [min] fuer alle zugeordneten Malze des Suds (erste Zuordnung zaehlt)
    QMap<QString, double> malzZugaben(const QVariant &sudId) const;
    bool hatMalzZugabe(const QVariant &sudId) const;
    // Malzanteil der Zugabe-Zeilen aus der Schuettung nachfuehren und Temperaturen neu rechnen
    void aktualisiereMalzZugabe(const QVariant &sudId);

private:

    void updateSud(const QVariant &sudId);
    void setzeAnteilZugabe(int row, double anteil);
    double anteilMalze(const QVariant &sudId, const QStringList &namen) const;
    double getTotalAnteil(const QVariant &sudId, int rowFrom, int colAnteil) const;
    double getPreviousValueTemperature(int row) const;
    void getPreviousMaischeValues(int row, double& m, double &c, double &T) const;

private slots:

    void onSudDataChanged(const QModelIndex &index);
    void onRowsSwapped(int row1, int row2);

private:

    Brauhelfer* bh;
    bool mUpdating;
};

#endif // MODELMAISCHPLAN_H
