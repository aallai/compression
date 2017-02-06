#include "engine.h"
#include <cassert>
#include <cstring>

using namespace std;

namespace compression
{

// Count leading zeroes.
int clz(uint64_t value)
{
    int count = 63;

    while (value >>= 1)
        count--;

    return count;
}

// Trailing zeros.
int ctz(uint64_t value)
{
    int count = 63;

    while (value <<= 1)
        count--;

    return count;
}

// Apparently C-style type puns are undefined behavior in C++.
uint64_t get_bits(double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof(uint64_t));
    return bits;
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
    auto v1 = get_bits(first.value);
    auto v2 = get_bits(second.value);

    stream.append(first.timestamp, 64);
    stream.append(v1, 64);

    auto delta = second.timestamp - first.timestamp;

    // Initial delta is stored using 14 bits.
    assert(delta >= 0 && delta < 0x400);

    stream.append(delta, 14);

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
        previous_tz = ctz(xor);
        previous_mb_length = 64 - previous_lz - previous_tz;
        previous_value = v2;

        stream.append("11");
        stream.append(previous_lz, 6);
        stream.append(previous_mb_length, 6);
        stream.append(xor >> previous_tz, previous_mb_length);
    }
}

void engine::compress_timestamp(uint64_t timestamp, bitstream& stream)
{
    int64_t d = (timestamp - tn_1) - (tn_1 - tn_2);

    printf("d: %lld\n", d);

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
        assert(d >=  INT32_MIN && d <= INT32_MAX);
        stream.append("1111");
        stream.append(d, 32);
    }

    tn_2 = tn_1;
    tn_1 = timestamp;
}

void engine::compress_value(double value, bitstream& stream)
{
    auto v = get_bits(value);

    auto xor = previous_value ^ v;

    if (xor == 0)
    {
        stream.append("0");
    }
    else
    {
        auto lz = clz(xor);
        auto tz = ctz(xor);
        auto mb_length = 64 - lz - tz;

        if (lz <= previous_lz && tz >= previous_tz)
        {
            stream.append("10");
            stream.append(xor >> previous_tz, previous_mb_length);
        }
        else
        {
            stream.append("11");
            stream.append(lz, 6);
            stream.append(mb_length, 6);
            stream.append(xor >> tz, mb_length);

            previous_tz = tz;
            previous_lz = lz;
            previous_mb_length = mb_length;
        }
    }

    previous_value = v;
}

}