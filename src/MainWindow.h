#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include "AirportManager.h"

class QTableView;
class QStandardItemModel;
class QLineEdit;
class QPushButton;
class QLabel;
class QTextEdit;
class QComboBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private:
    void onFindNearby();
    void onFindPath();
    void onLoadFile();
    void onSelectionChanged();

private:
    void setupUI();
    void populateTable();
    void updateAirportComboBoxes();

    void displayNearbyResults(int selIdx, const QVector<NearbyResult> &results, double radius);
    void displayPathResults(const QVector<int> &path);

    AirportManager m_manager;

    QTableView         *m_tableView      = nullptr;
    QStandardItemModel *m_model          = nullptr;
    QLineEdit          *m_radiusEdit     = nullptr;
    QLineEdit          *m_maxRangeEdit   = nullptr;
    QPushButton        *m_loadFileBtn    = nullptr;
    QPushButton        *m_findNearbyBtn  = nullptr;
    QPushButton        *m_findPathBtn    = nullptr;
    QLabel             *m_statusLabel    = nullptr;
    QComboBox          *m_fromAirportCombo = nullptr;
    QComboBox          *m_toAirportCombo   = nullptr;
    QTextEdit          *m_resultEdit     = nullptr;
};

#endif // MAIN_WINDOW_H
