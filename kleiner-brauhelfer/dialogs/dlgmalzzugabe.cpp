#include "dlgmalzzugabe.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include "brauhelfer.h"
#include "biercalc.h"
#include "sudobject.h"
#include "proxymodel.h"
#include "modelmaischplan.h"
#include "modelmalzschuettung.h"
#include "settings.h"

extern Settings* gSettings;

enum Spalte { SpMalz, SpMenge, SpFarbe, SpAusbeute, SpHinweis, SpAnzahl };

DlgMalzZugabe::DlgMalzZugabe(SudObject *sud, int rowMaischplan, QWidget *parent) :
    DlgAbstract(staticMetaObject.className(), parent),
    mSud(sud),
    mKontaktzeit(0.0)
{
    setWindowTitle(tr("Malz-Zugabe im Maischplan"));

    ProxyModel *maischplan = mSud->modelMaischplan();
    mKontaktzeit = mSud->bh()->modelMaischplan()->kontaktzeit(maischplan->mapRowToSource(rowMaischplan));
    const QStringList gewaehlt = ModelMaischplan::malzListe(maischplan->data(rowMaischplan, ModelMaischplan::ColMalze));

    // Malze, die bereits in einer anderen Zeile zugegeben werden (die erste Zuordnung zaehlt)
    QMap<QString, QString> andereZeile;
    for (int r = 0; r < maischplan->rowCount(); ++r)
    {
        if (r == rowMaischplan)
            continue;
        if (static_cast<Brauhelfer::RastTyp>(maischplan->data(r, ModelMaischplan::ColTyp).toInt()) != Brauhelfer::RastTyp::Zuschuetten)
            continue;
        const QStringList namen = ModelMaischplan::malzListe(maischplan->data(r, ModelMaischplan::ColMalze));
        for (const QString &name : namen)
            if (!andereZeile.contains(name))
                andereZeile.insert(name, maischplan->data(r, ModelMaischplan::ColName).toString());
    }

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *lblInfo = new QLabel(tr("Angehakte Malze werden in diesem Schritt zugegeben, alle anderen beim Einmaischen. "
                                    "Die Malze stehen bereits in der Schüttung und zählen dort mit Menge, Extrakt und Farbe. "
                                    "Ein spät zugegebenes Malz (z. B. Farbmalz zur Läuterruhe) gibt nur einen Teil seiner Farbe ab – "
                                    "die Bierfarbe ist daher niedriger als beim Einmaischen; der Extrakt wird wie bisher voll gerechnet. "
                                    "Mehr Farbe gibt es über eine größere Menge in der Schüttung."), this);
    lblInfo->setWordWrap(true);
    layout->addWidget(lblInfo);

    mLblKontakt = new QLabel(this);
    mLblKontakt->setWordWrap(true);
    layout->addWidget(mLblKontakt);

    QHBoxLayout *layoutTau = new QHBoxLayout();
    QLabel *lblTau = new QLabel(tr("Zeitkonstante Farbausbeute τ"), this);
    mTau = new QDoubleSpinBox(this);
    mTau->setDecimals(1);
    mTau->setRange(0.5, 120.0);
    mTau->setSingleStep(1.0);
    mTau->setSuffix(tr(" min"));
    mTau->setValue(gSettings->farbmalzTau());
    const QString tipTau = tr("Farbausbeute f(t) = 1 − e^(−t/τ) mit der Kontaktzeit t. Richtwert τ = 10 min. "
                              "Annahme, ungeprüft: Gilt für alle Sude; bitte mit eigenen Farbmessungen abgleichen.");
    lblTau->setToolTip(tipTau);
    mTau->setToolTip(tipTau);
    QLabel *lblAnnahme = new QLabel(tr("(Annahme, ungeprüft)"), this);
    lblAnnahme->setToolTip(tipTau);
    layoutTau->addWidget(lblTau);
    layoutTau->addWidget(mTau);
    layoutTau->addWidget(lblAnnahme);
    layoutTau->addStretch();
    layout->addLayout(layoutTau);

    ProxyModel *schuettung = mSud->modelMalzschuettung();
    mTable = new QTableWidget(schuettung->rowCount(), SpAnzahl, this);
    mTable->setHorizontalHeaderLabels({tr("Malz"), tr("Menge (kg)"), tr("Farbe (EBC)"), tr("Farbausbeute"), tr("Hinweis")});
    mTable->verticalHeader()->setVisible(false);
    mTable->setSelectionMode(QAbstractItemView::NoSelection);
    mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QLocale locale;
    for (int r = 0; r < schuettung->rowCount(); ++r)
    {
        const QString name = schuettung->data(r, ModelMalzschuettung::ColName).toString().trimmed();
        QTableWidgetItem *item = new QTableWidgetItem(name);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(gewaehlt.contains(name) ? Qt::Checked : Qt::Unchecked);
        mTable->setItem(r, SpMalz, item);
        item = new QTableWidgetItem(locale.toString(schuettung->data(r, ModelMalzschuettung::Colerg_Menge).toDouble(), 'f', 2));
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        mTable->setItem(r, SpMenge, item);
        item = new QTableWidgetItem(locale.toString(schuettung->data(r, ModelMalzschuettung::ColFarbe).toDouble(), 'f', 0));
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        mTable->setItem(r, SpFarbe, item);
        item = new QTableWidgetItem();
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        mTable->setItem(r, SpAusbeute, item);
        QString hinweis;
        if (andereZeile.contains(name))
            hinweis = tr("bereits zugegeben in „%1“ (erste Zuordnung zählt)").arg(andereZeile.value(name));
        mTable->setItem(r, SpHinweis, new QTableWidgetItem(hinweis));
    }
    // Zugeordnete Namen, die nicht (mehr) in der Schuettung stehen, anzeigen, damit sie nicht still verschwinden
    for (const QString &name : gewaehlt)
    {
        if (!mTable->findItems(name, Qt::MatchExactly).isEmpty())
            continue;
        int r = mTable->rowCount();
        mTable->insertRow(r);
        QTableWidgetItem *item = new QTableWidgetItem(name);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        mTable->setItem(r, SpMalz, item);
        for (int c = SpMenge; c < SpHinweis; ++c)
            mTable->setItem(r, c, new QTableWidgetItem());
        mTable->setItem(r, SpHinweis, new QTableWidgetItem(tr("nicht in der Schüttung – wirkt nicht")));
    }
    mTable->horizontalHeader()->setSectionResizeMode(SpMalz, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(SpHinweis, QHeaderView::ResizeToContents);
    layout->addWidget(mTable);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    connect(mTau, &QDoubleSpinBox::valueChanged, this, &DlgMalzZugabe::updateAusbeute);
    updateAusbeute();
    resize(620, 380);
}

void DlgMalzZugabe::restoreView()
{
    DlgAbstract::restoreView(staticMetaObject.className());
}

void DlgMalzZugabe::updateAusbeute()
{
    double f = BierCalc::farbausbeute(mKontaktzeit, mTau->value());
    QLocale locale;
    mLblKontakt->setText(tr("Kontaktzeit ab diesem Schritt bis Ende Maischplan: %1 min (Rasten, Dekoktion und Aufheizzeiten "
                            "mit dem Richtwert Aufheizrate %2 K/min) → Farbausbeute %3 %.")
                         .arg(locale.toString(mKontaktzeit, 'f', 0),
                              locale.toString(mSud->bh()->aufheizrate(), 'f', 2),
                              locale.toString(f * 100, 'f', 0)));
    for (int r = 0; r < mTable->rowCount(); ++r)
    {
        if (!mTable->item(r, SpMenge)->text().isEmpty())
            mTable->item(r, SpAusbeute)->setText(locale.toString(f * 100, 'f', 0) + " %");
    }
}

QStringList DlgMalzZugabe::malze() const
{
    QStringList namen;
    for (int r = 0; r < mTable->rowCount(); ++r)
    {
        QTableWidgetItem *item = mTable->item(r, SpMalz);
        if (item->checkState() == Qt::Checked)
            namen.append(item->text());
    }
    return namen;
}

double DlgMalzZugabe::tau() const
{
    return mTau->value();
}
