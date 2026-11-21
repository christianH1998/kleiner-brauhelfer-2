#include "dlgbrauuebersicht.h"
#include "ui_dlgbrauuebersicht.h"
#include "brauhelfer.h"
#include "settings.h"
#include "proxymodelsud.h"
#include "model/textdelegate.h"
#include "model/datedelegate.h"
#include "model/spinboxdelegate.h"
#include "model/doublespinboxdelegate.h"
#include "model/sudnamedelegate.h"

extern Brauhelfer* bh;
extern Settings* gSettings;

DlgBrauUebersicht* DlgBrauUebersicht::Dialog = nullptr;

class ProxyModelBrauuebersicht : public ProxyModelSud
{
public:
    ProxyModelBrauuebersicht(QObject* parent = nullptr) :
        ProxyModelSud(parent),
        mColAuswahl1(-1),
        mColAuswahl2(-1),
        mColAuswahl3(-1)
    {
    }

    using ProxyModelSud::data;

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE
    {
        if (role == Qt::BackgroundRole)
        {
            int col = index.column();
            if (col == mColAuswahl3)
                return gSettings->colorBackground3;
            if (col == mColAuswahl2)
                return gSettings->colorBackground2;
            if (col == mColAuswahl1)
                return gSettings->colorBackground1;
        }
        return ProxyModelSud::data(index, role);
    }

    int mColAuswahl1;
    int mColAuswahl2;
    int mColAuswahl3;
};

DlgBrauUebersicht::DlgBrauUebersicht(QWidget *parent) :
    DlgAbstract(staticMetaObject.className(), parent),
    ui(new Ui::DlgBrauUebersicht)
{
    ui->setupUi(this);

    ui->splitter->setSizes({INT_MAX, INT_MAX});

    ui->lblKostenEinheit->setText(QLocale().currencySymbol());
    ui->lblGesamtkostenEinheit->setText(QLocale().currencySymbol());

    ProxyModelBrauuebersicht *model = new ProxyModelBrauuebersicht(this);
    model->setSourceModel(bh->modelSud());
    model->sort(ModelSud::ColBraudatum, Qt::DescendingOrder);
    ui->table->setModel(model);

    modulesChanged(Settings::ModuleAlle);
    connect(gSettings, &Settings::modulesChanged, this, &DlgBrauUebersicht::modulesChanged);

    connect(model, &ProxyModel::layoutChanged, this, &DlgBrauUebersicht::onLayoutChanged);
    connect(model, &ProxyModel::rowsInserted, this, &DlgBrauUebersicht::onLayoutChanged);
    connect(model, &ProxyModel::rowsRemoved, this, &DlgBrauUebersicht::onLayoutChanged);
    connect(model, &ProxyModel::dataChanged, this, &DlgBrauUebersicht::modelDataChanged);
    connect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged,this, &DlgBrauUebersicht::table_selectionChanged);
    connect(ui->diagram, &Chart3::selectionChanged, this, &DlgBrauUebersicht::diagram_selectionChanged);
}

DlgBrauUebersicht::~DlgBrauUebersicht()
{
    delete ui;
}

void DlgBrauUebersicht::saveSettings()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->table->model());
    gSettings->beginGroup(staticMetaObject.className());
    gSettings->setValue("tableState", ui->table->horizontalHeader()->saveState());
    gSettings->setValue("Auswahl1", ui->cbAuswahlL1->currentIndex());
    gSettings->setValue("Auswahl2", ui->cbAuswahlL2->currentIndex());
    gSettings->setValue("Auswahl3", ui->cbAuswahlL3->currentIndex());
    gSettings->setValue("splitterState", ui->splitter->saveState());
    model->saveSetting(gSettings);
    gSettings->endGroup();
}

void DlgBrauUebersicht::loadSettings()
{
    ProxyModelSud *proxyModel = static_cast<ProxyModelSud*>(ui->table->model());
    gSettings->beginGroup(staticMetaObject.className());
    proxyModel->loadSettings(gSettings, {{"filterSudStatus", (ProxyModelSud::Abgefuellt | ProxyModelSud::Verbraucht).toInt()}});
    ui->table->restoreState(gSettings->value("tableState").toByteArray());
    ui->cbAuswahlL1->setCurrentIndex(gSettings->value("Auswahl1", 0).toInt());
    ui->cbAuswahlL2->setCurrentIndex(gSettings->value("Auswahl2", 0).toInt());
    ui->cbAuswahlL3->setCurrentIndex(gSettings->value("Auswahl3", 0).toInt());
    ui->splitter->restoreState(gSettings->value("splitterState").toByteArray());
    gSettings->endGroup();
    ui->btnFilter->setModel(proxyModel, FilterButtonSud::Braudatum | FilterButtonSud::Kategorie | FilterButtonSud::Anlage);
    onLayoutChanged();
}

