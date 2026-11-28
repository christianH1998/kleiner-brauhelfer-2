#include "tababfuellen.h"
#include "ui_tababfuellen.h"
#include <QMessageBox>
#include <qmath.h>
#include "brauhelfer.h"
#include "biercalc.h"
#include "settings.h"
#include "dialogs/dlgrestextrakt.h"
#include "widgets/widgetdecorator.h"
#include "commands/undostack.h"
#include "commands/setmesswertcommand.h"

extern Settings* gSettings;

TabAbfuellen::TabAbfuellen(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TabAbfuellen),
    mSud(nullptr)
{
    ui->setupUi(this);
}

TabAbfuellen::~TabAbfuellen()
{
    delete ui;
}

void TabAbfuellen::setup(SudObject *sud)
{
    mSud = sud;

    ui->tbReifezeit->setColumn(mSud, ModelSud::ColWoche);
    ui->tbSWSchnellgaerprobe->setColumn(mSud, ModelSud::ColSWSchnellgaerprobe);
    ui->tbSWJungbier->setColumn(mSud, ModelSud::ColSWJungbier);
    ui->tbSWJungbier->setMesswert(ModelMesswerte::SchrittAbfuellen, ModelMesswerte::GroesseExtrakt);
    ui->tbSWJungbierSoll->setColumn(mSud, ModelSud::ColSREErwartet);
    ui->tbBiermengeAbfuellen->setColumn(mSud, ModelSud::Colerg_AbgefuellteBiermenge);
    ui->tbJungbiermengeAbfuellen->setColumn(mSud, ModelSud::ColJungbiermengeAbfuellen);
    ui->tbJungbiermengeAbfuellen->setMesswert(ModelMesswerte::SchrittAbfuellen, ModelMesswerte::GroesseVolumen);
    ui->tbSpeisemengeAbgefuellt->setColumn(mSud, ModelSud::ColSpeisemenge);
    ui->tbTemperaturJungbier->setColumn(mSud, ModelSud::ColTemperaturJungbier);
    ui->tbNebenkosten->setColumn(mSud, ModelSud::ColKostenWasserStrom);
    ui->tbSw->setColumn(mSud, ModelSud::ColSWIst);
    ui->tbEVG->setColumn(mSud, ModelSud::ColsEVG);
    ui->tbEVGRezept->setColumn(mSud, ModelSud::ColVergaerungsgrad);
    ui->tbGruenschlauchzeitpunkt->setColumn(mSud, ModelSud::ColGruenschlauchzeitpunkt);
    ui->tbAlkohol->setColumn(mSud, ModelSud::Colerg_Alkohol);
    ui->tbAlkoholRezept->setColumn(mSud, ModelSud::ColAlkoholSoll);
    ui->tbSpundungsdruck->setColumn(mSud, ModelSud::ColSpundungsdruck);
    ui->tbTemperaturKarbonisierung->setColumn(mSud, ModelSud::ColTemperaturKarbonisierung);
    ui->tbWassserZuckerloesung->setColumn(mSud, ModelSud::ColVerschneidungAbfuellen);
    ui->tbZuckerProLiterFest->setColumn(mSud, ModelSud::ColZuckerProLiter);
    // Karbonisierungsrechner (ext-s13): Ziel-CO2 hier eingeben, Zucker/Speise/Druck folgen daraus
    ui->tbCO2->setColumn(mSud, ModelSud::ColCO2);
    ui->tbCO2->setToolTip(co2RichtwerteHtml());
    ui->lblCO2Richtwerte->setText(co2RichtwerteKurz());
    ui->lblCO2Richtwerte->setToolTip(co2RichtwerteHtml());
    ui->tbZuckerProLiterFest->setToolTip(tr("<p>Fester Zuckerwert in g je Liter Jungbier (Zucker mit der eingestellten Süßkraft), z. B. weil der Zucker je Flasche abgewogen wird. "
                                            "Daraus folgen die Mengen je Flasche und der Alkohol des fertigen Biers.</p>"
                                            "<p>Der Vorschlag des Rechners (rechts) ergibt sich aus Ziel-CO₂, Restextrakt und Temperatur und wird nicht automatisch übernommen.</p>"));
    ui->btnZuckerProLiterVorschlag->setToolTip(tr("Vorschlag des Rechners als festen Wert übernehmen"));
    // ext-s13: Umschaltung Berechnung aus Ziel-CO2 <-> fester Zuckerwert je Liter (NULL = Berechnung)
    connect(ui->lblZuckerProLiterFest, &QCheckBox::clicked, this, [this](bool fest) {
        gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColZuckerProLiter,
                                                 fest ? QVariant(qRound(zuckerProLiterVorschlag() * 10) / 10.0) : QVariant()));
    });
    // Abfuellart (ext-s13): Index = Brauhelfer::Abfuellart - 1
    ui->cbAbfuellart->setItemData(0, tr("Zuckerlösung in den ganzen Sud (Bottich oder Fass). Benötigter Zucker aus Ziel-CO₂, Restextrakt und Temperatur. Das Wasser der Zuckerlösung verdünnt das Bier."), Qt::ToolTipRole);
    ui->cbAbfuellart->setItemData(1, tr("Fester Zuckerwert je Liter (z. B. je Flasche abgewogen), vorbelegt aus dem Rechner. Ergibt g je Flasche. Keine Verdünnung."), Qt::ToolTipRole);
    ui->cbAbfuellart->setItemData(2, tr("Speise (zurückbehaltene Würze) wird zugegeben. Reicht sie nicht, wird der Rest mit Zucker ergänzt."), Qt::ToolTipRole);
    ui->cbAbfuellart->setItemData(3, tr("Rechtzeitig schlauchen: Der Restextrakt vergärt im Fass oder in der Flasche (Spundungsdruck). Kein Zucker. Alkohol des fertigen Biers wie im Rezept, bis eine Schnellgärprobe gemessen ist."), Qt::ToolTipRole);
    ui->cbAbfuellart->setItemData(4, tr("Fass, CO₂ aus der Gasflasche. Kein Zucker, kein Alkoholzuwachs. Druck aus Ziel-CO₂ und Temperatur."), Qt::ToolTipRole);
    ui->tbKosten->setColumn(mSud, ModelSud::Colerg_Preis);
    ui->lblNebenkostenEinheit->setText(QLocale().currencySymbol());
    ui->lblKostenEinheit->setText(QLocale().currencySymbol() + "/L");

    ui->wdgBemerkungAbfuellen->setSudObject(mSud);
    ui->wdgBemerkungAbfuellen->setPlaceholderText(tr("Bemerkung Abfüllen"));
    ui->wdgBemerkungGaerung->setSudObject(mSud);
    ui->wdgBemerkungGaerung->setPlaceholderText(tr("Bemerkung Gärung & Reifung"));

    gSettings->beginGroup(staticMetaObject.className());

    ui->splitter->setSizes({100, 400});
    mDefaultSplitterState = ui->splitter->saveState();
    ui->splitter->restoreState(gSettings->value("splitterState").toByteArray());

    ui->splitterCharts->setSizes({300, 300, 100});
    mDefaultSplitterChartsState = ui->splitterCharts->saveState();
    ui->splitterCharts->restoreState(gSettings->value("splitterChartsState").toByteArray());

    ui->tbZuckerFaktor->setValue(gSettings->value("ZuckerFaktor", 1.0).toDouble());
    ui->tbFlaschengroesse->setValue(gSettings->value("FlaschenGroesse", 0.5).toDouble());

    gSettings->endGroup();

    connect(mSud->bh(), &Brauhelfer::modified, this, &TabAbfuellen::updateValues);
    connect(mSud->bh(), &Brauhelfer::discarded, this, &TabAbfuellen::sudLoaded);
    connect(mSud, &SudObject::loadedChanged, this, &TabAbfuellen::sudLoaded);
    connect(mSud, &SudObject::dataChanged, this, &TabAbfuellen::sudDataChanged);

    connect(ui->wdgBemerkungAbfuellen, &WdgBemerkung::changed, this, [this](const QString& html){gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColBemerkungAbfuellen, html));});
    connect(ui->wdgBemerkungGaerung, &WdgBemerkung::changed, this, [this](const QString& html){gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColBemerkungGaerung, html));});

    WidgetDecorator::addDecorator(findChildren<QWidget*>());
}

