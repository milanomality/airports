#include "MainWindow.h"

#include <QTableView>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUI();

    const QString defaultFile = QStringLiteral("airports.csv");

    if (m_manager.loadFromFile(defaultFile))
    {
        populateTable();
        updateAirportComboBoxes();
        m_statusLabel->setText(QStringLiteral("Загружено %1 аэропортов")
                                   .arg(m_manager.count()));
    }
}

void MainWindow::setupUI()
{
    setWindowTitle(QStringLiteral("Аэропорты — Поиск маршрутов"));
    resize(1000, 700);

    auto *central    = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    setCentralWidget(central);

    m_tableView = new QTableView(this);
    m_model     = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({
        QStringLiteral("Название"),
        QStringLiteral("Широта"),
        QStringLiteral("Долгота")
    });

    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->horizontalHeader()->setSectionResizeMode(0,
        QHeaderView::ResizeToContents);

    mainLayout->addWidget(new QLabel(QStringLiteral("Список аэропортов:"), this));
    mainLayout->addWidget(m_tableView);

    auto *ctrlGroup  = new QGroupBox(QStringLiteral("Параметры поиска"), this);
    auto *ctrlLayout = new QGridLayout(ctrlGroup);

    int row = 0;

    m_loadFileBtn = new QPushButton(QStringLiteral("Загрузить файл…"), this);
    ctrlLayout->addWidget(m_loadFileBtn, row++, 0);

    ctrlLayout->addWidget(
        new QLabel(QStringLiteral("Радиус поиска (км):"), this), row, 0);
    m_radiusEdit = new QLineEdit(QStringLiteral("500"), this);
    m_radiusEdit->setMaximumWidth(100);
    ctrlLayout->addWidget(m_radiusEdit, row, 1);
    m_findNearbyBtn = new QPushButton(QStringLiteral("Найти ближайшие"), this);
    ctrlLayout->addWidget(m_findNearbyBtn, row++, 2);

    ctrlLayout->addWidget(
        new QLabel(QStringLiteral("Макс. дальность (км):"), this), row, 0);
    m_maxRangeEdit = new QLineEdit(QStringLiteral("1000"), this);
    m_maxRangeEdit->setMaximumWidth(100);
    ctrlLayout->addWidget(m_maxRangeEdit, row++, 1);

    ctrlLayout->addWidget(
        new QLabel(QStringLiteral("От:"), this), row, 0);
    m_fromAirportCombo = new QComboBox(this);
    m_fromAirportCombo->setMinimumWidth(200);
    ctrlLayout->addWidget(m_fromAirportCombo, row++, 1, 1, 2);

    ctrlLayout->addWidget(
        new QLabel(QStringLiteral("До:"), this), row, 0);
    m_toAirportCombo = new QComboBox(this);
    m_toAirportCombo->setMinimumWidth(200);
    ctrlLayout->addWidget(m_toAirportCombo, row++, 1, 1, 2);

    m_findPathBtn = new QPushButton(QStringLiteral("Найти кратчайший путь"), this);
    ctrlLayout->addWidget(m_findPathBtn, row++, 0, 1, 3);

    mainLayout->addWidget(ctrlGroup);

    m_resultEdit = new QTextEdit(this);
    m_resultEdit->setReadOnly(true);
    m_resultEdit->setPlaceholderText(
        QStringLiteral("Здесь появятся результаты поиска…"));

    mainLayout->addWidget(new QLabel(QStringLiteral("Результаты:"), this));
    mainLayout->addWidget(m_resultEdit);

    m_statusLabel = new QLabel(QStringLiteral("Загрузите файл airports.csv"), this);
    mainLayout->addWidget(m_statusLabel);

    connect(m_loadFileBtn,    &QPushButton::clicked,
            this,             &MainWindow::onLoadFile);
    connect(m_findNearbyBtn,  &QPushButton::clicked,
            this,             &MainWindow::onFindNearby);
    connect(m_findPathBtn,    &QPushButton::clicked,
            this,             &MainWindow::onFindPath);
    connect(m_tableView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,             &MainWindow::onSelectionChanged);
}

void MainWindow::populateTable()
{
    m_model->removeRows(0, m_model->rowCount());

    for (const auto &a : m_manager.airports())
    {
        QList<QStandardItem *> row;
        row << new QStandardItem(a.name)
            << new QStandardItem(QStringLiteral("%1").arg(a.lat, 0, 'f', 6))
            << new QStandardItem(QStringLiteral("%1").arg(a.lon, 0, 'f', 6));
        m_model->appendRow(row);
    }
}

void MainWindow::updateAirportComboBoxes()
{
    m_fromAirportCombo->clear();
    m_toAirportCombo->clear();

    for (const auto &a : m_manager.airports())
    {
        const QString display = QStringLiteral("%1 (%2, %3)")
            .arg(a.name)
            .arg(a.lat, 0, 'f', 4)
            .arg(a.lon, 0, 'f', 4);

        m_fromAirportCombo->addItem(display);
        m_toAirportCombo->addItem(display);
    }
}