void DlgBrauUebersicht::restoreView()
{
    DlgAbstract::restoreView(staticMetaObject.className(), Dialog);
    gSettings->beginGroup(staticMetaObject.className());
    gSettings->remove("tableState");
    gSettings->remove("splitterState");
    gSettings->endGroup();
}

void DlgBrauUebersicht::select(const QVariant &sudId)
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->table->model());
    ui->table->selectRow(model->getRowWithValue(ModelSud::ColID, sudId));
}

void DlgBrauUebersicht::modulesChanged(Settings::Modules modules)
{
    if (modules.testFlag(Settings::ModulePreiskalkulation))
    {
        gSettings->setVisibleModule(Settings::ModulePreiskalkulation,
                                    {ui->tbKosten,
                                     ui->lblKosten,
                                     ui->lblKostenEinheit,
                                     ui->tbGesamtkosten,
                                     ui->lblGesamtkosten,
                                     ui->lblGesamtkostenEinheit});
        build();
    }
}

void DlgBrauUebersicht::build()
{
    mAuswahlListe.clear();
    mAuswahlListe.append({-1, 0, tr("<keine>"), ""});
    mAuswahlListe.append({ModelSud::Colerg_AbgefuellteBiermenge, 1, tr("Abgefüllte Biermenge"), "L"});
    mAuswahlListe.append({ModelSud::Colerg_S_Gesamt, 2, tr("Schüttung"), "kg"});
    mAuswahlListe.append({ModelSud::ColSWIst, 1, tr("Stammwürze (Ist)"), "°P"});
    mAuswahlListe.append({ModelSud::Colerg_Sudhausausbeute, 0, tr("Sudhausausbeute (klassisch)"), "%"});
    mAuswahlListe.append({ModelSud::Colerg_EffektiveAusbeute, 0, tr("Effektive Ausbeute (Anstellwürze)"), "%"});
    mAuswahlListe.append({ModelSud::ColVerdampfungsrateIst, 1, tr("Verdampfungsrate"), "L/h"});
    mAuswahlListe.append({ModelSud::Colerg_Alkohol, 1, tr("Alkohol fertiges Bier"), "%vol"});
    mAuswahlListe.append({ModelSud::ColAlkoholJungbier, 1, tr("Alkohol Jungbier"), "%vol"});
    mAuswahlListe.append({ModelSud::ColSREIst, 1, tr("Scheinbarer Restextrakt"), "°P"});
    mAuswahlListe.append({ModelSud::ColsEVG, 0, tr("Scheinbarer Endvergärungsgrad"), "%"});
    mAuswahlListe.append({ModelSud::ColtEVG, 0, tr("Tatsächlicher Endvergärungsgrad"), "%"});
    if (gSettings->isModuleEnabled(Settings::ModulePreiskalkulation))
        mAuswahlListe.append({ModelSud::Colerg_Preis, 2, tr("Kosten"), QString("%1/L").arg(QLocale().currencySymbol())});
    // ext-s05: Soll-Werte zum Vergleich mit Ist, Gaerung/Hefe (Spalten standardmaessig ausgeblendet,
    // am Ende angehaengt, damit gespeicherte Auswahl-Indizes gueltig bleiben)
    mAuswahlListe.append({ModelSud::ColSW, 1, tr("Stammwürze Rezept (Soll)"), "°P", false});
    mAuswahlListe.append({ModelSud::ColWuerzemengeAnstellen, 1, tr("Anstellmenge (Ist)"), "L", false});
    mAuswahlListe.append({ModelSud::ColMengeSollAnstellen, 1, tr("Anstellmenge Rezept (Soll)"), "L", false});
    mAuswahlListe.append({ModelSud::ColSudhausausbeute, 1, tr("Ausbeute Rezept (Planung)"), "%", false});
    mAuswahlListe.append({ModelSud::ColVerdampfungsrate, 1, tr("Verdampfungsrate Rezept"), "L/h", false});
    mAuswahlListe.append({ModelSud::ColVergaerungsgrad, 0, tr("Endvergärungsgrad Rezept"), "%", false});
    mAuswahlListe.append({ModelSud::ColAlkoholSoll, 1, tr("Alkohol Rezept (Soll)"), "%vol", false});
    mAuswahlListe.append({ModelSud::ColDauerHauptgaerung, 0, tr("Dauer Hauptgärung"), tr("Tage"), false});
    mAuswahlListe.append({ModelSud::ColTemperaturHauptgaerung, 1, tr("Gärtemperatur (⌀ Hauptgärung)"), "°C", false});
    mAuswahlListe.append({ModelSud::ColTemperaturJungbier, 1, tr("Temperatur Jungbier (Abfüllen)"), "°C", false});

    ui->table->clearCols();
    ui->table->appendCol({ModelSud::ColSudname, true, false, 200, new SudNameDelegate(ui->table)});
    ui->table->appendCol({ModelSud::ColSudnummer, true, true, 80, new SpinBoxDelegate(ui->table)});
    ui->table->appendCol({ModelSud::ColKategorie, true, true, 100, new TextDelegate(false, Qt::AlignCenter, ui->table)});
    ui->table->appendCol({ModelSud::ColAnlage, true, true, 100, new TextDelegate(false, Qt::AlignCenter, ui->table)});
    ui->table->appendCol({ModelSud::ColBraudatum, true, false, 100, new DateDelegate(false, false, ui->table)});
    ui->table->appendCol({ModelSud::ColHefe, true, true, 160, new TextDelegate(false, Qt::AlignCenter, ui->table)});
    ui->cbAuswahlL1->clear();
    ui->cbAuswahlL2->clear();
    ui->cbAuswahlL3->clear();
    ui->cbAuswahlL2->addItem(mAuswahlListe[0].label);
    ui->cbAuswahlL3->addItem(mAuswahlListe[0].label);
    for (int i = 1; i < mAuswahlListe.count(); ++i)
    {
        ui->table->appendCol({mAuswahlListe[i].col, mAuswahlListe[i].sichtbar, true, 80, new DoubleSpinBoxDelegate(mAuswahlListe[i].precision, ui->table)});
        ui->cbAuswahlL1->addItem(mAuswahlListe[i].label);
        ui->cbAuswahlL2->addItem(mAuswahlListe[i].label);
        ui->cbAuswahlL3->addItem(mAuswahlListe[i].label);
    }
    ui->table->build();
    ui->table->setDefaultContextMenu();
}

