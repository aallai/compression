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

double get_bits(uint64_t value)
{
    double bits;
    memcpy(&bits, &value, sizeof(double));
    return bits;
}

bitstream engine::compress(std::vector<datapoint> data)
{
    assert(data.size() > 1);

    stream = bitstream();
    initialize_deltas(data[0], data[1]);

    data.erase(data.begin());
    data.erase(data.begin());

    for (auto const& p : data)
    {
        compress_timestamp(p.timestamp);
        compress_value(p.value);
    }

    return stream;
}

vector<datapoint> engine::decompress(bitstream& s)
{
    stream = s;
    vector<datapoint> data;

    auto initial = decompress_initial_values();

    data.insert(data.begin(), initial.begin(), initial.end());

    while (stream.has_unread_bits())
    {
        datapoint d;
        d.timestamp = decompress_timestamp();
        d.value = decompress_value();
        data.push_back(d);
    }

    return data;
}

vector<datapoint> engine::decompress_initial_values()
{
    datapoint first, second;

    first.timestamp = stream.read_word(64);
    first.value = get_bits(stream.read_word(64));

    second.timestamp = first.timestamp + stream.read_word(14);

    auto bit = stream.read_str(1);

    if (bit == "0")
    {
        second.value = first.value;
    }
    else
    {
        // Read out 1 bit.
        stream.read_str(1);

        previous_lz = stream.read_word(6);
        previous_mb_length = stream.read_word(6);

        assert(previous_lz + previous_mb_length <= 64);

        previous_tz = 64 - previous_lz - previous_mb_length;

        auto mb = stream.read_word(previous_mb_length);
        auto xor = mb << previous_tz;

        second.value = get_bits(get_bits(first.value) ^ xor);
    }

    tn_1 = second.timestamp;
    tn_2 = first.timestamp;
    previous_value = get_bits(second.value);

    vector<datapoint> data;
    data.push_back(first);
    data.push_back(second);
    return data;
}

uint64_t engine::decompress_timestamp()
{
    uint64_t d;
    auto bit = stream.read_str(1);

    if (bit == "0")
    {
        // 0
        d = 0;
    }
    else
    {
        // 1
        bit = stream.read_str(1);

        if (bit == "0")
        {
            // 10
            d = stream.read_word(7);
        }
        else
        {
            // 11
            bit = stream.read_str(1);

            if (bit == "0")
            {
                // 110
                d = stream.read_word(9);
            }
            else
            {
                // 111
                bit = stream.read_str(1);

                if (bit == "0")
                {
                    // 1110
                    d = stream.read_word(12);
                }
                else
                {
                    // 1111
                    d = stream.read_word(32);
                }
            }
        }
    }

    auto tn = d + 2 * tn_1 - tn_2;
    tn_2 = tn_1;
    tn_1 = tn;
    return tn;
}

double engine::decompress_value()
{
    auto bit = stream.read_str(1);

    if (bit == "0")
    {
        // 0
        return get_bits(previous_value);
    }
    else
    {
        uint64_t xor;
        bit = stream.read_str(1);

        if (bit == "0")
        {
            // 10
            xor = stream.read_word(previous_mb_length) << previous_tz;
        }
        else
        {
            // 11
            previous_lz = stream.read_word(6);
            previous_mb_length = stream.read_word(6);

            assert(previous_lz + previous_mb_length <= 64);

            previous_tz = 64 - previous_lz - previous_mb_length;

            auto mb = stream.read_word(previous_mb_length);
            xor = mb << previous_tz;
        }

        return get_bits(previous_value ^ xor);
    }
}

void engine::initialize_deltas(datapoint& first, datapoint& second)
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

void engine::compress_timestamp(uint64_t timestamp)
{
    int64_t d = (timestamp - tn_1) - (tn_1 - tn_2);

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

void engine::compress_value(double value)
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