void TabAbfuellen::saveSettings()
{
    gSettings->beginGroup(staticMetaObject.className());
    gSettings->setValue("splitterState", ui->splitter->saveState());
    gSettings->setValue("splitterChartsState", ui->splitterCharts->saveState());
    gSettings->setValue("ZuckerFaktor", ui->tbZuckerFaktor->value());
    gSettings->setValue("FlaschenGroesse", ui->tbFlaschengroesse->value());
    gSettings->endGroup();
}

void TabAbfuellen::restoreView()
{
    ui->splitter->restoreState(mDefaultSplitterState);
    ui->splitterCharts->restoreState(mDefaultSplitterChartsState);
}

void TabAbfuellen::modulesChanged(Settings::Modules modules)
{
    if (modules.testFlag(Settings::ModulePreiskalkulation))
    {
        gSettings->setVisibleModule(Settings::ModulePreiskalkulation,
                         {ui->tbKosten,
                          ui->lblKosten,
                          ui->lblKostenEinheit,
                          ui->tbNebenkosten,
                          ui->lblNebenkosten,
                          ui->lblNebenkostenEinheit,
                          ui->lblNebenkostenSpacer,
                          ui->lineKosten,
                          ui->lineKosten2});
    }
    if (modules.testFlag(Settings::ModuleSpeise))
    {
        gSettings->setVisibleModule(Settings::ModuleSpeise,
                         {ui->tbSpeisemengeAbgefuellt,
                          ui->lblSpeisemengeAbgefuellt,
                          ui->lblSpeisemengeAbgefuelltEinheit,
                          ui->tbSpeisemengeGesamt,
                          ui->lblSpeisemengeGesamt,
                          ui->lblSpeisemengeGesamtEinheit,
                          ui->tbSpeisemengeFlasche,
                          ui->lblSpeisemengeFlasche,
                          ui->lblSpeisemengeFlascheEinheit});
    }
    if (modules.testFlag(Settings::ModuleSchnellgaerprobe))
    {
        gSettings->setVisibleModule(Settings::ModuleSchnellgaerprobe,
                         {ui->cbSchnellgaerprobeAktiv});
    }
    if (mSud->isLoaded())
    {
        checkEnabled();
        updateValues();
    }
}

void TabAbfuellen::sudLoaded()
{
    checkEnabled();
    updateValues();
    ui->wdgBemerkungAbfuellen->setHtml(mSud->getBemerkungAbfuellen());
    ui->wdgBemerkungGaerung->setHtml(mSud->getBemerkungGaerung());
}

