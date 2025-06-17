#include <bytewrapper.h>

ByteWrapper::ByteWrapper(char* buffer, unsigned size, bool copy) : ByteWrapper(reinterpret_cast<unsigned char*>(buffer), size, copy)
{

}

ByteWrapper::ByteWrapper(unsigned char* buffer, unsigned size, bool copy)
{
    wrap(buffer, size, copy);
}

ByteWrapper::ByteWrapper(const ByteWrapper& other)
{
    wrap(other.bytes, other._size, true);
}

ByteWrapper& ByteWrapper::operator=(const ByteWrapper& other)
{
    wrap(other.bytes, other._size, true);
    return *this;
}

ByteWrapper ByteWrapper::ones(unsigned size)
{
    unsigned char * bytes = new unsigned char[size];
    
    //Set to false to prevent bytes to copy memory
    ByteWrapper result(bytes, size, false);
    result.fill((unsigned char)0xFF);

    //Tell ByteWrapper instance that he needs to handle memory itself (delete)
    result.copy = true;

    return result;
}

ByteWrapper ByteWrapper::zeroes(unsigned size)
{
    unsigned char * bytes = new unsigned char[size];
    
    //Set to false to prevent bytes to copy memory
    ByteWrapper result(bytes, size, false);
    result.fill((unsigned char)0x0);

    //Tell ByteWrapper instance that he needs to handle memory itself (delete)
    result.copy = true;

    return result;
}

ByteWrapper::~ByteWrapper()
{
    if(copy && bytes != nullptr)
        delete[] bytes;
    
    bytes = nullptr;
}

const unsigned ByteWrapper::size() const
{
    return _size;
}

bool ByteWrapper::is_copied() const
{
    return copy;
}

const char* const ByteWrapper::data() const
{
    return reinterpret_cast<const char* const>(bytes);
}

ByteWrapper& ByteWrapper::operator<<=(unsigned shift)
{
    std::size_t big_shift = shift / 8;

    if(big_shift)
    {
        //Shift all data
        std::memmove(bytes, bytes+big_shift, _size-big_shift);

        //Fill empty bytes with null bytes
        std::memset(bytes+_size-big_shift, '\0', big_shift);
    }

    shift %= 8;
    if(shift)
    {
        //Shift bytes until reaching last byte before 0-filled words
        for(std::size_t i = 0; i + 1 < _size-big_shift; ++i)
            bytes[i] = (bytes[i] << shift) | (bytes[i+1] >> (8-shift));

        //Handle last byte
        bytes[_size-big_shift-1] <<= shift;
    }


    return *this;
} 

ByteWrapper& ByteWrapper::operator>>=(unsigned shift)
{
    unsigned big_shift = shift / 8;

    if(big_shift >= _size)
    {
        fill('\0');
        return *this;
    }

    if(big_shift)
    {
        //Shift all data
        std::memmove(bytes+big_shift, bytes, _size-big_shift);

        //Fill empty bytes with null bytes
        std::memset(bytes, '\0', big_shift);
    }

    shift %= 8;
    if(shift)
    {
        //Shift bytes until reaching last byte before 0-filled words
        for(std::size_t i = _size-1; i > big_shift; --i)
            bytes[i] = (bytes[i-1] << (8-shift)) | (bytes[i] >> shift);
    
        //Handle first byte to be shifted
        bytes[big_shift] >>= shift;
    }

    return *this;
} 

ByteWrapper ByteWrapper::operator>>(unsigned shift) const
{
    ByteWrapper result(*this);
    result >>= shift;

    return result;
} 

ByteWrapper ByteWrapper::operator<<(unsigned shift) const
{
    ByteWrapper result(*this);
    result <<= shift;

    return result;
} 

void ByteWrapper::wrap(char* buffer, unsigned size, bool copy)
{
    wrap(reinterpret_cast<unsigned char*>(buffer), size, copy);
}

