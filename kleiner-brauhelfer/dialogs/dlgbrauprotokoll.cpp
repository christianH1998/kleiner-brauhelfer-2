#include "dlgbrauprotokoll.h"
#include "ui_dlgbrauprotokoll.h"
#include <QApplication>
#include <QComboBox>
#include <QHeaderView>
#include <QPushButton>
#include <QToolButton>
#include "brauhelfer.h"
#include "settings.h"
#include "commands/undostack.h"
#include "commands/setmesswertcommand.h"
#include "dialogs/dlgrestextrakt.h"
#include "dialogs/dlgvolumen.h"

extern Brauhelfer* bh;
extern Settings* gSettings;

DlgBrauprotokoll* DlgBrauprotokoll::Dialog = nullptr;

namespace
{
enum Spalte { SpSchritt, SpGroesse, SpSoll, SpIst, SpAbweichung, SpMethode, SpZeit, SpBemerkung, SpHilfe, SpAnzahl };

int standardMethode(int groesse)
{
    switch (groesse)
    {
    case ModelMesswerte::GroesseVolumen: return ModelMesswerte::MethodeMessstab;
    case ModelMesswerte::GroesseExtrakt: return ModelMesswerte::MethodeSpindel;
    case ModelMesswerte::GroesseTemperatur: return ModelMesswerte::MethodeThermometer;
    case ModelMesswerte::GroesseMasse: return ModelMesswerte::MethodeWaage;
    case ModelMesswerte::GroesseDauerAufheizen:
    case ModelMesswerte::GroesseDauerRast: return ModelMesswerte::MethodeUhr;
    default: return ModelMesswerte::MethodeEingabe;
    }
}
}

DlgBrauprotokoll::DlgBrauprotokoll(QWidget *parent) :
    DlgAbstract(staticMetaObject.className(), parent),
    ui(new Ui::DlgBrauprotokoll),
    mSud(nullptr),
    mWerteSetzen(false)
{
    ui->setupUi(this);
    // Eingabetaste in einer Zelle soll den Wert uebernehmen, nicht den Dialog schliessen
    for (QAbstractButton *b : ui->buttonBox->buttons())
    {
        if (QPushButton *pb = qobject_cast<QPushButton*>(b))
        {
            pb->setAutoDefault(false);
            pb->setDefault(false);
        }
    }
    ui->tbAufheizrate->setValue(aufheizrate());
    ui->table->setColumnCount(SpAnzahl);
    ui->table->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->table->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked |
                               QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
    ui->table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->table->horizontalHeader()->setStretchLastSection(false);
    connect(ui->table, &QTableWidget::itemChanged, this, &DlgBrauprotokoll::zelleGeaendert);
    mTimer.setSingleShot(true);
    mTimer.setInterval(0);
    connect(&mTimer, &QTimer::timeout, this, &DlgBrauprotokoll::aktualisieren);
    connect(bh, &Brauhelfer::modified, this, &DlgBrauprotokoll::aktualisierenSpaeter);
    connect(bh, &Brauhelfer::discarded, this, &DlgBrauprotokoll::aktualisierenSpaeter);
}

DlgBrauprotokoll::~DlgBrauprotokoll()
{
    delete ui;
}

void DlgBrauprotokoll::saveSettings()
{
    gSettings->beginGroup(staticMetaObject.className());
    gSettings->setValue("tableState", ui->table->horizontalHeader()->saveState());
    gSettings->endGroup();
}

void DlgBrauprotokoll::loadSettings()
{
    gSettings->beginGroup(staticMetaObject.className());
    QByteArray state = gSettings->value("tableState").toByteArray();
    gSettings->endGroup();
    if (!state.isEmpty())
        ui->table->horizontalHeader()->restoreState(state);
}

void DlgBrauprotokoll::restoreView()
{
    DlgAbstract::restoreView(staticMetaObject.className(), Dialog);
    gSettings->beginGroup(staticMetaObject.className());
    gSettings->remove("tableState");
    gSettings->endGroup();
}

double DlgBrauprotokoll::aufheizrate()
{
    return gSettings->aufheizrate();
}

void DlgBrauprotokoll::on_tbAufheizrate_valueChanged(double value)
{
    gSettings->setAufheizrate(value);
    aktualisierenSpaeter();
}

void DlgBrauprotokoll::setSud(SudObject *sud, bool erzwingen)
{
    if (mSud == sud && !erzwingen)
        return;
    if (mSud)
        disconnect(mSud, nullptr, this, nullptr);
    mSud = sud;
    if (mSud)
    {
        // nach disconnect neu verbinden (auch bei erzwingen mit gleichem Sud)
        connect(mSud, &SudObject::loadedChanged, this, &DlgBrauprotokoll::aktualisierenSpaeter);
        connect(mSud, &SudObject::dataChanged, this, &DlgBrauprotokoll::aktualisierenSpaeter);
    }
    mEintraege.clear();
    aktualisieren();
}

void DlgBrauprotokoll::aktualisierenSpaeter()
{
    mTimer.start();
}