void TabAbfuellen::sudDataChanged(const QModelIndex& index)
{
    switch (index.column())
    {
    case ModelSud::ColStatus:
        checkEnabled();
        break;
    case ModelSud::ColBemerkungAbfuellen:
        ui->wdgBemerkungAbfuellen->setHtml(mSud->getBemerkungAbfuellen());
        break;
    case ModelSud::ColBemerkungGaerung:
        ui->wdgBemerkungGaerung->setHtml(mSud->getBemerkungGaerung());
        break;
    }
}

void TabAbfuellen::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    updateValues();
}

void TabAbfuellen::changeEvent(QEvent* event)
{
    switch(event->type())
    {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
    QWidget::changeEvent(event);
}

void TabAbfuellen::checkEnabled()
{
    Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(mSud->getStatus());
    bool abgefuellt = status >= Brauhelfer::SudStatus::Abgefuellt && !gSettings->ForceEnabled;
    bool verbraucht = status >= Brauhelfer::SudStatus::Verbraucht && !gSettings->ForceEnabled;
    ui->tbAbfuelldatum->setDisabled(abgefuellt);
    ui->tbAbfuelldatumZeit->setDisabled(abgefuellt);
    ui->btnAbfuelldatumHeute->setVisible(!abgefuellt);
    ui->btnAbfuelldatumClear->setVisible(!abgefuellt);
    ui->tbReifung->setDisabled(verbraucht);
    ui->btnReifungHeute->setVisible(!verbraucht);
    ui->btnReifungClear->setVisible(!verbraucht);
    ui->tbReifezeit->setVisible(!verbraucht);
    ui->lblReifezeit->setVisible(!verbraucht);
    ui->lblReifezeitEinheit->setVisible(!verbraucht);
    ui->cbSchnellgaerprobeAktiv->setEnabled(!abgefuellt);
    ui->tbSWSchnellgaerprobe->setDisabled(abgefuellt);
    ui->btnSWSchnellgaerprobe->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked() && !abgefuellt);
    ui->tbSWJungbier->setDisabled(abgefuellt);
    ui->btnSWJungbier->setVisible(!abgefuellt);
    ui->tbTemperaturJungbier->setDisabled(abgefuellt);
    ui->cbAbfuellart->setEnabled(!abgefuellt);
    ui->tbCO2->setDisabled(abgefuellt);
    ui->cbSpundenZucker->setEnabled(!abgefuellt);
    ui->tbZuckerProLiterFest->setDisabled(abgefuellt);
    ui->lblZuckerProLiterFest->setEnabled(!abgefuellt);
    ui->tbJungbiermengeAbfuellen->setDisabled(abgefuellt);
    ui->tbBiermengeAbfuellen->setDisabled(abgefuellt);
    ui->tbSpeisemengeAbgefuellt->setDisabled(abgefuellt);
    ui->tbZuckerFaktor->setDisabled(abgefuellt);
    ui->tbWassserZuckerloesung->setDisabled(abgefuellt);
    ui->tbTemperaturKarbonisierung->setDisabled(abgefuellt);
    ui->tbFlaschengroesse->setDisabled(abgefuellt);
    ui->tbNebenkosten->setDisabled(abgefuellt);
    ui->btnSudAbgefuellt->setEnabled(status == Brauhelfer::SudStatus::Gebraut && !gSettings->ForceEnabled);
    ui->btnSudAbgefuelltReset->setEnabled(status == Brauhelfer::SudStatus::Abgefuellt);
    ui->btnSudVerbraucht->setEnabled(status == Brauhelfer::SudStatus::Abgefuellt && !gSettings->ForceEnabled);
    ui->btnSudVerbrauchtReset->setEnabled(status == Brauhelfer::SudStatus::Verbraucht);
}

