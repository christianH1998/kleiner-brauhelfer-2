// clazy:excludeall=skipped-base-method
#include "modelmaischplan.h"
#include "proxymodel.h"
#include "brauhelfer.h"
#include "modelmalzschuettung.h"
#include "biercalc.h"
#include <QtMath>

ModelMaischplan::ModelMaischplan(Brauhelfer* bh, const QSqlDatabase &db) :
    SqlTableModel(bh, db),
    bh(bh),
    mUpdating(false)
{
    mVirtualField.append(QStringLiteral("MengeWasser"));
    mVirtualField.append(QStringLiteral("TotalMengeWasser"));
    mVirtualField.append(QStringLiteral("MengeMalz"));
    mVirtualField.append(QStringLiteral("TotalMengeMalz"));
    mVirtualField.append(QStringLiteral("MengeMaische"));
    connect(bh->modelSud(), &SqlTableModel::dataChanged, this, &ModelMaischplan::onSudDataChanged);
    connect(this, &SqlTableModel::rowsSwapped, this, &ModelMaischplan::onRowsSwapped);
}

QVariant ModelMaischplan::dataExt(const QModelIndex &idx) const
{
    double total;
    QVariant sudId;
    switch(idx.column())
    {
    case ColMengeWasser:
        sudId = data(idx.row(), ColSudID);
        total = bh->modelSud()->dataSud(sudId, ModelSud::Colerg_WHauptguss).toDouble();
        return data(idx.row(), ColAnteilWasser).toDouble() / 100 * total;
    case ColTotalMengeWasser:
        sudId = data(idx.row(), ColSudID);
        total = bh->modelSud()->dataSud(sudId, ModelSud::Colerg_WHauptguss).toDouble();
        return getTotalAnteil(sudId, idx.row(), ColAnteilWasser) / 100 * total;
    case ColMengeMalz:
        sudId = data(idx.row(), ColSudID);
        total = bh->modelSud()->dataSud(sudId, ModelSud::Colerg_S_Gesamt).toDouble();
        return data(idx.row(), ColAnteilMalz).toDouble() / 100 * total;
    case ColTotalMengeMalz:
        sudId = data(idx.row(), ColSudID);
        total = bh->modelSud()->dataSud(sudId, ModelSud::Colerg_S_Gesamt).toDouble();
        return getTotalAnteil(sudId, idx.row(), ColAnteilMalz) / 100 * total;;
    case ColMengeMaische:
        sudId = data(idx.row(), ColSudID);
        total = BierCalc::MalzVerdraengung * data(idx.row(), ColTotalMengeMalz).toDouble() + data(idx.row(), ColTotalMengeWasser).toDouble();
        return data(idx.row(), ColAnteilMaische).toDouble() / 100 * total;
    default:
        return QVariant();
    }
}

