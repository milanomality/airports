#include <gtest/gtest.h>
#include "AirportManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <QCoreApplication>


static QString createTempCsv(const QString &content)
{
    const QString path = QDir::tempPath() + "/test_airports_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".csv";
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return QString();
    QTextStream out(&f);
    out << content;
    f.close();
    return path;
}

static QString findAirportsCsv()
{
    const QStringList candidates = {
        "airports.csv",
        "res/airports.csv",
        "../res/airports.csv",
        "../../res/airports.csv",
        "../airports.csv",
    };
    for (const auto &path : candidates) {
        if (QFile::exists(path))
            return QFileInfo(path).absoluteFilePath();
    }
    return QString();
}


TEST(LoadFromFile, loadsValidEntries)
{
    const QString csv =
        "name\tlat\tlon\n"
        "AAAA\t55.0\t37.0\n"
        "BBBB\t48.0\t2.0\n";

    const QString path = createTempCsv(csv);

    AirportManager mgr;
    EXPECT_TRUE(mgr.loadFromFile(path));
    EXPECT_EQ(mgr.count(), 2);

    QFile::remove(path);
}

TEST(LoadFromFile, returnsFalseForMissingFile)
{
    AirportManager mgr;
    EXPECT_FALSE(mgr.loadFromFile("/nonexistent/path/file.csv"));
    EXPECT_EQ(mgr.count(), 0);
}

TEST(LoadFromFile, preservesExistingDataOnFailedLoad)
{
    const QString csv =
        "name\tlat\tlon\n"
        "AAAA\t55.0\t37.0\n";
    const QString path = createTempCsv(csv);

    AirportManager mgr;
    mgr.loadFromFile(path);
    EXPECT_EQ(mgr.count(), 1);

    // Попытка загрузить несуществующий файл не должна удалить данные
    mgr.loadFromFile("/nonexistent/file.csv");
    EXPECT_EQ(mgr.count(), 1);
    EXPECT_EQ(mgr.airports()[0].name, "AAAA");

    QFile::remove(path);
}

TEST(LoadFromFile, skipsMalformedLines)
{
    const QString csv =
        "name\tlat\tlon\n"
        "AAAA\t55.0\t37.0\n"
        "bad_line\n"
        "BBBB\tnot_a_number\t2.0\n"
        "\t48.0\t2.0\n"
        "CCCC\t48.0\t2.0\n";

    const QString path = createTempCsv(csv);

    AirportManager mgr;
    mgr.loadFromFile(path);
    EXPECT_EQ(mgr.count(), 2);

    QFile::remove(path);
}

TEST(LoadFromFile, commaSeparated)
{
    const QString csv =
        "name,lat,lon\n"
        "AAAA,55.0,37.0\n"
        "BBBB,48.0,2.0\n";

    const QString path = createTempCsv(csv);

    AirportManager mgr;
    mgr.loadFromFile(path);
    EXPECT_EQ(mgr.count(), 2);

    QFile::remove(path);
}


TEST(FindByName, returnsCorrectIndex)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);

    if (mgr.count() > 0) {
        const auto &first = mgr.airports()[0];
        auto result = mgr.findByName(first.name);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), 0);
    }
}

TEST(FindByName, returnsNulloptForMissing)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);
    EXPECT_FALSE(mgr.findByName("ZZZZ").has_value());
}


TEST(DistanceBetween, sameAirportIsZero)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);

    if (mgr.count() > 0) {
        double d = mgr.distanceBetween(0, 0);
        EXPECT_DOUBLE_EQ(d, 0.0);
    }
}

TEST(DistanceBetween, symmetric)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);

    if (mgr.count() >= 2) {
        double d1 = mgr.distanceBetween(0, 1);
        double d2 = mgr.distanceBetween(1, 0);
        EXPECT_DOUBLE_EQ(d1, d2);
    }
}

TEST(DistanceBetween, knownValue)
{
    // Тест на известных координатах: Москва (55.75, 37.62) → Санкт-Петербург (59.93, 30.32)
    // Ожидаемое расстояние ~635 км
    Airport a1{"UUUU", 55.75, 37.62};
    Airport a2{"ULLL", 59.93, 30.32};

    const QString csv =
        "name\tlat\tlon\n"
        "UUUU\t55.75\t37.62\n"
        "ULLL\t59.93\t30.32\n";

    const QString path = createTempCsv(csv);
    AirportManager mgr;
    mgr.loadFromFile(path);

    double d = mgr.distanceBetween(0, 1);
    EXPECT_NEAR(d, 635.0, 10.0);

    QFile::remove(path);
}

