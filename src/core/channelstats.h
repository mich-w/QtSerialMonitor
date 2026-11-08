#ifndef CHANNELSTATS_H
#define CHANNELSTATS_H

#include <QtGlobal>

#include <limits>

// Running statistics of one data channel (O(1) memory).
struct ChannelStats
{
    double last = 0.0;
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();
    double mean = 0.0;
    qint64 count = 0;
    long firstTimeMs = 0;
    long lastTimeMs = 0;

    void add(double value, long timeMs)
    {
        if (count == 0)
            firstTimeMs = timeMs;
        ++count;
        last = value;
        min = qMin(min, value);
        max = qMax(max, value);
        mean += (value - mean) / double(count); // incremental mean, no overflow on long runs
        lastTimeMs = timeMs;
    }

    // Average samples per second over the observed time span; 0 if unknown.
    double rateHz() const
    {
        const long spanMs = lastTimeMs - firstTimeMs;
        return (count > 1 && spanMs > 0) ? (count - 1) * 1000.0 / spanMs : 0.0;
    }
};

#endif // CHANNELSTATS_H
