// clazy:excludeall=skipped-base-method
#include "modelsud.h"
#include "proxymodel.h"
#include "brauhelfer.h"
#include <math.h>
#include <qmath.h>
#include "biercalc.h"

ModelSud::ModelSud(Brauhelfer *bh, const QSqlDatabase &db) :
    SqlTableModel(bh, db),
    bh(bh),
    mUpdating(false),
    mSkipUpdateOnOtherModelChanged(false),
    swWzMaischenRecipe(QVector<double>()),
    swWzKochenRecipe(QVector<double>()),
    swWzGaerungRecipe(QVector<double>()),
    swWzGaerungCurrent(QVector<double>()),
    swWzUnvergaerbarRecipe(QVector<double>())
{
    mVirtualField.append(QStringLiteral("MengeSollAnstellen"));
    mVirtualField.append(QStringLiteral("MengeSollKochende"));
    mVirtualField.append(QStringLiteral("MengeSollKochbeginn"));
    mVirtualField.append(QStringLiteral("MengeSollVerdampfung"));
    mVirtualField.append(QStringLiteral("MengeSollHgf"));
    mVirtualField.append(QStringLiteral("SWSollAnstellen"));
    mVirtualField.append(QStringLiteral("SWSollKochende"));
    mVirtualField.append(QStringLiteral("SWSollKochbeginn"));
    mVirtualField.append(QStringLiteral("SWSollKochbeginnMitWz"));
    mVirtualField.append(QStringLiteral("SWAnteilMalz"));
    mVirtualField.append(QStringLiteral("SWAnteilZusatzMaischen"));
    mVirtualField.append(QStringLiteral("SWAnteilZusatzKochen"));
    mVirtualField.append(QStringLiteral("SWAnteilZusatzGaerung"));
    mVirtualField.append(QStringLiteral("SWAnteilHefestarter"));
    mVirtualField.append(QStringLiteral("SWAnteilZutaten"));
    mVirtualField.append(QStringLiteral("SRESoll"));
    mVirtualField.append(QStringLiteral("AlkoholSoll"));
    mVirtualField.append(QStringLiteral("SWIst"));
    mVirtualField.append(QStringLiteral("SREErwartet"));
    mVirtualField.append(QStringLiteral("SREIst"));
    mVirtualField.append(QStringLiteral("MengeIst"));
    mVirtualField.append(QStringLiteral("IbuIst"));
    mVirtualField.append(QStringLiteral("FarbeIst"));
    mVirtualField.append(QStringLiteral("CO2Ist"));
    mVirtualField.append(QStringLiteral("Spundungsdruck"));
    mVirtualField.append(QStringLiteral("Gruenschlauchzeitpunkt"));
    mVirtualField.append(QStringLiteral("SpeiseNoetig"));
    mVirtualField.append(QStringLiteral("SpeiseAnteil"));
    mVirtualField.append(QStringLiteral("ZuckerAnteil"));
    mVirtualField.append(QStringLiteral("Woche"));
    mVirtualField.append(QStringLiteral("ReifezeitDelta"));
    mVirtualField.append(QStringLiteral("AbfuellenBereitZutaten"));
    mVirtualField.append(QStringLiteral("WuerzemengeAnstellenTotal"));
    mVirtualField.append(QStringLiteral("VerdampfungsrateIst"));
    mVirtualField.append(QStringLiteral("sEVG"));
    mVirtualField.append(QStringLiteral("tEVG"));
    mVirtualField.append(QStringLiteral("RestalkalitaetWasser"));
    mVirtualField.append(QStringLiteral("RestalkalitaetIst"));
    mVirtualField.append(QStringLiteral("PhMalz"));
    mVirtualField.append(QStringLiteral("PhMaische"));
    mVirtualField.append(QStringLiteral("PhMaischeSoll"));
    mVirtualField.append(QStringLiteral("BewertungMittel"));
    mVirtualField.append(QStringLiteral("AlkoholJungbier"));
    mVirtualField.append(QStringLiteral("Hefe"));
    mVirtualField.append(QStringLiteral("DauerHauptgaerung"));
    mVirtualField.append(QStringLiteral("TemperaturHauptgaerung"));
    mVirtualField.append(QStringLiteral("WuerzemengeKochbeginnErwartet"));
    mVirtualField.append(QStringLiteral("WhirlpoolEndtemperatur"));
    mVirtualField.append(QStringLiteral("WhirlpoolWirksameZeit"));
    mVirtualField.append(QStringLiteral("SWErwartet"));
    mVirtualField.append(QStringLiteral("MengeEffektiv"));
    mVirtualField.append(QStringLiteral("MengeEffektivQuelle"));
    mVirtualField.append(QStringLiteral("AbfuellartEffektiv"));
    mVirtualField.append(QStringLiteral("ZuckerProLiterVorschlag"));
    connect(this, &SqlTableModel::modelReset, this, &ModelSud::onModelReset);
    connect(this, &SqlTableModel::rowsInserted, this, &ModelSud::onModelReset);
    connect(this, &SqlTableModel::rowsRemoved, this, &ModelSud::onModelReset);
    connect(this, &SqlTableModel::rowChanged, this, &ModelSud::onRowChanged);
}

void ModelSud::createConnections()
{
    connect(bh->modelMalzschuettung(), &SqlTableModel::rowChanged, this, &ModelSud::onOtherModelRowChanged);
    connect(bh->modelHopfengaben(), &SqlTableModel::rowChanged, this, &ModelSud::onOtherModelRowChanged);
    connect(bh->modelHefegaben(), &SqlTableModel::rowChanged, this, &ModelSud::onOtherModelRowChanged);
    connect(bh->modelWeitereZutatenGaben(), &SqlTableModel::rowChanged, this, &ModelSud::onOtherModelRowChanged);
    connect(bh->modelAusruestung(), &SqlTableModel::rowChanged, this, &ModelSud::onAnlageRowChanged);
    connect(bh->modelMaischplan(), &SqlTableModel::rowChanged, this, &ModelSud::onMaischplanRowChanged);
    connect(bh->modelMaischplan(), &SqlTableModel::rowsAboutToBeRemoved, this, &ModelSud::onMaischplanRowsAboutToBeRemoved);
    connect(bh->modelMaischplan(), &SqlTableModel::rowsRemoved, this, &ModelSud::onMaischplanRowsRemoved);
}

void ModelSud::onModelReset()
{
    int rows = rowCount();
    swWzMaischenRecipe = QVector<double>(rows);
    swWzKochenRecipe = QVector<double>(rows);
    swWzGaerungRecipe = QVector<double>(rows);
    swWzGaerungCurrent = QVector<double>(rows);
    swWzUnvergaerbarRecipe = QVector<double>(rows);
    for (int row = 0; row < rows; ++row)
        updateSwWeitereZutaten(row);
    emit modified();
}

void ModelSud::onRowChanged(const QModelIndex &idx)
{
    update(idx.row());
}

void ModelSud::onOtherModelRowChanged(const QModelIndex &idx)
{
    if (mSkipUpdateOnOtherModelChanged)
        return;
    const SqlTableModel* model = static_cast<const SqlTableModel*>(idx.model());
    int row = getRowWithValue(ColID, model->data(idx.row(), model->fieldIndex(QStringLiteral("SudID"))));
    update(row);
}

void ModelSud::onMaischplanRowChanged(const QModelIndex &idx)
{
    // Farbmalz-Zugabe (ext-s12b): Kontaktzeit haengt am Maischplan. Ohne Zuordnung keine
    // Neuberechnung (Verhalten wie bisher).
    if (mSkipUpdateOnOtherModelChanged || mUpdating)
        return;
    ModelMaischplan* model = bh->modelMaischplan();
    QVariant sudId = model->data(idx.row(), ModelMaischplan::ColSudID);
    if (idx.column() != ModelMaischplan::ColMalze && !model->hatMalzZugabe(sudId) &&
        ModelMaischplan::malzListe(model->data(idx.row(), ModelMaischplan::ColMalze)).isEmpty())
        return;
    int row = getRowWithValue(ColID, sudId);
    if (row >= 0 && static_cast<Brauhelfer::SudStatus>(data(row, ColStatus).toInt()) == Brauhelfer::SudStatus::Rezept)
        update(row);
}

void ModelSud::onMaischplanRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    // Noch nicht gespeicherte Zeilen entfernt Qt sofort (ohne rowChanged): betroffene Sude merken
    Q_UNUSED(parent)
    ModelMaischplan* model = bh->modelMaischplan();
    for (int r = first; r <= last; ++r)
    {
        QVariant sudId = model->data(r, ModelMaischplan::ColSudID);
        if (!mMaischplanEntfernt.contains(sudId) &&
            (!ModelMaischplan::malzListe(model->data(r, ModelMaischplan::ColMalze)).isEmpty() || model->hatMalzZugabe(sudId)))
            mMaischplanEntfernt.append(sudId);
    }
}

void ModelSud::onMaischplanRowsRemoved()
{
    const QList<QVariant> sudIds = mMaischplanEntfernt;
    mMaischplanEntfernt.clear();
    if (mSkipUpdateOnOtherModelChanged)
        return;
    for (const QVariant &sudId : sudIds)
    {
        int row = getRowWithValue(ColID, sudId);
        if (row >= 0 && static_cast<Brauhelfer::SudStatus>(data(row, ColStatus).toInt()) == Brauhelfer::SudStatus::Rezept)
            update(row);
    }
}

void ModelSud::updateMalzZugabe()
{
    for (int row = 0; row < rowCount(); ++row)
    {
        if (static_cast<Brauhelfer::SudStatus>(data(row, ColStatus).toInt()) != Brauhelfer::SudStatus::Rezept)
            continue;
        if (bh->modelMaischplan()->hatMalzZugabe(data(row, ColID)))
            update(row);
    }
}

void ModelSud::onAnlageRowChanged(const QModelIndex &idx)
{
    const QList<int> updateList = {ModelAusruestung::ColTyp,
                                   ModelAusruestung::ColKorrekturWasser,
                                   ModelAusruestung::ColKorrekturFarbe,
                                   ModelAusruestung::ColKorrekturMenge,
                                   ModelAusruestung::ColKosten,
                                   ModelAusruestung::ColTreberaufnahme,
                                   ModelAusruestung::ColSudpfanne_Durchmesser,
                                   ModelAusruestung::ColWhirlpoolFaktor};
    if (!updateList.contains(idx.column()))
        return;
    QVariant name = bh->modelAusruestung()->data(idx.row(), ModelAusruestung::ColName);
    for (int row = 0; row < rowCount(); ++row)
    {
        if (data(row, ColAnlage) == name)
        {
            if (idx.column() == ModelAusruestung::ColKorrekturMenge)
                update(row, ColMengeSollAnstellen);
            else
                update(row);
        }
    }
}

