#include "AirportManager.h"

#include <QFile>
#include <QTextStream>
#include <QSet>
#include <QDebug>

#include <algorithm>
#include <limits>

bool AirportManager::loadFromFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "[AirportManager] failed to open file:" << filename;
        return false;
    }

    // Читаем во временный буфер — не трогаем m_airports до успешного завершения
    QVector<Airport> buffer;

    QTextStream in(&file);
    in.readLine();

    while (!in.atEnd())
    {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        const QChar sep = line.contains(u'\t') ? u'\t' : u',';
        const auto  parts = line.split(sep);
        if (parts.size() < 3)
        {
            qWarning() << "[AirportManager] missing line:" << line;
            continue;
        }

        Airport airport;
        airport.name = parts[0].trimmed();

        bool okLat = false, okLon = false;
        airport.lat = parts[1].trimmed().toDouble(&okLat);
        airport.lon = parts[2].trimmed().toDouble(&okLon);

        if (!okLat || !okLon || airport.name.isEmpty())
        {
            qWarning() << "[AirportManager] parsing error:" << line;
            continue;
        }

        buffer.push_back(std::move(airport));
    }

    file.close();

    if (buffer.isEmpty())
        return false;

    m_airports = std::move(buffer);
    return true;
}

int AirportManager::findByName(const QString &name) const
{
    for (int i = 0; i < m_airports.size(); ++i)
    {
        if (m_airports[i].name == name)
        {
            return i;
        }
    }

    return -1;
}

double AirportManager::distanceBetween(int idx1, int idx2) const
{
    if (idx1 < 0 || idx1 >= m_airports.size() ||
        idx2 < 0 || idx2 >= m_airports.size())
    {
        return -1.0;
    }

    const auto &a = m_airports[idx1];
    const auto &b = m_airports[idx2];

    constexpr double R     = 6371.0;
    constexpr double toRad = M_PI / 180.0;

    const double dLat = (b.lat - a.lat) * toRad;
    const double dLon = (b.lon - a.lon) * toRad;

    const double sinDLat = std::sin(dLat / 2);
    const double sinDLon = std::sin(dLon / 2);

    double h = sinDLat * sinDLat +
               std::cos(a.lat * toRad) * std::cos(b.lat * toRad) *
               sinDLon * sinDLon;

    h = std::clamp(h, 0.0, 1.0);

    return R * 2.0 * std::atan2(std::sqrt(h), std::sqrt(1.0 - h));
}

QVector<NearbyResult> AirportManager::findNearby(int airportIndex,
                                                  double radiusKm) const
{
    QVector<NearbyResult> result;

    if (airportIndex < 0 || airportIndex >= m_airports.size())
        return result;

    result.reserve(m_airports.size());

    for (int i = 0; i < m_airports.size(); ++i)
    {
        if (i == airportIndex)
            continue;

        const double dist = distanceBetween(airportIndex, i);
        if (dist >= 0 && dist <= radiusKm)
            result.push_back({i, dist});
    }

    std::sort(result.begin(), result.end(),
              [](const NearbyResult &a, const NearbyResult &b) {
                  return a.distance < b.distance;
              });

    return result;
}

QVector<int> AirportManager::findShortestPath(int fromIndex, int toIndex,
                                               double maxRangeKm) const
{
    if (fromIndex < 0 || fromIndex >= m_airports.size() ||
        toIndex   < 0 || toIndex   >= m_airports.size())
    {
        return {};
    }

    const int n       = m_airports.size();
    constexpr double INF = std::numeric_limits<double>::infinity();

    QVector<double> dist(n, INF);
    QVector<int>    prev(n, -1);
    QSet<int>       visited;

    dist[fromIndex] = 0.0;

    while (visited.size() < static_cast<size_t>(n))
    {
        int    u    = -1;
        double minD = INF;

        for (int i = 0; i < n; ++i)
        {
            if (!visited.contains(i) && dist[i] < minD)
            {
                minD = dist[i];
                u    = i;
            }
        }

        if (u == -1 || minD == INF)
            break;
        if (u == toIndex)
            break;

        visited.insert(u);

        for (int v = 0; v < n; ++v)
        {
            if (visited.contains(v))
                continue;

            const double edge = distanceBetween(u, v);
            if (edge >= 0 && edge <= maxRangeKm && dist[u] + edge < dist[v])
            {
                dist[v] = dist[u] + edge;
                prev[v] = u;
            }
        }
    }

    if (dist[toIndex] == INF)
        return {};

    QVector<int> path;
    for (int at = toIndex; at != -1; at = prev[at])
        path.push_back(at);

    std::reverse(path.begin(), path.end());
    return path;
}