TEST(DistanceBetween, returnsNegativeForInvalidIndices)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);

    if (mgr.count() > 0) {
        EXPECT_LT(mgr.distanceBetween(-1, 0), 0);
        EXPECT_LT(mgr.distanceBetween(0, -1), 0);
        EXPECT_LT(mgr.distanceBetween(mgr.count(), 0), 0);
        EXPECT_LT(mgr.distanceBetween(0, mgr.count()), 0);
    }
}

TEST(FindNearby, zeroRadiusReturnsEmpty)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);

    if (mgr.count() > 1) {
        auto res = mgr.findNearby(0, 0.0);
        EXPECT_TRUE(res.isEmpty());
    }
}

TEST(FindNearby, largeRadiusFindsAll)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);

    if (mgr.count() > 1) {
        auto res = mgr.findNearby(0, 50000.0);
        EXPECT_EQ(res.size(), mgr.count() - 1);
    }
}

TEST(FindNearby, returnsEmptyForInvalidIndex)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);

    EXPECT_TRUE(mgr.findNearby(-1, 1000).isEmpty());
    EXPECT_TRUE(mgr.findNearby(mgr.count(), 1000).isEmpty());
}

TEST(FindNearby, sortedByDistance)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);

    if (mgr.count() > 2) {
        auto res = mgr.findNearby(0, 20000.0);
        for (int i = 1; i < res.size(); ++i) {
            EXPECT_LE(res[i - 1].distance, res[i].distance);
        }
    }
}

TEST(FindShortestPath, sameStartAndEnd)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);

    if (mgr.count() > 0) {
        auto path = mgr.findShortestPath(0, 0, 1000.0);
        ASSERT_FALSE(path.isEmpty());
        ASSERT_EQ(path.size(), 1);
        EXPECT_EQ(path[0], 0);
    }
}

TEST(FindShortestPath, directFlightPossible)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);

    if (mgr.count() >= 2) {
        double d = mgr.distanceBetween(0, 1);
        auto path = mgr.findShortestPath(0, 1, d + 100);
        ASSERT_EQ(path.size(), 2);
        EXPECT_EQ(path[0], 0);
        EXPECT_EQ(path[1], 1);
    }
}

TEST(FindShortestPath, noPathWhenRangeTooSmall)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);

    if (mgr.count() >= 2) {
        auto path = mgr.findShortestPath(0, 1, 0.1);
        EXPECT_TRUE(path.isEmpty());
    }
}

TEST(FindShortestPath, returnsEmptyForInvalidIndices)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);

    EXPECT_TRUE(mgr.findShortestPath(-1, 0, 1000).isEmpty());
    EXPECT_TRUE(mgr.findShortestPath(0, -1, 1000).isEmpty());
    EXPECT_TRUE(mgr.findShortestPath(mgr.count(), 0, 1000).isEmpty());
    EXPECT_TRUE(mgr.findShortestPath(0, mgr.count(), 1000).isEmpty());
}

TEST(FindShortestPath, pathReturnsValidSequence)
{
    AirportManager mgr;
    const QString csv = findAirportsCsv();
    if (csv.isEmpty()) { GTEST_SKIP() << "airports.csv not found"; }
    mgr.loadFromFile(csv);

    if (mgr.count() >= 3) {
        auto path = mgr.findShortestPath(0, 2, 50000.0);
        ASSERT_FALSE(path.isEmpty());
        EXPECT_EQ(path.first(), 0);
        EXPECT_EQ(path.last(), 2);
    }
}


TEST(Integration, realCsvLoadsAndHasEntries)
{
    const QString csvPath = findAirportsCsv();
    if (csvPath.isEmpty()) {
        GTEST_SKIP() << "airports.csv not found";
    }

    AirportManager mgr;
    ASSERT_TRUE(mgr.loadFromFile(csvPath)) << "Failed to load: " << csvPath.toStdString();

    ASSERT_GT(mgr.count(), 100);

    for (int i = 0; i < mgr.count(); ++i) {
        EXPECT_FALSE(mgr.airports()[i].name.isEmpty());
        EXPECT_GE(mgr.airports()[i].lat, -90.0);
        EXPECT_LE(mgr.airports()[i].lat, 90.0);
        EXPECT_GE(mgr.airports()[i].lon, -180.0);
        EXPECT_LE(mgr.airports()[i].lon, 180.0);
    }
}