void DlgBrauprotokoll::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        mEintraege.clear();
        aktualisierenSpaeter();
    }
    DlgAbstract::changeEvent(event);
}

QString DlgBrauprotokoll::zahl(double wert, int dezimalen) const
{
    return QLocale().toString(wert, 'f', dezimalen);
}

QString DlgBrauprotokoll::istText(double wert, int dezimalen) const
{
    // Ist-Werte eine Stelle genauer anzeigen als die Sollwerte (Dauer ganzzahlig)
    return zahl(wert, dezimalen > 0 ? dezimalen + 1 : 0);
}

bool DlgBrauprotokoll::gesperrt() const
{
    if (!mSud || !mSud->isLoaded())
        return true;
    Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(mSud->getStatus());
    return status != Brauhelfer::SudStatus::Rezept && !gSettings->ForceEnabled;
}

QList<DlgBrauprotokoll::Eintrag> DlgBrauprotokoll::eintraege() const
{
    QList<Eintrag> liste;
    if (!mSud || !mSud->isLoaded())
        return liste;
    const QString L = tr("L"), P = QStringLiteral("°P"), C = QStringLiteral("°C"), kg = tr("kg"), min = tr("min");
    ModelMesswerte *mw = bh->modelMesswerte();
    const QVariant sudId = mSud->id();
    auto soll = [this](int schritt, int groesse) {
        int col = ModelMesswerte::sollSpalte(schritt, groesse);
        return col >= 0 ? mSud->getValue(col) : QVariant();
    };
    auto add = [&](int schritt, int groesse, const QString &sText, const QString &gText, const QString &einheit, int dez, Hilfe hilfe,
                   const QVariant &s, const QString &info, const QString &sollInfo, const QString &feld = QString()) {
        liste.append({schritt, groesse, sText, gText, einheit, dez, s, hilfe, info, sollInfo, feld});
    };
    auto grad = [this](double t) { return zahl(t, 0) + QStringLiteral(" °C"); };
    const QString keinSoll = tr("Kein Sollwert im Rezept. Der Messwert dient der Dokumentation und späteren Auswertung (Kalibrierung der Anlage).");
    // Soll-Aufheizdauer aus gemessener Starttemperatur und Zieltemperatur (Richtwert Aufheizrate)
    auto aufheizSoll = [&](int schrittStart, double tZiel, QString &sollInfo) {
        QVariant tStart = mw->wert(sudId, schrittStart, ModelMesswerte::GroesseTemperatur);
        if (!tStart.isValid())
        {
            sollInfo = tr("Sollwert erscheint, sobald die Starttemperatur gemessen ist (Temperaturdifferenz / Richtwert Aufheizrate %1 K/min).").arg(zahl(aufheizrate(), 2));
            return QVariant();
        }
        sollInfo = tr("Temperaturdifferenz (%1 → %2) geteilt durch den Richtwert Aufheizrate (%3 K/min). Bleibt Vorgabe; die gemessene Zeit zeigt nur die Abweichung.")
                       .arg(grad(tStart.toDouble()), grad(tZiel), zahl(aufheizrate(), 2));
        return QVariant(gSettings->aufheizdauer(tStart.toDouble(), tZiel));
    };

    add(ModelMesswerte::SchrittAussentemperatur, ModelMesswerte::GroesseTemperatur, tr("Außentemperatur am Brautag"),
        tr("Lufttemperatur [%1]").arg(C), C, 1, HilfeKeine, soll(ModelMesswerte::SchrittAussentemperatur, ModelMesswerte::GroesseTemperatur),
        tr("Temperatur der Umgebung am Brautag. Beeinflusst Aufheiz- und Abkühlzeiten. Wird nur dokumentiert und ändert das Rezept (z. B. die Bittere im Whirlpool) nicht."),
        tr("Außentemperatur aus dem Rezept (Whirlpool-Abkühlmodell)."));
    add(ModelMesswerte::SchrittSchuettung, ModelMesswerte::GroesseMasse, tr("Schüttung (gewogen)"),
        tr("Malzmenge gesamt [%1]").arg(kg), kg, 2, HilfeKeine, soll(ModelMesswerte::SchrittSchuettung, ModelMesswerte::GroesseMasse),
        tr("Tatsächlich abgewogene Malzmenge (alle Malze zusammen)."),
        tr("Gesamtschüttung laut Rezept."));

    // Hauptguss: Menge, Starttemperatur, Aufheizen auf Einmaischtemperatur
    ProxyModel *mp = mSud->modelMaischplan();
    const QString hg = tr("Hauptguss (Einmaischwasser)");
    add(ModelMesswerte::SchrittHauptguss, ModelMesswerte::GroesseVolumen, hg,
        tr("Wassermenge [%1]").arg(L), L, 1, HilfeVolumen, soll(ModelMesswerte::SchrittHauptguss, ModelMesswerte::GroesseVolumen),
        tr("Tatsächlich verwendetes Wasser zum Einmaischen."),
        tr("Hauptguss laut Wasserberechnung im Rezept."));
    add(ModelMesswerte::SchrittHauptguss, ModelMesswerte::GroesseTemperatur, hg,
        tr("Starttemperatur Wasser [%1]").arg(C), C, 1, HilfeKeine, QVariant(),
        tr("Temperatur des Hauptgusses vor dem Aufheizen (Leitungswasser)."), keinSoll);
    if (mp->rowCount() > 0 && static_cast<Brauhelfer::RastTyp>(mp->data(0, ModelMaischplan::ColTyp).toInt()) == Brauhelfer::RastTyp::Einmaischen)
    {
        const double tEin = mp->data(0, ModelMaischplan::ColTempWasser).toDouble();
        QString sollInfo;
        QVariant s = aufheizSoll(ModelMesswerte::SchrittHauptguss, tEin, sollInfo);
        add(ModelMesswerte::SchrittHauptguss, ModelMesswerte::GroesseDauerAufheizen, hg,
            tr("Aufheizdauer bis %1 [%2]").arg(grad(tEin), min), min, 0, HilfeKeine, s,
            tr("Zeit, bis der Hauptguss die Einmaischtemperatur (Wassertemperatur laut Maischplan) erreicht hat."), sollInfo);
    }

    // Maischschritte: Aufheizdauer (Soll = Temperaturdifferenz / Richtwert) und Rastdauer
    double tVorher = 0.0;
    for (int r = 0; r < mp->rowCount(); ++r)
    {
        const int schritt = ModelMesswerte::SchrittMaischschritt + r;
        const Brauhelfer::RastTyp typ = static_cast<Brauhelfer::RastTyp>(mp->data(r, ModelMaischplan::ColTyp).toInt());
        const double tRast = mp->data(r, ModelMaischplan::ColTempRast).toDouble();
        const QString tText = grad(tRast);
        const QString name = mp->data(r, ModelMaischplan::ColName).toString();
        const QString text = tr("Maischen %1: %2 (%3)").arg(r + 1).arg(name, tText);
        const QString info = tr("Schritt %1 des Maischplans.").arg(r + 1);
        if (typ != Brauhelfer::RastTyp::Einmaischen)
        {
            QVariant sollAufheizen;
            QString sollInfo;
            if (typ == Brauhelfer::RastTyp::Aufheizen && r > 0)
            {
                sollAufheizen = gSettings->aufheizdauer(tVorher, tRast);
                sollInfo = tr("Temperaturdifferenz zum vorigen Schritt (%1 → %2) geteilt durch den Richtwert Aufheizrate (%3 K/min). Bleibt Vorgabe; die gemessene Zeit zeigt nur die Abweichung.")
                               .arg(grad(tVorher), tText, zahl(aufheizrate(), 2));
            }
            else
            {
                sollInfo = tr("Kein Sollwert: Die Temperatur ändert sich hier durch Zubrühen, Zuschütten oder Dekoktion, nicht durch Aufheizen.");
            }
            add(schritt, ModelMesswerte::GroesseDauerAufheizen, text, tr("Aufheizdauer bis %1 [%2]").arg(tText, min), min, 0, HilfeKeine,
                sollAufheizen, info + QStringLiteral(" ") + tr("Aufheizdauer: Zeit vom Beginn des Aufheizens bis die Rasttemperatur erreicht ist."), sollInfo);
        }
        add(schritt, ModelMesswerte::GroesseDauerRast, text, tr("Rastdauer bei %1 [%2]").arg(tText, min), min, 0, HilfeKeine,
            mp->data(r, ModelMaischplan::ColDauerRast),
            info + QStringLiteral(" ") + tr("Rastdauer: Zeit, die die Maische auf der Rasttemperatur gehalten wurde."),
            tr("Rastdauer laut Maischplan."));
        tVorher = tRast;
    }

    // Laeuterruhe (ext-s12b): eigener Schritt, sofern sie nicht schon als Maischplan-Zeile dokumentiert wird
    bool laeuterruheImPlan = false;
    for (int r = 0; r < mp->rowCount(); ++r)
        if (mp->data(r, ModelMaischplan::ColName).toString().contains(tr("Läuterruhe"), Qt::CaseInsensitive))
            laeuterruheImPlan = true;
    if (!laeuterruheImPlan)
        add(ModelMesswerte::SchrittLaeuterruhe, ModelMesswerte::GroesseDauer, tr("Läuterruhe"),
            tr("Dauer [%1]").arg(min), min, 0, HilfeKeine, QVariant(),
            tr("Zeit vom Abmaischen bis zum Beginn des Abläuterns, in der sich der Treber setzt. "
               "Steht die Läuterruhe als Zeile im Maischplan, wird sie dort dokumentiert."), keinSoll);

    // Laeutern
    add(ModelMesswerte::SchrittLaeutern, ModelMesswerte::GroesseDauer, tr("Läutern"),
        tr("Läuterdauer [%1]").arg(min), min, 0, HilfeKeine, QVariant(),
        tr("Zeit vom Beginn des Abläuterns bis die Pfannevollwürze gesammelt ist (ohne Läuterruhe)."), keinSoll);
    add(ModelMesswerte::SchrittNachguss, ModelMesswerte::GroesseVolumen, tr("Nachguss (Läuterwasser)"),
        tr("Wassermenge [%1]").arg(L), L, 1, HilfeVolumen, soll(ModelMesswerte::SchrittNachguss, ModelMesswerte::GroesseVolumen),
        tr("Tatsächlich verwendetes Wasser zum Nachgießen beim Läutern."),
        tr("Nachguss laut Wasserberechnung im Rezept."));
    add(ModelMesswerte::SchrittVorderwuerze, ModelMesswerte::GroesseVolumen, tr("Vorderwürze (vor dem Nachguss)"),
        tr("Würzemenge [%1]").arg(L), L, 1, HilfeVolumen, QVariant(),
        tr("Würze, die vor dem ersten Nachguss aus dem Läuterbottich abläuft."), keinSoll);
    add(ModelMesswerte::SchrittVorderwuerze, ModelMesswerte::GroesseExtrakt, tr("Vorderwürze (vor dem Nachguss)"),
        tr("Stammwürze [%1]").arg(P), P, 1, HilfeExtrakt, QVariant(),
        tr("Extrakt der Vorderwürze. Über „...“ auch mit Spindel oder Refraktometer (Brix)."), keinSoll);

    // Pfannevollwuerze und Kochen
    const QString pv = tr("Pfannevollwürze (Kochbeginn)");
    add(ModelMesswerte::SchrittKochbeginn, ModelMesswerte::GroesseVolumen, pv,
        tr("Würzemenge [%1]").arg(L), L, 1, HilfeVolumenSudpfanne, soll(ModelMesswerte::SchrittKochbeginn, ModelMesswerte::GroesseVolumen),
        tr("Würze in der Sudpfanne nach dem Läutern. Über „...“ Messung von oben/unten mit den Maßen der Sudpfanne."),
        tr("Würzemenge Kochbeginn laut Rezept (bei 20 °C)."), tr("Braudaten: Würzemenge Kochbeginn"));
    add(ModelMesswerte::SchrittKochbeginn, ModelMesswerte::GroesseExtrakt, pv,
        tr("Stammwürze [%1]").arg(P), P, 1, HilfeExtrakt, soll(ModelMesswerte::SchrittKochbeginn, ModelMesswerte::GroesseExtrakt),
        tr("Stammwürze der Pfannevollwürze. Über „...“ auch mit Spindel oder Refraktometer (Brix)."),
        tr("Stammwürze Kochbeginn laut Rezept."), tr("Braudaten: Stammwürze Kochbeginn"));
    add(ModelMesswerte::SchrittKochbeginn, ModelMesswerte::GroesseTemperatur, pv,
        tr("Temperatur vor dem Aufheizen [%1]").arg(C), C, 1, HilfeKeine, QVariant(),
        tr("Temperatur der Pfannevollwürze, bevor sie zum Kochen aufgeheizt wird."), keinSoll);
    {
        QString sollInfo;
        QVariant s = aufheizSoll(ModelMesswerte::SchrittKochbeginn, 100.0, sollInfo);
        add(ModelMesswerte::SchrittKochen, ModelMesswerte::GroesseDauerAufheizen, tr("Kochen"),
            tr("Aufheizdauer bis zum Kochen [%1]").arg(min), min, 0, HilfeKeine, s,
            tr("Zeit vom Beginn des Aufheizens der Pfannevollwürze bis sie sprudelnd kocht."), sollInfo);
    }
    add(ModelMesswerte::SchrittKochen, ModelMesswerte::GroesseDauer, tr("Kochen"),
        tr("Kochdauer [%1]").arg(min), min, 0, HilfeKeine, soll(ModelMesswerte::SchrittKochen, ModelMesswerte::GroesseDauer),
        tr("Zeit vom Kochbeginn bis zum Ende des Kochens."), tr("Kochdauer laut Rezept."));
    add(ModelMesswerte::SchrittVorHopfenseihen, ModelMesswerte::GroesseVolumen, tr("Kochende, vor dem Hopfenseihen"),
        tr("Würzemenge [%1]").arg(L), L, 1, HilfeVolumenSudpfanne, soll(ModelMesswerte::SchrittVorHopfenseihen, ModelMesswerte::GroesseVolumen),
        tr("Würze in der Sudpfanne am Kochende, noch mit Hopfen."),
        tr("Würzemenge Kochende laut Rezept (bei 20 °C)."), tr("Braudaten: Würzemenge vor Hopfenseihen"));

    // Whirlpool / Nachisomerisierung
    const QString wp = tr("Whirlpool / Absetzen");
    add(ModelMesswerte::SchrittWhirlpool, ModelMesswerte::GroesseDauer, wp,
        tr("Dauer bis zum Ablassen [%1]").arg(min), min, 0, HilfeKeine, soll(ModelMesswerte::SchrittWhirlpool, ModelMesswerte::GroesseDauer),
        tr("Zeit vom Kochende bis zum Beginn des Ablassens bzw. Kühlens (Whirlpool, Absetzen, Hopfenseihen)."),
        tr("Nachisomerisierungs- bzw. Whirlpoolzeit laut Rezept."));
    {
        const bool whirlpool = bh->modelSud()->istWhirlpool(mSud->row());
        add(ModelMesswerte::SchrittWhirlpool, ModelMesswerte::GroesseTemperatur, wp,
            tr("Temperatur beim Ablassen [%1]").arg(C), C, 1, HilfeKeine,
            whirlpool ? mSud->getValue(ModelSud::ColWhirlpoolEndtemperatur) : QVariant(),
            tr("Würzetemperatur am Ende des Whirlpools, bevor gekühlt wird. Grundlage für die Kalibrierung des Abkühlmodells."),
            whirlpool ? tr("Berechnete Endtemperatur des Abkühlmodells (Whirlpool mit Abkühlkurve).")
                      : tr("Kein Sollwert: Modell „Whirlpool mit Abkühlkurve“ ist im Rezept nicht gewählt."));
    }
    add(ModelMesswerte::SchrittKochende, ModelMesswerte::GroesseVolumen, tr("Ausschlagwürze (nach dem Hopfenseihen)"),
        tr("Würzemenge [%1]").arg(L), L, 1, HilfeVolumen, soll(ModelMesswerte::SchrittKochende, ModelMesswerte::GroesseVolumen),
        tr("Würze nach dem Hopfenseihen bzw. Ausschlagen."),
        tr("Würzemenge Kochende laut Rezept (bei 20 °C)."), tr("Braudaten: Würzemenge Kochende"));
    add(ModelMesswerte::SchrittKochende, ModelMesswerte::GroesseExtrakt, tr("Ausschlagwürze (nach dem Hopfenseihen)"),
        tr("Stammwürze [%1]").arg(P), P, 1, HilfeExtrakt, soll(ModelMesswerte::SchrittKochende, ModelMesswerte::GroesseExtrakt),
        tr("Stammwürze am Kochende."),
        tr("Stammwürze Kochende laut Rezept."), tr("Braudaten: Stammwürze Kochende"));

    // Kuehlen und Anstellen
    add(ModelMesswerte::SchrittKuehlen, ModelMesswerte::GroesseDauer, tr("Kühlen"),
        tr("Kühldauer [%1]").arg(min), min, 0, HilfeKeine, QVariant(),
        tr("Zeit vom Beginn des Kühlens bis die Anstelltemperatur erreicht ist."), keinSoll);
    add(ModelMesswerte::SchrittKuehlen, ModelMesswerte::GroesseTemperatur, tr("Kühlen"),
        tr("Anstelltemperatur [%1]").arg(C), C, 1, HilfeKeine, QVariant(),
        tr("Temperatur der Würze beim Überführen in den Gärbehälter."), keinSoll);
    add(ModelMesswerte::SchrittAnstellen, ModelMesswerte::GroesseVolumen, tr("Anstellwürze (im Gärbehälter)"),
        tr("Würzemenge ohne Speise [%1]").arg(L), L, 1, HilfeVolumen, soll(ModelMesswerte::SchrittAnstellen, ModelMesswerte::GroesseVolumen),
        tr("Würze im Gärbehälter beim Anstellen (ohne zurückgehaltene Speise)."),
        tr("Anstellmenge laut Rezept."), tr("Braudaten: Würzemenge Anstellen"));
    add(ModelMesswerte::SchrittAnstellen, ModelMesswerte::GroesseExtrakt, tr("Anstellwürze (im Gärbehälter)"),
        tr("Stammwürze [%1]").arg(P), P, 1, HilfeExtrakt, soll(ModelMesswerte::SchrittAnstellen, ModelMesswerte::GroesseExtrakt),
        tr("Stammwürze beim Anstellen (nach evtl. Verdünnung)."),
        tr("Stammwürze laut Rezept."), tr("Braudaten: Stammwürze Anstellen"));

    // Abfuellen
    add(ModelMesswerte::SchrittAbfuellen, ModelMesswerte::GroesseVolumen, tr("Jungbier beim Abfüllen"),
        tr("Jungbiermenge [%1]").arg(L), L, 1, HilfeVolumen, soll(ModelMesswerte::SchrittAbfuellen, ModelMesswerte::GroesseVolumen),
        tr("Jungbier, das abgefüllt wird (ohne Speise und Zuckerlösung)."),
        keinSoll, tr("Abfüllen: Jungbiermenge"));
    add(ModelMesswerte::SchrittAbfuellen, ModelMesswerte::GroesseExtrakt, tr("Jungbier beim Abfüllen"),
        tr("Restextrakt [%1]").arg(P), P, 1, HilfeExtrakt, soll(ModelMesswerte::SchrittAbfuellen, ModelMesswerte::GroesseExtrakt),
        tr("Restextrakt des Jungbiers vor dem Abfüllen."),
        tr("Erwarteter Restextrakt aus dem Endvergärungsgrad."), tr("Abfüllen: Restextrakt Jungbier"));
    return liste;
}