void MainWindow::onLoadFile()
{
    const QString filename = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Выберите файл airports.csv"),
        QString(),
        QStringLiteral("CSV Files (*.csv);;All Files (*)"));

    if (filename.isEmpty())
        return;

    if (!m_manager.loadFromFile(filename))
    {
        QMessageBox::warning(this, QStringLiteral("Ошибка"),
                             QStringLiteral("Не удалось загрузить аэропорты из файла"));
        return;
    }

    populateTable();
    updateAirportComboBoxes();
    m_statusLabel->setText(QStringLiteral("Загружено %1 аэропортов: %2")
                               .arg(m_manager.count()).arg(filename));
    m_resultEdit->clear();
}

void MainWindow::onSelectionChanged()
{
    const auto selected = m_tableView->selectionModel()->selectedRows();

    if (!selected.isEmpty())
    {
        const int row = selected.first().row();
        if (row < m_fromAirportCombo->count())
            m_fromAirportCombo->setCurrentIndex(row);
    }
}

void MainWindow::onFindNearby()
{
    const auto selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("Внимание"),
                             QStringLiteral("Выберите аэропорт в таблице"));
        return;
    }

    bool ok = false;
    const double radius = m_radiusEdit->text().toDouble(&ok);
    
    if (!ok || radius <= 0)
    {
        QMessageBox::warning(this, QStringLiteral("Внимание"),
                             QStringLiteral("Введите корректный радиус поиска"));
        return;
    }

    const int  idx     = selected.first().row();
    const auto results = m_manager.findNearby(idx, radius);
    displayNearbyResults(idx, results, radius);
}

void MainWindow::onFindPath()
{
    const int from = m_fromAirportCombo->currentIndex();
    const int to   = m_toAirportCombo->currentIndex();

    if (from < 0 || to < 0)
    {
        QMessageBox::warning(this, QStringLiteral("Внимание"),
                             QStringLiteral("Выберите начальный и конечный аэропорты"));
        return;
    }

    if (from == to)
    {
        QMessageBox::information(this, QStringLiteral("Информация"),
                                 QStringLiteral("Начальный и конечный аэропорты совпадают"));
        return;
    }

    bool ok = false;
    const double maxRange = m_maxRangeEdit->text().toDouble(&ok);

    if (!ok || maxRange <= 0)
    {
        QMessageBox::warning(this, QStringLiteral("Внимание"),
                             QStringLiteral("Введите корректную максимальную дальность"));
        return;
    }

    const auto path = m_manager.findShortestPath(from, to, maxRange);
    displayPathResults(path);
}

void MainWindow::displayNearbyResults(int selIdx,
                                       const QVector<NearbyResult> &results,
                                       double radius)
{
    const auto &center = m_manager.airports()[selIdx];

    QString out = QStringLiteral("=== Аэропорты в радиусе %1 км ===\n"
                                 "Центр: %2 (Lat: %3, Lon: %4)\n\n")
                      .arg(radius, 0, 'f', 1)
                      .arg(center.name)
                      .arg(center.lat, 0, 'f', 6)
                      .arg(center.lon, 0, 'f', 6);

    if (results.isEmpty())
    {
        out += QStringLiteral("Аэропорты не найдены\n");
    } 
    else 
    {
        out += QStringLiteral("Найдено: %1\n\n").arg(results.size());

        for (int i = 0; i < results.size(); ++i)
        {
            const auto &r = results[i];
            const auto &a = m_manager.airports()[r.index];

            out += QStringLiteral("%1.  %2  |  Lat: %3, Lon: %4  |  %5 км\n")
                       .arg(i + 1, -4)
                       .arg(a.name, -8)
                       .arg(a.lat, 0, 'f', 6)
                       .arg(a.lon, 0, 'f', 6)
                       .arg(r.distance, 0, 'f', 2);
        }
    }

    m_resultEdit->setPlainText(out);
}

void MainWindow::displayPathResults(const QVector<int> &path)
{
    const double maxRange = m_maxRangeEdit->text().toDouble();

    if (path.isEmpty())
    {
        m_resultEdit->setPlainText(
            QStringLiteral("=== Кратчайший путь ===\n\n"
                           "Путь не найден (макс. дальность сегмента: %1 км)\n")
                .arg(maxRange, 0, 'f', 1));
        return;
    }

    double totalDist = 0;
    QString out = QStringLiteral("=== Кратчайший путь ===\n\n"
                                 "Точек в маршруте: %1\n\n")
                      .arg(path.size());

    for (int i = 0; i < path.size(); ++i)
    {
        const auto &a = m_manager.airports()[path[i]];

        out += QStringLiteral("%1.  %2  (Lat: %3, Lon: %4)\n")
                   .arg(i + 1)
                   .arg(a.name)
                   .arg(a.lat, 0, 'f', 6)
                   .arg(a.lon, 0, 'f', 6);

        if (i > 0)
        {
            const double seg = m_manager.distanceBetween(path[i - 1], path[i]);
            totalDist += seg;
            out += QStringLiteral("     └─ от пред.: %1 км\n").arg(seg, 0, 'f', 2);
        }
    }

    out += QStringLiteral("\nОбщее расстояние: %1 км\n").arg(totalDist, 0, 'f', 2);
    m_resultEdit->setPlainText(out);
}