bool ModelMaischplan::setDataExt(const QModelIndex &idx, const QVariant &value)
{
    bool ret = false;
    double total;
    QVariant sudId = data(idx.row(), ColSudID);
    switch(idx.column())
    {
    case ColSudID:
        if (QSqlTableModel::setData(idx, value))
        {
            updateSud(value);
            ret = true;
        }
        break;
    case ColMengeWasser:
        total = bh->modelSud()->dataSud(sudId, ModelSud::Colerg_WHauptguss).toDouble();
        ret = setData(idx.row(), ColAnteilWasser, value.toDouble() / total * 100);
        break;
    case ColTotalMengeWasser:
        qWarning(Brauhelfer::loggingCategory) << "ModelMaischplan::setData(ColTotalMengeWasser) not implemented";
        return true;
    case ColMengeMalz:
        total = bh->modelSud()->dataSud(sudId, ModelSud::Colerg_S_Gesamt).toDouble();
        ret = setData(idx.row(), ColAnteilMalz, value.toDouble() / total * 100);
        break;
    case ColTotalMengeMalz:
        qWarning(Brauhelfer::loggingCategory) << "ModelMaischplan::setData(ColTotalMengeMalz) not implemented";
        return true;
    case ColMengeMaische:
        total = BierCalc::MalzVerdraengung * data(idx.row(), ColTotalMengeMalz).toDouble() + data(idx.row(), ColTotalMengeWasser).toDouble();
        ret = setData(idx.row(), ColAnteilMaische, value.toDouble() / total * 100);
        break;
    case ColMalze:
        // Malz-Zugabe (ext-s12b): Namen normalisiert speichern, Malzanteil der Zeile folgt der Schuettung
        if (QSqlTableModel::setData(idx, malzText(malzListe(value))))
        {
            if (static_cast<Brauhelfer::RastTyp>(data(idx.row(), ColTyp).toInt()) == Brauhelfer::RastTyp::Zuschuetten)
            {
                const QStringList namen = malzListe(data(idx.row(), ColMalze));
                if (!namen.isEmpty())
                    setzeAnteilZugabe(idx.row(), anteilMalze(sudId, namen));
            }
            ret = true;
        }
        break;
    case ColTyp:
        if (QSqlTableModel::setData(idx, value))
        {
            switch (static_cast<Brauhelfer::RastTyp>(value.toInt()))
            {
            case Brauhelfer::RastTyp::Einmaischen:
                QSqlTableModel::setData(index(idx.row(), ColAnteilWasser), 100);
                QSqlTableModel::setData(index(idx.row(), ColAnteilMalz), 100);
                QSqlTableModel::setData(index(idx.row(), ColAnteilMaische), 0);
                QSqlTableModel::setData(index(idx.row(), ColTempRast), getPreviousValueTemperature(idx.row()));
                QSqlTableModel::setData(index(idx.row(), ColDauerRast), 5);
                QSqlTableModel::setData(index(idx.row(), ColTempMalz), 21);
                break;
            case Brauhelfer::RastTyp::Aufheizen:
                QSqlTableModel::setData(index(idx.row(), ColAnteilWasser), 0);
                QSqlTableModel::setData(index(idx.row(), ColAnteilMalz), 0);
                QSqlTableModel::setData(index(idx.row(), ColAnteilMaische), 0);
                QSqlTableModel::setData(index(idx.row(), ColTempRast), getPreviousValueTemperature(idx.row()));
                QSqlTableModel::setData(index(idx.row(), ColDauerRast), 15);
                QSqlTableModel::setData(index(idx.row(), ColTempMalz), 0);
                break;
            case Brauhelfer::RastTyp::Zubruehen:
                QSqlTableModel::setData(index(idx.row(), ColAnteilWasser), 33.33);
                QSqlTableModel::setData(index(idx.row(), ColAnteilMalz), 0);
                QSqlTableModel::setData(index(idx.row(), ColAnteilMaische), 0);
                QSqlTableModel::setData(index(idx.row(), ColTempRast), getPreviousValueTemperature(idx.row()));
                QSqlTableModel::setData(index(idx.row(), ColDauerRast), 15);
                QSqlTableModel::setData(index(idx.row(), ColTempMalz), 0);
                break;
            case Brauhelfer::RastTyp::Zuschuetten:
                QSqlTableModel::setData(index(idx.row(), ColAnteilWasser), 20);
                QSqlTableModel::setData(index(idx.row(), ColAnteilMalz), 20);
                QSqlTableModel::setData(index(idx.row(), ColAnteilMaische), 0);
                QSqlTableModel::setData(index(idx.row(), ColTempRast), getPreviousValueTemperature(idx.row()));
                QSqlTableModel::setData(index(idx.row(), ColDauerRast), 15);
                QSqlTableModel::setData(index(idx.row(), ColTempMalz), 21);
                break;
            case Brauhelfer::RastTyp::Dekoktion:
                QSqlTableModel::setData(index(idx.row(), ColAnteilWasser), 0);
                QSqlTableModel::setData(index(idx.row(), ColAnteilMalz), 0);
                QSqlTableModel::setData(index(idx.row(), ColDauerRast), 15);
                QSqlTableModel::setData(index(idx.row(), ColTempMalz), 0);
                QSqlTableModel::setData(index(idx.row(), ColTempExtra1), 95);
                QSqlTableModel::setData(index(idx.row(), ColDauerExtra1), 15);
                QSqlTableModel::setData(index(idx.row(), ColTempExtra2), 0);
                QSqlTableModel::setData(index(idx.row(), ColDauerExtra2), 0);
                setData(index(idx.row(), ColAnteilMaische), 50);
                break;
            }
            ret = true;
        }
        break;
    case ColTempRast:
        if (static_cast<Brauhelfer::RastTyp>(data(idx.row(), ColTyp).toInt()) == Brauhelfer::RastTyp::Dekoktion)
        {
            if (QSqlTableModel::setData(idx, value))
            {
                double m_maische0 = 0, c_maische0 = 0, T_maische0 = 0;
                getPreviousMaischeValues(idx.row(), m_maische0, c_maische0, T_maische0);
                double T_rast = data(idx.row(), ColTempRast).toDouble();
              #if 0 // -> ColTempExtra1
                double m_maische = data(idx.row(), ColAnteilMaische).toDouble() / 100 * m_maische0;
                double T_maische = BierCalc::mischungstemperaturT2(T_rast, m_maische0 - m_maische, c_maische0, T_maische0, m_maische, c_maische0);
                QSqlTableModel::setData(index(idx.row(), ColTempExtra1), T_maische);
              #else // -> ColAnteilMaische
                double T_maische = data(idx.row(), ColTempExtra1).toDouble();
                double anteil = (T_rast - T_maische0) / (T_maische - T_maische0);
                QSqlTableModel::setData(index(idx.row(), ColAnteilMaische), anteil * 100);
              #endif
                ret = true;
            }
            break;
        }
        [[fallthrough]];
    case ColAnteilWasser:
    case ColAnteilMalz:
        if (QSqlTableModel::setData(idx, value))
        {
            double m_wasser = data(idx.row(), ColMengeWasser).toDouble() * BierCalc::dichteWasser(20);
            double m_malz = data(idx.row(), ColMengeMalz).toDouble();
            if (m_wasser > 0 || m_malz > 0)
            {
                double T_rast = data(idx.row(), ColTempRast).toDouble();
                double m_maische0 = 0, c_maische0 = 0, T_maische0 = 0;
                getPreviousMaischeValues(idx.row(), m_maische0, c_maische0, T_maische0);
                if (m_wasser > 0)
                {
                    double T_malz = data(idx.row(), ColTempMalz).toDouble();
                    double m_zugabe = m_malz + m_wasser;
                    double c_zugabe = (m_malz * BierCalc::cMalz + m_wasser * BierCalc::cWasser) / m_zugabe;
                    double T_zugabe = BierCalc::mischungstemperaturT2(T_rast, m_maische0, c_maische0, T_maische0, m_zugabe, c_zugabe);
                    double T_wasser = BierCalc::mischungstemperaturT2(T_zugabe, m_malz, BierCalc::cMalz, T_malz, m_wasser, BierCalc::cWasser);
                    QSqlTableModel::setData(index(idx.row(), ColTempWasser), T_wasser);
                }
                else
                {
                    double T_malz = BierCalc::mischungstemperaturT2(T_rast, m_maische0, c_maische0, T_maische0, m_malz, BierCalc::cMalz);
                    QSqlTableModel::setData(index(idx.row(), ColTempMalz), T_malz);
                }
            }
            ret = true;
        }
        break;
    case ColTempWasser:
        if (QSqlTableModel::setData(idx, value))
        {
            double m_wasser = data(idx.row(), ColMengeWasser).toDouble() * BierCalc::dichteWasser(20);
            double m_malz = data(idx.row(), ColMengeMalz).toDouble();
            if (m_wasser > 0 || m_malz > 0)
            {
                double T_wasser = data(idx.row(), ColTempWasser).toDouble();
                double T_malz = data(idx.row(), ColTempMalz).toDouble();
                double T_zugabe = BierCalc::mischungstemperaturTm(m_malz, BierCalc::cMalz, T_malz, m_wasser, BierCalc::cWasser, T_wasser);
                double m_zugabe = m_malz + m_wasser;
                double c_zugabe = (m_malz * BierCalc::cMalz + m_wasser * BierCalc::cWasser) / m_zugabe;
                double m_maische0 = 0, c_maische0 = 0, T_maische0 = 0;
                getPreviousMaischeValues(idx.row(), m_maische0, c_maische0, T_maische0);
                double T_rast = BierCalc::mischungstemperaturTm(m_maische0, c_maische0, T_maische0, m_zugabe, c_zugabe, T_zugabe);
                QSqlTableModel::setData(index(idx.row(), ColTempRast), T_rast);
            }
            ret = true;
        }
        break;
    case ColTempMalz:
        if (QSqlTableModel::setData(idx, value))
        {
            double m_wasser = data(idx.row(), ColMengeWasser).toDouble() * BierCalc::dichteWasser(20);
            double m_malz = data(idx.row(), ColMengeMalz).toDouble();
            if (m_wasser > 0 || m_malz > 0)
            {
                double T_malz = data(idx.row(), ColTempMalz).toDouble();
                double m_maische0 = 0, c_maische0 = 0, T_maische0 = 0;
                getPreviousMaischeValues(idx.row(), m_maische0, c_maische0, T_maische0);
                if (m_wasser > 0)
                {
                    double T_rast = data(idx.row(), ColTempRast).toDouble();
                    double m_zugabe = m_malz + m_wasser;
                    double c_zugabe = (m_malz * BierCalc::cMalz + m_wasser * BierCalc::cWasser) / m_zugabe;
                    double T_zugabe = BierCalc::mischungstemperaturT2(T_rast, m_maische0, c_maische0, T_maische0, m_zugabe, c_zugabe);
                    double T_wasser = BierCalc::mischungstemperaturT2(T_zugabe, m_malz, BierCalc::cMalz, T_malz, m_wasser, BierCalc::cWasser);
                    QSqlTableModel::setData(index(idx.row(), ColTempWasser), T_wasser);
                }
                else
                {
                    double T_rast = BierCalc::mischungstemperaturTm(m_maische0, c_maische0, T_maische0, m_malz, BierCalc::cMalz, T_malz);
                    QSqlTableModel::setData(index(idx.row(), ColTempRast), T_rast);
                }
            }
            ret = true;
        }
        break;
    case ColAnteilMaische:
        if (QSqlTableModel::setData(idx, value))
        {
            double m_maische0 = 0, c_maische0 = 0, T_maische0 = 0;
            getPreviousMaischeValues(idx.row(), m_maische0, c_maische0, T_maische0);
            double m_maische = data(idx.row(), ColAnteilMaische).toDouble() / 100 * m_maische0;
          #if 0 // -> ColTempExtra1
            double T_rast = data(idx.row(), ColTempRast).toDouble();
            double T_maische = BierCalc::mischungstemperaturT2(T_rast, m_maische0 - m_maische, c_maische0, T_maische0, m_maische, c_maische0);
            QSqlTableModel::setData(index(idx.row(), ColTempExtra1), T_maische);
          #else // -> ColTempRast
            double T_maische = data(idx.row(), ColTempExtra1).toDouble();
            double T_rast = BierCalc::mischungstemperaturTm(m_maische0 - m_maische, c_maische0, T_maische0, m_maische, c_maische0, T_maische);
            QSqlTableModel::setData(index(idx.row(), ColTempRast), T_rast);
          #endif
            ret = true;
        }
        break;
    case ColTempExtra1:
        if (QSqlTableModel::setData(idx, value))
        {
            double m_maische0 = 0, c_maische0 = 0, T_maische0 = 0;
            getPreviousMaischeValues(idx.row(), m_maische0, c_maische0, T_maische0);
            double T_maische = data(idx.row(), ColTempExtra1).toDouble();
          #if 0 // -> ColTempRast
            double m_maische = data(idx.row(), ColAnteilMaische).toDouble() / 100 * m_maische0;
            double T_rast = BierCalc::mischungstemperaturTm(m_maische0 - m_maische, c_maische0, T_maische0, m_maische, c_maische0, T_maische);
            QSqlTableModel::setData(index(idx.row(), ColTempRast), T_rast);
          #else // -> ColAnteilMaische
            double T_rast = data(idx.row(), ColTempRast).toDouble();
            double anteil = (T_rast - T_maische0) / (T_maische - T_maische0);
            QSqlTableModel::setData(index(idx.row(), ColAnteilMaische), anteil * 100);
          #endif
            ret = true;
        }
        break;
    default:
        break;
    }
    if (ret)
        updateSud(sudId);
    return ret;
}