void DlgBrauprotokoll::aktualisieren()
{
    QList<Eintrag> neu = eintraege();
    bool gleicheStruktur = neu.size() == mEintraege.size();
    for (int i = 0; gleicheStruktur && i < neu.size(); ++i)
        gleicheStruktur = neu[i].schritt == mEintraege[i].schritt && neu[i].groesse == mEintraege[i].groesse
                          && neu[i].schrittText == mEintraege[i].schrittText && neu[i].groesseText == mEintraege[i].groesseText;
    mEintraege = neu;
    if (!gleicheStruktur)
        aufbauen();
    werteSetzen();
}

void DlgBrauprotokoll::aufbauen()
{
    mWerteSetzen = true;
    ui->table->clearContents();
    ui->table->setRowCount(mEintraege.size());
    const QStringList titel = {tr("Brauschritt"), tr("Messgröße"), tr("Soll (Rezept)"), tr("Ist (gemessen)"), tr("Abweichung"),
                               tr("Messmethode"), tr("Erfasst am"), tr("Bemerkung"), QString()};
    const QStringList tips = {
        tr("Brauschritt, zu dem gemessen wird."),
        tr("Was gemessen wird, mit Einheit."),
        tr("Sollwert aus dem Rezept bzw. Richtwert. Mit der Maus über einen Sollwert fahren, um seine Herkunft zu sehen."),
        tr("Gemessener Wert. Zelle anklicken und tippen, mit Enter (oder Tab / Klick daneben) übernehmen. Leeren entfernt den Messwert. Leer = nicht gemessen, es gilt dann der Sollwert."),
        tr("Ist minus Soll; bei Mengen zusätzlich in Prozent."),
        tr("Womit gemessen wurde. Wird mit dem Messwert gespeichert."),
        tr("Zeitpunkt der letzten Änderung des Messwerts."),
        tr("Freie Bemerkung zur Messung (erst nach Eingabe eines Messwerts möglich)."),
        tr("Messhilfe: Volumen über Höhe bzw. Stammwürze über Spindel/Refraktometer bestimmen.")};
    for (int c = 0; c < SpAnzahl; ++c)
    {
        QTableWidgetItem *h = new QTableWidgetItem(titel[c]);
        h->setToolTip(tips[c]);
        ui->table->setHorizontalHeaderItem(c, h);
    }
    const QStringList methoden = {tr("Eingabe"), tr("Spindel"), tr("Refraktometer"), tr("Messstab/Skala"),
                                  tr("Waage"), tr("Thermometer"), tr("Uhr"), tr("geschätzt")};
    for (int i = 0; i < mEintraege.size(); ++i)
    {
        const Eintrag &e = mEintraege[i];
        auto item = [](const QString &text, const QString &tip, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
            QTableWidgetItem *it = new QTableWidgetItem(text);
            it->setTextAlignment(align);
            it->setToolTip(tip);
            it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            return it;
        };
        ui->table->setItem(i, SpSchritt, item(e.schrittText, e.info));
        ui->table->setItem(i, SpGroesse, item(e.groesseText, e.info));
        ui->table->setItem(i, SpSoll, item(QString(), e.sollInfo, Qt::AlignRight | Qt::AlignVCenter));
        QString istTip = tips[SpIst] + QStringLiteral("\n\n") +
                         (e.feld.isEmpty() ? tr("Wird nur im Protokoll gespeichert und ändert das Rezept nicht.")
                                           : tr("Wird übernommen in: %1. Dort ist der Wert dann vor Rezeptänderungen geschützt.").arg(e.feld));
        ui->table->setItem(i, SpIst, item(QString(), istTip, Qt::AlignRight | Qt::AlignVCenter));
        ui->table->setItem(i, SpAbweichung, item(QString(), tips[SpAbweichung], Qt::AlignRight | Qt::AlignVCenter));
        ui->table->setItem(i, SpZeit, item(QString(), tips[SpZeit]));
        ui->table->setItem(i, SpBemerkung, item(QString(), tips[SpBemerkung]));

        QComboBox *methode = new QComboBox(ui->table);
        methode->addItems(methoden);
        methode->setCurrentIndex(standardMethode(e.groesse));
        methode->setToolTip(tips[SpMethode]);
        connect(methode, &QComboBox::activated, this, [this, i](){methodeGeaendert(i);});
        ui->table->setCellWidget(i, SpMethode, methode);

        if (e.hilfe != HilfeKeine)
        {
            QToolButton *btn = new QToolButton(ui->table);
            btn->setText(QStringLiteral("..."));
            btn->setToolTip(e.hilfe == HilfeExtrakt ? tr("Stammwürze über Spindel, Refraktometer (Brix) oder direkte Eingabe in °P bestimmen")
                                                    : tr("Volumen über Messung von oben/unten bzw. mit Temperaturkorrektur bestimmen"));
            connect(btn, &QToolButton::clicked, this, [this, i](){hilfeGeklickt(i);});
            ui->table->setCellWidget(i, SpHilfe, btn);
        }
    }
    if (ui->table->horizontalHeader()->sectionSize(SpSchritt) < 150)
        ui->table->resizeColumnsToContents();
    mWerteSetzen = false;
}

