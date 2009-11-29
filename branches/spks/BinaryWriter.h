#ifndef BINARY_WRITER_H
#define BINARY_WRITER_H

#include <iostream>

class BinaryWriter
{

public:
    BinaryWriter(std::ostream& stream): _stream(stream) { };

    template <typename InputT>
    void write(const InputT& input)
    {
        _stream.write(&input, sizeof(InputT));
    };

    template <>
    void write(const std::string& input)
    {
        size_t size = input.size();
        _stream.write(&size, sizeof(size_t));
        _stream.write(input.c_str(), size);
    };

private:
    std::ostream& stream;

}; // class BinaryWriter

#endif
 