void TabAbfuellen::updateValues()
{
    if (!isVisible())
        return;

    Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(mSud->getStatus());
    bool abgefuellt = status >= Brauhelfer::SudStatus::Abgefuellt && !gSettings->ForceEnabled;

    for (auto& wdg : findChildren<DoubleSpinBoxSud*>())
        wdg->updateValue();
    for (auto& wdg : findChildren<SpinBoxSud*>())
        wdg->updateValue();

    QDateTime dt = mSud->getAbfuelldatum();
    ui->tbAbfuelldatum->setDate(dt.isValid() ? dt.date() : QDate(2000,1,1));
    ui->tbAbfuelldatumZeit->setTime(dt.isValid() ? dt.time() : QTime(0,0));
    ui->tbDauerHauptgaerung->setValue((int)mSud->getBraudatum().daysTo(ui->tbAbfuelldatum->dateTime()));
    dt = mSud->getReifungStart();
    ui->tbReifung->setDate(dt.isValid() ? dt.date() : QDate(2000,1,1));

    ui->cbSchnellgaerprobeAktiv->setChecked(mSud->getSchnellgaerprobeAktiv() && gSettings->isModuleEnabled(Settings::ModuleSchnellgaerprobe));
    ui->tbSWSchnellgaerprobe->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked());
    ui->lblSWSchnellgaerprobeEinheit->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked());
    ui->btnSWSchnellgaerprobe->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked() && !abgefuellt);
    ui->tbGruenschlauchzeitpunkt->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked());
    ui->lblGruenschlauchzeitpunkt->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked());
    ui->lblGruenschlauchzeitpunktEinheit->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked());
    ui->tbAlkoholGaerung->setValue(mSud->getAlkoholJungbier());

    ui->tbJungbierVerlust->setValue(mSud->getWuerzemengeAnstellen() - mSud->getJungbiermengeAbfuellen());

    // Abfuellart (ext-s13): nur die zur Art gehoerenden Felder anzeigen
    const int art = mSud->getAbfuellartEffektiv();
    const bool artGewaehlt = mSud->getAbfuellart() != static_cast<int>(Brauhelfer::Abfuellart::Abgeleitet);
    const bool artZuckerGesamt = art == static_cast<int>(Brauhelfer::Abfuellart::ZuckerGesamt);
    const bool artZuckerFlasche = art == static_cast<int>(Brauhelfer::Abfuellart::ZuckerFlasche);
    const bool artSpeise = art == static_cast<int>(Brauhelfer::Abfuellart::Speise);
    const bool artSpundenZucker = art == static_cast<int>(Brauhelfer::Abfuellart::SpundenZucker);
    const bool artSpunden = art == static_cast<int>(Brauhelfer::Abfuellart::Spunden) || artSpundenZucker;
    const bool artZwang = art == static_cast<int>(Brauhelfer::Abfuellart::Zwangskarbonisierung);
    if (!ui->cbAbfuellart->hasFocus())
        ui->cbAbfuellart->setCurrentIndex(artSpundenZucker ? static_cast<int>(Brauhelfer::Abfuellart::Spunden) - 1 : art - 1);
    QString hinweis;
    if (artZuckerGesamt)
        hinweis = tr("Zuckerlösung in den ganzen Sud. Benötigter Zucker aus Ziel-CO₂, Restextrakt und Temperatur.");
    else if (artZuckerFlasche)
        hinweis = tr("Zucker direkt in die Flasche, g je Flasche aus Ziel-CO₂ berechnet oder aus einem festen Wert je Liter. Keine Verdünnung durch Zuckerlösung.");
    else if (artSpeise)
        hinweis = tr("Speise wird zugegeben, ein fehlender Rest mit Zucker.");
    else if (artSpunden)
        hinweis = artSpundenZucker ? tr("Spunden, fehlender Restextrakt wird mit Zucker ausgeglichen (siehe Spundungsdruck).")
                : artGewaehlt ? tr("Rechtzeitig schlauchen, kein Zucker. Alkohol fertiges Bier wie im Rezept (Vergärungsgrad), bis eine Schnellgärprobe gemessen ist.")
                              : tr("Spunden, kein Zucker.");
    else if (artZwang)
        hinweis = tr("CO₂ aus der Gasflasche, kein Zucker und kein Alkoholzuwachs. Druck siehe oben.");
    if (!artGewaehlt)
        hinweis += QStringLiteral(" ") + tr("(Abgeleitet aus den bisherigen Angaben, Rechnung wie bisher. Eine Auswahl speichert die Abfüllart.)");
    ui->lblAbfuellartHinweis->setText(hinweis);

    ui->groupKarbonisierung->setVisible((!artSpunden || artSpundenZucker) && !artZwang);
    ui->groupBox_4->setTitle(artZwang ? tr("Zwangskarbonisierung") : tr("Spundungsdruck"));
    ui->label_sp->setText(artZwang ? tr("Druck CO₂-Flasche") : tr("Spundungsdruck"));
    for (QWidget* wdg : {static_cast<QWidget*>(ui->label_sp), static_cast<QWidget*>(ui->line_6), static_cast<QWidget*>(ui->tbSpundungsdruck), static_cast<QWidget*>(ui->label_esp)})
        wdg->setVisible(artSpunden || artZwang);
    ui->label_alc->setText(artSpunden ? tr("Alkohol beim Schlauchen") : tr("Alkohol Jungbier"));

    // Spunden (ext-s13): Restextrakt, bei dem geschlaucht werden muss, damit der Rest der Gaerung
    // das Ziel-CO2 liefert. Mit Schnellgaerprobe daraus, sonst aus dem Vergaerungsgrad der Hefe geschaetzt.
    for (QWidget* wdg : {static_cast<QWidget*>(ui->lblSchlauchenBei), static_cast<QWidget*>(ui->tbSchlauchenBei),
                         static_cast<QWidget*>(ui->lblSchlauchenBeiEinheit), static_cast<QWidget*>(ui->lblSchlauchenHinweis),
                         static_cast<QWidget*>(ui->cbSpundenZucker)})
        wdg->setVisible(artSpunden);
    if (artSpunden)
    {
        const bool sgp = ui->cbSchnellgaerprobeAktiv->isChecked();
        const double sreEnde = sgp ? mSud->getSWSchnellgaerprobe() : mSud->getSREErwartet();
        const double soll = BierCalc::gruenschlauchzeitpunkt(mSud->getCO2(), mSud->getSWIst(), sreEnde, mSud->getTemperaturJungbier());
        ui->tbSchlauchenBei->setValue(soll);
        QStringList text;
        if (sgp)
            text << tr("Berechnet aus der Schnellgärprobe (Endvergärung %1 °P), Ziel-CO₂ und Temperatur Jungbier.").arg(QLocale().toString(sreEnde, 'f', 1));
        else
            text << tr("Nur aus dem Vergärungsgrad der Hefe geschätzt (%1 %, Endvergärung ca. %2 °P). "
                       "Für eine zuverlässige Karbonisierung eine Schnellgärprobe ansetzen und oben aktivieren.")
                    .arg(QLocale().toString(mSud->getVergaerungsgrad(), 'f', 0), QLocale().toString(sreEnde, 'f', 1));
        // Immer melden, wenn der Restextrakt des Jungbiers unter dem Sollwert liegt (ext-s13)
        const bool zuTief = std::round(mSud->getSWJungbier() * 10.0) < std::round(soll * 10.0); // wie Kern (0,1 °P)
        if (zuTief)
            text << (artSpundenZucker ? tr("Der Restextrakt des Jungbiers (%1 °P) liegt unter dem Sollwert; das fehlende CO₂ wird mit Zucker ausgeglichen.")
                                      : tr("Der Restextrakt des Jungbiers (%1 °P) liegt unter dem Sollwert: Das Ziel-CO₂ wird so nicht erreicht. "
                                           "Abhilfe: unten den Zuckerausgleich aktivieren."))
                    .arg(QLocale().toString(mSud->getSWJungbier(), 'f', 1));
        QPalette pal = palette();
        if (!sgp || (zuTief && !artSpundenZucker))
            pal.setColor(QPalette::WindowText, gSettings->theme() == Qt::ColorScheme::Dark ? QColor(245, 158, 11) : QColor(204, 102, 0));
        ui->lblSchlauchenHinweis->setPalette(pal);
        ui->lblSchlauchenHinweis->setText(text.join(QStringLiteral(" ")));
        ui->cbSpundenZucker->setChecked(artSpundenZucker);
    }
    ui->lblSpundenZucker->setVisible(false); // ext-s13: Zuckerausgleich erscheint in der Gruppe Karbonisierung
    double flascheFaktor = ui->tbFlaschengroesse->value() / mSud->getJungbiermengeAbfuellen();

    // ModuleSpeise
    if (gSettings->isModuleEnabled(Settings::ModuleSpeise))
    {
        // ext-s13: Eingabe Speise nur bei Abfuellart Speise (oder wenn bereits eine Menge eingetragen ist)
        bool speiseEingabe = artSpeise || mSud->getSpeisemenge() > 0.0;
        ui->tbSpeisemengeAbgefuellt->setVisible(speiseEingabe);
        ui->lblSpeisemengeAbgefuellt->setVisible(speiseEingabe);
        ui->lblSpeisemengeAbgefuelltEinheit->setVisible(speiseEingabe);
        ui->tbSpeisemengeGesamt->setValue((int)mSud->getSpeiseAnteil());
        ui->tbSpeisemengeGesamt->setVisible(ui->tbSpeisemengeGesamt->value() > 0.0);
        ui->lblSpeisemengeGesamt->setVisible(ui->tbSpeisemengeGesamt->value() > 0.0);
        ui->lblSpeisemengeGesamtEinheit->setVisible(ui->tbSpeisemengeGesamt->value() > 0.0);
        ui->tbSpeisemengeFlasche->setValue(ui->tbSpeisemengeGesamt->value() * flascheFaktor);
        ui->tbSpeisemengeFlasche->setVisible(ui->tbSpeisemengeFlasche->value() > 0.0);
        ui->lblSpeisemengeFlasche->setVisible(ui->tbSpeisemengeFlasche->value() > 0.0);
        ui->lblSpeisemengeFlascheEinheit->setVisible(ui->tbSpeisemengeFlasche->value() > 0.0);
    }

    // Zucker (ext-s06, ext-s13): exakter Wert je Liter statt gerundeter Gesamtmenge
    double jungbiermenge = mSud->getJungbiermengeAbfuellen();
    double vorschlag = zuckerProLiterVorschlag();
    double zuckerProLiter = 0.0;
    if (artZuckerFlasche)
    {
        const bool fest = !mSud->bh()->modelSud()->data(mSud->row(), ModelSud::ColZuckerProLiter).isNull();
        zuckerProLiter = fest ? mSud->getZuckerProLiter() : vorschlag;
        ui->lblZuckerProLiterFest->setChecked(fest);
    }
    else if (jungbiermenge > 0.0)
    {
        zuckerProLiter = mSud->getZuckerAnteil() / ui->tbZuckerFaktor->value() / jungbiermenge;
    }
    if (zuckerProLiter < 0.0 || !std::isfinite(zuckerProLiter))
        zuckerProLiter = 0.0;
    ui->tbZuckerGesamt->setValue((int)(zuckerProLiter * jungbiermenge));
    ui->tbZuckerFlasche->setValue(zuckerProLiter * ui->tbFlaschengroesse->value());
    ui->tbFlaschen->setValue(mSud->geterg_AbgefuellteBiermenge() / ui->tbFlaschengroesse->value());
    ui->tbKonzentrationZuckerloesung->setValue(ui->tbZuckerGesamt->value() / ui->tbWassserZuckerloesung->value());
    ui->tbZuckerProLiter->setValue(artZuckerFlasche ? vorschlag : zuckerProLiter);
    ui->tbZucker05->setValue(zuckerProLiter * 0.5);
    ui->tbZucker033->setValue(zuckerProLiter * 0.33);
    const bool hasZucker = ui->tbZuckerGesamt->value() > 0.0 || artZuckerFlasche || artSpundenZucker;
    const bool zuckerLoesung = hasZucker && !artZuckerFlasche && !artSpundenZucker;
    ui->groupKarbonisierung->setTitle(artSpundenZucker ? (zuckerProLiter > 0.0 ? tr("Zuckerausgleich beim Spunden") : tr("Zuckerausgleich beim Spunden: kein Zucker nötig")) : tr("Karbonisierung"));
    ui->lblZuckerGesamt->setText(artZuckerFlasche ? tr("Zucker gesamt") : artSpundenZucker ? tr("Zucker zum Ausgleich") : tr("Zusätzlich benötigter Zucker"));
    for (QWidget* wdg : {static_cast<QWidget*>(ui->tbZuckerGesamt), static_cast<QWidget*>(ui->lblZuckerGesamt), static_cast<QWidget*>(ui->lblZuckerGesamtEinheit),
                         static_cast<QWidget*>(ui->tbZuckerFaktor), static_cast<QWidget*>(ui->lblZuckerFaktor),
                         static_cast<QWidget*>(ui->tbZuckerFlasche), static_cast<QWidget*>(ui->lblZuckerFlasche), static_cast<QWidget*>(ui->lblZuckerFlascheEinheit),
                         static_cast<QWidget*>(ui->lblZuckerProLiter), static_cast<QWidget*>(ui->tbZuckerProLiter), static_cast<QWidget*>(ui->lblZuckerProLiterEinheit)})
        wdg->setVisible(hasZucker);
    for (QWidget* wdg : {static_cast<QWidget*>(ui->lblWassserZuckerloesung), static_cast<QWidget*>(ui->tbWassserZuckerloesung), static_cast<QWidget*>(ui->tbWassserZuckerloesungEinheit)})
        wdg->setVisible(zuckerLoesung);
    for (QWidget* wdg : {static_cast<QWidget*>(ui->lblZucker05), static_cast<QWidget*>(ui->tbZucker05), static_cast<QWidget*>(ui->lblZucker05Einheit),
                         static_cast<QWidget*>(ui->lblZucker033), static_cast<QWidget*>(ui->tbZucker033), static_cast<QWidget*>(ui->lblZucker033Einheit),
                         static_cast<QWidget*>(ui->lblZuckerProLiterFest), static_cast<QWidget*>(ui->tbZuckerProLiterFest), static_cast<QWidget*>(ui->lblZuckerProLiterFestEinheit)})
        wdg->setVisible(artZuckerFlasche);
    {
        const bool fest = artZuckerFlasche && !mSud->bh()->modelSud()->data(mSud->row(), ModelSud::ColZuckerProLiter).isNull();
        ui->tbZuckerProLiterFest->setVisible(fest);
        ui->lblZuckerProLiterFestEinheit->setVisible(fest);
        ui->lblZuckerProLiter->setText(fest ? tr("Vorschlag Rechner je Liter") : tr("Zucker je Liter Jungbier"));
    }
    ui->tbZucker05->setVisible(artZuckerFlasche || artSpundenZucker);
    ui->lblZucker05->setVisible(artZuckerFlasche || artSpundenZucker);
    ui->lblZucker05Einheit->setVisible(artZuckerFlasche || artSpundenZucker);
    ui->tbZucker033->setVisible(artZuckerFlasche || artSpundenZucker);
    ui->lblZucker033->setVisible(artZuckerFlasche || artSpundenZucker);
    ui->lblZucker033Einheit->setVisible(artZuckerFlasche || artSpundenZucker);
    ui->btnZuckerProLiterVorschlag->setVisible(false); // ersetzt durch Umschaltung "Fester Wert je Liter"
    bool hasZuckerLoesung = ui->tbWassserZuckerloesung->value() > 0.0 && zuckerLoesung;
    ui->lblKonzentrationZuckerloesung->setVisible(hasZuckerLoesung);
    ui->tbKonzentrationZuckerloesung->setVisible(hasZuckerLoesung);
    ui->tbKonzentrationZuckerloesungEinheit->setVisible(hasZuckerLoesung);

    ui->chartAbfuelldaten->update(mSud);
    ui->chartRestextrakt->update(mSud);
}

