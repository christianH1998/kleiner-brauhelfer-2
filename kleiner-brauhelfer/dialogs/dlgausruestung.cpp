#include "dlgausruestung.h"
#include "ui_dlgausruestung.h"
#include <QKeyEvent>
#include <QMessageBox>
#include <QScrollBar>
#include "brauhelfer.h"
#include "settings.h"
#include "proxymodelsud.h"
#include "mainwindow.h"
#include "model/textdelegate.h"
#include "model/spinboxdelegate.h"
#include "model/doublespinboxdelegate.h"
#include "model/checkboxdelegate.h"
#include "model/datedelegate.h"
#include "model/sudnamedelegate.h"
#include "dialogs/dlgverdampfung.h"
#include "biercalc.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>

extern Brauhelfer* bh;
extern Settings* gSettings;

DlgAusruestung* DlgAusruestung::Dialog = nullptr;

DlgAusruestung::DlgAusruestung(QWidget *parent) :
    DlgAbstract(staticMetaObject.className(), parent),
    ui(new Ui::DlgAusruestung),
    mRow(0)
{
    ui->setupUi(this);
    for (const auto& type : std::as_const(MainWindow::AnlageTypname))
        ui->cbTyp->addItem(type.first, type.second);
    ui->lblCurrency->setText(QLocale().currencySymbol());

    TableView *table = ui->tableViewAnlagen;
    ProxyModel *proxyModel = new ProxyModel(this);
    proxyModel->setSourceModel(bh->modelAusruestung());
    table->setModel(proxyModel);
    table->appendCol({ModelAusruestung::ColName, true, false, -1, new TextDelegate(true, Qt::AlignLeft | Qt::AlignVCenter, table)});
    table->appendCol({ModelAusruestung::ColVermoegen, true, true, 100, new DoubleSpinBoxDelegate(1, table)});
    table->appendCol({ModelAusruestung::ColAnzahlSude, true, true, 100, new SpinBoxDelegate(table)});
    table->appendCol({ModelAusruestung::ColAnzahlGebrauteSude, true, true, 100, new SpinBoxDelegate(table)});
    table->build();
    table->setDefaultContextMenu(false, false);

    table = ui->tableViewGeraete;
    proxyModel = new ProxyModel(this);
    proxyModel->setSourceModel(bh->modelGeraete());
    proxyModel->setFilterKeyColumn(ModelGeraete::ColAusruestungAnlagenID);
    table->setModel(proxyModel);
    table->appendCol({ModelGeraete::ColBezeichnung, true, false, -1, new TextDelegate(table)});
    table->build();
    table->setDefaultContextMenu();

    table = ui->tableViewSude;
    proxyModel = new ProxyModelSud(this);
    proxyModel->setSourceModel(bh->modelSud());
    proxyModel->setFilterKeyColumn(ModelSud::ColAnlage);
    table->setModel(proxyModel);
    table->appendCol({ModelSud::ColSudname, true, false, 200, new SudNameDelegate(true, Qt::AlignLeft | Qt::AlignVCenter, table)});
    table->appendCol({ModelSud::ColSudnummer, true, true, 80, new SpinBoxDelegate(table)});
    table->appendCol({ModelSud::ColKategorie, true, true, 100, new TextDelegate(false, Qt::AlignCenter, table)});
    table->appendCol({ModelSud::ColBraudatum, true, true, 100, new DateDelegate(false, true, table)});
    table->appendCol({ModelSud::Colerg_EffektiveAusbeute, true, false, 100, new DoubleSpinBoxDelegate(1, table)});
    table->appendCol({ModelSud::ColVerdampfungsrateIst, true, false, 100, new DoubleSpinBoxDelegate(1, table)});
    table->appendCol({ModelSud::ColAusbeuteIgnorieren, true, false, 150, new CheckBoxDelegate(table)});
    table->build();
    table->setDefaultContextMenu(false, false);

    ui->splitterVertical->setSizes({100, 100});
    ui->splitterLeft->setSizes({200, 100, 50});

    modulesChanged(Settings::ModuleAlle);
    connect(gSettings, &Settings::modulesChanged, this, &DlgAusruestung::modulesChanged);

    connect(bh->modelSud(), &ModelSud::modified, this, &DlgAusruestung::updateDurchschnitt);
    connect(bh->modelAusruestung(), &ModelAusruestung::modified, this, &DlgAusruestung::updateValues);
    connect(ui->tableViewAnlagen->selectionModel(), &QItemSelectionModel::selectionChanged, this, &DlgAusruestung::anlage_selectionChanged);

    connect(ui->wdgBemerkung, &WdgBemerkung::changed, this, [this](const QString& html){setData(ModelAusruestung::ColBemerkung, html);});
}

