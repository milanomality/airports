#ifndef AIRPORT_MANAGER_H
#define AIRPORT_MANAGER_H

#include <QString>
#include <QVector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

    int findByName(const QString &name) const;

    double distanceBetween(int idx1, int idx2) const;

    QVector<NearbyResult> findNearby(int airportIndex, double radiusKm) const;

    QVector<int> findShortestPath(int fromIndex, int toIndex, double maxRangeKm) const;

private:
    QVector<Airport> m_airports;
};

#endif // AIRPORT_MANAGER_H