void DlgBrauUebersicht::onLayoutChanged()
{
    updateDiagram();
    updateFilterLabel();
    updateStatistics();
}

void DlgBrauUebersicht::modelDataChanged(const QModelIndex& index)
{
    switch (index.column())
    {
    case ModelSud::ColStatus:
        ProxyModelBrauuebersicht *model = static_cast<ProxyModelBrauuebersicht*>(ui->table->model());
        model->invalidate();
        break;
    }
    onLayoutChanged();
}

void DlgBrauUebersicht::updateDiagram()
{
    ui->diagram->clear();
    ProxyModelBrauuebersicht *model = static_cast<ProxyModelBrauuebersicht*>(ui->table->model());
    if (model->rowCount() > 1)
    {
        int index = ui->cbAuswahlL1->currentIndex() + 1;
        if (index >= 0 && index < mAuswahlListe.size())
        {
            AuswahlType *auswahl1 = &mAuswahlListe[index];
            model->mColAuswahl1 = auswahl1->col;
            QVector<double> x(model->rowCount());
            QVector<double> y(model->rowCount());
            int i = 0;
            for (int row = model->rowCount() - 1; row >= 0; --row)
            {
                QDateTime dt = model->index(row, ModelSud::ColBraudatum).data().toDateTime();
                x[i] = dt.toSecsSinceEpoch();
                y[i] = model->index(row, auswahl1->col).data().toDouble();
                i++;
            }
            ui->diagram->setData1(x, y, auswahl1->label, auswahl1->unit, auswahl1->precision);
        }
        index = ui->cbAuswahlL2->currentIndex();
        if (index >= 0 && index < mAuswahlListe.size())
        {
            AuswahlType *auswahl2 = &mAuswahlListe[index];
            model->mColAuswahl2 = auswahl2->col;
            if (auswahl2->col >= 0)
            {
                QVector<double> x(model->rowCount());
                QVector<double> y(model->rowCount());
                int i = 0;
                for (int row = model->rowCount() - 1; row >= 0; --row)
                {
                    QDateTime dt = model->index(row, ModelSud::ColBraudatum).data().toDateTime();
                    x[i] = dt.toSecsSinceEpoch();
                    y[i] = model->index(row, auswahl2->col).data().toDouble();
                    i++;
                }
                ui->diagram->setData2(x, y, auswahl2->label, auswahl2->unit, auswahl2->precision);
            }
        }
        index = ui->cbAuswahlL3->currentIndex();
        if (index >= 0 && index < mAuswahlListe.size())
        {
            AuswahlType *auswahl3 = &mAuswahlListe[index];
            model->mColAuswahl3 = auswahl3->col;
            if (auswahl3->col >= 0)
            {
                QVector<double> x(model->rowCount());
                QVector<double> y(model->rowCount());
                int i = 0;
                for (int row = model->rowCount() - 1; row >= 0; --row)
                {
                    QDateTime dt = model->index(row, ModelSud::ColBraudatum).data().toDateTime();
                    x[i] = dt.toSecsSinceEpoch();
                    y[i] = model->index(row, auswahl3->col).data().toDouble();
                    i++;
                }
                ui->diagram->setData3(x, y, auswahl3->label, auswahl3->unit, auswahl3->precision);
            }
        }
    }
    table_selectionChanged(ui->table->selectionModel()->selection());
    ui->diagram->rescale();
    ui->diagram->replot();
}

