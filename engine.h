
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
    bitstream compress(std::vector<datapoint> data);
    std::vector<datapoint> decompress(bitstream& stream);

private:
    void compress_timestamp(uint64_t timestamp, bitstream& stream);
    void compress_value(double value, bitstream& stream);
    void initialize_deltas(datapoint& first, datapoint& second, bitstream& stream);

    // Timestamp compression uses the last two timestamps.
    // Value compression uses the previous value, the previous xor's leading zeros, trailing zeros and meaningful bits.
    uint64_t tn_1, tn_2, previous_value;
    int previous_lz, previous_tz, previous_mb_length;
};

}

#endif