QVariant ModelSud::dataExt(const QModelIndex &idx) const
{
    switch(idx.column())
    {
    case ColBraudatum:
    {
        return QDateTime::fromString(QSqlTableModel::data(idx).toString(), Qt::ISODate);
    }
    case ColAbfuelldatum:
    {
        return QDateTime::fromString(QSqlTableModel::data(idx).toString(), Qt::ISODate);
    }
    case ColErstellt:
    {
        return QDateTime::fromString(QSqlTableModel::data(idx).toString(), Qt::ISODate);
    }
    case ColGespeichert:
    {
        return QDateTime::fromString(QSqlTableModel::data(idx).toString(), Qt::ISODate);
    }
    case ColReifungStart:
    {
        return QDateTime::fromString(QSqlTableModel::data(idx).toString(), Qt::ISODate);
    }
    case ColMengeSollAnstellen:
    {
        double menge = data(idx.row(), ColMenge).toDouble();
        double mengeKorrektur = dataAnlage(idx.row(), ModelAusruestung::ColKorrekturMenge).toDouble();
        return menge + mengeKorrektur;
    }
    case ColMengeSollKochende:
    {
        double mengeSoll = data(idx.row(), ColMengeSollAnstellen).toDouble();
        double mengeHgf = data(idx.row(), ColMengeSollHgf).toDouble();
        double mengeHefestarter = data(idx.row(), ColMengeHefestarter).toDouble();
        return mengeSoll - mengeHgf - mengeHefestarter;
    }
    case ColMengeSollKochbeginn:
    {
        double mengeKochende = data(idx.row(), ColMengeSollKochende).toDouble();
        double mengeVerdampfung = data(idx.row(), ColMengeSollVerdampfung).toDouble();
        return mengeKochende + mengeVerdampfung;
    }
    case ColMengeSollVerdampfung:
    {
        double kochdauer = data(idx.row(), ColKochdauer).toDouble();
        double verdampfungsrate = data(idx.row(), ColVerdampfungsrate).toDouble();
        return verdampfungsrate * kochdauer / 60;
    }
    case ColMengeSollHgf:
    {
        double mengeSoll = data(idx.row(), ColMengeSollAnstellen).toDouble();
        double mengeHefestarter = data(idx.row(), ColMengeHefestarter).toDouble();
        double hgf = data(idx.row(), ColhighGravityFaktor).toInt();
        return (mengeSoll-mengeHefestarter) * hgf/(100+hgf);
    }
    case ColSWSollAnstellen:
    {
        double sw = data(idx.row(), ColSW).toDouble();
        return sw - swWzGaerungRecipe[idx.row()];
    }
    case ColSWSollKochende:
    {
        double swAnstellen = data(idx.row(), ColSWSollAnstellen).toDouble();
        double mengeAnstellen = data(idx.row(), ColMengeSollAnstellen).toDouble();
        double swHefestarter = data(idx.row(), ColSWHefestarter).toDouble();
        double mengeHefestarter = data(idx.row(), ColMengeHefestarter).toDouble();
        double hgf = data(idx.row(), ColhighGravityFaktor).toInt();
        return (swAnstellen*mengeAnstellen-swHefestarter*mengeHefestarter)/(mengeAnstellen-mengeHefestarter)*(1+hgf/100);
    }
    case ColSWSollKochbeginn:
    {
        double mengeKochende = data(idx.row(), ColMengeSollKochende).toDouble();
        double mengeVerdampfung = data(idx.row(), ColMengeSollVerdampfung).toDouble();
        double swKochende = data(idx.row(), ColSWSollKochende).toDouble() - swWzKochenRecipe[idx.row()];
        return swKochende*mengeKochende/(mengeKochende+mengeVerdampfung);
    }
    case ColSWSollKochbeginnMitWz:
    {
        double mengeKochende = data(idx.row(), ColMengeSollKochende).toDouble();
        double mengeVerdampfung = data(idx.row(), ColMengeSollVerdampfung).toDouble();
        double swKochende = data(idx.row(), ColSWSollKochende).toDouble();
        return swKochende*mengeKochende/(mengeKochende+mengeVerdampfung);
    }
    case ColSWAnteilMalz:
    {
        double sw = data(idx.row(), ColSW).toDouble();
        double anteilZusatzMaischen = swWzMaischenRecipe[idx.row()];
        double anteilZusatzKochen = swWzKochenRecipe[idx.row()];
        double anteilZusatzGaerung = swWzGaerungRecipe[idx.row()];
        double anteilHefestarter = data(idx.row(), ColSWAnteilHefestarter).toDouble();
        return sw - anteilZusatzMaischen - anteilZusatzKochen - anteilZusatzGaerung - anteilHefestarter;
    }
    case ColSWAnteilZusatzMaischen:
    {
        return swWzMaischenRecipe[idx.row()];
    }
    case ColSWAnteilZusatzKochen:
    {
        return swWzKochenRecipe[idx.row()];
    }
    case ColSWAnteilZusatzGaerung:
    {
        return swWzGaerungRecipe[idx.row()];
    }
    case ColSWAnteilHefestarter:
    {
        double mengeHefestarter = data(idx.row(), ColMengeHefestarter).toDouble();
        if (mengeHefestarter > 0)
        {
            double swHefestarter = data(idx.row(), ColSWHefestarter).toDouble();
            double mengeAnstellen = data(idx.row(), ColMengeSollAnstellen).toDouble();
            return swHefestarter*mengeHefestarter/mengeAnstellen;
        }
        return 0;
    }
    case ColSWAnteilZutaten:
    {
        double sw = data(idx.row(), ColSW).toDouble();
        double swAnteilHefestarter = data(idx.row(), ColSWAnteilHefestarter).toDouble();
        return sw - swAnteilHefestarter;
    }
    case ColSRESoll:
    {
        double sw = data(idx.row(), ColSW).toDouble() - swWzUnvergaerbarRecipe[idx.row()];
        double vg = data(idx.row(), ColVergaerungsgrad).toDouble();
        return BierCalc::sreAusVergaerungsgrad(sw, vg) + swWzUnvergaerbarRecipe[idx.row()];
    }
    case ColAlkoholSoll:
    {
        double sw = data(idx.row(), ColSW).toDouble();
        double sre = data(idx.row(), ColSRESoll).toDouble();
        return BierCalc::alkohol(sw, sre);
    }
    case ColSWIst:
    {
        return data(idx.row(), ColSWAnstellen).toDouble() + swWzGaerungCurrent[idx.row()];
    }
    case ColHefe:
    {
        // Namen der Hefegaben (ohne Doppelte), z. B. fuer Auswertungen je Hefe (ext-s05)
        ProxyModel modelHefegaben;
        modelHefegaben.setSourceModel(bh->modelHefegaben());
        modelHefegaben.setFilterKeyColumn(ModelHefegaben::ColSudID);
        modelHefegaben.setFilterRegularExpression(QStringLiteral("^%1$").arg(data(idx.row(), ColID).toInt()));
        QStringList namen;
        for (int r = 0; r < modelHefegaben.rowCount(); ++r)
        {
            QString name = modelHefegaben.data(r, ModelHefegaben::ColName).toString();
            if (!name.isEmpty() && !namen.contains(name))
                namen.append(name);
        }
        return namen.join(QStringLiteral(", "));
    }
    case ColDauerHauptgaerung:
    {
        // Tage von Braudatum bis Abfuelldatum (0 = nicht bekannt) (ext-s05)
        QDateTime braudatum = data(idx.row(), ColBraudatum).toDateTime();
        QDateTime abfuelldatum = data(idx.row(), ColAbfuelldatum).toDateTime();
        if (!braudatum.isValid() || !abfuelldatum.isValid())
            return 0;
        qint64 tage = braudatum.daysTo(abfuelldatum);
        return tage > 0 ? static_cast<int>(tage) : 0;
    }
    case ColTemperaturHauptgaerung:
    {
        // Mittlere Temperatur aller Messungen im Hauptgaerverlauf (0 = keine Messung) (ext-s05)
        ProxyModel modelHauptgaerverlauf;
        modelHauptgaerverlauf.setSourceModel(bh->modelHauptgaerverlauf());
        modelHauptgaerverlauf.setFilterKeyColumn(ModelHauptgaerverlauf::ColSudID);
        modelHauptgaerverlauf.setFilterRegularExpression(QStringLiteral("^%1$").arg(data(idx.row(), ColID).toInt()));
        int n = modelHauptgaerverlauf.rowCount();
        if (n == 0)
            return 0.0;
        double summe = 0.0;
        for (int r = 0; r < n; ++r)
            summe += modelHauptgaerverlauf.data(r, ModelHauptgaerverlauf::ColTemp).toDouble();
        return summe / n;
    }
    case ColAussentemperatur:
    {
        // leer (Altsude, neue Zeilen) = 20 °C (ext-s11)
        QVariant v = QSqlTableModel::data(idx);
        return v.isNull() ? QVariant(20.0) : v;
    }
    case ColMengeEffektiv:
    case ColMengeEffektivQuelle:
    {
        // Effektive Biermenge (ext-s12): letzter gemessener Volumenwert (Abfuellen > Anstellen >
        // Kochende), sonst bei gebrauten Suden die Braudaten (MengeIst), sonst Soll (geschaetzt).
        const int row = idx.row();
        const QVariant sudId = data(row, ColID);
        ModelMesswerte *mw = bh->modelMesswerte();
        double menge = 0.0;
        MengeQuelle quelle = MengeQuelle::Soll;
        const int schritte[] = {ModelMesswerte::SchrittAbfuellen, ModelMesswerte::SchrittAnstellen, ModelMesswerte::SchrittKochende};
        bool gefunden = false;
        for (int schritt : schritte)
        {
            QVariant v = mw->wert(sudId, schritt, ModelMesswerte::GroesseVolumen);
            if (v.isValid())
            {
                menge = v.toDouble();
                quelle = MengeQuelle::Gemessen;
                gefunden = true;
                break;
            }
        }
        if (!gefunden)
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(row, ColStatus).toInt());
            menge = data(row, ColMengeIst).toDouble();
            quelle = status == Brauhelfer::SudStatus::Rezept ? MengeQuelle::Soll : MengeQuelle::Braudaten;
        }
        if (idx.column() == ColMengeEffektiv)
            return menge;
        return static_cast<int>(quelle);
    }
    case ColSWErwartet:
    {
        // Stammwuerze, die sich mit dem Wasser aus den Faktoren ergibt (ext-s11b): gleiche Kette
        // wie "Rezept nachbrauen" (Menge aus Schuettung + Faktoren, SW aus Schuettung, Menge und
        // Sudhausausbeute). Extrakt der Zutaten/Hefestarter verduennt sich mit der Menge.
        // Ausser bei Wasser "Faktoren" gleich der Stammwuerze des Rezepts.
        const int row = idx.row();
        double sw = data(row, ColSW).toDouble();
        Brauhelfer::AnlageTyp anlageTyp = static_cast<Brauhelfer::AnlageTyp>(dataAnlage(row, ModelAusruestung::ColTyp).toInt());
        if (!istWasserFaktoren(row) || anlageTyp == Brauhelfer::AnlageTyp::GrainfatherG30 || anlageTyp == Brauhelfer::AnlageTyp::BrauheldPro30)
            return sw;
        double schuet = data(row, Colerg_S_Gesamt).toDouble();
        double msa = data(row, ColMengeSollAnstellen).toDouble();
        double msaErw = mengeAusWasser(row, schuet) + dataAnlage(row, ModelAusruestung::ColKorrekturMenge).toDouble();
        double ausb = data(row, ColSudhausausbeute).toDouble();
        if (schuet <= 0.0 || msa <= 0.0 || msaErw <= 0.0 || ausb <= 0.0 || row >= swWzMaischenRecipe.size())
            return sw;
        double k = msa / msaErw;
        double wzMaischen = swWzMaischenRecipe[row] * k;
        double wzKochen = swWzKochenRecipe[row] * k;
        double wzGaerung = swWzGaerungRecipe[row] * k;
        double swMalz = BierCalc::swAusSchuettung(schuet, wzMaischen + wzKochen, msaErw, ausb);
        double swErw = swMalz + wzMaischen + wzKochen + wzGaerung + data(row, ColSWAnteilHefestarter).toDouble() * k;
        return std::isfinite(swErw) ? swErw : sw;
    }
    case ColWhirlpoolEndtemperatur:
    {
        // Temperatur am Ende der Whirlpool-/Absetzzeit (ext-s11), nur Modell Whirlpool
        if (!istWhirlpool(idx.row()))
            return QVariant();
        double tEnde = 100.0;
        whirlpoolMinuten(idx.row(), 0.0, &tEnde);
        return tEnde;
    }
    case ColWhirlpoolWirksameZeit:
    {
        // Wirksame Kochminuten der Absetzzeit fuer Gaben bis Kochende (ext-s11):
        // Modell Whirlpool = Integral der Isomerisierungsrate, sonst 0,549 * Nachisomerisierungszeit
        if (istWhirlpool(idx.row()))
            return whirlpoolMinuten(idx.row(), 0.0);
        return data(idx.row(), ColNachisomerisierungszeit).toDouble() * BierCalc::isomerisierungsrate(80.0);
    }
    case ColWuerzemengeKochbeginnErwartet:
    {
        // Wuerzemenge zu Kochbeginn, die sich aus Haupt- und Nachguss ergibt (ext-s10):
        // HG + NG - Treberaufnahme * Schuettung - Korrektur Nachguss der Anlage.
        // Im Modus "Nachguss folgt Wuerzemenge" gleich der Sollmenge (ausser NG < 0 gekappt).
        Brauhelfer::AnlageTyp anlageTyp = static_cast<Brauhelfer::AnlageTyp>(dataAnlage(idx.row(), ModelAusruestung::ColTyp).toInt());
        if (anlageTyp == Brauhelfer::AnlageTyp::GrainfatherG30 || anlageTyp == Brauhelfer::AnlageTyp::BrauheldPro30)
            return data(idx.row(), ColMengeSollKochbeginn);
        double schuet = data(idx.row(), Colerg_S_Gesamt).toDouble();
        double hg = data(idx.row(), Colerg_WHauptguss).toDouble();
        double ng = data(idx.row(), Colerg_WNachguss).toDouble();
        double korrektur = dataAnlage(idx.row(), ModelAusruestung::ColKorrekturWasser).toDouble();
        return hg + ng - schuet * treberaufnahme(idx.row()) - korrektur;
    }
    case ColAlkoholJungbier:
    {
        // Alkohol des Jungbiers beim Abfüllen (gemessener Restextrakt Jungbier), ohne
        // Nachgärung. Erwarteter Wert des fertigen Biers = erg_Alkohol (ext-s03).
        double sw = data(idx.row(), ColSWIst).toDouble();
        double sre = data(idx.row(), ColSWJungbier).toDouble();
        return BierCalc::alkohol(sw, sre);
    }
    case ColSREErwartet:
    {
        double sw = data(idx.row(), ColSWIst).toDouble() - swWzUnvergaerbarRecipe[idx.row()];
        double vg = data(idx.row(), ColVergaerungsgrad).toDouble();
        return BierCalc::sreAusVergaerungsgrad(sw, vg) + swWzUnvergaerbarRecipe[idx.row()];
    }
    case ColSREIst:
    {
        if (data(idx.row(), ColSchnellgaerprobeAktiv).toBool())
            return data(idx.row(), ColSWSchnellgaerprobe).toDouble();
        else
            return data(idx.row(), ColSWJungbier).toDouble();
    }
    case ColMengeIst:
    {
        Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
        switch (status)
        {
        case Brauhelfer::SudStatus::Rezept:
            return data(idx.row(), ColMenge).toDouble();
        case Brauhelfer::SudStatus::Gebraut:
            return data(idx.row(), ColWuerzemengeAnstellen).toDouble();
        case Brauhelfer::SudStatus::Abgefuellt:
        case Brauhelfer::SudStatus::Verbraucht:
            return data(idx.row(), Colerg_AbgefuellteBiermenge).toDouble();
        }
        return 0;
    }
    case ColIbuIst:
    {
        Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
        if (status == Brauhelfer::SudStatus::Rezept)
            return data(idx.row(), ColIBU).toDouble();
        double mengeIst = data(idx.row(), ColWuerzemengeAnstellenTotal).toDouble();
        if (mengeIst <= 0.0)
            return 0.0;
        double mengeFaktor = data(idx.row(), ColMenge).toDouble() / mengeIst;
        return data(idx.row(), ColIBU).toDouble() * mengeFaktor;
    }
    case ColFarbeIst:
    {
        Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
        if (status == Brauhelfer::SudStatus::Rezept)
            return data(idx.row(), Colerg_Farbe).toDouble();
        double mengeIst = data(idx.row(), ColWuerzemengeAnstellenTotal).toDouble();
        if (mengeIst <= 0.0)
            return 0.0;
        double mengeFaktor = data(idx.row(), ColMenge).toDouble() / mengeIst;
        return data(idx.row(), Colerg_Farbe).toDouble() * mengeFaktor;
    }
    case ColCO2Ist:
    {
        return bh->modelNachgaerverlauf()->getLastCO2(data(idx.row(), ColID));
    }
    case ColSpundungsdruck:
    {
        double co2 = data(idx.row(), ColCO2).toDouble();
        double T = data(idx.row(), ColTemperaturJungbier).toDouble();
        return BierCalc::spundungsdruck(co2, T);
    }
    case ColGruenschlauchzeitpunkt:
    {
        double co2Soll = data(idx.row(), ColCO2).toDouble();
        double sw = data(idx.row(), ColSWIst).toDouble();
        double T = data(idx.row(), ColTemperaturJungbier).toDouble();
        double sre = data(idx.row(), ColSWSchnellgaerprobe).toDouble();
        return BierCalc::gruenschlauchzeitpunkt(co2Soll, sw, sre, T);
    }
    case ColSpeiseNoetig:
    {
        double co2Soll = data(idx.row(), ColCO2).toDouble();
        double sw = data(idx.row(), ColSWIst).toDouble();
        double sreJungbier = data(idx.row(), ColSWJungbier).toDouble();
        double T = data(idx.row(), ColTemperaturKarbonisierung).toDouble();
        double sreSchnellgaerprobe = data(idx.row(), ColSREIst).toDouble();
        double jungbiermenge = data(idx.row(), ColJungbiermengeAbfuellen).toDouble();
        return BierCalc::speise(co2Soll, sw, sreSchnellgaerprobe, sreJungbier, T) * jungbiermenge * 1000;
    }
    case ColAbfuellartEffektiv:
    {
        // Gewaehlte Abfuellart (ext-s13); ohne Auswahl wie bisher aus Spunden/Speisemenge abgeleitet
        int art = data(idx.row(), ColAbfuellart).toInt();
        if (art > static_cast<int>(Brauhelfer::Abfuellart::Abgeleitet) && art <= static_cast<int>(Brauhelfer::Abfuellart::SpundenZucker))
            return art;
        if (data(idx.row(), ColSpunden).toBool())
            return static_cast<int>(Brauhelfer::Abfuellart::Spunden);
        if (data(idx.row(), ColSpeisemenge).toDouble() > 0.0)
            return static_cast<int>(Brauhelfer::Abfuellart::Speise);
        return static_cast<int>(Brauhelfer::Abfuellart::ZuckerGesamt);
    }
    case ColZuckerProLiterVorschlag:
    {
        // Rechner (ext-s06/s13): benoetigter Zucker [g/L Jungbier] fuer das Ziel-CO2, ohne Speise
        double co2Soll = data(idx.row(), ColCO2).toDouble();
        double sw = data(idx.row(), ColSWIst).toDouble();
        double sreJungbier = data(idx.row(), ColSWJungbier).toDouble();
        double T = data(idx.row(), ColTemperaturKarbonisierung).toDouble();
        double sreSchnellgaerprobe = data(idx.row(), ColSREIst).toDouble();
        double zucker = BierCalc::zucker(co2Soll, sw, sreSchnellgaerprobe, sreJungbier, T);
        if (!std::isfinite(zucker) || zucker < 0.0)
            zucker = 0.0;
        return zucker;
    }
    case ColSpeiseAnteil:
    {
        int art = data(idx.row(), ColAbfuellart).toInt();
        if (art != static_cast<int>(Brauhelfer::Abfuellart::Abgeleitet) && art != static_cast<int>(Brauhelfer::Abfuellart::Speise))
            return 0.0; // ext-s13: Speise nur bei Abfuellart "Speise"
        if (data(idx.row(), ColSpunden).toBool())
            return 0.0;
        double speiseVerfuegbar = data(idx.row(), ColSpeisemenge).toDouble() * 1000;
        double speiseNoetig = data(idx.row(), ColSpeiseNoetig).toDouble();
        if (speiseNoetig == std::numeric_limits<double>::infinity())
            return speiseVerfuegbar;
        if (speiseNoetig > speiseVerfuegbar)
            speiseNoetig = speiseVerfuegbar;
        return speiseNoetig;
    }
    case ColZuckerAnteil:
    {
        // ext-s13: je gewaehlter Abfuellart; ohne Auswahl (0) wie bisher
        switch (static_cast<Brauhelfer::Abfuellart>(data(idx.row(), ColAbfuellart).toInt()))
        {
        case Brauhelfer::Abfuellart::Spunden:
        case Brauhelfer::Abfuellart::Zwangskarbonisierung:
            return 0.0;
        case Brauhelfer::Abfuellart::ZuckerGesamt:
            return data(idx.row(), ColZuckerProLiterVorschlag).toDouble() * data(idx.row(), ColJungbiermengeAbfuellen).toDouble();
        case Brauhelfer::Abfuellart::SpundenZucker:
        {
            // Zucker fuer das CO2, das der verbleibende Restextrakt nicht mehr liefert. Ende der
            // Gaerung = Schnellgaerprobe, sonst Schaetzung aus dem Vergaerungsgrad; Temperatur Jungbier.
            double sreEnde = data(idx.row(), ColSchnellgaerprobeAktiv).toBool() ? data(idx.row(), ColSWSchnellgaerprobe).toDouble() : data(idx.row(), ColSREErwartet).toDouble();
            const double co2 = data(idx.row(), ColCO2).toDouble();
            const double sw = data(idx.row(), ColSWIst).toDouble();
            const double sre = data(idx.row(), ColSWJungbier).toDouble();
            const double temp = data(idx.row(), ColTemperaturJungbier).toDouble();
            // Kein Zucker, solange der Restextrakt (auf 0,1 °P wie angezeigt) den Sollwert erreicht
            const double soll = BierCalc::gruenschlauchzeitpunkt(co2, sw, sreEnde, temp);
            if (std::round(sre * 10.0) >= std::round(soll * 10.0))
                return 0.0;
            double zucker = BierCalc::zucker(co2, sw, sreEnde, sre, temp);
            if (!std::isfinite(zucker) || zucker < 0.0)
                zucker = 0.0;
            return zucker * data(idx.row(), ColJungbiermengeAbfuellen).toDouble();
        }
        case Brauhelfer::Abfuellart::ZuckerFlasche:
        {
            QVariant fest = data(idx.row(), ColZuckerProLiter);
            double proLiter = fest.isNull() ? data(idx.row(), ColZuckerProLiterVorschlag).toDouble() : fest.toDouble();
            return proLiter * data(idx.row(), ColJungbiermengeAbfuellen).toDouble();
        }
        default:
            break;
        }
        if (data(idx.row(), ColSpunden).toBool())
            return 0.0;
        double co2Soll = data(idx.row(), ColCO2).toDouble();
        double sw = data(idx.row(), ColSWIst).toDouble();
        double sreJungbier = data(idx.row(), ColSWJungbier).toDouble();
        double T = data(idx.row(), ColTemperaturKarbonisierung).toDouble();
        double sreSchnellgaerprobe = data(idx.row(), ColSREIst).toDouble();
        double jungbiermenge = data(idx.row(), ColJungbiermengeAbfuellen).toDouble();
        double zucker = BierCalc::zucker(co2Soll, sw, sreSchnellgaerprobe, sreJungbier, T) * jungbiermenge;
        double speiseNoetig = data(idx.row(), ColSpeiseNoetig).toDouble() / 1000;
        double speiseVerfuegbar = data(idx.row(), ColSpeisemenge).toDouble();
        if (speiseNoetig == std::numeric_limits<double>::infinity())
            return zucker;
        if (speiseNoetig <= speiseVerfuegbar)
            return 0;
        double potSpeise = BierCalc::co2Vergaerung(sw, sreSchnellgaerprobe);
        double potZucker = BierCalc::co2Zucker();
        return zucker - speiseVerfuegbar * potSpeise / potZucker;
    }
    case ColWoche:
    {
        Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
        if (status >= Brauhelfer::SudStatus::Abgefuellt)
        {
            QDateTime dt = data(idx.row(), ColReifungStart).toDateTime();
            if (!dt.isValid())
                dt = data(idx.row(), ColAbfuelldatum).toDateTime();
            if (dt.isValid())
                return dt.daysTo(QDateTime::currentDateTime()) / 7 + 1;
        }
        return 0;
    }
    case ColReifezeitDelta:
    {
        Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
        if (status >= Brauhelfer::SudStatus::Abgefuellt)
        {
            QDateTime dt = data(idx.row(), ColReifungStart).toDateTime();
            if (!dt.isValid())
                dt = data(idx.row(), ColAbfuelldatum).toDateTime();
            if (dt.isValid())
            {
                int tageReifung = dt.daysTo(QDateTime::currentDateTime());
                int tageReifungSoll = data(idx.row(), ColReifezeit).toInt() * 7;
                return tageReifungSoll - tageReifung;
            }
        }
        return 0;
    }
    case ColAbfuellenBereitZutaten:
    {
        ProxyModel modelHefegaben;
        modelHefegaben.setSourceModel(bh->modelHefegaben());
        modelHefegaben.setFilterKeyColumn(bh->modelHefegaben()->ColSudID);
        modelHefegaben.setFilterRegularExpression(QStringLiteral("^%1$").arg(data(idx.row(), ColID).toInt()));
        for (int r = 0; r < modelHefegaben.rowCount(); ++r)
        {
            if (!modelHefegaben.data(r, ModelHefegaben::ColAbfuellbereit).toBool())
                return false;
        }
        ProxyModel modelWeitereZutatenGaben;
        modelWeitereZutatenGaben.setSourceModel(bh->modelWeitereZutatenGaben());
        modelWeitereZutatenGaben.setFilterKeyColumn(bh->modelWeitereZutatenGaben()->ColSudID);
        modelWeitereZutatenGaben.setFilterRegularExpression(QStringLiteral("^%1$").arg(data(idx.row(), ColID).toInt()));
        for (int r = 0; r < modelWeitereZutatenGaben.rowCount(); ++r)
        {
            if (!modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::ColAbfuellbereit).toBool())
                return false;
        }
        return true;
    }
    case ColWuerzemengeAnstellenTotal:
    {
        return data(idx.row(), ColWuerzemengeAnstellen).toDouble() + data(idx.row(), ColSpeisemenge).toDouble();
    }
    case ColVerdampfungsrateIst:
    {
        double V1 = data(idx.row(), ColWuerzemengeKochbeginn).toDouble();
        double V2 = data(idx.row(), ColWuerzemengeVorHopfenseihen).toDouble();
        double t = data(idx.row(), ColKochdauer).toDouble();
        return BierCalc::verdampfungsrate(V1, V2, t);
    }
    case ColsEVG:
    {
        double sw = data(idx.row(), ColSWIst).toDouble();
        double sre = data(idx.row(), ColSREIst).toDouble();
        return BierCalc::vergaerungsgrad(sw, sre);
    }
    case ColtEVG:
    {
        double sw = data(idx.row(), ColSWIst).toDouble();
        double sre = data(idx.row(), ColSREIst).toDouble();
        double tre = BierCalc::toTRE(sw, sre);
        return BierCalc::vergaerungsgrad(sw, tre);
    }
    case ColRestalkalitaetWasser:
    {
        return dataWasser(idx.row(), ModelWasser::ColRestalkalitaet).toDouble();
    }
    case ColRestalkalitaetIst:
    {
        return data(idx.row(), ColRestalkalitaetWasser).toDouble() + bh->modelWasseraufbereitung()->restalkalitaet(data(idx.row(), ColID));
    }
    case ColPhMalz:
    {
        double phGesamt = 0;
        double schuet = data(idx.row(), Colerg_S_Gesamt).toDouble();
        ProxyModel proxy;
        proxy.setSourceModel(bh->modelMalzschuettung());
        proxy.setFilterKeyColumn(ModelMalzschuettung::ColSudID);
        proxy.setFilterRegularExpression(QStringLiteral("^%1$").arg(data(idx.row(), ColID).toInt()));
        for (int r = 0; r < proxy.rowCount(); ++r)
        {
            double ph = proxy.data(r, ModelMalzschuettung::ColpH).toDouble();
            if (ph <= 0)
                return 0.0;
            double m = proxy.data(r, ModelMalzschuettung::Colerg_Menge).toDouble();
            phGesamt += std::pow(10, -ph) * m/schuet;
        }
        if (phGesamt > 0)
            phGesamt = -std::log10(phGesamt);
        return phGesamt;
    }
    case ColPhMaische:
    {
        double phMalz = data(idx.row(), ColPhMalz).toDouble();
        if (phMalz > 0)
        {
            double ra = data(idx.row(), ColRestalkalitaetIst).toDouble();
            double V = data(idx.row(), Colerg_WHauptguss).toDouble();
            double schuet = data(idx.row(), Colerg_S_Gesamt).toDouble();
            double phRa = (0.013 * V / schuet + 0.013) * ra / 2.8;
            return phMalz + phRa;
        }
        return 0;
    }
    case ColPhMaischeSoll:
    {
        double phMalz = data(idx.row(), ColPhMalz).toDouble();
        if (phMalz > 0)
        {
            double ra = data(idx.row(), ColRestalkalitaetSoll).toDouble();
            double V = data(idx.row(), Colerg_WHauptguss).toDouble();
            double schuet = data(idx.row(), Colerg_S_Gesamt).toDouble();
            double phRa = (0.013 * V / schuet + 0.013) * ra / 2.8;
            return phMalz + phRa;
        }
        return 0;
    }
    case ColBewertungMittel:
    {
        return bh->modelBewertungen()->mean(data(idx.row(), ColID));
    }
    default:
        return QVariant();
    }
}