DlgAusruestung::~DlgAusruestung()
{
    delete ui;
}

void DlgAusruestung::saveSettings()
{
    gSettings->beginGroup(staticMetaObject.className());
    gSettings->setValue("tableStateAnlagen", ui->tableViewAnlagen->horizontalHeader()->saveState());
    gSettings->setValue("tableStateGeraete", ui->tableViewGeraete->horizontalHeader()->saveState());
    gSettings->setValue("tableStateSude", ui->tableViewSude->horizontalHeader()->saveState());
    gSettings->setValue("splitterStateLeft", ui->splitterLeft->saveState());
    gSettings->setValue("splitterStateVertical", ui->splitterVertical->saveState());
    gSettings->setValue("AnzahlDurchschnitt", ui->sliderAusbeuteSude->value());
    gSettings->endGroup();
}

void DlgAusruestung::loadSettings()
{
    gSettings->beginGroup(staticMetaObject.className());
    ui->tableViewAnlagen->restoreState(gSettings->value("tableStateAnlagen").toByteArray());
    ui->tableViewGeraete->restoreState(gSettings->value("tableStateGeraete").toByteArray());
    ui->tableViewSude->restoreState(gSettings->value("tableStateSude").toByteArray());
    ui->splitterLeft->restoreState(gSettings->value("splitterStateLeft").toByteArray());
    ui->splitterVertical->restoreState(gSettings->value("splitterStateVertical").toByteArray());
    ui->sliderAusbeuteSude->setValue(gSettings->value("AnzahlDurchschnitt").toInt());
    gSettings->endGroup();
    ui->tableViewAnlagen->selectRow(0);
}

void DlgAusruestung::restoreView()
{
    DlgAbstract::restoreView(staticMetaObject.className(), Dialog);
    gSettings->beginGroup(staticMetaObject.className());
    gSettings->remove("tableStateAnlagen");
    gSettings->remove("tableStateGeraete");
    gSettings->remove("tableStateSude");
    gSettings->remove("splitterState");
    gSettings->remove("splitterStateSude");
    gSettings->endGroup();
}

void DlgAusruestung::modulesChanged(Settings::Modules modules)
{
    if (modules.testFlag(Settings::ModulePreiskalkulation))
    {
        gSettings->setVisibleModule(Settings::ModulePreiskalkulation,
                                      {ui->tbKosten,
                                       ui->lblKosten,
                                       ui->lblCurrency});
    }
}

void DlgAusruestung::keyPressEvent(QKeyEvent* event)
{
    DlgAbstract::keyPressEvent(event);
    if (ui->tableViewAnlagen->hasFocus())
    {
        switch (event->key())
        {
        case Qt::Key::Key_Delete:
            on_btnAnlageLoeschen_clicked();
            break;
        case Qt::Key::Key_Insert:
            on_btnNeueAnlage_clicked();
            break;
        }
    }
    else if (ui->tableViewGeraete->hasFocus())
    {
        switch (event->key())
        {
        case Qt::Key::Key_Delete:
            on_btnGeraetLoeschen_clicked();
            break;
        case Qt::Key::Key_Insert:
            on_btnNeuesGeraet_clicked();
            break;
        }
    }
}

void DlgAusruestung::select(const QString &name)
{
    ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewAnlagen->model());
    ui->tableViewAnlagen->selectRow(model->getRowWithValue(ModelAusruestung::ColName, name));
}