QStringList ModelMaischplan::malzListe(const QVariant &value)
{
    QStringList namen;
    const QStringList teile = value.toString().split(QLatin1Char('\n'));
    for (const QString &teil : teile)
    {
        const QString name = teil.trimmed();
        if (!name.isEmpty() && !namen.contains(name))
            namen.append(name);
    }
    return namen;
}

QString ModelMaischplan::malzText(const QStringList &namen)
{
    return namen.join(QLatin1Char('\n'));
}

double ModelMaischplan::anteilMalze(const QVariant &sudId, const QStringList &namen) const
{
    ProxyModel model;
    model.setSourceModel(bh->modelMalzschuettung());
    model.setFilterKeyColumn(ModelMalzschuettung::ColSudID);
    model.setFilterRegularExpression(QStringLiteral("^%1$").arg(sudId.toInt()));
    double anteil = 0.0;
    for (int r = 0; r < model.rowCount(); r++)
    {
        if (namen.contains(model.data(r, ModelMalzschuettung::ColName).toString().trimmed()))
            anteil += model.data(r, ModelMalzschuettung::ColProzent).toDouble();
    }
    return anteil;
}

void ModelMaischplan::setzeAnteilZugabe(int row, double anteil)
{
    // Malzanteil der Zugabe-Zeile setzen; die Differenz geht zu Lasten der ersten Zeile
    // "Einmaischen" (nicht zugeordnete Malze werden eingemaischt), damit die Summe gleich bleibt
    double delta = anteil - data(row, ColAnteilMalz).toDouble();
    QSqlTableModel::setData(index(row, ColAnteilMalz), anteil);
    if (delta == 0.0)
        return;
    ProxyModel model;
    model.setSourceModel(this);
    model.setFilterKeyColumn(ColSudID);
    model.setFilterRegularExpression(QStringLiteral("^%1$").arg(data(row, ColSudID).toInt()));
    for (int r = 0; r < model.rowCount(); r++)
    {
        int rowSrc = model.mapRowToSource(r);
        if (rowSrc == row)
            continue;
        if (static_cast<Brauhelfer::RastTyp>(data(rowSrc, ColTyp).toInt()) == Brauhelfer::RastTyp::Einmaischen)
        {
            double rest = data(rowSrc, ColAnteilMalz).toDouble() - delta;
            if (rest >= 0.0 && rest <= 100.0)
                QSqlTableModel::setData(index(rowSrc, ColAnteilMalz), rest);
            break;
        }
    }
}

