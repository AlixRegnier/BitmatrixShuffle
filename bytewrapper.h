#ifndef BYTEWRAPPER_H
#define BYTEWRAPPER_H

#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <iostream>

class ByteWrapper
{
    public:
        ByteWrapper(char* buffer, unsigned size, bool copy = true);
        ByteWrapper(unsigned char* buffer, unsigned size, bool copy = true);

        ByteWrapper(const ByteWrapper& other);

        ByteWrapper& operator=(const ByteWrapper& other);
        
        ~ByteWrapper();

        static ByteWrapper ones(unsigned size);
        static ByteWrapper zeroes(unsigned size);

        template <typename T>
        static ByteWrapper fromInteger(T x)
        {
            unsigned char * x_ptr = reinterpret_cast<unsigned char*>(&x);

            //If system is little-endian, swap bytes to big-endian representation
            int n = 1;
            if(*((char *)&n) == 1)
                toggleEndianness(x_ptr, sizeof(x));
            
            return ByteWrapper(x_ptr, sizeof(x), true);
        }

        const char* const data() const;
        void fill(unsigned char value);

        void wrap(char* buffer, unsigned size, bool copy);
        void wrap(unsigned char* buffer, unsigned size, bool copy);

        void flip(); //~x operator without copy

        const unsigned size() const;
        bool is_copied() const;
        bool is_filled_of(unsigned char value) const;

        //Shift operators
        ByteWrapper& operator<<=(unsigned shift);
        ByteWrapper& operator>>=(unsigned shift);
        ByteWrapper operator>>(unsigned shift) const;
        ByteWrapper operator<<(unsigned shift) const;

        //Bitwise operators
        ByteWrapper& operator^=(const ByteWrapper& other);
        ByteWrapper& operator&=(const ByteWrapper& other);
        ByteWrapper& operator|=(const ByteWrapper& other);
        ByteWrapper operator^(const ByteWrapper& other) const;
        ByteWrapper operator&(const ByteWrapper& other) const;
        ByteWrapper operator|(const ByteWrapper& other) const;
        ByteWrapper operator~() const;

        //Logical operators
        bool operator<(const ByteWrapper& other) const;
        bool operator<=(const ByteWrapper& other) const;
        bool operator>(const ByteWrapper& other) const;
        bool operator>=(const ByteWrapper& other) const;
        bool operator==(const ByteWrapper& other) const;
        bool operator!=(const ByteWrapper& other) const;

        char operator[](unsigned index) const;

        unsigned char* bytes = nullptr;
    private:
        unsigned _size;
        bool copy = true;

        static void toggleEndianness(unsigned char* buffer, const std::size_t SIZE);
};

std::ostream& operator<<(std::ostream& o, const ByteWrapper& wrapper);

#endif