void DlgAusruestung::anlage_selectionChanged()
{
    QRegularExpression regExpId;
    QRegularExpression regExpId2;
    QModelIndexList selectedRows = ui->tableViewAnlagen->selectionModel()->selectedRows();
    if (selectedRows.count() > 0)
    {
        mRow = selectedRows[0].row();
        ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewAnlagen->model());
        QString anlage = model->data(model->index(mRow, ModelAusruestung::ColName)).toString();
        regExpId = QRegularExpression(QStringLiteral("^%1$").arg(QRegularExpression::escape(anlage)));
        regExpId2 = QRegularExpression(QStringLiteral("^%1$").arg(model->data(model->index(mRow, ModelAusruestung::ColID)).toInt()));
        ui->scrollArea->setEnabled(true);
    }
    else
    {
        regExpId = QRegularExpression(QStringLiteral("--dummy--"));
        regExpId2 = QRegularExpression(QStringLiteral("--dummy--"));
        ui->scrollArea->setEnabled(false);
    }
    static_cast<ProxyModel*>(ui->tableViewGeraete->model())->setFilterRegularExpression(regExpId2);
    static_cast<ProxyModel*>(ui->tableViewSude->model())->setFilterRegularExpression(regExpId);
    ui->sliderAusbeuteSude->setMaximum(9999);
    if (ui->sliderAusbeuteSude->value() == 0)
        ui->sliderAusbeuteSude->setValue(9999);
    updateValues();
    updateDurchschnitt();
}

void DlgAusruestung::on_btnNeueAnlage_clicked()
{
    QMap<int, QVariant> values({{ModelAusruestung::ColName, tr("Neue Brauanlage")}});
    ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewAnlagen->model());
    int row = model->append(values);
    if (row >= 0)
    {
        const QModelIndex index = model->index(row, ModelAusruestung::ColName);
        ui->tableViewAnlagen->setCurrentIndex(index);
        ui->tableViewAnlagen->scrollTo(ui->tableViewAnlagen->currentIndex());
        ui->tbName->selectAll();
        ui->tbName->setFocus();
        ui->scrollArea->verticalScrollBar()->setValue(0);
    }
}

void DlgAusruestung::on_btnAnlageLoeschen_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableViewAnlagen->model());
    QModelIndexList indices = ui->tableViewAnlagen->selectionModel()->selectedRows();
    std::sort(indices.begin(), indices.end(), [](const QModelIndex & a, const QModelIndex & b){ return a.row() > b.row(); });
    for (const QModelIndex& index : std::as_const(indices))
    {
        QString name = model->data(index.row(), ModelAusruestung::ColName).toString();
        int ret = QMessageBox::question(this, tr("Brauanlage löschen?"),
                                        tr("Soll die Brauanlage \"%1\" gelöscht werden?").arg(name));
        if (ret == QMessageBox::Yes)
        {
            ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewAnlagen->model());
            int row = model->getRowWithValue(ModelAusruestung::ColName, name);
            model->removeRow(row);
        }
    }
}

void DlgAusruestung::on_btnNeuesGeraet_clicked()
{
    QModelIndexList selected = ui->tableViewAnlagen->selectionModel()->selectedRows();
    if (selected.count() > 0)
    {
        QMap<int, QVariant> values({{ModelGeraete::ColAusruestungAnlagenID, data(ModelAusruestung::ColID)},
                                    {ModelGeraete::ColBezeichnung, tr("Neues Gerät")}});
        ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewGeraete->model());
        int row = model->append(values);
        if (row >= 0)
        {
            const QModelIndex index = model->index(row, ModelGeraete::ColBezeichnung);
            ui->tableViewGeraete->setCurrentIndex(index);
            ui->tableViewGeraete->scrollTo(ui->tableViewGeraete->currentIndex());
            ui->tableViewGeraete->edit(ui->tableViewGeraete->currentIndex());
        }
    }
}

