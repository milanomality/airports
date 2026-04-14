#include "AirportManager.h"

#include <QFile>
#include <QTextStream>
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
    in.setEncoding(QStringConverter::Utf8);
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

        if (airport.lat < MIN_LATITUDE || airport.lat > MAX_LATITUDE ||
            airport.lon < MIN_LONGITUDE || airport.lon > MAX_LONGITUDE)
        {
            qWarning() << "[AirportManager] invalid coordinates:" << line;
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

std::optional<int> AirportManager::findByName(const QString &name) const
{
    for (int i = 0; i < m_airports.size(); ++i)
    {
        if (m_airports[i].name == name)
        {
            return i;
        }
    }

    return std::nullopt;
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

    const double dLat = (b.lat - a.lat) * DEG_TO_RAD;
    const double dLon = (b.lon - a.lon) * DEG_TO_RAD;

    const double sinDLat = std::sin(dLat / 2);
    const double sinDLon = std::sin(dLon / 2);

    double h = sinDLat * sinDLat +
               std::cos(a.lat * DEG_TO_RAD) * std::cos(b.lat * DEG_TO_RAD) *
               sinDLon * sinDLon;

    h = std::clamp(h, 0.0, 1.0);

    return EARTH_RADIUS_KM * 2.0 * std::atan2(std::sqrt(h), std::sqrt(1.0 - h));
}

QVector<NearbyResult> AirportManager::findNearby(int airportIndex,
                                                  double radiusKm) const
{
    QVector<NearbyResult> result;

    if (airportIndex < 0 || airportIndex >= m_airports.size() ||
        radiusKm < 0.0)
    {
        return result;
    }

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
        toIndex   < 0 || toIndex   >= m_airports.size() ||
        maxRangeKm < 0.0)
    {
        return {};
    }

    const int n = m_airports.size();
    constexpr double INF = std::numeric_limits<double>::infinity();

    QVector<double> dist(n, INF);
    QVector<int>    prev(n, -1);
    QVector<bool>   visited(n, false);

    dist[fromIndex] = 0.0;

    while (true)
    {
        int    u    = -1;
        double minD = INF;

        for (int i = 0; i < n; ++i)
        {
            if (!visited[i] && dist[i] < minD)
            {
                minD = dist[i];
                u    = i;
            }
        }

        if (u == -1 || minD == INF)
            break;
        if (u == toIndex)
            break;

        visited[u] = true;

        for (int v = 0; v < n; ++v)
        {
            if (visited[v])
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