int DlgBrauprotokoll::messwertZeile(int zeile) const
{
    const Eintrag &e = mEintraege[zeile];
    return bh->modelMesswerte()->zeile(mSud->id(), e.schritt, e.groesse);
}

QVariant DlgBrauprotokoll::messwert(int zeile) const
{
    const Eintrag &e = mEintraege[zeile];
    return bh->modelMesswerte()->wert(mSud->id(), e.schritt, e.groesse);
}

void DlgBrauprotokoll::werteSetzen()
{
    if (!mSud || !mSud->isLoaded())
    {
        ui->lblSud->setText(tr("Kein Sud geladen"));
        ui->lblMengeEffektiv->clear();
        return;
    }
    ui->lblSud->setText(tr("Brauprotokoll: %1").arg(mSud->getSudname()));
    const double menge = mSud->getValue(ModelSud::ColMengeEffektiv).toDouble();
    QString quelle;
    switch (static_cast<ModelSud::MengeQuelle>(mSud->getValue(ModelSud::ColMengeEffektivQuelle).toInt()))
    {
    case ModelSud::MengeQuelle::Gemessen: quelle = tr("gemessen"); break;
    case ModelSud::MengeQuelle::Braudaten: quelle = tr("aus den Braudaten"); break;
    default: quelle = tr("Sollwert, geschätzt"); break;
    }
    ui->lblMengeEffektiv->setText(tr("Effektive Biermenge: %1 L (%2)").arg(zahl(menge, 1), quelle));

    mWerteSetzen = true;
    const bool sperre = gesperrt();
    ModelMesswerte *mw = bh->modelMesswerte();
    QTableWidgetItem *aktiv = ui->table->currentItem();
    // Zelle, die gerade bearbeitet wird, nicht ueberschreiben (Editor liegt im Viewport der Tabelle)
    QWidget *fokus = QApplication::focusWidget();
    const bool editiert = fokus && ui->table->viewport()->isAncestorOf(fokus);
    for (int i = 0; i < mEintraege.size(); ++i)
    {
        const Eintrag &e = mEintraege[i];
        const int r = messwertZeile(i);
        const QVariant ist = messwert(i);
        const bool hatSoll = e.soll.isValid() && !e.soll.isNull();
        ui->table->item(i, SpSoll)->setText(hatSoll ? zahl(e.soll.toDouble(), e.dezimalen) : QStringLiteral("–"));
        QString abw;
        if (ist.isValid() && hatSoll)
        {
            const double d = ist.toDouble() - e.soll.toDouble();
            abw = (d > 0 ? QStringLiteral("+") : QString()) + zahl(d, e.dezimalen) + QStringLiteral(" ") + e.einheit;
            if ((e.groesse == ModelMesswerte::GroesseVolumen || e.groesse == ModelMesswerte::GroesseMasse) && e.soll.toDouble() > 0.0)
                abw += QStringLiteral(" (") + (d > 0 ? QStringLiteral("+") : QString()) + zahl(d / e.soll.toDouble() * 100.0, 0) + QStringLiteral(" %)");
        }
        ui->table->item(i, SpAbweichung)->setText(abw);
        ui->table->item(i, SpZeit)->setText(r >= 0 ? QLocale().toString(mw->data(r, ModelMesswerte::ColZeitstempel).toDateTime(), QLocale::ShortFormat) : QString());

        QTableWidgetItem *istItem = ui->table->item(i, SpIst);
        if (!(editiert && istItem == aktiv))
            istItem->setText(ist.isValid() ? istText(ist.toDouble(), e.dezimalen) : QString());
        istItem->setFlags(sperre ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable) : (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable));
        QFont f = istItem->font();
        f.setBold(ist.isValid());
        istItem->setFont(f);

        QComboBox *methode = static_cast<QComboBox*>(ui->table->cellWidget(i, SpMethode));
        if (r >= 0)
            methode->setCurrentIndex(mw->data(r, ModelMesswerte::ColMethode).toInt());
        methode->setEnabled(!sperre);

        QTableWidgetItem *bemItem = ui->table->item(i, SpBemerkung);
        if (!(editiert && bemItem == aktiv))
            bemItem->setText(r >= 0 ? mw->data(r, ModelMesswerte::ColBemerkung).toString() : QString());
        bemItem->setFlags((!sperre && r >= 0) ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable) : (Qt::ItemIsEnabled | Qt::ItemIsSelectable));

        QWidget *hilfe = ui->table->cellWidget(i, SpHilfe);
        if (hilfe)
            hilfe->setEnabled(!sperre);
    }
    mWerteSetzen = false;
}

