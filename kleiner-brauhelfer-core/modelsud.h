#ifndef MODELSUD_H
#define MODELSUD_H

#include "kleiner-brauhelfer-core_global.h"
#include "sqltablemodel.h"

class Brauhelfer;

class LIB_EXPORT ModelSud : public SqlTableModel
{
    Q_OBJECT

    friend class ProxyModelSud;

public:

    enum Column
    {
        ColID,
        ColSudname,
        ColSudnummer,
        ColKategorie,
        ColAnlage,
        ColMenge,
        ColSW,
        ColhighGravityFaktor,
        ColFaktorHauptguss,
        ColWasserprofil,
        ColRestalkalitaetSoll,
        ColCO2,
        ColIBU,
        ColberechnungsArtHopfen,
        ColKochdauer,
        ColNachisomerisierungszeit,
        ColReifezeit,
        ColKostenWasserStrom,
        ColKommentar,
        ColStatus,
        ColBraudatum,
        ColAbfuelldatum,
        ColErstellt,
        ColGespeichert,
        Colerg_S_Gesamt,
        Colerg_W_Gesamt,
        Colerg_WHauptguss,
        Colerg_WNachguss,
        Colerg_Farbe,
        ColSWKochende,
        ColSWAnstellen,
        ColSchnellgaerprobeAktiv,
        ColSWSchnellgaerprobe,
        ColSWJungbier,
        ColTemperaturJungbier,
        ColWuerzemengeVorHopfenseihen,
        ColWuerzemengeKochende,
        ColWuerzemengeAnstellen,
        ColSpunden,
        ColSpeisemenge,
        ColJungbiermengeAbfuellen,
        Colerg_AbgefuellteBiermenge,
        Colerg_Sudhausausbeute,
        Colerg_EffektiveAusbeute,
        Colerg_Preis,
        Colerg_Alkohol,
        ColAusbeuteIgnorieren,
        ColMerklistenID,
        ColSudhausausbeute,
        ColVerdampfungsrate,
        ColVergaerungsgrad,
        ColWuerzemengeKochbeginn,
        ColSWKochbeginn,
        ColVerschneidungAbfuellen,
        ColTemperaturKarbonisierung,
        ColBemerkungBrauen,
        ColBemerkungAbfuellen,
        ColBemerkungGaerung,
        ColReifungStart,
        ColVerduennungAnstellen,
        ColBemerkungZutatenMaischen,
        ColBemerkungZutatenKochen,
        ColBemerkungZutatenGaerung,
        ColBemerkungMaischplan,
        ColBemerkungWasseraufbereitung,
        ColMengeHefestarter,
        ColSWHefestarter,
        // DB-Version 2010 (ext-s07); 0/leer = Standard (bisheriges Verhalten)
        ColberechnungsArtMalz,
        ColberechnungsArtWasser,
        ColWasserPreset,
        ColFaktorNachguss,
        ColNachisoModell,
        ColAbfuellart,
        ColZuckerProLiter,
        // DB-Version 2011 (ext-s11)
        ColAussentemperatur,
        ColWhirlpoolDeckel,
        // virtual
        ColDeleted,
        ColMengeSollAnstellen,
        ColMengeSollKochende,
        ColMengeSollKochbeginn,
        ColMengeSollVerdampfung,
        ColMengeSollHgf,
        ColSWSollAnstellen,
        ColSWSollKochende,
        ColSWSollKochbeginn,
        ColSWSollKochbeginnMitWz,
        ColSWAnteilMalz,
        ColSWAnteilZusatzMaischen,
        ColSWAnteilZusatzKochen,
        ColSWAnteilZusatzGaerung,
        ColSWAnteilHefestarter,
        ColSWAnteilZutaten,
        ColSRESoll,
        ColAlkoholSoll,
        ColSWIst,
        ColSREErwartet,
        ColSREIst,
        ColMengeIst,
        ColIbuIst,
        ColFarbeIst,
        ColCO2Ist,
        ColSpundungsdruck,
        ColGruenschlauchzeitpunkt,
        ColSpeiseNoetig,
        ColSpeiseAnteil,
        ColZuckerAnteil,
        ColWoche,
        ColReifezeitDelta,
        ColAbfuellenBereitZutaten,
        ColWuerzemengeAnstellenTotal,
        ColVerdampfungsrateIst,
        ColsEVG,
        ColtEVG,
        ColRestalkalitaetWasser,
        ColRestalkalitaetIst,
        ColPhMalz,
        ColPhMaische,
        ColPhMaischeSoll,
        ColBewertungMittel,
        ColAlkoholJungbier,
        ColHefe,
        ColDauerHauptgaerung,
        ColTemperaturHauptgaerung,
        ColWuerzemengeKochbeginnErwartet,
        ColWhirlpoolEndtemperatur,
        ColWhirlpoolWirksameZeit,
        ColSWErwartet,
        ColMengeEffektiv,
        ColMengeEffektivQuelle,
        // number of columns
        NumCols
    };
    Q_ENUM(Column)

