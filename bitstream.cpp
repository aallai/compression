#include <iostream>
#include <iomanip>
#include <cassert>
#include "bitstream.h"

using namespace std;

namespace compression {

bitstream::bitstream()
	: position( 0 )
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
	for (int i = --width; i >= 0; i--)
		append_bit((bits >> i) & 1);
}

void bitstream::append_bit(bool bit)
{
	auto index = position / 8;

	if (index == buf.size())
		buf.push_back(0);

	if (bit)
	{
		auto offset = position % 8;

		buf[index] |= (1 << (7 - offset));
	}

	position++;
}

void bitstream::print()
{
	for (auto const& byte : buf)
		printf("%02x", byte);
}

}