void ByteWrapper::wrap(unsigned char* buffer, unsigned size, bool copy)
{
    if(size == 0U)
        throw std::runtime_error("Wrapped buffer can't be of size 0");

    //If pointing to same memory location and wrapping same way, do nothing
    if(buffer == bytes && copy == this->copy && size == _size)
        return;

    if(this->copy)
    {
        if(copy)
        {
            if(bytes == nullptr)
                bytes = new unsigned char[size];
            else if(_size != size)
            {
                delete[] bytes;
                bytes = new unsigned char[size];
            }

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
    this->_size = size;
}

void ByteWrapper::fill(unsigned char value)
{
    std::memset(bytes, value, _size);
}

char ByteWrapper::operator[](unsigned index) const
{
    if(index >= _size)
        throw std::invalid_argument("Index out of range");
    
    return bytes[index];
}

ByteWrapper& ByteWrapper::operator^=(const ByteWrapper& other)
{
    if(other._size != _size)
        throw std::invalid_argument("ByteWrapper instances don't have same size");

    std::uint64_t* ptr64 = reinterpret_cast<std::uint64_t*>(bytes);
    std::uint64_t* other_ptr64 = reinterpret_cast<std::uint64_t*>(other.bytes);

    std::size_t i = 0;
    for(; i + 8 <= _size; i += 8)
        ptr64[i >> 3] ^= other_ptr64[i >> 3];

    for(; i < _size; ++i)
        bytes[i] ^= other.bytes[i]; 

    return *this;
}

ByteWrapper& ByteWrapper::operator&=(const ByteWrapper& other)
{
    if(other._size != _size)
        throw std::invalid_argument("ByteWrapper instances don't have same size");

    std::uint64_t* ptr64 = reinterpret_cast<std::uint64_t*>(bytes);
    std::uint64_t* other_ptr64 = reinterpret_cast<std::uint64_t*>(other.bytes);

    std::size_t i = 0;
    for(; i + 8 <= _size; i += 8)
        ptr64[i >> 3] &= other_ptr64[i >> 3];

    for(; i < _size; ++i)
        bytes[i] &= other.bytes[i]; 

    return *this;
}

ByteWrapper& ByteWrapper::operator|=(const ByteWrapper& other)
{
    if(other._size != _size)
        throw std::invalid_argument("ByteWrapper instances don't have same size");

    std::uint64_t* ptr64 = reinterpret_cast<std::uint64_t*>(bytes);
    std::uint64_t* other_ptr64 = reinterpret_cast<std::uint64_t*>(other.bytes);

    std::size_t i = 0;
    for(; i + 8 <= _size; i += 8)
        ptr64[i >> 3] |= other_ptr64[i >> 3];

    for(; i < _size; ++i)
        bytes[i] |= other.bytes[i]; 

    return *this;
}

ByteWrapper ByteWrapper::operator^(const ByteWrapper& other) const
{
    ByteWrapper result(*this);
    result ^= other;

    return result;
} 

ByteWrapper ByteWrapper::operator&(const ByteWrapper& other) const
{
    ByteWrapper result(*this);
    result &= other;

    return result;
} 

ByteWrapper ByteWrapper::operator|(const ByteWrapper& other) const
{
    ByteWrapper result(*this);
    result |= other;

    return result;
} 

ByteWrapper ByteWrapper::operator~() const
{
    ByteWrapper result(*this);
    result.flip();

    return result;
}

void ByteWrapper::flip()
{
    std::uint64_t* ptr64 = reinterpret_cast<std::uint64_t*>(bytes);

    std::size_t i = 0;
    for(; i + 8 <= _size; i += 8)
        ptr64[i >> 3] = ~ptr64[i >> 3];

    for(; i < _size; ++i)
        bytes[i] = ~bytes[i]; 
}

void ByteWrapper::toggleEndianness(unsigned char* buffer, const std::size_t SIZE)
{
    for(std::size_t i = 0; i < SIZE/2; ++i)
        std::swap(buffer[i], buffer[SIZE-i-1]);
}

bool ByteWrapper::operator<(const ByteWrapper& other) const
{
    return !operator>=(other);
}

bool ByteWrapper::operator<=(const ByteWrapper& other) const
{
    if(_size != other._size)
        throw std::runtime_error("Comparison between wrapped bytes of diffent size is not handled yet");
    
    std::size_t i = 0;
    for(; i < _size; ++i)
        if(bytes[i] != other.bytes[i])
            return bytes[i] < other.bytes[i];
    
    return true;
}

bool ByteWrapper::operator>(const ByteWrapper& other) const
{
    return !operator<=(other);
}

bool ByteWrapper::operator>=(const ByteWrapper& other) const
{
    if(_size != other._size)
        throw std::runtime_error("Comparison between wrapped bytes of diffent size is not handled yet");

    std::size_t i = 0;
    for(; i < _size; ++i)
        if(bytes[i] != other.bytes[i])
            return bytes[i] > other.bytes[i];
    
    return true;
}

bool ByteWrapper::operator==(const ByteWrapper& other) const
{
    if(other._size != _size)
        return false;

    if(bytes == other.bytes)
        return true;

    std::size_t i = 0;
    for(; i < _size; ++i)
        if(bytes[i] != other.bytes[i])
            return false; 

    return true;
}

bool ByteWrapper::operator!=(const ByteWrapper& other) const
{
    return !operator==(other);
}

bool ByteWrapper::is_filled_of(unsigned char value) const
{
    std::uint64_t v64 = value;
    v64 = (v64 << 56) | (v64 << 48) | (v64 << 40) | (v64 << 32) | (v64 << 24) | (v64 << 16) | (v64 << 8) | v64;

    std::uint64_t* ptr64 = reinterpret_cast<std::uint64_t*>(bytes);

    std::size_t i = 0;
    for(; i + 8 <= _size; i += 8)
        if(ptr64[i >> 3] != v64)
            return false;

    for(; i < _size; ++i)
        if(bytes[i] != value)
            return false; 

    return true;
}

std::ostream& operator<<(std::ostream& o, const ByteWrapper& wrapper)
{
    unsigned char c;
    for(std::size_t i = 0; i < wrapper.size(); ++i)
    {
        c = (unsigned char)wrapper[i];

        for(unsigned char mask = (unsigned char)0x80; mask != (unsigned char)0x0; mask >>= 1)
            o << (c & mask ? '1' : '0');
        
        o << ' ';
    }

    return o;
}