    // Herkunft von ColMengeEffektiv (ext-s12)
    enum class MengeQuelle
    {
        Soll = 0,       // nicht gemessen, Sollwert (geschaetzt)
        Gemessen = 1,   // Messwert aus dem Brauprotokoll
        Braudaten = 2   // gebrauter Sud ohne Messwert: Werte aus den Braudaten
    };
    Q_ENUM(MengeQuelle)

public:

    ModelSud(Brauhelfer* bh, const QSqlDatabase &db = QSqlDatabase());
    void createConnections();
    QVariant dataExt(const QModelIndex &index) const Q_DECL_OVERRIDE;
    bool setDataExt(const QModelIndex &index, const QVariant &value) Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    bool removeRows(int row, int count = 1, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
    void defaultValues(QMap<int, QVariant> &values) const Q_DECL_OVERRIDE;
    QMap<int, QVariant> copyValues(int row) const Q_DECL_OVERRIDE;
    QVariant dataSud(const QVariant &sudId, int col);
    QVariant dataAnlage(int row, int col) const;
    void setDataAnlage(int row, int col, const QVariant& value);
    QVariant dataWasser(int row, int col) const;
    void setDataWasser(int row, int col, const QVariant& value);
    void update(int row, int colChanged = -1);
    // Rezepte mit Malz-Zugabe im Maischplan neu rechnen (ext-s12b, nach Aenderung der Richtwerte)
    void updateMalzZugabe();
    // Bierfarbe [EBC] wie in updateFarbe; mitZugabe = false rechnet alle Malze voll (wie eingemaischt)
    double berechneFarbe(int row, bool mitZugabe) const;
    double treberaufnahme(int row) const;
    bool istWhirlpool(int row) const;
    double whirlpoolMinuten(int row, double abMinute, double* temperaturEnde = nullptr) const;
    double topfDurchmesser(int row) const;
    bool istWasserFaktoren(int row) const;
    bool istNachbrauen(int row) const;
    double mengeAusWasser(int row, double schuettung) const;
    static bool wasserPresetFaktoren(int preset, double &faktorHauptguss, double &faktorNachguss);

    // Brauprotokoll (ext-s12)
    bool istGemessen(int row, int col) const;
    QVariant istVorbelegung(int row, int col) const;
    // Messwert setzen (wert gueltig) bzw. entfernen (wert ungueltig) inkl. zugehoerigem Sud-Feld;
    // beim Entfernen faellt das Sud-Feld im Rezeptstatus auf die Soll-Vorbelegung zurueck
    bool setzeMesswert(int row, int schritt, int groesse, const QVariant &wert, int methode = 0,
                       const QString &bemerkung = QString(), const QVariant &sollwert = QVariant());

private slots:

    void onModelReset();
    void onRowChanged(const QModelIndex &index);
    void onOtherModelRowChanged(const QModelIndex &index);
    void onMaischplanRowChanged(const QModelIndex &index);
    void onMaischplanRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onMaischplanRowsRemoved();
    void onAnlageRowChanged(const QModelIndex &index);

private:

    bool setDataExt_impl(const QModelIndex &index, const QVariant &value);
    void updateSwWeitereZutaten(int row);
    void updateWasser(int row);
    void updateFarbe(int row);
    void updateIbuBerechnet(int row);
    double updateSwAusMalzmengen(int row);
    double summeMalzmengen(int row) const;
    void updateMengeAusWasser(int row);
    void skaliereMengen(int row, double mengeAlt, double mengeNeu);
    void updatePreis(int row);
    void removeRowsFrom(SqlTableModel* model, int colId, const QList<int>& sudIds);
    bool setDataVorbelegung(int row, int col, const QVariant &value);

private:

    Brauhelfer* bh;
    bool mUpdating;
    bool mSkipUpdateOnOtherModelChanged;
    QList<QVariant> mMaischplanEntfernt;
    QVector<double> swWzMaischenRecipe;
    QVector<double> swWzKochenRecipe;
    QVector<double> swWzGaerungRecipe;
    QVector<double> swWzGaerungCurrent;
    QVector<double> swWzUnvergaerbarRecipe;
};

#endif // MODELSUD_H
