#include <bitwrapper.h>

BitWrapper::BitWrapper(char* buffer, unsigned size, bool copy) : BitWrapper(reinterpret_cast<unsigned char*>(buffer), size, copy)
{

}

BitWrapper::BitWrapper(unsigned char* buffer, unsigned size, bool copy)
{
    if(size == 0)
        throw std::invalid_argument("BitWrapper size can't be 0");

    this->size = size;
    wrap(buffer, copy);
}

BitWrapper::BitWrapper(const BitWrapper& other)
{
    size = other.size;
    wrap(other.bytes, true);
}

BitWrapper& BitWrapper::operator=(const BitWrapper& other)
{
    size = other.size;
    wrap(other.bytes, true);
    return *this;
}

BitWrapper::~BitWrapper()
{
    if(copy && bytes != nullptr)
        delete[] bytes;
    
    bytes = nullptr;
}

const unsigned BitWrapper::get_size() const
{
    return size;
}

bool BitWrapper::is_copied() const
{
    return copy;
}

const char* const BitWrapper::data() const
{
    return reinterpret_cast<const char* const>(bytes);
}

BitWrapper& BitWrapper::operator<<=(unsigned shift)
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
        for(unsigned i = 0; i + 1 < size-big_shift; ++i)
            bytes[i] = (bytes[i] << shift) | (bytes[i+1] >> (8-shift));

        //Handle last byte
        bytes[size-big_shift-1] <<= shift;
    }


    return *this;
} 

BitWrapper& BitWrapper::operator>>=(unsigned shift)
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
        for(unsigned i = size-1; i > big_shift; --i)
            bytes[i] = (bytes[i-1] << (8-shift)) | (bytes[i] >> shift);

        //Handle first byte
        bytes[big_shift] >>= shift;
    }


    return *this;
} 

BitWrapper BitWrapper::operator>>(unsigned shift) const
{
    BitWrapper result(*this);
    result >>= shift;

    return result;
} 

BitWrapper BitWrapper::operator<<(unsigned shift) const
{
    BitWrapper result(*this);
    result <<= shift;

    return result;
} 

void BitWrapper::wrap(char* buffer, bool copy)
{
    wrap(reinterpret_cast<unsigned char*>(buffer), copy);
}

void BitWrapper::wrap(unsigned char* buffer, bool copy)
{
    //If pointing to same memory location and wrapping same way, do nothing
    if(buffer == bytes && copy == this->copy)
        return;

    if(this->copy)
    {
        if(copy)
        {
            if(bytes == nullptr)
                bytes = new unsigned char[size];
            
            std::memcpy(bytes, buffer, size);
        }
        else
        {
            if(bytes != nullptr)
                delete[] bytes;

            bytes = buffer;
        }
    }
    else
    {
        if(copy)
        {
            bytes = new unsigned char[size];
            std::memcpy(bytes, buffer, size);
        }
        else
            bytes = buffer;
    }

    this->copy = copy;
}

void BitWrapper::fill(unsigned char value)
{
    std::memset(bytes, value, size);
}

char BitWrapper::operator[](unsigned index) const
{
    if(index >= size)
        throw std::invalid_argument("Index out of range");
    
    return bytes[index];
}

BitWrapper& BitWrapper::operator^=(const BitWrapper& other)
{
    if(other.size != size)
        throw std::invalid_argument("BitWrapper instances don't have same size");

    for(unsigned i = 0; i < size; ++i)
        bytes[i] ^= other.bytes[i];

    return *this;
}

BitWrapper& BitWrapper::operator&=(const BitWrapper& other)
{
    if(other.size != size)
        throw std::invalid_argument("BitWrapper instances don't have same size");

    for(unsigned i = 0; i < size; ++i)
        bytes[i] &= other.bytes[i];

    return *this;
}

BitWrapper& BitWrapper::operator|=(const BitWrapper& other)
{
    if(other.size != size)
        throw std::invalid_argument("BitWrapper instances don't have same size");

    for(unsigned i = 0; i < size; ++i)
        bytes[i] |= other.bytes[i];

    return *this;
}

BitWrapper BitWrapper::operator^(const BitWrapper& other) const
{
    BitWrapper result(*this);
    result ^= other;

    return result;
} 

BitWrapper BitWrapper::operator&(const BitWrapper& other) const
{
    BitWrapper result(*this);
    result &= other;

    return result;
} 

BitWrapper BitWrapper::operator|(const BitWrapper& other) const
{
    BitWrapper result(*this);
    result |= other;

    return result;
} 

void BitWrapper::toggleEndianness(unsigned char* buffer, const std::size_t SIZE)
{
    for(std::size_t i = 0; i < SIZE/2; ++i)
        std::swap(buffer[i], buffer[SIZE-i-1]);
}