bool ModelSud::setDataExt(const QModelIndex &idx, const QVariant &value)
{
    bool ret;
    mSkipUpdateOnOtherModelChanged = true;
    ret = setDataExt_impl(idx, value);
    mSkipUpdateOnOtherModelChanged = false;
    return ret;
}

bool ModelSud::setDataExt_impl(const QModelIndex &idx, const QVariant &value)
{
    switch(idx.column())
    {
    case ColBraudatum:
    {
        return QSqlTableModel::setData(idx, value.toDateTime().toString(Qt::ISODate));
    }
    case ColAbfuelldatum:
    {
        return QSqlTableModel::setData(idx, value.toDateTime().toString(Qt::ISODate));
    }
    case ColErstellt:
    {
        return QSqlTableModel::setData(idx, value.toDateTime().toString(Qt::ISODate));
    }
    case ColGespeichert:
    {
        return QSqlTableModel::setData(idx, value.toDateTime().toString(Qt::ISODate));
    }
    case ColReifungStart:
    {
        return QSqlTableModel::setData(idx, value.toDateTime().toString(Qt::ISODate));
    }
    case ColAnlage:
    {
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            if (status == Brauhelfer::SudStatus::Rezept)
            {
                setData(idx.row(), ColSudhausausbeute, dataAnlage(idx.row(), ModelAusruestung::ColSudhausausbeute));
                setData(idx.row(), ColVerdampfungsrate, dataAnlage(idx.row(), ModelAusruestung::ColVerdampfungsrate));
            }
            return true;
        }
        return false;
    }
    case ColMenge:
    {
        // ext-s10: Eingabe der Menge im Modus "Rezept nachbrauen" skaliert Malz- und Hopfenmengen
        // (nicht waehrend update(), das die Menge selbst aus dem Wasser setzt).
        // Erst die Menge setzen, dann skalieren: Jede Malz-/Hopfenaenderung meldet "modified",
        // die GUI liest dann bereits die neue Menge (sonst schreibt das fokussierte Mengenfeld
        // den alten Wert zurueck -> Rekursion beim Tippen).
        bool skalieren = !mUpdating && istNachbrauen(idx.row())
                && static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt()) == Brauhelfer::SudStatus::Rezept;
        double mengeAlt = data(idx.row(), ColMenge).toDouble();
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            if (status == Brauhelfer::SudStatus::Rezept)
                setDataVorbelegung(idx.row(), ColWuerzemengeKochbeginn, data(idx.row(), ColMengeSollKochbeginn));
            if (skalieren)
            {
                // Die Kaskade oben (verschachteltes setDataExt) setzt den Merker bereits zurueck;
                // ohne ihn rechnete update() mit halb skalierten Malzmengen eine Zwischenmenge.
                mSkipUpdateOnOtherModelChanged = true;
                skaliereMengen(idx.row(), mengeAlt, value.toDouble());
            }
            return true;
        }
        return false;
    }
    case ColWuerzemengeKochbeginn:
    {
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            if (status == Brauhelfer::SudStatus::Rezept)
                setDataVorbelegung(idx.row(), ColWuerzemengeVorHopfenseihen, value);
            return true;
        }
        return false;
    }
    case ColWuerzemengeVorHopfenseihen:
    {
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            if (status == Brauhelfer::SudStatus::Rezept)
                setDataVorbelegung(idx.row(), ColWuerzemengeKochende, value);
            return true;
        }
        return false;
    }
    case ColWuerzemengeKochende:
    {
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            if (status == Brauhelfer::SudStatus::Rezept)
            {
                double v = value.toDouble() + data(idx.row(), ColVerduennungAnstellen).toDouble() + data(idx.row(), ColMengeHefestarter).toDouble() - data(idx.row(), ColSpeisemenge).toDouble();
                setDataVorbelegung(idx.row(), ColWuerzemengeAnstellen, v);
            }
            return true;
        }
        return false;
    }
    case ColVerduennungAnstellen:
    {
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            if (status == Brauhelfer::SudStatus::Rezept)
            {
                double v = data(idx.row(), ColWuerzemengeKochende).toDouble() + value.toDouble() + data(idx.row(), ColMengeHefestarter).toDouble()  - data(idx.row(), ColSpeisemenge).toDouble();
                setDataVorbelegung(idx.row(), ColWuerzemengeAnstellen, v);
            }
            return true;
        }
        return false;
    }
    case ColMengeHefestarter:
    {
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            if (status == Brauhelfer::SudStatus::Rezept)
            {
                double v = data(idx.row(), ColWuerzemengeKochende).toDouble() + data(idx.row(), ColVerduennungAnstellen).toDouble() + value.toDouble() - data(idx.row(), ColSpeisemenge).toDouble();
                setDataVorbelegung(idx.row(), ColWuerzemengeAnstellen, v);
            }
            return true;
        }
        return false;
    }
    case ColWuerzemengeAnstellenTotal:
    {
        double v = value.toDouble() - data(idx.row(), ColSpeisemenge).toDouble();
        return setData(idx.row(), ColWuerzemengeAnstellen, v);
    }
    case ColWuerzemengeAnstellen:
    {
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            if (status == Brauhelfer::SudStatus::Rezept)
                setDataVorbelegung(idx.row(), ColJungbiermengeAbfuellen, value);
            return true;
        }
        return false;
    }
    case ColSpeisemenge:
    {
        double v = data(idx.row(), ColWuerzemengeAnstellen).toDouble() + data(idx).toDouble() - value.toDouble();
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            if (status == Brauhelfer::SudStatus::Rezept && !istGemessen(idx.row(), ColWuerzemengeAnstellen))
                QSqlTableModel::setData(index(idx.row(), ColWuerzemengeAnstellen), v);
            return true;
        }
        return false;
    }
    case ColJungbiermengeAbfuellen:
    {
        double preVal = data(idx).toDouble();
        if (QSqlTableModel::setData(idx, value))
        {
            double preValTotal = data(idx.row(), Colerg_AbgefuellteBiermenge).toDouble();
            QSqlTableModel::setData(index(idx.row(), Colerg_AbgefuellteBiermenge), preValTotal + (value.toDouble() - preVal));
            return true;
        }
        return false;
    }
    case ColVerschneidungAbfuellen:
    {
        double preVal = data(idx).toDouble();
        if (QSqlTableModel::setData(idx, value))
        {
            if (!data(idx.row(), ColSpunden).toBool())
            {
                double preValTotal = data(idx.row(), Colerg_AbgefuellteBiermenge).toDouble();
                QSqlTableModel::setData(index(idx.row(), Colerg_AbgefuellteBiermenge), preValTotal + (value.toDouble() - preVal));
            }
            return true;
        }
        return false;
    }
    case ColSW:
    {
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            if (status == Brauhelfer::SudStatus::Rezept)
                setDataVorbelegung(idx.row(), ColSWKochbeginn, data(idx.row(), ColSWSollKochbeginn));
            return true;
        }
        return false;
    }
    case ColSWKochbeginn:
    {
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            if (status == Brauhelfer::SudStatus::Rezept)
                setDataVorbelegung(idx.row(), ColSWKochende, value);
            return true;
        }
        return false;
    }
    case ColSWKochende:
    {
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            if (status == Brauhelfer::SudStatus::Rezept)
                setDataVorbelegung(idx.row(), ColSWAnstellen, value);
            return true;
        }
        return false;
    }
    case ColSWAnstellen:
    {
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            if (status == Brauhelfer::SudStatus::Rezept)
            {
                double vg = data(idx.row(), ColVergaerungsgrad).toDouble();
                double sre = BierCalc::sreAusVergaerungsgrad(value.toDouble() - swWzUnvergaerbarRecipe[idx.row()], vg) + swWzUnvergaerbarRecipe[idx.row()];
                setData(idx.row(), ColSWSchnellgaerprobe, sre);
                setDataVorbelegung(idx.row(), ColSWJungbier, sre);
            }
            return true;
        }
        return false;
    }
    case ColberechnungsArtWasser:
    {
        // ext-s10: Wechsel auf "Faktoren HG+NG" uebernimmt den aktuellen Nachguss als Faktor,
        // damit sich beim Umschalten keine Werte aendern (Invariante Moduswechsel).
        // Gilt auch fuer "Rezept nachbrauen" (2): dort folgt die Menge dann aus dem Wasser und
        // ist beim Umschalten unveraendert.
        bool warFaktoren = istWasserFaktoren(idx.row());
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            bool istFaktoren = istWasserFaktoren(idx.row());
            if (status == Brauhelfer::SudStatus::Rezept && istFaktoren && !warFaktoren)
            {
                double schuet = data(idx.row(), Colerg_S_Gesamt).toDouble();
                if (schuet > 0.0)
                    QSqlTableModel::setData(index(idx.row(), ColFaktorNachguss), data(idx.row(), Colerg_WNachguss).toDouble() / schuet);
            }
            return true;
        }
        return false;
    }
    case ColNachisoModell:
    {
        // ext-s11: Modellwechsel = Moduswechsel. Die Hopfenmengen bleiben, die Bittere folgt
        // dem neuen Modell (Anwenderentscheidung 2026-11-22). In den Ziel-IBU-Modi daher ueber
        // "Mengen fest -> Bittere berechnen" umschalten (beide Wechsel sind wertneutral, S01).
        Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
        int artHopfen = data(idx.row(), ColberechnungsArtHopfen).toInt();
        bool ziel = status == Brauhelfer::SudStatus::Rezept
                && (artHopfen == static_cast<int>(Brauhelfer::BerechnungsartHopfen::IBU)
                    || artHopfen == static_cast<int>(Brauhelfer::BerechnungsartHopfen::Gewicht));
        if (ziel)
            setData(idx.row(), ColberechnungsArtHopfen, static_cast<int>(Brauhelfer::BerechnungsartHopfen::IBUBerechnet));
        bool ret = QSqlTableModel::setData(idx, value);
        if (status == Brauhelfer::SudStatus::Rezept)
            updateIbuBerechnet(idx.row());
        if (ziel)
            setData(idx.row(), ColberechnungsArtHopfen, artHopfen);
        return ret;
    }
    case ColWasserPreset:
    {
        // ext-s10: Voreinstellung setzt nur die beiden Faktoren; spaeteres Aendern der Faktoren
        // laesst die gewaehlte Voreinstellung stehen.
        if (QSqlTableModel::setData(idx, value))
        {
            Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(idx.row(), ColStatus).toInt());
            double fhg, fng;
            if (status == Brauhelfer::SudStatus::Rezept && wasserPresetFaktoren(value.toInt(), fhg, fng))
            {
                QSqlTableModel::setData(index(idx.row(), ColFaktorHauptguss), fhg);
                QSqlTableModel::setData(index(idx.row(), ColFaktorNachguss), fng);
            }
            return true;
        }
        return false;
    }
    case ColPhMaischeSoll:
    {
        double phMalz = data(idx.row(), ColPhMalz).toDouble();
        if (phMalz > 0)
        {
            double phRa = value.toDouble() - phMalz;
            double V = data(idx.row(), Colerg_WHauptguss).toDouble();
            double schuet = data(idx.row(), Colerg_S_Gesamt).toDouble();
            double ra = phRa * 2.8 / (0.013 * V / schuet + 0.013);
            return QSqlTableModel::setData(index(idx.row(), ColRestalkalitaetSoll), ra);
        }
        return true;
    }
    default:
        return QSqlTableModel::setData(idx, value);
    }
}