void DlgBrauprotokoll::zelleGeaendert(QTableWidgetItem *item)
{
    if (mWerteSetzen || !item)
        return;
    if (item->column() == SpIst)
        istGeaendert(item->row(), item->text());
    else if (item->column() == SpBemerkung)
        bemerkungGeaendert(item->row(), item->text());
}

void DlgBrauprotokoll::setzen(int zeile, const QVariant &wert, int methode, const QString &bemerkung)
{
    const Eintrag &e = mEintraege[zeile];
    // Sollwert-Snapshot = angezeigter Sollwert (Richtwerte, Rezeptwerte); ohne Soll faellt ModelSud::setzeMesswert auf das Sud-Feld zurueck
    QVariant soll = e.soll.isValid() && !e.soll.isNull() ? e.soll : QVariant();
    gUndoStack->push(new SetMesswertCommand(bh, mSud->row(), e.schritt, e.groesse, wert, methode, bemerkung, soll));
}

void DlgBrauprotokoll::istGeaendert(int zeile, const QString &eingabe)
{
    if (zeile >= mEintraege.size() || gesperrt())
        return;
    const QString text = eingabe.trimmed();
    const QVariant alt = messwert(zeile);
    if (text.isEmpty())
    {
        if (alt.isValid())
            setzen(zeile, QVariant(), 0, QString());
        return;
    }
    bool ok = false;
    double wert = QLocale().toDouble(text, &ok);
    if (!ok)
        wert = QLocale::c().toDouble(text, &ok);
    if (!ok)
    {
        aktualisierenSpaeter();
        return;
    }
    if (alt.isValid() && alt.toDouble() == wert)
        return;
    // Anzeige ist gerundet: unveraenderter Anzeigetext darf den genauen Wert nicht ersetzen
    if (alt.isValid() && istText(alt.toDouble(), mEintraege[zeile].dezimalen) == text)
        return;
    QComboBox *methode = static_cast<QComboBox*>(ui->table->cellWidget(zeile, SpMethode));
    const int r = messwertZeile(zeile);
    QString bemerkung = r >= 0 ? bh->modelMesswerte()->data(r, ModelMesswerte::ColBemerkung).toString() : QString();
    setzen(zeile, wert, methode->currentIndex(), bemerkung);
}

