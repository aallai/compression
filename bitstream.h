
#ifndef __compression_bitstream__
#define __compression_bitstream__

#include <vector>

namespace compression {

struct bitstream
{
public:
    bitstream();

    void append(std::string const& bits);
    void append(uint64_t bits, unsigned int width);

    std::string read_str(unsigned int width);
    uint64_t read_word(unsigned int width);

    void print();
    uint64_t size();
    bool has_unread_bits();

private:
    void append_bit(bool bit);
    unsigned int read_bit();

    uint64_t write_head;
    uint64_t read_head;
    std::vector<uint8_t> buf;
};

}

#endif