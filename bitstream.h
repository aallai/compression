#include <vector>

namespace compression {

struct bitstream
{
public:
	bitstream();

	void append(std::string const& bits);
	void append(uint64_t bits, unsigned int width);
	void print();

private:
	void append_bit(bool bit);

	uint64_t position;
	std::vector<uint8_t> buf;
};

}