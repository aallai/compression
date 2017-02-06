#include <iostream>
#include <string>
#include "engine.h"
#include "bitstream.h"

using namespace std;
using namespace compression;

int main(int, char **)
{
    vector<datapoint> data = {{0, 32}, {5, 32.5}, {10, 33}, {16, 32.75}, {21, 33.004}, {25, 33.203}, {30, 34.084}, {35, 34.675}, {41, 35}, {46, 35.135}};

    engine e;
    auto stream = e.compress(data);

    cout << "Compressed Size: " << stream.size() << endl;
    auto s = stream.read_str(static_cast<unsigned int>(stream.size()));
    cout << s << endl;
}