#ifndef BITPACKER_H
#define BITPACKER_H

#include <cstdint>
#include <stdexcept>
#include <cstring>

class BitPacker
{
    public:
        BitPacker(char* buffer, unsigned size, bool copy = true);
        BitPacker(const BitPacker& other);
        ~BitPacker();

        const char* const data() const;
        void fill(char value);

        //Operators
        BitPacker& operator<<=(unsigned shift);
        BitPacker& operator>>=(unsigned shift);
        BitPacker operator>>(unsigned shift) const;
        BitPacker operator<<(unsigned shift) const;

        BitPacker& operator^=(const BitPacker& other);
        BitPacker& operator&=(const BitPacker& other);
        BitPacker& operator|=(const BitPacker& other);
        BitPacker operator^(const BitPacker& other) const;
        BitPacker operator&(const BitPacker& other) const;
        BitPacker operator|(const BitPacker& other) const;
        char operator[](unsigned index) const;
    private:
        const unsigned size;
        char* bytes = nullptr;
        bool copy = true;
};

#endif