void DlgAusruestung::on_btnGeraetLoeschen_clicked()
{
    ProxyModelSud *model = static_cast<ProxyModelSud*>(ui->tableViewGeraete->model());
    QModelIndexList indices = ui->tableViewGeraete->selectionModel()->selectedRows();
    std::sort(indices.begin(), indices.end(), [](const QModelIndex & a, const QModelIndex & b){ return a.row() > b.row(); });
    for (const QModelIndex& index : std::as_const(indices))
    {
        QString name = model->data(index.row(), ModelGeraete::ColBezeichnung).toString();
        int ret = QMessageBox::question(this, tr("Gerät löschen?"),
                                        tr("Soll das Gerät \"%1\" gelöscht werden?").arg(name));
        if (ret == QMessageBox::Yes)
        {
            int row = model->getRowWithValue(ModelGeraete::ColBezeichnung, name);
            model->removeRow(row);
        }
    }
}

QVariant DlgAusruestung::data(int col) const
{
    ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewAnlagen->model());
    return model->data(mRow, col);
}

bool DlgAusruestung::setData(int col, const QVariant &value)
{
    ProxyModel *model = static_cast<ProxyModel*>(ui->tableViewAnlagen->model());
    return model->setData(mRow, col, value);
}

void DlgAusruestung::updateValues()
{
    Brauhelfer::AnlageTyp typ = static_cast<Brauhelfer::AnlageTyp>(data(ModelAusruestung::ColTyp).toInt());
    if (!ui->tbName->hasFocus())
        ui->tbName->setText(data(ModelAusruestung::ColName).toString());
    if (!ui->cbTyp->hasFocus())
        ui->cbTyp->setCurrentIndex(ui->cbTyp->findData(data(ModelAusruestung::ColTyp)));
    if (!ui->tbAusbeute->hasFocus())
        ui->tbAusbeute->setValue(data(ModelAusruestung::ColSudhausausbeute).toDouble());
    if (!ui->tbVerdampfung->hasFocus())
        ui->tbVerdampfung->setValue(data(ModelAusruestung::ColVerdampfungsrate).toDouble());
    if (!ui->tbKorrekturNachguss->hasFocus())
        ui->tbKorrekturNachguss->setValue(data(ModelAusruestung::ColKorrekturWasser).toDouble());
    if (!ui->tbTreberaufnahme->hasFocus())
        ui->tbTreberaufnahme->setValue(data(ModelAusruestung::ColTreberaufnahme).isNull() ? 0.96 : data(ModelAusruestung::ColTreberaufnahme).toDouble());
    ui->tbTreberaufnahme->setEnabled(typ != Brauhelfer::AnlageTyp::GrainfatherG30 && typ != Brauhelfer::AnlageTyp::BrauheldPro30);
    if (!ui->tbWhirlpoolFaktor->hasFocus())
    {
        QVariant f = data(ModelAusruestung::ColWhirlpoolFaktor);
        ui->tbWhirlpoolFaktor->setValue((f.isNull() || f.toDouble() <= 0.0) ? 1.0 : f.toDouble());
    }
    if (!ui->tbKorrekturFarbe->hasFocus())
        ui->tbKorrekturFarbe->setValue(data(ModelAusruestung::ColKorrekturFarbe).toInt());
    if (!ui->tbKorrekturSollmenge->hasFocus())
        ui->tbKorrekturSollmenge->setValue(data(ModelAusruestung::ColKorrekturMenge).toDouble());
    if (!ui->tbKosten->hasFocus())
        ui->tbKosten->setValue(data(ModelAusruestung::ColKosten).toDouble());
    if (!ui->tbMaischebottichHoehe->hasFocus())
        ui->tbMaischebottichHoehe->setValue(data(ModelAusruestung::ColMaischebottich_Hoehe).toDouble());
    ui->tbMaischebottichHoehe->setReadOnly(typ != Brauhelfer::AnlageTyp::Standard);
    if (!ui->tbMaischebottichDurchmesser->hasFocus())
        ui->tbMaischebottichDurchmesser->setValue(data(ModelAusruestung::ColMaischebottich_Durchmesser).toDouble());
    ui->tbMaischebottichDurchmesser->setReadOnly(typ != Brauhelfer::AnlageTyp::Standard);
    ui->tbMaischebottichMaxFuellhoehe->setMaximum(data(ModelAusruestung::ColMaischebottich_Hoehe).toDouble());
    if (!ui->tbMaischebottichMaxFuellhoehe->hasFocus())
        ui->tbMaischebottichMaxFuellhoehe->setValue(data(ModelAusruestung::ColMaischebottich_MaxFuellhoehe).toDouble());
    ui->tbMaischebottichMaxFuellhoehe->setReadOnly(typ != Brauhelfer::AnlageTyp::Standard);
    if (!ui->tbSudpfanneHoehe->hasFocus())
        ui->tbSudpfanneHoehe->setValue(data(ModelAusruestung::ColSudpfanne_Hoehe).toDouble());
    ui->tbSudpfanneHoehe->setReadOnly(typ != Brauhelfer::AnlageTyp::Standard);
    if (!ui->tbSudpfanneDurchmesser->hasFocus())
        ui->tbSudpfanneDurchmesser->setValue(data(ModelAusruestung::ColSudpfanne_Durchmesser).toDouble());
    ui->tbSudpfanneDurchmesser->setReadOnly(typ != Brauhelfer::AnlageTyp::Standard);
    ui->tbSudpfanneMaxFuellhoehe->setMaximum(data(ModelAusruestung::ColSudpfanne_Hoehe).toDouble());
    if (!ui->tbSudpfanneMaxFuellhoehe->hasFocus())
        ui->tbSudpfanneMaxFuellhoehe->setValue(data(ModelAusruestung::ColSudpfanne_MaxFuellhoehe).toDouble());
    ui->tbSudpfanneMaxFuellhoehe->setReadOnly(typ != Brauhelfer::AnlageTyp::Standard);
    ui->tbMaischenVolumen->setValue(data(ModelAusruestung::ColMaischebottich_Volumen).toDouble());
    ui->tbMaischenMaxNutzvolumen->setValue(data(ModelAusruestung::ColMaischebottich_MaxFuellvolumen).toDouble());
    ui->tbSudpfanneVolumen->setValue(data(ModelAusruestung::ColSudpfanne_Volumen).toDouble());
    ui->tbSudpfanneMaxNutzvolumen->setValue(data(ModelAusruestung::ColSudpfanne_MaxFuellvolumen).toDouble());
    ui->wdgBemerkung->setHtml(data(ModelAusruestung::ColBemerkung).toString());
}