Qt::ItemFlags ModelSud::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags itemFlags = SqlTableModel::flags(idx);
    switch (idx.column())
    {
    case ColWuerzemengeAnstellenTotal:
        itemFlags |= Qt::ItemIsEditable;
        break;
    }
    return itemFlags;
}

QVariant ModelSud::dataSud(const QVariant &sudId, int col)
{
    return getValueFromSameRow(ModelSud::ColID, sudId, col);
}

QVariant ModelSud::dataAnlage(int row, int col) const
{
    return bh->modelAusruestung()->getValueFromSameRow(ModelAusruestung::ColName, data(row, ColAnlage), col);
}

void ModelSud::setDataAnlage(int row, int col, const QVariant& value)
{
    bh->modelAusruestung()->setValueFromSameRow(ModelAusruestung::ColName, data(row, ColAnlage), col, value);
}

QVariant ModelSud::dataWasser(int row, int col) const
{
    return bh->modelWasser()->getValueFromSameRow(ModelWasser::ColName, data(row, ColWasserprofil), col);
}

void ModelSud::setDataWasser(int row, int col, const QVariant& value)
{
    bh->modelWasser()->setValueFromSameRow(ModelWasser::ColName, data(row, ColWasserprofil), col, value);
}

void ModelSud::update(int row, int colChanged)
{
    if (row < 0 || row >= rowCount())
        return;

    if (mUpdating)
        return;
    mUpdating = true;
    mSignalModifiedBlocked = true;

    qInfo(Brauhelfer::loggingCategory) << "ModelSud::update():" << data(row, ColID).toInt();

    updateSwWeitereZutaten(row);

    Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(row, ColStatus).toInt());
    if (status == Brauhelfer::SudStatus::Rezept)
    {
        // ext-s10: Rezept nachbrauen -> Menge aus Malzmengen und Wasserfaktoren
        if (istNachbrauen(row))
            updateMengeAusWasser(row);

        // MengeSoll
        if (colChanged == ColMengeSollAnstellen)
        {
            QModelIndex idx2 = index(row, colChanged);
            emit dataChanged(idx2, idx2);
        }

        // erg_S_Gesamt
        double sw = data(row, ColSWAnteilMalz).toDouble();
        double sw_dichte = sw + swWzMaischenRecipe[row] + swWzKochenRecipe[row];
        double menge = data(row, ColMengeSollAnstellen).toDouble();
        double ausb = data(row, ColSudhausausbeute).toDouble();
        double schuet;
        if (static_cast<Brauhelfer::BerechnungsartMalz>(data(row, ColberechnungsArtMalz).toInt()) == Brauhelfer::BerechnungsartMalz::MengenFest)
        {
            // ext-s09: Malzmengen fest -> Schuettung = Summe, Stammwuerze rueckwaerts berechnet
            schuet = updateSwAusMalzmengen(row);
        }
        else
        {
            schuet = BierCalc::schuettung(sw, sw_dichte, menge, ausb);
        }
        setData(row, Colerg_S_Gesamt, schuet);

        // erg_W_Gesamt, erg_WHauptguss, erg_WNachguss
        updateWasser(row);

        // erg_Farbe
        // ext-s12b: Malzanteil der Zugabe-Zeilen im Maischplan an die Schuettung anpassen
        if (bh->modelMaischplan()->hatMalzZugabe(data(row, ColID)))
            bh->modelMaischplan()->aktualisiereMalzZugabe(data(row, ColID));
        updateFarbe(row);

        // IBU aus Hopfenmengen (nur Berechnungsart IBUBerechnet)
        updateIbuBerechnet(row);

        // erg_Sudhausausbeute
        sw_dichte = data(row, ColSWKochende).toDouble();
        sw = sw_dichte - swWzMaischenRecipe[row] - swWzKochenRecipe[row];
        menge = data(row, ColWuerzemengeVorHopfenseihen).toDouble();
        setData(row, Colerg_Sudhausausbeute, BierCalc::sudhausausbeute(sw, sw_dichte, menge, schuet));

        // erg_EffektiveAusbeute
        sw_dichte = data(row, ColSWAnstellen).toDouble();
        double sw_starter = data(row, ColSWAnteilHefestarter).toDouble();
        sw = sw_dichte - swWzMaischenRecipe[row] - swWzKochenRecipe[row] - sw_starter;
        menge = data(row, ColWuerzemengeAnstellenTotal).toDouble();
        setData(row, Colerg_EffektiveAusbeute, BierCalc::sudhausausbeute(sw, sw_dichte, menge, schuet));
    }
    if (status <= Brauhelfer::SudStatus::Gebraut)
    {
        // erg_Alkohol
        double sre = data(row, ColSREIst).toDouble();
        double sw = data(row, ColSWIst).toDouble();
        double menge = data(row, ColJungbiermengeAbfuellen).toDouble();
        double verschneidung = 0.0;
        double alcZucker = 0.0;
        Brauhelfer::Abfuellart art = static_cast<Brauhelfer::Abfuellart>(data(row, ColAbfuellart).toInt());
        if (art == Brauhelfer::Abfuellart::Abgeleitet)
        {
            // keine Auswahl: wie bisher
            if (!data(row, ColSpunden).toBool())
            {
                alcZucker = BierCalc::alkoholVonZucker(data(row, ColZuckerAnteil).toDouble() / menge);
                if (alcZucker > 0)
                    verschneidung = data(row, ColVerschneidungAbfuellen).toDouble();
            }
        }
        else
        {
            // ext-s13: Zuckeranteil je Abfuellart (Spunden/Zwangskarbonisierung: 0)
            alcZucker = BierCalc::alkoholVonZucker(data(row, ColZuckerAnteil).toDouble() / menge);
            // Verduennung durch Zuckerloesung nur, wenn Zucker in den ganzen Sud kommt
            if (alcZucker > 0 && (art == Brauhelfer::Abfuellart::ZuckerGesamt || art == Brauhelfer::Abfuellart::Speise))
                verschneidung = data(row, ColVerschneidungAbfuellen).toDouble();
            // Rechtzeitig schlauchen: Restextrakt vergaert in Flasche/Fass. Fertiges Bier = Rezept
            // (erwarteter Vergaerungsgrad), bis eine Schnellgaerprobe gemessen ist.
            if ((art == Brauhelfer::Abfuellart::Spunden || art == Brauhelfer::Abfuellart::SpundenZucker) && !data(row, ColSchnellgaerprobeAktiv).toBool())
                sre = data(row, ColSREErwartet).toDouble();
        }
        setData(row, Colerg_Alkohol, BierCalc::alkohol(sw, sre, alcZucker) / (1 + verschneidung/menge));

        // erg_Preis
        updatePreis(row);
    }

    setData(row, ColGespeichert, QDateTime::currentDateTime());

    mSignalModifiedBlocked = false;
    mUpdating = false;

    emit modified();
}