double ModelMaischplan::kontaktzeit(int row) const
{
    ProxyModel model;
    model.setSourceModel(const_cast<ModelMaischplan*>(this));
    model.setFilterKeyColumn(ColSudID);
    model.setFilterRegularExpression(QStringLiteral("^%1$").arg(data(row, ColSudID).toInt()));
    int r0 = model.mapRowFromSource(row);
    if (r0 < 0)
        return 0.0;
    // Zugabe-Zeile: nur ihre Rastdauer; danach alle Schritte wie im Maischplan-Diagramm (ext-s12)
    double t = model.data(r0, ColDauerRast).toDouble();
    double lastT = model.data(r0, ColTempRast).toDouble();
    for (int r = r0 + 1; r < model.rowCount(); r++)
    {
        double T = model.data(r, ColTempRast).toDouble();
        switch (static_cast<Brauhelfer::RastTyp>(model.data(r, ColTyp).toInt()))
        {
        case Brauhelfer::RastTyp::Aufheizen:
            if (T > lastT)
                t += qRound((T - lastT) / bh->aufheizrate());
            break;
        case Brauhelfer::RastTyp::Dekoktion:
            t += model.data(r, ColDauerExtra1).toDouble() + model.data(r, ColDauerExtra2).toDouble();
            break;
        default:
            break;
        }
        t += model.data(r, ColDauerRast).toDouble();
        lastT = T;
    }
    return t;
}