void DlgAusruestung::updateDurchschnitt()
{
    QString anlage = data(ModelAusruestung::ColName).toString();
    ProxyModelSud modelSud;
    modelSud.setSourceModel(bh->modelSud());
    QRegularExpression regExp(QStringLiteral("^%1$").arg(QRegularExpression::escape(anlage)));
    modelSud.setFilterKeyColumn(ModelSud::ColAnlage);
    modelSud.setFilterRegularExpression(regExp);
    modelSud.setFilterStatus(ProxyModelSud::Gebraut | ProxyModelSud::Abgefuellt | ProxyModelSud::Verbraucht);
    modelSud.sort(ModelSud::ColBraudatum, Qt::DescendingOrder);
    int nAusbeute = 0;
    int nVerdampfung = 0;
    int N = 0;
    double ausbeute = 0.0;
    double verdampfung = 0.0;
    for (int i = 0; i < modelSud.rowCount(); ++i)
    {
        if (!modelSud.index(i, ModelSud::ColAusbeuteIgnorieren).data().toBool())
        {
            if (N < ui->sliderAusbeuteSude->value())
            {
                double val = modelSud.index(i, ModelSud::Colerg_EffektiveAusbeute).data().toDouble();
                if (val > 0)
                {
                    ausbeute += val;
                    nAusbeute++;
                }
                val = modelSud.index(i, ModelSud::ColVerdampfungsrateIst).data().toDouble();
                if (val > 0)
                {
                    verdampfung += val;
                    ++nVerdampfung;
                }
            }
            ++N;
        }
    }
    if (nAusbeute > 0)
        ausbeute /= nAusbeute;
    if (nVerdampfung > 0)
        verdampfung /= nVerdampfung;
    ui->sliderAusbeuteSude->setMaximum(N);
    ui->sliderAusbeuteSude->setEnabled(N > 1);
    ui->tbAusbeuteMittel->setValue(ausbeute);
    ui->tbVerdampfungMittel->setValue(verdampfung);
    ui->tbAusbeuteSude->setValue(ui->sliderAusbeuteSude->value());
}

