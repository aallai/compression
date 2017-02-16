
#ifndef __compression_engine__
#define __compression_engine__

#include <vector>
#include "bitstream.h"

namespace compression {

struct datapoint
{
    uint64_t timestamp;
    double value;
};

struct engine
{
public:
    engine() {}
    bitstream compress(std::vector<datapoint> data);
    std::vector<datapoint> decompress(bitstream& s);

private:
    void initialize_deltas(datapoint& first, datapoint& second);
    void compress_timestamp(uint64_t timestamp);
    void compress_value(double value);

    std::vector<datapoint> decompress_initial_values();
    uint64_t decompress_timestamp();
    double decompress_value();

    // Timestamp compression uses the last two timestamps.
    // Value compression uses the previous value, the previous xor's leading zeros, trailing zeros and meaningful bits.
    bitstream stream;
    uint64_t tn_1, tn_2, previous_value;
    uint64_t previous_lz, previous_tz, previous_mb_length;
};

}

#endif