void ModelSud::updateSwWeitereZutaten(int row)
{
    swWzMaischenRecipe[row] = 0.0;
    swWzKochenRecipe[row] = 0.0;
    swWzGaerungRecipe[row] = 0.0;
    swWzGaerungCurrent[row] = 0.0;
    swWzUnvergaerbarRecipe[row] = 0.0;

    ProxyModel modelWeitereZutatenGaben;
    modelWeitereZutatenGaben.setSourceModel(bh->modelWeitereZutatenGaben());
    modelWeitereZutatenGaben.setFilterKeyColumn(ModelWeitereZutatenGaben::ColSudID);
    modelWeitereZutatenGaben.setFilterRegularExpression(QRegularExpression(QStringLiteral("^%1$").arg(data(row, ColID).toInt())));
    for (int r = 0; r < modelWeitereZutatenGaben.rowCount(); ++r)
    {
        Brauhelfer::ZusatzTyp typ = static_cast<Brauhelfer::ZusatzTyp>(modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::ColTyp).toInt());
        if (typ == Brauhelfer::ZusatzTyp::Hopfen)
            continue;
        double extrakt = modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::ColExtrakt).toDouble();
        if (extrakt > 0.0)
        {
            Brauhelfer::ZusatzZeitpunkt zeitpunkt = static_cast<Brauhelfer::ZusatzZeitpunkt>(modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::ColZeitpunkt).toInt());
            Brauhelfer::ZusatzStatus status = static_cast<Brauhelfer::ZusatzStatus>(modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::ColZugabestatus).toInt());
            bool unvergaerbar = modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::ColUnvergaerbar).toBool();
            switch (zeitpunkt)
            {
            case Brauhelfer::ZusatzZeitpunkt::Gaerung:
                swWzGaerungRecipe[row] += extrakt ;
                if (status != Brauhelfer::ZusatzStatus::NichtZugegeben)
                    swWzGaerungCurrent[row] += extrakt;
                break;
            case Brauhelfer::ZusatzZeitpunkt::Kochen:
                swWzKochenRecipe[row] += extrakt;
                break;
            case Brauhelfer::ZusatzZeitpunkt::Maischen:
                swWzMaischenRecipe[row] += extrakt;
                break;
            }
            if (unvergaerbar)
                swWzUnvergaerbarRecipe[row] += extrakt;
        }
    }
}

