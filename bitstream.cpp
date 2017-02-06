#include <cassert>
#include "bitstream.h"

using namespace std;

namespace compression {

bitstream::bitstream()
    : write_head(0), read_head(0)
{
}

void bitstream::append(string const& bits)
{
    for (auto const& c : bits)
    {
        assert(c == '1' || c == '0');

        append_bit(c == '1');
    }
}

void bitstream::append(uint64_t bits, unsigned int width)
{
    assert(width <= 64);

    for (int i = --width; i >= 0; i--)
        append_bit((bits >> i) & 1);
}

string bitstream::read_str(unsigned int width)
{
    vector<char> characters;

    for (unsigned int i = 0; i < width; i++)
        characters.push_back(read_bit() ? '1' : '0');

    return string(characters.begin(), characters.end());
}

uint64_t bitstream::read_word(unsigned int width)
{
    assert(width <= 64);

    uint64_t word = 0;

    for (unsigned int i = 0; i < width; i++)
        word = (word << 1) | read_bit();

    return word;
}

void bitstream::append_bit(bool bit)
{
    auto index = write_head / 8;

    if (index == buf.size())
        buf.push_back(0);

    if (bit)
    {
        auto offset = write_head % 8;
        buf[index] |= (1 << (7 - offset));
    }

    write_head++;
}

unsigned int bitstream::read_bit()
{
    assert(read_head < write_head);

    auto index = read_head / 8;
    auto offset = read_head % 8;

    read_head++;

    return (buf[index] >> (7 - offset)) & 1;
}

void bitstream::print()
{
    for (auto const& byte : buf)
    printf("%02x", byte);
}

uint64_t bitstream::size()
{
    return write_head;
}

bool bitstream::has_unread_bits()
{
    return read_head < write_head;
}

}