void TabAbfuellen::on_tbAbfuelldatum_dateChanged(const QDate &date)
{
    if (ui->tbAbfuelldatum->hasFocus())
        gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColAbfuelldatum, QDateTime(date, ui->tbAbfuelldatumZeit->time())));
}

void TabAbfuellen::on_tbAbfuelldatumZeit_timeChanged(const QTime &time)
{
    if (ui->tbAbfuelldatumZeit->hasFocus())
        gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColAbfuelldatum, QDateTime(ui->tbAbfuelldatum->date(), time)));
}

void TabAbfuellen::on_btnAbfuelldatumHeute_clicked()
{
    gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColAbfuelldatum, QDateTime::currentDateTime()));
}

void TabAbfuellen::on_btnAbfuelldatumClear_clicked()
{
    gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColAbfuelldatum, QDateTime()));
}

void TabAbfuellen::on_tbReifung_dateChanged(const QDate &date)
{
    if (ui->tbReifung->hasFocus())
        gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColReifungStart, QDateTime(date, QTime())));
}

void TabAbfuellen::on_btnReifungHeute_clicked()
{
    gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColReifungStart, QDateTime::currentDateTime()));
}

void TabAbfuellen::on_btnReifungClear_clicked()
{
    gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColReifungStart, QDateTime()));
}