double ModelSud::treberaufnahme(int row) const
{
    // Treberaufnahme der Anlage in L/kg Schuettung (ext-s10, bisher fest 0,96 im Code).
    // Fehlender oder ungueltiger Wert (z. B. neue, noch ungespeicherte Anlage) -> 0,96.
    QVariant v = dataAnlage(row, ModelAusruestung::ColTreberaufnahme);
    bool ok = false;
    double t = v.toDouble(&ok);
    if (v.isNull() || !ok || t <= 0.0)
        return 0.96;
    return t;
}

bool ModelSud::istWhirlpool(int row) const
{
    return static_cast<Brauhelfer::NachisoModell>(data(row, ColNachisoModell).toInt()) == Brauhelfer::NachisoModell::Whirlpool;
}

double ModelSud::topfDurchmesser(int row) const
{
    // Durchmesser der Sudpfanne [cm] aus der Ausruestung (ext-s11). Fehlt er, wird ein runder
    // Topf mit Hoehe = Durchmesser und 1,5-fachem Volumen der Wuerze zu Kochbeginn angenommen.
    double d = dataAnlage(row, ModelAusruestung::ColSudpfanne_Durchmesser).toDouble();
    if (d > 0.0)
        return d;
    double v = 1.5 * data(row, ColMengeSollKochbeginn).toDouble();   // L
    if (v <= 0.0)
        return 0.0;
    return cbrt(4.0 * v * 1000.0 / 3.14159265358979);               // cm
}

double ModelSud::whirlpoolMinuten(int row, double abMinute, double* temperaturEnde) const
{
    // Wirksame Kochminuten in der Whirlpool-/Absetzzeit ab Minute abMinute nach Kochende (ext-s11)
    double dauer = data(row, ColNachisomerisierungszeit).toDouble();
    double tAussen = data(row, ColAussentemperatur).toDouble();
    bool deckel = data(row, ColWhirlpoolDeckel).toInt() != 0;
    QVariant f = dataAnlage(row, ModelAusruestung::ColWhirlpoolFaktor);
    double faktor = (f.isNull() || f.toDouble() <= 0.0) ? 1.0 : f.toDouble();
    double volumen = data(row, ColMengeSollKochende).toDouble();
    return BierCalc::whirlpoolMinuten(volumen, topfDurchmesser(row), tAussen, deckel, faktor, abMinute, dauer, temperaturEnde);
}

bool ModelSud::istWasserFaktoren(int row) const
{
    // Haupt- und Nachguss ueber Faktoren (ext-s10), nur Standardanlagen
    Brauhelfer::BerechnungsartWasser art = static_cast<Brauhelfer::BerechnungsartWasser>(data(row, ColberechnungsArtWasser).toInt());
    return art == Brauhelfer::BerechnungsartWasser::Faktoren || art == Brauhelfer::BerechnungsartWasser::Nachbrauen;
}

bool ModelSud::istNachbrauen(int row) const
{
    // "Rezept nachbrauen" (ext-s10): Wasser Nachbrauen + Malz Mengen fest + Standardanlage
    if (static_cast<Brauhelfer::BerechnungsartWasser>(data(row, ColberechnungsArtWasser).toInt()) != Brauhelfer::BerechnungsartWasser::Nachbrauen)
        return false;
    if (static_cast<Brauhelfer::BerechnungsartMalz>(data(row, ColberechnungsArtMalz).toInt()) != Brauhelfer::BerechnungsartMalz::MengenFest)
        return false;
    Brauhelfer::AnlageTyp anlageTyp = static_cast<Brauhelfer::AnlageTyp>(dataAnlage(row, ModelAusruestung::ColTyp).toInt());
    return anlageTyp != Brauhelfer::AnlageTyp::GrainfatherG30 && anlageTyp != Brauhelfer::AnlageTyp::BrauheldPro30;
}

double ModelSud::mengeAusWasser(int row, double schuettung) const
{
    // Rueckwaertsrechnung der Mengenkette (ext-s10), Umkehrung von MengeSollKochbeginn:
    //   Kochbeginn = S*(fHG + fNG - Treberaufnahme) - KorrekturWasser
    //   Kochende   = Kochbeginn - Verdampfung
    //   MengeSollAnstellen = Kochende * (100 + HGF) / 100 + Hefestarter
    //   Menge      = MengeSollAnstellen - KorrekturMenge
    // Linear in der Schuettung.
    double fhg = data(row, ColFaktorHauptguss).toDouble();
    double fng = data(row, ColFaktorNachguss).toDouble();
    double kochbeginn = schuettung * (fhg + fng - treberaufnahme(row)) - dataAnlage(row, ModelAusruestung::ColKorrekturWasser).toDouble();
    double kochende = kochbeginn - data(row, ColMengeSollVerdampfung).toDouble();
    double hgf = data(row, ColhighGravityFaktor).toInt();
    double mengeSollAnstellen = kochende * (100 + hgf) / 100 + data(row, ColMengeHefestarter).toDouble();
    return mengeSollAnstellen - dataAnlage(row, ModelAusruestung::ColKorrekturMenge).toDouble();
}

double ModelSud::summeMalzmengen(int row) const
{
    QRegularExpression sudReg(QStringLiteral("^%1$").arg(data(row, ColID).toInt()));
    ProxyModel modelMalzschuettung;
    modelMalzschuettung.setSourceModel(bh->modelMalzschuettung());
    modelMalzschuettung.setFilterKeyColumn(ModelMalzschuettung::ColSudID);
    modelMalzschuettung.setFilterRegularExpression(sudReg);
    double schuet = 0.0;
    for (int r = 0; r < modelMalzschuettung.rowCount(); ++r)
        schuet += modelMalzschuettung.data(r, ModelMalzschuettung::Colerg_Menge).toDouble();
    return schuet;
}

void ModelSud::updateMengeAusWasser(int row)
{
    // Menge nur bei echter Aenderung setzen: setData(Menge) belegt im Rezeptstatus die Braudaten vor
    double menge = mengeAusWasser(row, summeMalzmengen(row));
    if (std::isfinite(menge) && menge > 0.0 && std::fabs(menge - data(row, ColMenge).toDouble()) > 1e-9)
        setData(row, ColMenge, menge);
}