void DlgBrauUebersicht::table_selectionChanged(const QItemSelection &selected)
{
    int index = -1;
    if (selected.indexes().count() > 0)
        index = ui->table->model()->rowCount() - 1 - selected.indexes()[0].row();
    ui->diagram->select(index);
}

void DlgBrauUebersicht::diagram_selectionChanged(int index)
{
    ui->table->selectRow(ui->table->model()->rowCount() - 1 - index);
    ui->table->setFocus();
}

void DlgBrauUebersicht::updateFilterLabel()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->table->model());
    ProxyModelSud proxy;
    proxy.setSourceModel(bh->modelSud());
    proxy.setFilterStatus(ProxyModelSud::Abgefuellt | ProxyModelSud::Verbraucht);
    ui->lblFilter->setText(QString::number(model->rowCount()) + " / " + QString::number(proxy.rowCount()));
}

void DlgBrauUebersicht::on_tbFilter_textChanged(const QString &pattern)
{
    static_cast<ProxyModelSud*>(ui->table->model())->setFilterText(pattern);
}

void DlgBrauUebersicht::on_cbAuswahlL1_currentIndexChanged(int)
{
    if (ui->cbAuswahlL1->hasFocus())
    {
        updateDiagram();
        ui->table->setFocus();
    }
}

void DlgBrauUebersicht::on_cbAuswahlL2_currentIndexChanged(int)
{
    if (ui->cbAuswahlL2->hasFocus())
    {
        updateDiagram();
        ui->table->setFocus();
    }
}

void DlgBrauUebersicht::on_cbAuswahlL3_currentIndexChanged(int)
{
    if (ui->cbAuswahlL3->hasFocus())
    {
        updateDiagram();
        ui->table->setFocus();
    }
}

void DlgBrauUebersicht::updateStatistics()
{
    ProxyModel* model = static_cast<ProxyModel*>(ui->table->model());
    ui->tbAnzahl->setValue(model->rowCount());
    ui->tbDatumVon->setDate(model->min(ModelSud::ColBraudatum).toDate());
    ui->tbDatumBis->setDate(model->max(ModelSud::ColBraudatum).toDate());
    ui->tbMenge->setValue(model->mean(ModelSud::Colerg_AbgefuellteBiermenge));
    ui->tbGesamtmenge->setValue(model->sum(ModelSud::Colerg_AbgefuellteBiermenge));
    ui->tbSchuettung->setValue(model->mean(ModelSud::Colerg_S_Gesamt));
    ui->tbGesamtschuettung->setValue(model->sum(ModelSud::Colerg_S_Gesamt));
    ui->tbStammwuerze->setValue(model->mean(ModelSud::ColSWIst));
    ui->tbRestextrakt->setValue(model->mean(ModelSud::ColSREIst));
    ui->tbVerdampfungsrate->setValue(model->mean(ModelSud::ColVerdampfungsrateIst));
    ui->tbSudhausaubeute->setValue(model->mean(ModelSud::Colerg_Sudhausausbeute));
    ui->tbEffSudhausausbeute->setValue(model->mean(ModelSud::Colerg_EffektiveAusbeute));
    ui->tbEndvergaerungsgrad->setValue(model->mean(ModelSud::ColsEVG));
    ui->tbAlkoholgehalt->setValue(model->mean(ModelSud::Colerg_Alkohol));
    ui->tbKosten->setValue(model->mean(ModelSud::Colerg_Preis));
    ui->tbGesamtkosten->setValue(model->sum(ModelSud::Colerg_Preis));
    updateHefeAuswertung();
}

