#include "bitstream.h"

using namespace std;
using namespace compression;

struct datapoint
{
	uint64_t timestamp;
	double value;
};

int main(int, char **)
{
	vector<datapoint> data = {{0, 32}, {5, 32.5}, {10, 33}, {16, 32.75}, {21, 33.004}, {25, 33.203}, {30, 34.084}, {35, 34.675}, {41, 35}, {46, 35.135}};

	bitstream stream;
	stream.append(0xf2f45, 12);
	stream.print();
}