void TabAbfuellen::on_cbSchnellgaerprobeAktiv_clicked(bool checked)
{
    gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColSchnellgaerprobeAktiv, checked));
}

void TabAbfuellen::on_btnSWSchnellgaerprobe_clicked()
{
    DlgRestextrakt dlg(ui->tbSWSchnellgaerprobe->value(),
                       mSud->getSWIst(),
                       ui->tbTemperaturJungbier->value(),
                       QDateTime(),
                       this);
    if (dlg.exec() == QDialog::Accepted)
    {
        gUndoStack->beginMacro(QStringLiteral("macro"));
        gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColTemperaturJungbier, dlg.temperatur()));
        gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColSWSchnellgaerprobe, dlg.value()));
        gUndoStack->endMacro();
    }
}

void TabAbfuellen::on_btnSWJungbier_clicked()
{
    DlgRestextrakt dlg(ui->tbSWJungbier->value(),
                       mSud->getSWIst(),
                       ui->tbTemperaturJungbier->value(),
                       QDateTime(),
                       this);
    if (dlg.exec() == QDialog::Accepted)
    {
        gUndoStack->beginMacro(QStringLiteral("macro"));
        gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColTemperaturJungbier, dlg.temperatur()));
        gUndoStack->push(new SetMesswertCommand(mSud->bh(), mSud->row(), ModelMesswerte::SchrittAbfuellen, ModelMesswerte::GroesseExtrakt, dlg.value(), dlg.messmethode()));
        gUndoStack->endMacro();
    }
}

