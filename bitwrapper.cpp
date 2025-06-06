#include <bitpacker.h>

BitPacker::BitPacker(char* buffer, unsigned size, bool copy) : size(size)
{
    if(size == 0)
        throw std::invalid_argument("BitPacker size can't be 0");

    if(copy)
    {
        bytes = new char[size];
        std::memcpy(bytes, buffer, size);
    
    }
    else
        bytes = buffer;

    
    this->copy = copy;
}

BitPacker::BitPacker(const BitPacker& other) : size(other.size)
{
    bytes = new char[other.size];
    std::memcpy(bytes, other.bytes, other.size);
    copy = true;
}

BitPacker::~BitPacker()
{
    if(copy && bytes != nullptr)
        delete[] bytes;
    
    bytes = nullptr;
}

const char* const BitPacker::data() const
{
    return reinterpret_cast<const char* const>(bytes);
}

BitPacker& BitPacker::operator<<=(unsigned shift)
{
    unsigned big_shift = shift / 8;

    if(big_shift)
    {
        //Shift all data
        std::memmove(bytes, bytes+big_shift, size-big_shift);

        //Fill empty bytes with null bytes
        std::memset(bytes+size-big_shift, '\0', big_shift);
    }

    shift %= 8;

    if(shift)
    {
        for(unsigned i = 0; i + 1 < size; ++i)
            bytes[i] = (bytes[i] << shift) | (bytes[i+1] >> (8-shift));

        //Handle last byte
        bytes[size-1] <<= shift;
    }

    return *this;
} 

BitPacker& BitPacker::operator>>=(unsigned shift)
{
    unsigned big_shift = shift / 8;

    if(big_shift >= size)
    {
        fill('\0');
        return *this;
    }

    if(big_shift)
    {
        //Shift all data
        std::memmove(bytes+big_shift, bytes, size-big_shift);

        //Fill empty bytes with null bytes
        std::memset(bytes, '\0', big_shift);
    }

    shift %= 8;

    if(shift)
    {
        for(unsigned i = size-1; i > 0; --i)
            bytes[i] = (bytes[i-1] << (8-shift)) | (bytes[i] >> shift);

        //Handle first byte
        bytes[0] >>= shift;
    }

    return *this;
} 

BitPacker BitPacker::operator>>(unsigned shift) const
{
    BitPacker result(*this);
    result >>= shift;

    return result;
} 

BitPacker BitPacker::operator<<(unsigned shift) const
{
    BitPacker result(*this);
    result <<= shift;

    return result;
} 


void BitPacker::fill(char value)
{
    std::memset(bytes, value, size);
}

char BitPacker::operator[](unsigned index) const
{
    if(index >= size)
        throw std::invalid_argument("Index out of range");
    
    return bytes[index];
}

BitPacker& BitPacker::operator^=(const BitPacker& other)
{
    if(other.size != size)
        throw std::invalid_argument("BitPacker instances don't have same size");

    for(unsigned i = 0; i < size; ++i)
        bytes[i] ^= other.bytes[i];

    return *this;
}

BitPacker& BitPacker::operator&=(const BitPacker& other)
{
    if(other.size != size)
        throw std::invalid_argument("BitPacker instances don't have same size");

    for(unsigned i = 0; i < size; ++i)
        bytes[i] &= other.bytes[i];

    return *this;
}

BitPacker& BitPacker::operator|=(const BitPacker& other)
{
    if(other.size != size)
        throw std::invalid_argument("BitPacker instances don't have same size");

    for(unsigned i = 0; i < size; ++i)
        bytes[i] |= other.bytes[i];

    return *this;
}

BitPacker BitPacker::operator^(const BitPacker& other) const
{
    BitPacker result(*this);
    result ^= other;

    return result;
} 

BitPacker BitPacker::operator&(const BitPacker& other) const
{
    BitPacker result(*this);
    result &= other;

    return result;
} 

BitPacker BitPacker::operator|(const BitPacker& other) const
{
    BitPacker result(*this);
    result |= other;

    return result;
} 