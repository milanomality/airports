#ifndef AIRPORT_MANAGER_H
#define AIRPORT_MANAGER_H

#include <QString>
#include <QVector>
#include <cmath>
#include <optional>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Физические константы
constexpr double EARTH_RADIUS_KM = 6371.0;
constexpr double DEG_TO_RAD      = M_PI / 180.0;

// Допустимые диапазоны координат
constexpr double MIN_LATITUDE  = -90.0;
constexpr double MAX_LATITUDE  = 90.0;
constexpr double MIN_LONGITUDE = -180.0;
constexpr double MAX_LONGITUDE = 180.0;

struct Airport
{
    QString name;
    double  lat{};
    double  lon{};
};

struct NearbyResult
{
    int    index{};
    double distance{};
};

class AirportManager
{
public:
    AirportManager() = default;

    bool loadFromFile(const QString &filename);

    const QVector<Airport> &airports() const { return m_airports; }
    int count() const { return m_airports.size(); }

    [[nodiscard]] std::optional<int> findByName(const QString &name) const;

    [[nodiscard]] double distanceBetween(int idx1, int idx2) const;

    [[nodiscard]] QVector<NearbyResult> findNearby(int airportIndex,
                                                    double radiusKm) const;

    [[nodiscard]] QVector<int> findShortestPath(int fromIndex, int toIndex,
                                                 double maxRangeKm) const;

private:
    QVector<Airport> m_airports;
};

#endif // AIRPORT_MANAGER_H