QMap<QString, double> ModelMaischplan::malzZugaben(const QVariant &sudId) const
{
    QMap<QString, double> zugaben;
    ProxyModel model;
    model.setSourceModel(const_cast<ModelMaischplan*>(this));
    model.setFilterKeyColumn(ColSudID);
    model.setFilterRegularExpression(QStringLiteral("^%1$").arg(sudId.toInt()));
    for (int r = 0; r < model.rowCount(); r++)
    {
        if (static_cast<Brauhelfer::RastTyp>(model.data(r, ColTyp).toInt()) != Brauhelfer::RastTyp::Zuschuetten)
            continue;
        const QStringList namen = malzListe(model.data(r, ColMalze));
        if (namen.isEmpty())
            continue;
        double t = kontaktzeit(model.mapRowToSource(r));
        for (const QString &name : namen)
        {
            if (!zugaben.contains(name))
                zugaben.insert(name, t);
        }
    }
    return zugaben;
}

bool ModelMaischplan::hatMalzZugabe(const QVariant &sudId) const
{
    ProxyModel model;
    model.setSourceModel(const_cast<ModelMaischplan*>(this));
    model.setFilterKeyColumn(ColSudID);
    model.setFilterRegularExpression(QStringLiteral("^%1$").arg(sudId.toInt()));
    for (int r = 0; r < model.rowCount(); r++)
    {
        if (static_cast<Brauhelfer::RastTyp>(model.data(r, ColTyp).toInt()) == Brauhelfer::RastTyp::Zuschuetten &&
            !malzListe(model.data(r, ColMalze)).isEmpty())
            return true;
    }
    return false;
}