QString TabAbfuellen::co2RichtwerteKurz()
{
    return tr("Richtwerte g/L: Brit. Ale 2,9–3,9 · Stout 3,3–4,5 · Belg. Ale 3,7–4,7 · Lager/Pils 4,3–5,3 · Weizen 6,5–8,8 (Details im Tooltip)");
}

QString TabAbfuellen::co2RichtwerteHtml()
{
    // Literatur (ext-s13): J. Palmer, How to Brew, Tabelle typische Karbonisierung je Bierstil in
    // Volumen CO2; umgerechnet mit 1 Vol. = 1,96 g/L (Dichte CO2 bei 0 °C, 1,013 bar), gerundet.
    return tr("<p><b>Ziel-Karbonisierung</b> in g CO₂ je Liter fertiges Bier. Daraus werden je nach Abfüllart "
              "Zucker, Speise, Schlauchzeitpunkt oder Druck berechnet.</p>"
              "<p><b>Richtwerte nach Bierart</b></p>"
              "<table>"
              "<tr><td>Britische Ales (Bitter, Mild)</td><td>&nbsp;2,9 – 3,9 g/L</td><td>&nbsp;(1,5 – 2,0 Vol.)</td></tr>"
              "<tr><td>Porter, Stout</td><td>&nbsp;3,3 – 4,5 g/L</td><td>&nbsp;(1,7 – 2,3 Vol.)</td></tr>"
              "<tr><td>Belgische Ales</td><td>&nbsp;3,7 – 4,7 g/L</td><td>&nbsp;(1,9 – 2,4 Vol.)</td></tr>"
              "<tr><td>Amerikanische Ales, Lager, Pils</td><td>&nbsp;4,3 – 5,3 g/L</td><td>&nbsp;(2,2 – 2,7 Vol.)</td></tr>"
              "<tr><td>Frucht-Lambic</td><td>&nbsp;5,9 – 8,8 g/L</td><td>&nbsp;(3,0 – 4,5 Vol.)</td></tr>"
              "<tr><td>Deutsches Weizenbier</td><td>&nbsp;6,5 – 8,8 g/L</td><td>&nbsp;(3,3 – 4,5 Vol.)</td></tr>"
              "</table>"
              "<p>Quelle: J. Palmer, <i>How to Brew</i>, Tabelle zur Karbonisierung je Bierstil (Volumen CO₂), "
              "umgerechnet mit 1 Vol. ≈ 1,96 g/L. Richtwerte, Bierstile streuen. Normale Flaschen vertragen "
              "bis etwa 6 g/L; höhere Werte nur in druckfesten Flaschen (z. B. Weizen- oder Sektflaschen).</p>");
}

double TabAbfuellen::zuckerProLiterVorschlag() const
{
    // Rechner (ext-s06): g/L Zucker mit der eingestellten Suesskraft
    double v = mSud->getZuckerProLiterVorschlag() / ui->tbZuckerFaktor->value();
    return std::isfinite(v) && v > 0.0 ? v : 0.0;
}

void TabAbfuellen::setAbfuellart(int art)
{
    // ext-s13: Auswahl als ein Undo-Schritt. Spunden-Spalte folgt der Art (bestehende Rechnung
    // und Anzeigen nutzen sie weiter). Ohne Zuckerloesung wird deren Wasser auf 0 gesetzt, damit
    // es nicht verdeckt in der abgefuellten Menge steckt (vor dem Spunden-Wechsel, s. setDataExt).
    ModelSud* model = mSud->bh()->modelSud();
    const int row = mSud->row();
    const bool ohneLoesung = art == static_cast<int>(Brauhelfer::Abfuellart::ZuckerFlasche)
            || art == static_cast<int>(Brauhelfer::Abfuellart::SpundenZucker)
            || art == static_cast<int>(Brauhelfer::Abfuellart::Spunden)
            || art == static_cast<int>(Brauhelfer::Abfuellart::Zwangskarbonisierung);
    const bool spunden = art == static_cast<int>(Brauhelfer::Abfuellart::Spunden) || art == static_cast<int>(Brauhelfer::Abfuellart::SpundenZucker);
    gUndoStack->beginMacro(QStringLiteral("macro"));
    if (ohneLoesung && mSud->getVerschneidungAbfuellen() != 0.0)
        gUndoStack->push(new SetModelDataCommand(model, row, ModelSud::ColVerschneidungAbfuellen, 0.0));
    if (mSud->getSpunden() != spunden)
        gUndoStack->push(new SetModelDataCommand(model, row, ModelSud::ColSpunden, spunden));
    gUndoStack->push(new SetModelDataCommand(model, row, ModelSud::ColAbfuellart, art));
    gUndoStack->endMacro();
}