void ModelSud::skaliereMengen(int row, double mengeAlt, double mengeNeu)
{
    // Rezept skalieren (ext-s10): Schuettung so waehlen, dass die Menge aus dem Wasser genau
    // mengeNeu ergibt (lineare Kette, siehe mengeAusWasser, unabhaengig von der Menge),
    // Malz kg anteilig; Hopfen g im Verhaeltnis der Mengen (Bittere bleibt etwa gleich).
    // Faktoren bleiben unveraendert. mengeAlt = Menge vor der Eingabe.
    double schuetAlt = summeMalzmengen(row);
    double b = mengeAusWasser(row, 0.0);
    double a = mengeAusWasser(row, 1.0) - b;
    if (mengeNeu <= 0.0 || mengeAlt <= 0.0 || schuetAlt <= 0.0 || a <= 0.0)
        return;
    double schuetNeu = (mengeNeu - b) / a;
    if (!std::isfinite(schuetNeu) || schuetNeu <= 0.0)
        return;
    QRegularExpression sudReg(QStringLiteral("^%1$").arg(data(row, ColID).toInt()));
    double fMalz = schuetNeu / schuetAlt;
    ProxyModel modelMalzschuettung;
    modelMalzschuettung.setSourceModel(bh->modelMalzschuettung());
    modelMalzschuettung.setFilterKeyColumn(ModelMalzschuettung::ColSudID);
    modelMalzschuettung.setFilterRegularExpression(sudReg);
    for (int r = 0; r < modelMalzschuettung.rowCount(); ++r)
        modelMalzschuettung.setData(r, ModelMalzschuettung::Colerg_Menge, modelMalzschuettung.data(r, ModelMalzschuettung::Colerg_Menge).toDouble() * fMalz);
    double fHopfen = mengeNeu / mengeAlt;
    ProxyModel modelHopfengaben;
    modelHopfengaben.setSourceModel(bh->modelHopfengaben());
    modelHopfengaben.setFilterKeyColumn(ModelHopfengaben::ColSudID);
    modelHopfengaben.setFilterRegularExpression(sudReg);
    for (int r = 0; r < modelHopfengaben.rowCount(); ++r)
        modelHopfengaben.setData(r, ModelHopfengaben::Colerg_Menge, modelHopfengaben.data(r, ModelHopfengaben::Colerg_Menge).toDouble() * fHopfen);
}

bool ModelSud::wasserPresetFaktoren(int preset, double &faktorHauptguss, double &faktorNachguss)
{
    // Richtwerte L/kg (Anwendervorgabe, Quelle Kling noch ungeprueft) (ext-s10)
    switch (static_cast<Brauhelfer::WasserPreset>(preset))
    {
    case Brauhelfer::WasserPreset::Helles:
        faktorHauptguss = 4.0;
        faktorNachguss = 3.0;
        return true;
    case Brauhelfer::WasserPreset::Dunkles:
        faktorHauptguss = 3.0;
        faktorNachguss = 4.0;
        return true;
    case Brauhelfer::WasserPreset::Benutzerdefiniert:
        faktorHauptguss = 3.5;
        faktorNachguss = 3.5;
        return true;
    default:
        return false;
    }
}

void ModelSud::updateWasser(int row)
{
    double hg = 0.0, ng = 0.0, hgf = 0.0;
    bool korrigieren = true;
    double schuet = data(row, Colerg_S_Gesamt).toDouble();
    double menge = data(row, ColMengeSollKochbeginn).toDouble();

    Brauhelfer::AnlageTyp anlageTyp = static_cast<Brauhelfer::AnlageTyp>(dataAnlage(row, ModelAusruestung::ColTyp).toInt());
    switch (anlageTyp)
    {
    case Brauhelfer::AnlageTyp::GrainfatherG30:
        hg = schuet * 2.7 + 3.5;
        if (schuet > 4.5)
            ng = menge - hg + schuet * 0.8;
        else
            ng = menge - hg + schuet * 0.8 - 2;
        break;
    case Brauhelfer::AnlageTyp::BrauheldPro30:
        hg = schuet * 2.7 + 3.2;
        ng = menge - hg + schuet * 0.8;
        break;
    default:
        hg = schuet * data(row, ColFaktorHauptguss).toDouble();
        if (istWasserFaktoren(row))
        {
            // ext-s10: Nachguss fest ueber Faktor, die Wuerzemenge zu Kochbeginn folgt daraus
            // (ColWuerzemengeKochbeginnErwartet); Korrektur der Anlage steckt dort.
            ng = schuet * data(row, ColFaktorNachguss).toDouble();
            korrigieren = false;
        }
        else
        {
            ng = menge - hg + schuet * treberaufnahme(row);
        }
        break;
    }
    if (korrigieren)
        ng += dataAnlage(row, ModelAusruestung::ColKorrekturWasser).toDouble();
    if (ng < 0)
        ng = 0;

    hgf = data(row, ColMengeSollHgf).toDouble();
    if (hgf < 0)
        hgf = 0;

    setData(row, Colerg_WHauptguss, hg);
    setData(row, Colerg_WNachguss, ng);
    setData(row, Colerg_W_Gesamt, hg + ng + hgf);
}

void ModelSud::updateFarbe(int row)
{
    setData(row, Colerg_Farbe, berechneFarbe(row, true));
}

double ModelSud::berechneFarbe(int row, bool mitZugabe) const
{
    QRegularExpression sudReg(QStringLiteral("^%1$").arg(data(row, ColID).toInt()));
    double ebc = 0.0;
    double d = 0.0;
    double gs = 0.0;

    // ext-s12b: im Maischplan spaet zugegebene Malze geben nur einen Teil ihrer Farbe ab
    // (Farbausbeute nach Kontaktzeit, Annahme ungeprueft); Extrakt bleibt unveraendert
    const QMap<QString, double> zugaben = mitZugabe ? bh->modelMaischplan()->malzZugaben(data(row, ColID)) : QMap<QString, double>();
    const double tau = bh->farbmalzTau();

    ProxyModel modelMalzschuettung;
    modelMalzschuettung.setSourceModel(bh->modelMalzschuettung());
    modelMalzschuettung.setFilterKeyColumn(ModelMalzschuettung::ColSudID);
    modelMalzschuettung.setFilterRegularExpression(sudReg);
    for (int r = 0; r < modelMalzschuettung.rowCount(); ++r)
    {
        double farbe = modelMalzschuettung.data(r, ModelMalzschuettung::ColFarbe).toDouble();
        if (!zugaben.isEmpty())
        {
            auto it = zugaben.constFind(modelMalzschuettung.data(r, ModelMalzschuettung::ColName).toString().trimmed());
            if (it != zugaben.constEnd())
                farbe *= BierCalc::farbausbeute(it.value(), tau);
        }
        double menge = modelMalzschuettung.data(r, ModelMalzschuettung::Colerg_Menge).toDouble();
        d += menge * farbe;
        gs += menge;
    }

    ProxyModel modelWeitereZutatenGaben;
    modelWeitereZutatenGaben.setSourceModel(bh->modelWeitereZutatenGaben());
    modelWeitereZutatenGaben.setFilterKeyColumn(ModelWeitereZutatenGaben::ColSudID);
    modelWeitereZutatenGaben.setFilterRegularExpression(sudReg);
    for (int r = 0; r < modelWeitereZutatenGaben.rowCount(); ++r)
    {
        Brauhelfer::ZusatzTyp typ = static_cast<Brauhelfer::ZusatzTyp>(modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::ColTyp).toInt());
        if (typ == Brauhelfer::ZusatzTyp::Hopfen)
            continue;
        double farbe = modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::ColFarbe).toDouble();
        if (farbe > 0.0)
        {
            double menge = modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::Colerg_Menge).toDouble();
            Brauhelfer::Einheit einheit = static_cast<Brauhelfer::Einheit>(modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::ColEinheit).toInt());
            if (einheit != Brauhelfer::Einheit::Stk)
                menge /= 1000;
            d += menge * farbe;
            gs += menge;
        }
    }

    if (gs > 0.0)
    {
        double sw = data(row, ColSW).toDouble() - swWzKochenRecipe[row] - swWzGaerungRecipe[row];
        double t = data(row, ColKochdauer).toDouble();
        ebc = (d / gs) * sw / 10 + 1.5 * t/60;
        ebc += dataAnlage(row, ModelAusruestung::ColKorrekturFarbe).toDouble();
    }
    return ebc;
}

double ModelSud::updateSwAusMalzmengen(int row)
{
    // Berechnungsart Malz "Mengen fest" (ext-s09): Schuettung = Summe der kg je Malz,
    // Stammwuerze = Umkehrung der Schuettungsformel (gleiche Formel wie Modus Ziel-SW, daher
    // liefert ein Moduswechsel in beide Richtungen dieselben Werte).
    double schuet = summeMalzmengen(row);

    double menge = data(row, ColMengeSollAnstellen).toDouble();
    double ausb = data(row, ColSudhausausbeute).toDouble();
    if (menge > 0.0 && ausb > 0.0)
    {
        double swZusatzDichte = swWzMaischenRecipe[row] + swWzKochenRecipe[row];
        double swMalz = BierCalc::swAusSchuettung(schuet, swZusatzDichte, menge, ausb);
        double sw = swMalz + swWzMaischenRecipe[row] + swWzKochenRecipe[row] + swWzGaerungRecipe[row]
                    + data(row, ColSWAnteilHefestarter).toDouble();
        // nur bei echter Aenderung setzen: setData(SW) belegt im Rezeptstatus die Braudaten vor
        if (std::isfinite(sw) && std::fabs(sw - data(row, ColSW).toDouble()) > 1e-9)
            setData(row, ColSW, sw);
    }
    return schuet;
}

void ModelSud::updateIbuBerechnet(int row)
{
    // Berechnungsart "Mengen fest -> Bittere berechnen": Sud.IBU = Summe der IBU-Anteile.
    // Dieselbe Formel (ModelHopfengaben ColIBUAnteil) nutzen die Ziel-IBU-Modi rückwärts,
    // daher liefert ein Moduswechsel in beide Richtungen dieselben Mengen bzw. dieselbe IBU.
    if (static_cast<Brauhelfer::BerechnungsartHopfen>(data(row, ColberechnungsArtHopfen).toInt()) != Brauhelfer::BerechnungsartHopfen::IBUBerechnet)
        return;
    QRegularExpression sudReg(QStringLiteral("^%1$").arg(data(row, ColID).toInt()));
    ProxyModel modelHopfengaben;
    modelHopfengaben.setSourceModel(bh->modelHopfengaben());
    modelHopfengaben.setFilterKeyColumn(ModelHopfengaben::ColSudID);
    modelHopfengaben.setFilterRegularExpression(sudReg);
    double ibu = 0.0;
    for (int r = 0; r < modelHopfengaben.rowCount(); ++r)
        ibu += modelHopfengaben.data(r, ModelHopfengaben::ColIBUAnteil).toDouble();
    if (std::isfinite(ibu) && std::fabs(ibu - data(row, ColIBU).toDouble()) > 1e-9)
        setData(row, ColIBU, ibu);
}