void ModelMaischplan::aktualisiereMalzZugabe(const QVariant &sudId)
{
    if (mUpdating)
        return;
    bool geaendert = false;
    mSignalModifiedBlocked = true;
    ProxyModel model;
    model.setSourceModel(this);
    model.setFilterKeyColumn(ColSudID);
    model.setFilterRegularExpression(QStringLiteral("^%1$").arg(sudId.toInt()));
    for (int r = 0; r < model.rowCount(); r++)
    {
        if (static_cast<Brauhelfer::RastTyp>(model.data(r, ColTyp).toInt()) != Brauhelfer::RastTyp::Zuschuetten)
            continue;
        const QStringList namen = malzListe(model.data(r, ColMalze));
        if (namen.isEmpty())
            continue;
        double anteil = anteilMalze(sudId, namen);
        if (anteil != model.data(r, ColAnteilMalz).toDouble())
        {
            setzeAnteilZugabe(model.mapRowToSource(r), anteil);
            geaendert = true;
        }
    }
    mSignalModifiedBlocked = false;
    if (geaendert)
        updateSud(sudId);
}

double ModelMaischplan::getTotalAnteil(const QVariant &sudId, int rowFrom, int colAnteil) const
{
    ProxyModel model;
    model.setSourceModel(const_cast<ModelMaischplan*>(this));
    model.setFilterKeyColumn(ColSudID);
    model.setFilterRegularExpression(QStringLiteral("^%1$").arg(sudId.toInt()));
    rowFrom = model.mapRowFromSource(rowFrom);
    double anteil = 0.0;
    for (int r = 0; r <= rowFrom; r++)
        anteil += model.data(r, colAnteil).toDouble();
    return anteil;
}

double ModelMaischplan::getPreviousValueTemperature(int row) const
{
    ProxyModel model;
    model.setSourceModel(const_cast<ModelMaischplan*>(this));
    model.setFilterKeyColumn(ColSudID);
    model.setFilterRegularExpression(QStringLiteral("^%1$").arg(data(row, ColSudID).toInt()));
    row = model.mapRowFromSource(row);
    if (row > 0)
    {
        return model.data(row - 1, ColTempRast).toDouble();
    }
    return 57;
}

void ModelMaischplan::getPreviousMaischeValues(int row, double& m, double &c, double &T) const
{
    ProxyModel model;
    model.setSourceModel(const_cast<ModelMaischplan*>(this));
    model.setFilterKeyColumn(ColSudID);
    model.setFilterRegularExpression(QStringLiteral("^%1$").arg(data(row, ColSudID).toInt()));
    row = model.mapRowFromSource(row);
    if (row > 0)
    {
        double m_malz = model.data(row - 1, ColTotalMengeMalz).toDouble();
        double m_wasser = model.data(row - 1, ColTotalMengeWasser).toDouble() * BierCalc::dichteWasser(20);
        m = m_malz + m_wasser;
        c = (m_malz * BierCalc::cMalz + m_wasser * BierCalc::cWasser) / m;
        T = model.data(row - 1, ColTempRast).toDouble();
    }
    else
    {
        m = 0;
        c = 0;
        T = 0;
    }
}

void ModelMaischplan::updateSud(const QVariant &sudId)
{
    if (mUpdating)
        return;
    mUpdating = true;
    mSignalModifiedBlocked = true;
    ProxyModel model;
    model.setSourceModel(this);
    model.setFilterKeyColumn(ColSudID);
    model.setFilterRegularExpression(QStringLiteral("^%1$").arg(sudId.toInt()));
    for (int r = 0; r < model.rowCount(); r++)
        model.setData(r, ColTempRast, model.data(r, ColTempRast));
    mSignalModifiedBlocked = false;
    mUpdating = false;
}

void ModelMaischplan::onSudDataChanged(const QModelIndex &idx)
{
    switch (idx.column())
    {
    case ModelSud::Colerg_S_Gesamt:
    case ModelSud::Colerg_WHauptguss:
        updateSud(bh->modelSud()->data(idx.row(), ModelSud::ColID));
        break;
    }
}

void ModelMaischplan::onRowsSwapped(int row1, int row2)
{
    QVariant sudId1 = data(row1, ColSudID);
    QVariant sudId2 = data(row2, ColSudID);
    updateSud(sudId1);
    if (sudId2 != sudId1)
        updateSud(sudId2);
}