void TabAbfuellen::on_cbAbfuellart_activated(int index)
{
    int art = index + 1;
    // Spunden mit Zuckerausgleich bleibt bei erneuter Auswahl "Spunden" erhalten
    if (art == static_cast<int>(Brauhelfer::Abfuellart::Spunden) && mSud->getAbfuellartEffektiv() == static_cast<int>(Brauhelfer::Abfuellart::SpundenZucker))
        return;
    if (art != mSud->getAbfuellart())
        setAbfuellart(art);
}

void TabAbfuellen::on_cbSpundenZucker_clicked(bool checked)
{
    setAbfuellart(static_cast<int>(checked ? Brauhelfer::Abfuellart::SpundenZucker : Brauhelfer::Abfuellart::Spunden));
}

void TabAbfuellen::on_btnZuckerProLiterVorschlag_clicked()
{
    gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColZuckerProLiter, qRound(zuckerProLiterVorschlag() * 10) / 10.0));
}

void TabAbfuellen::on_tbZuckerFaktor_valueChanged(double)
{
    if (ui->tbZuckerFaktor->hasFocus())
        updateValues();
}

void TabAbfuellen::on_tbFlaschengroesse_valueChanged(double)
{
    if (ui->tbFlaschengroesse->hasFocus())
        updateValues();
}

void TabAbfuellen::on_btnSudAbgefuellt_clicked()
{
    if (!mSud->getAbfuellenBereitZutaten())
    {
        if (QMessageBox::warning(this, tr("Zutaten Gärung"),
                                 tr("Es wurden noch nicht alle Zutaten für die Gärung zugegeben oder entnommen.")
                                     + "\n" + tr("Soll der Sud trotzdem als abgefüllt markiert werden?"),
                                 QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Cancel)
            return;
    }

    if (mSud->getSchnellgaerprobeAktiv())
    {
        if (mSud->getSWJungbier() > mSud->getGruenschlauchzeitpunkt())
        {
            if (QMessageBox::warning(this, tr("Grünschlauchzeitpunkt nicht erreicht"),
                                     tr("Der Grünschlauchzeitpunkt wurde noch nicht erreicht.")
                                         + "\n" + tr("Soll der Sud trotzdem als abgefüllt markiert werden?"),
                                     QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Cancel)
                return;
        }
        else if (mSud->getSWJungbier() < mSud->getSWSchnellgaerprobe())
        {
            if (QMessageBox::warning(this, tr("Schnellgärprobe"),
                                     tr("Die Stammwürze des Jungbiers liegt tiefer als die der Schnellgärprobe.")
                                         + "\n" + tr("Soll der Sud trotzdem als abgefüllt markiert werden?"),
                                     QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Cancel)
                return;
        }
    }

    QDateTime dt = mSud->getAbfuelldatum();
    if (!dt.isValid())
        dt = QDateTime::currentDateTime();
    QString dtStr = QLocale().toString(dt, QLocale::ShortFormat);
    if (QMessageBox::question(this, tr("Stadium auf \"abgefüllt\" setzen?"),
                              tr("Soll das Stadium auf \"abgefüllt\" gesetzt werden?\nAbfülldatum: %1").arg(dtStr),
                              QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes)
    {
        gUndoStack->beginMacro(QStringLiteral("macro"));
        gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColAbfuelldatum, dt));
        QDateTime dtReifung = mSud->getReifungStart();
        if (!dtReifung.isValid())
            dtReifung = QDateTime::currentDateTime();
        gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColReifungStart, dtReifung));
        gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColStatus, static_cast<int>(Brauhelfer::SudStatus::Abgefuellt)));
        gUndoStack->endMacro();

        QMap<int, QVariant> values({{ModelNachgaerverlauf::ColSudID, mSud->id()},
                                    {ModelNachgaerverlauf::ColZeitstempel, mSud->getAbfuelldatum()},
                                    {ModelNachgaerverlauf::ColDruck, 0.0},
                                    {ModelNachgaerverlauf::ColTemp, mSud->getTemperaturJungbier()}});
        if (mSud->modelNachgaerverlauf()->rowCount() == 0)
            mSud->modelNachgaerverlauf()->append(values);
    }
}

void TabAbfuellen::on_btnSudAbgefuelltReset_clicked()
{
    if (QMessageBox::question(this, tr("Stadium \"abgefüllt\" zurücksetzen?"),
                              tr("Soll das Stadium \"abgefüllt\" zurückgesetzt werden?"),
                              QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes)
    {
        mSud->setStatus(static_cast<int>(Brauhelfer::SudStatus::Gebraut));
        if (mSud->modelNachgaerverlauf()->rowCount() == 1)
            mSud->modelNachgaerverlauf()->removeRow(0);
    }
}

void TabAbfuellen::on_btnSudVerbraucht_clicked()
{
    if (QMessageBox::question(this, tr("Stadium auf \"ausgetrunken\" setzen?"),
                              tr("Soll das Stadium auf \"ausgetrunken\" gesetzt werden?"),
                              QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes)
    {
        gUndoStack->push(new SetModelDataCommand(mSud->bh()->modelSud(), mSud->row(), ModelSud::ColStatus, static_cast<int>(Brauhelfer::SudStatus::Verbraucht)));
    }
}

void TabAbfuellen::on_btnSudVerbrauchtReset_clicked()
{
    if (QMessageBox::question(this, tr("Stadium \"ausgetrunken\" zurücksetzen?"),
                              tr("Soll das Stadium \"ausgetrunken\" zurückgesetzt werden?"),
                              QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes)
    {
        mSud->setStatus(static_cast<int>(Brauhelfer::SudStatus::Abgefuellt));
    }
}