void ModelSud::updatePreis(int row)
{
    QRegularExpression sudReg(QStringLiteral("^%1$").arg(data(row, ColID).toInt()));
    double summe = 0.0;

    double kostenSchuettung = 0.0;
    ProxyModel modelMalzschuettung;
    modelMalzschuettung.setSourceModel(bh->modelMalzschuettung());
    modelMalzschuettung.setFilterKeyColumn(ModelMalzschuettung::ColSudID);
    modelMalzschuettung.setFilterRegularExpression(sudReg);
    for (int r = 0; r < modelMalzschuettung.rowCount(); ++r)
    {
        QVariant name = modelMalzschuettung.data(r, ModelMalzschuettung::ColName);
        double menge = modelMalzschuettung.data(r, ModelMalzschuettung::Colerg_Menge).toDouble();
        double preis = bh->modelMalz()->getValueFromSameRow(ModelMalz::ColName, name, ModelMalz::ColPreis).toDouble();
        kostenSchuettung += preis * menge;
    }
    summe += kostenSchuettung;

    double kostenHopfen = 0.0;
    ProxyModel modelHopfengaben;
    modelHopfengaben.setSourceModel(bh->modelHopfengaben());
    modelHopfengaben.setFilterKeyColumn(ModelHopfengaben::ColSudID);
    modelHopfengaben.setFilterRegularExpression(sudReg);
    for (int r = 0; r < modelHopfengaben.rowCount(); ++r)
    {
        QVariant name = modelHopfengaben.data(r, ModelHopfengaben::ColName);
        double menge = modelHopfengaben.data(r, ModelHopfengaben::Colerg_Menge).toDouble();
        double preis = bh->modelHopfen()->getValueFromSameRow(ModelHopfen::ColName, name, ModelHopfen::ColPreis).toDouble();
        kostenHopfen += preis * menge / 1000;
    }
    summe += kostenHopfen;

    double kostenHefe = 0.0;
    ProxyModel modelHefegaben;
    modelHefegaben.setSourceModel(bh->modelHefegaben());
    modelHefegaben.setFilterKeyColumn(ModelHefegaben::ColSudID);
    modelHefegaben.setFilterRegularExpression(sudReg);
    for (int r = 0; r < modelHefegaben.rowCount(); ++r)
    {
        QVariant name = modelHefegaben.data(r, ModelHefegaben::ColName);
        int menge = modelHefegaben.data(r, ModelHefegaben::ColMenge).toInt();
        double preis = bh->modelHefe()->getValueFromSameRow(ModelHefe::ColName, name, ModelHefe::ColPreis).toDouble();
        kostenHefe += preis * menge;
    }
    summe += kostenHefe;

    double kostenWeitereZutaten = 0.0;
    ProxyModel modelWeitereZutatenGaben;
    modelWeitereZutatenGaben.setSourceModel(bh->modelWeitereZutatenGaben());
    modelWeitereZutatenGaben.setFilterKeyColumn(ModelWeitereZutatenGaben::ColSudID);
    modelWeitereZutatenGaben.setFilterRegularExpression(sudReg);
    for (int r = 0; r < modelWeitereZutatenGaben.rowCount(); ++r)
    {
        QVariant name = modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::ColName);
        double menge = modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::Colerg_Menge).toDouble();
        double preis;
        Brauhelfer::ZusatzTyp typ = static_cast<Brauhelfer::ZusatzTyp>(modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::ColTyp).toInt());
        if (typ == Brauhelfer::ZusatzTyp::Hopfen)
        {
            preis = bh->modelHopfen()->getValueFromSameRow(ModelHopfen::ColName, name, ModelHopfen::ColPreis).toDouble();
            preis /= 1000;
        }
        else
        {
            preis = bh->modelWeitereZutaten()->getValueFromSameRow(ModelWeitereZutaten::ColName, name, ModelWeitereZutaten::ColPreis).toDouble();
            Brauhelfer::Einheit einheit = static_cast<Brauhelfer::Einheit>(modelWeitereZutatenGaben.data(r, ModelWeitereZutatenGaben::ColEinheit).toInt());
            if (einheit == Brauhelfer::Einheit::Stk)
                menge = qCeil(menge);
            else
                preis /= 1000;
        }
        kostenWeitereZutaten += preis * menge;
    }
    summe += kostenWeitereZutaten;

    double kostenSonstiges = data(row, ColKostenWasserStrom).toDouble();
    summe += kostenSonstiges;

    double kostenAnlage = dataAnlage(row, ModelAusruestung::ColKosten).toDouble();
    summe += kostenAnlage;

    setData(row, Colerg_Preis, summe / data(row, ColMengeIst).toDouble());
}

bool ModelSud::removeRows(int row, int count, const QModelIndex &parent)
{
    QList<int> sudIds;
    sudIds.reserve(count);
    for (int i = 0; i < count; ++i)
        sudIds.append(data(row + i, ColID).toInt());
    if (SqlTableModel::removeRows(row, count, parent))
    {
        removeRowsFrom(bh->modelMaischplan(), ModelMaischplan::ColSudID, sudIds);
        removeRowsFrom(bh->modelMalzschuettung(), ModelMalzschuettung::ColSudID, sudIds);
        removeRowsFrom(bh->modelHopfengaben(), ModelHopfengaben::ColSudID, sudIds);
        removeRowsFrom(bh->modelWeitereZutatenGaben(), ModelWeitereZutatenGaben::ColSudID, sudIds);
        removeRowsFrom(bh->modelHefegaben(), ModelHefegaben::ColSudID, sudIds);
        removeRowsFrom(bh->modelSchnellgaerverlauf(), ModelSchnellgaerverlauf::ColSudID, sudIds);
        removeRowsFrom(bh->modelHauptgaerverlauf(), ModelHauptgaerverlauf::ColSudID, sudIds);
        removeRowsFrom(bh->modelNachgaerverlauf(), ModelNachgaerverlauf::ColSudID, sudIds);
        removeRowsFrom(bh->modelBewertungen(), ModelBewertungen::ColSudID, sudIds);
        removeRowsFrom(bh->modelMesswerte(), ModelMesswerte::ColSudID, sudIds);
        removeRowsFrom(bh->modelAnhang(), ModelAnhang::ColSudID, sudIds);
        removeRowsFrom(bh->modelEtiketten(), ModelEtiketten::ColSudID, sudIds);
        removeRowsFrom(bh->modelTags(), ModelTags::ColSudID, sudIds);
        return true;
    }
    return false;
}

void ModelSud::removeRowsFrom(SqlTableModel* model, int colId, const QList<int> &sudIds)
{
    for (int r = 0; r < model->rowCount(); ++r)
    {
        if (sudIds.contains(model->data(r, colId).toInt()))
        {
            int count = model->rowCount();
            model->removeRows(r);
            if (count != model->rowCount())
                r--;
        }
    }
}

void ModelSud::defaultValues(QMap<int, QVariant> &values) const
{
    if (!values.contains(ColID))
        values.insert(ColID, getNextId());
    if (!values.contains(ColErstellt))
        values.insert(ColErstellt, QDateTime::currentDateTime());
    if (!values.contains(ColSudname))
        values.insert(ColSudname, "Sudname");
    if (!values.contains(ColSudnummer))
        values.insert(ColSudnummer, 0);
    if (!values.contains(ColAnlage) && bh->modelAusruestung()->rowCount() == 1)
        values.insert(ColAnlage, bh->modelAusruestung()->data(0, ModelAusruestung::ColName));
    if (!values.contains(ColWasserprofil) && bh->modelWasser()->rowCount() == 1)
        values.insert(ColWasserprofil, bh->modelWasser()->data(0, ModelWasser::ColName));
    if (!values.contains(ColMenge))
        values.insert(ColMenge, 20);
    if (!values.contains(ColSW))
        values.insert(ColSW, 12);
    if (!values.contains(ColFaktorHauptguss))
        values.insert(ColFaktorHauptguss, 3);
    if (!values.contains(ColCO2))
        values.insert(ColCO2, 5);
    if (!values.contains(ColIBU))
        values.insert(ColIBU, 26);
    if (!values.contains(ColKochdauer))
        values.insert(ColKochdauer, 60);
    if (!values.contains(ColberechnungsArtHopfen))
        values.insert(ColberechnungsArtHopfen, static_cast<int>(Brauhelfer::BerechnungsartHopfen::IBU));
    if (!values.contains(ColVergaerungsgrad))
        values.insert(ColVergaerungsgrad, 70);
    if (!values.contains(ColTemperaturJungbier))
        values.insert(ColTemperaturJungbier, 12.0);
    if (!values.contains(ColTemperaturKarbonisierung))
        values.insert(ColTemperaturKarbonisierung, 12.0);
    if (!values.contains(ColSudhausausbeute))
        values.insert(ColSudhausausbeute, 60.0);
    if (!values.contains(ColVerdampfungsrate))
        values.insert(ColVerdampfungsrate, 2.0);
    if (!values.contains(ColSpeisemenge))
        values.insert(ColSpeisemenge, 0);
    if (!values.contains(ColStatus))
        values.insert(ColStatus, static_cast<int>(Brauhelfer::SudStatus::Rezept));
}

QMap<int, QVariant> ModelSud::copyValues(int row) const
{
    QMap<int, QVariant> values = SqlTableModel::copyValues(row);
    values[ColID] = getNextId();
    return values;
}

bool ModelSud::istGemessen(int row, int col) const
{
    return bh->modelMesswerte()->istGemessen(data(row, ColID), col);
}

bool ModelSud::setDataVorbelegung(int row, int col, const QVariant &value)
{
    // ext-s12: gemessene Ist-Felder (Brauprotokoll) nicht mit Sollwerten ueberschreiben
    if (istGemessen(row, col))
        return false;
    return setData(row, col, value);
}

QVariant ModelSud::istVorbelegung(int row, int col) const
{
    // Wert, den die Kaskaden in setDataExt_impl im Rezeptstatus in ein Ist-Feld schreiben
    switch (col)
    {
    case ColWuerzemengeKochbeginn:
        return data(row, ColMengeSollKochbeginn);
    case ColWuerzemengeVorHopfenseihen:
        return data(row, ColWuerzemengeKochbeginn);
    case ColWuerzemengeKochende:
        return data(row, ColWuerzemengeVorHopfenseihen);
    case ColWuerzemengeAnstellen:
        return data(row, ColWuerzemengeKochende).toDouble() + data(row, ColVerduennungAnstellen).toDouble() + data(row, ColMengeHefestarter).toDouble() - data(row, ColSpeisemenge).toDouble();
    case ColJungbiermengeAbfuellen:
        return data(row, ColWuerzemengeAnstellen);
    case ColSWKochbeginn:
        return data(row, ColSWSollKochbeginn);
    case ColSWKochende:
        return data(row, ColSWKochbeginn);
    case ColSWAnstellen:
        return data(row, ColSWKochende);
    case ColSWJungbier:
    {
        double vg = data(row, ColVergaerungsgrad).toDouble();
        double unvergaerbar = row < swWzUnvergaerbarRecipe.size() ? swWzUnvergaerbarRecipe[row] : 0.0;
        return BierCalc::sreAusVergaerungsgrad(data(row, ColSWAnstellen).toDouble() - unvergaerbar, vg) + unvergaerbar;
    }
    default:
        return QVariant();
    }
}

bool ModelSud::setzeMesswert(int row, int schritt, int groesse, const QVariant &wert, int methode, const QString &bemerkung, const QVariant &sollwert)
{
    ModelMesswerte *mw = bh->modelMesswerte();
    const QVariant sudId = data(row, ColID);
    const int sudCol = ModelMesswerte::sudSpalte(schritt, groesse);
    if (wert.isValid() && !wert.isNull())
    {
        QVariant soll = sollwert;
        if (!soll.isValid())
        {
            int sollCol = ModelMesswerte::sollSpalte(schritt, groesse);
            if (sollCol >= 0)
                soll = data(row, sollCol);
        }
        mw->setze(sudId, schritt, groesse, wert.toDouble(), methode, soll, bemerkung);
        if (sudCol >= 0)
            setData(row, sudCol, wert);
        return true;
    }
    bool warGemessen = mw->entferne(sudId, schritt, groesse);
    if (warGemessen && sudCol >= 0)
    {
        Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(data(row, ColStatus).toInt());
        if (status == Brauhelfer::SudStatus::Rezept)
            setData(row, sudCol, istVorbelegung(row, sudCol));
    }
    return warGemessen;
}