void DlgAusruestung::on_tbName_editingFinished()
{
    QString prevValue = data(ModelAusruestung::ColName).toString();
    if (prevValue != ui->tbName->text())
    {
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
        setData(ModelAusruestung::ColName, ui->tbName->text());
        QGuiApplication::restoreOverrideCursor();
    }
}

void DlgAusruestung::on_cbTyp_activated(int index)
{
    int prevValue = data(ModelAusruestung::ColTyp).toInt();
    if (prevValue != ui->cbTyp->itemData(index).toInt())
    {
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
        setData(ModelAusruestung::ColTyp, ui->cbTyp->itemData(index));
        QGuiApplication::restoreOverrideCursor();
    }
}

void DlgAusruestung::on_btnVerdampfungsrate_clicked()
{
    DlgVerdampfung dlg;
    dlg.setDurchmesser(data(ModelAusruestung::ColSudpfanne_Durchmesser).toDouble());
    dlg.setHoehe(data(ModelAusruestung::ColSudpfanne_Hoehe).toDouble());
    dlg.setKochdauer(90);
    dlg.setMenge1(20);
    dlg.setMenge2(18);
    if (dlg.exec() == QDialog::Accepted)
    {
        setData(ModelAusruestung::ColVerdampfungsrate, dlg.getVerdampfungsrate());
    }
}

void DlgAusruestung::on_tbAusbeute_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColSudhausausbeute).toDouble();
    if (prevValue != ui->tbAusbeute->value())
        setData(ModelAusruestung::ColSudhausausbeute, ui->tbAusbeute->value());
}

void DlgAusruestung::on_btnAusbeuteMittel_clicked()
{
    double prevValue = data(ModelAusruestung::ColSudhausausbeute).toDouble();
    if (prevValue != ui->tbAusbeuteMittel->value())
        setData(ModelAusruestung::ColSudhausausbeute, ui->tbAusbeuteMittel->value());
}

void DlgAusruestung::on_tbVerdampfung_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColVerdampfungsrate).toDouble();
    if (prevValue != ui->tbVerdampfung->value())
        setData(ModelAusruestung::ColVerdampfungsrate, ui->tbVerdampfung->value());
}

void DlgAusruestung::on_btnVerdampfungMittel_clicked()
{
    double prevValue = data(ModelAusruestung::ColVerdampfungsrate).toDouble();
    if (prevValue != ui->tbVerdampfungMittel->value())
        setData(ModelAusruestung::ColVerdampfungsrate, ui->tbVerdampfungMittel->value());
}

void DlgAusruestung::on_sliderAusbeuteSude_valueChanged(int)
{
    updateDurchschnitt();
}

void DlgAusruestung::on_tbKorrekturNachguss_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColKorrekturWasser).toDouble();
    if (prevValue != ui->tbKorrekturNachguss->value())
    {
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
        setData(ModelAusruestung::ColKorrekturWasser, ui->tbKorrekturNachguss->value());
        QGuiApplication::restoreOverrideCursor();
    }
}

void DlgAusruestung::on_tbTreberaufnahme_editingFinished()
{
    // ext-s10: Treberaufnahme der Anlage (bisher fest 0,96 L/kg)
    double prevValue = data(ModelAusruestung::ColTreberaufnahme).toDouble();
    if (prevValue != ui->tbTreberaufnahme->value())
    {
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
        setData(ModelAusruestung::ColTreberaufnahme, ui->tbTreberaufnahme->value());
        QGuiApplication::restoreOverrideCursor();
    }
}