void DlgBrauUebersicht::updateHefeAuswertung()
{
    // Kennzahlen je Hefe ueber die gefilterten Sude (ext-s05).
    // Mittelwerte nur ueber vorhandene Werte (0 = nicht erfasst); Anzahl in Klammern.
    struct Mittel
    {
        double summe = 0, min = 0, max = 0;
        int n = 0;
        void add(double v)
        {
            if (v <= 0.0)
                return;
            min = n == 0 ? v : qMin(min, v);
            max = n == 0 ? v : qMax(max, v);
            summe += v;
            ++n;
        }
        QString text(int dez, bool bereich = false) const
        {
            if (n == 0)
                return QStringLiteral("–");
            QString s = QLocale().toString(summe / n, 'f', dez);
            if (bereich && n > 1)
                s += QStringLiteral(" (%1–%2)").arg(QLocale().toString(min, 'f', dez), QLocale().toString(max, 'f', dez));
            return s + QStringLiteral(" [%1]").arg(n);
        }
    };
    struct HefeDaten
    {
        int sude = 0;
        Mittel sEVG, sre, sw, temperatur, dauer, tempJungbier, alkohol;
    };

    QMap<QString, HefeDaten> daten;
    ProxyModel* model = static_cast<ProxyModel*>(ui->table->model());
    for (int row = 0; row < model->rowCount(); ++row)
    {
        const QStringList hefen = model->data(row, ModelSud::ColHefe).toString().split(QStringLiteral(", "), Qt::SkipEmptyParts);
        for (const QString& hefe : hefen)
        {
            HefeDaten& d = daten[hefe];
            d.sude++;
            d.sEVG.add(model->data(row, ModelSud::ColsEVG).toDouble());
            d.sre.add(model->data(row, ModelSud::ColSREIst).toDouble());
            d.sw.add(model->data(row, ModelSud::ColSWIst).toDouble());
            d.temperatur.add(model->data(row, ModelSud::ColTemperaturHauptgaerung).toDouble());
            d.dauer.add(model->data(row, ModelSud::ColDauerHauptgaerung).toDouble());
            d.tempJungbier.add(model->data(row, ModelSud::ColTemperaturJungbier).toDouble());
            d.alkohol.add(model->data(row, ModelSud::Colerg_Alkohol).toDouble());
        }
    }

    const QStringList kopf = {tr("Hefe"), tr("Sude"), tr("Endvergärungsgrad\n(scheinbar, %)"), tr("Restextrakt\n(°P)"),
                              tr("Stammwürze\n(°P)"), tr("Gärtemperatur\n(⌀ °C)"), tr("Hauptgärung\n(Tage)"),
                              tr("Temperatur Jungbier\n(°C)"), tr("Alkohol fertig\n(%vol)"), tr("EVG laut Hersteller")};
    ui->tableHefe->clear();
    ui->tableHefe->setColumnCount(kopf.size());
    ui->tableHefe->setHorizontalHeaderLabels(kopf);
    ui->tableHefe->setRowCount(daten.size());
    int r = 0;
    for (auto it = daten.constBegin(); it != daten.constEnd(); ++it, ++r)
    {
        const HefeDaten& d = it.value();
        QString evgHersteller = bh->modelHefe()->getValueFromSameRow(ModelHefe::ColName, it.key(), ModelHefe::ColEVG).toString();
        const QStringList zellen = {it.key(), QString::number(d.sude), d.sEVG.text(0, true), d.sre.text(1, true),
                                    d.sw.text(1), d.temperatur.text(1, true), d.dauer.text(0, true),
                                    d.tempJungbier.text(1), d.alkohol.text(1), evgHersteller};
        for (int c = 0; c < zellen.size(); ++c)
        {
            QTableWidgetItem* item = new QTableWidgetItem(zellen[c]);
            item->setTextAlignment(c == 0 ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignCenter);
            ui->tableHefe->setItem(r, c, item);
        }
    }
    ui->tableHefe->resizeColumnsToContents();
}

void DlgBrauUebersicht::on_tableHefe_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column)
    QTableWidgetItem* item = ui->tableHefe->item(row, 0);
    if (item)
        ui->tbFilter->setText(item->text());
}
