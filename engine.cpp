#include "engine.h"
#include <cassert>

using namespace std;

namespace compression
{

// Count leading zeroes.
int clz(uint64_t value)
{
    int count = 64;

    while (value >>= 1)
        count--;

    return count;
}

// Trailing zeros.
int ctz(uint64_t value)
{
    int count = 64;

    while (value <<= 1)
        count--;

    return count;
}


bitstream engine::compress(std::vector<datapoint> data)
{
    assert(data.size() > 1);

    bitstream stream;
    initialize_deltas(data[0], data[1], stream);

    data.erase(data.begin());
    data.erase(data.begin());

    for (auto const& p : data)
    {
        compress_timestamp(p.timestamp, stream);
        compress_value(p.value, stream);
    }

    return stream;
}

vector<datapoint> engine::decompress(bitstream&)
{
    vector<datapoint> data;
    return data;
}

void engine::initialize_deltas(datapoint& first, datapoint& second, bitstream& stream)
{
    // Casts like this are undefined behavior in C++.
    uint64_t v1 = reinterpret_cast<uint64_t&>(first.value);
    uint64_t v2 = reinterpret_cast<uint64_t&>(second.value);

    stream.append(first.timestamp, 64);
    stream.append(v1, 64);

    // Initial delta is stored using 14 bits.
    assert(second.timestamp - first.timestamp < 0x400);

    stream.append(second.timestamp - first.timestamp, 14);

    tn_1 = second.timestamp;
    tn_2 = first.timestamp;

    auto xor = v1 ^ v2;

    if (xor == 0)
    {
        stream.append("0");
    }
    else
    {
        previous_lz = clz(xor);
        auto tz = ctz(xor);
        previous_mb_length = 64 - previous_lz - tz;

        stream.append("11");
        stream.append(previous_lz, 6);
        stream.append(previous_mb_length, 6);
        stream.append(xor >> tz, previous_mb_length);
    }
}

void engine::compress_timestamp(uint64_t timestamp, bitstream& stream)
{
    auto d = (timestamp - tn_1) - (tn_1 - tn_2);

    if (d == 0)
    {
        stream.append("0");
    }

    else if (d >= -63 && d <= 64)
    {
        stream.append("10");
        stream.append(d, 7);
    }

    else if (d >= -255 && d <= 256)
    {
        stream.append("110");
        stream.append(d, 9);
    }

    else if (d >= -2047 && d <= 2048)
    {
        stream.append("1110");
        stream.append(d, 12);
    }

    else
    {
        assert(d < 0x100000000);
        stream.append("1111");
        stream.append(d, 32);
    }
}

void engine::compress_value(double, bitstream&)
{
}

}