void DlgAusruestung::on_tbWhirlpoolFaktor_editingFinished()
{
    // ext-s11: Korrekturfaktor Waermeverlust Whirlpool/Absetzen
    double prevValue = data(ModelAusruestung::ColWhirlpoolFaktor).toDouble();
    if (prevValue != ui->tbWhirlpoolFaktor->value())
    {
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
        setData(ModelAusruestung::ColWhirlpoolFaktor, ui->tbWhirlpoolFaktor->value());
        QGuiApplication::restoreOverrideCursor();
    }
}

void DlgAusruestung::on_btnWhirlpoolKalibrieren_clicked()
{
    // ext-s11: Faktor so bestimmen, dass die berechnete Temperatur am Ende der Absetzzeit der
    // gemessenen entspricht (Bisektion, Endtemperatur faellt mit steigendem Faktor).
    double durchmesser = data(ModelAusruestung::ColSudpfanne_Durchmesser).toDouble();
    if (durchmesser <= 0.0)
    {
        QMessageBox::information(this, tr("Abkühlfaktor kalibrieren"),
                                 tr("Bitte zuerst den Durchmesser der Sudpfanne eintragen. Das Modell nimmt einen runden Topf an."));
        return;
    }
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Abkühlfaktor kalibrieren"));
    QFormLayout* form = new QFormLayout(&dlg);
    QLabel* info = new QLabel(tr("Werte eines gebrauten Suds eintragen: Würzemenge nach Kochende, Dauer von Kochende bis zur Messung, "
                                 "Außentemperatur, Deckel und die gemessene Temperatur der Würze. Sudpfanne: Durchmesser %1 cm.")
                              .arg(QLocale().toString(durchmesser, 'f', 1)), &dlg);
    info->setWordWrap(true);
    form->addRow(info);
    auto spin = [&](double min, double max, double wert, int dez, const QString& einheit) {
        QDoubleSpinBox* sb = new QDoubleSpinBox(&dlg);
        sb->setRange(min, max);
        sb->setDecimals(dez);
        sb->setValue(wert);
        sb->setSuffix(QStringLiteral(" ") + einheit);
        return sb;
    };
    double volVorschlag = data(ModelAusruestung::ColSudpfanne_MaxFuellvolumen).toDouble() * 0.7;
    QDoubleSpinBox* sbMenge = spin(1, 10000, volVorschlag > 0 ? volVorschlag : 20.0, 1, QStringLiteral("L"));
    QDoubleSpinBox* sbDauer = spin(1, 600, 30, 0, QStringLiteral("min"));
    QDoubleSpinBox* sbAussen = spin(-30, 50, 20, 1, QStringLiteral("°C"));
    QComboBox* cbDeckel = new QComboBox(&dlg);
    cbDeckel->addItems({tr("offen"), tr("mit Deckel")});
    QDoubleSpinBox* sbGemessen = spin(0, 100, 85, 1, QStringLiteral("°C"));
    form->addRow(tr("Würzemenge nach Kochende"), sbMenge);
    form->addRow(tr("Zeit von Kochende bis Messung"), sbDauer);
    form->addRow(tr("Außentemperatur"), sbAussen);
    form->addRow(tr("Topf"), cbDeckel);
    form->addRow(tr("Gemessene Temperatur"), sbGemessen);
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);
    if (dlg.exec() != QDialog::Accepted)
        return;
    const double menge = sbMenge->value(), dauer = sbDauer->value(), aussen = sbAussen->value(), gemessen = sbGemessen->value();
    const bool deckel = cbDeckel->currentIndex() == 1;
    auto tEnde = [&](double f) {
        double t = 100.0;
        BierCalc::whirlpoolMinuten(menge, durchmesser, aussen, deckel, f, 0.0, dauer, &t);
        return t;
    };
    double lo = 0.05, hi = 20.0;
    if (gemessen > tEnde(lo) || gemessen < tEnde(hi))
    {
        QMessageBox::warning(this, tr("Abkühlfaktor kalibrieren"),
                             tr("Die gemessene Temperatur liegt außerhalb dessen, was das Modell abbilden kann "
                                "(%1 bis %2 °C). Bitte die Eingaben prüfen.")
                             .arg(QLocale().toString(tEnde(hi), 'f', 1), QLocale().toString(tEnde(lo), 'f', 1)));
        return;
    }
    for (int i = 0; i < 60; ++i)
    {
        double mid = 0.5 * (lo + hi);
        if (tEnde(mid) > gemessen)
            lo = mid;
        else
            hi = mid;
    }
    double faktor = 0.5 * (lo + hi);
    QMessageBox::information(this, tr("Abkühlfaktor kalibrieren"),
                             tr("Ohne Korrektur berechnet das Modell %1 °C, gemessen wurden %2 °C.\nNeuer Abkühlfaktor: %3")
                             .arg(QLocale().toString(tEnde(1.0), 'f', 1), QLocale().toString(gemessen, 'f', 1), QLocale().toString(faktor, 'f', 2)));
    ui->tbWhirlpoolFaktor->setValue(faktor);
    setData(ModelAusruestung::ColWhirlpoolFaktor, faktor);
}

