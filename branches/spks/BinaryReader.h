#ifndef BINARY_READER_H
#define BINARY_READER_H 1

#include <iostream>

class BinaryReader
{

public:
    BinaryReader(std::istream& stream): _stream(stream) { };

    template <typename OutputT>
    void read(OutputT& output)
    {
        _stream.read((char*) &output, sizeof(OutputT));
    };

    template <>
    void read(std::string& output)
    {
        size_t size;
        _stream.read(&size, sizeof(size_t));

        char buf[size+1];
        _stream.read(buf, size);
        buf[size] = 0;

        output = std::string(buf);
        delete[] buf;
    };

private:
    std::istream& _stream;

#endif
 