void DlgBrauprotokoll::methodeGeaendert(int zeile)
{
    if (zeile >= mEintraege.size() || gesperrt())
        return;
    const QVariant wert = messwert(zeile);
    if (!wert.isValid())
        return;
    QComboBox *methode = static_cast<QComboBox*>(ui->table->cellWidget(zeile, SpMethode));
    const int r = messwertZeile(zeile);
    setzen(zeile, wert, methode->currentIndex(), bh->modelMesswerte()->data(r, ModelMesswerte::ColBemerkung).toString());
}

void DlgBrauprotokoll::bemerkungGeaendert(int zeile, const QString &text)
{
    if (zeile >= mEintraege.size() || gesperrt())
        return;
    const QVariant wert = messwert(zeile);
    const int r = messwertZeile(zeile);
    if (!wert.isValid() || r < 0)
        return;
    ModelMesswerte *mw = bh->modelMesswerte();
    if (mw->data(r, ModelMesswerte::ColBemerkung).toString() == text)
        return;
    setzen(zeile, wert, mw->data(r, ModelMesswerte::ColMethode).toInt(), text);
}

void DlgBrauprotokoll::hilfeGeklickt(int zeile)
{
    if (zeile >= mEintraege.size() || gesperrt())
        return;
    const Eintrag &e = mEintraege[zeile];
    const QVariant ist = messwert(zeile);
    const double start = ist.isValid() ? ist.toDouble() : (e.soll.isValid() ? e.soll.toDouble() : 0.0);
    const int r = messwertZeile(zeile);
    const QString bemerkung = r >= 0 ? bh->modelMesswerte()->data(r, ModelMesswerte::ColBemerkung).toString() : QString();
    if (e.hilfe == HilfeExtrakt)
    {
        DlgRestextrakt dlg(start, 0.0, -1, QDateTime(), this);
        if (dlg.exec() == QDialog::Accepted)
            setzen(zeile, dlg.value(), dlg.messmethode(), bemerkung);
    }
    else if (e.hilfe == HilfeVolumen || e.hilfe == HilfeVolumenSudpfanne)
    {
        double d = 0, h = 0;
        if (e.hilfe == HilfeVolumenSudpfanne && gSettings->isModuleEnabled(Settings::ModuleAusruestung))
        {
            d = mSud->getAnlageData(ModelAusruestung::ColSudpfanne_Durchmesser).toDouble();
            h = mSud->getAnlageData(ModelAusruestung::ColSudpfanne_Hoehe).toDouble();
        }
        DlgVolumen dlg(d, h, this);
        dlg.setValue(start);
        if (dlg.exec() == QDialog::Accepted)
            setzen(zeile, dlg.value(), ModelMesswerte::MethodeMessstab, bemerkung);
    }
}