void DlgAusruestung::on_tbKorrekturFarbe_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColKorrekturFarbe).toDouble();
    if (prevValue != ui->tbKorrekturFarbe->value())
    {
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
        setData(ModelAusruestung::ColKorrekturFarbe, ui->tbKorrekturFarbe->value());
        QGuiApplication::restoreOverrideCursor();
    }
}

void DlgAusruestung::on_tbKorrekturSollmenge_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColKorrekturMenge).toDouble();
    if (prevValue != ui->tbKorrekturSollmenge->value())
    {
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
        setData(ModelAusruestung::ColKorrekturMenge, ui->tbKorrekturSollmenge->value());
        QGuiApplication::restoreOverrideCursor();
    }
}

void DlgAusruestung::on_tbKosten_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColKosten).toDouble();
    if (prevValue != ui->tbKosten->value())
    {
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
        setData(ModelAusruestung::ColKosten, ui->tbKosten->value());
        QGuiApplication::restoreOverrideCursor();
    }
}

void DlgAusruestung::on_tbMaischebottichHoehe_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColMaischebottich_Hoehe).toDouble();
    if (prevValue != ui->tbMaischebottichHoehe->value())
        setData(ModelAusruestung::ColMaischebottich_Hoehe, ui->tbMaischebottichHoehe->value());
}

void DlgAusruestung::on_tbMaischebottichDurchmesser_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColMaischebottich_Durchmesser).toDouble();
    if (prevValue != ui->tbMaischebottichDurchmesser->value())
        setData(ModelAusruestung::ColMaischebottich_Durchmesser, ui->tbMaischebottichDurchmesser->value());
}

void DlgAusruestung::on_tbMaischebottichMaxFuellhoehe_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColMaischebottich_MaxFuellhoehe).toDouble();
    if (prevValue != ui->tbMaischebottichMaxFuellhoehe->value())
        setData(ModelAusruestung::ColMaischebottich_MaxFuellhoehe, ui->tbMaischebottichMaxFuellhoehe->value());
}

void DlgAusruestung::on_tbSudpfanneHoehe_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColSudpfanne_Hoehe).toDouble();
    if (prevValue != ui->tbSudpfanneHoehe->value())
        setData(ModelAusruestung::ColSudpfanne_Hoehe, ui->tbSudpfanneHoehe->value());
}

void DlgAusruestung::on_tbSudpfanneDurchmesser_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColSudpfanne_Durchmesser).toDouble();
    if (prevValue != ui->tbSudpfanneDurchmesser->value())
        setData(ModelAusruestung::ColSudpfanne_Durchmesser, ui->tbSudpfanneDurchmesser->value());
}

void DlgAusruestung::on_tbSudpfanneMaxFuellhoehe_editingFinished()
{
    double prevValue = data(ModelAusruestung::ColSudpfanne_MaxFuellhoehe).toDouble();
    if (prevValue != ui->tbSudpfanneMaxFuellhoehe->value())
        setData(ModelAusruestung::ColSudpfanne_MaxFuellhoehe, ui->tbSudpfanneMaxFuellhoehe->value());
}
