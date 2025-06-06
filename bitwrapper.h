#ifndef BITWRAPPER_H
#define BITWRAPPER_H

#include <cstdint>
#include <stdexcept>
#include <cstring>

class BitWrapper
{
    public:
        BitWrapper(char* buffer, unsigned size, bool copy = true);
        BitWrapper(unsigned char* buffer, unsigned size, bool copy = true);

        BitWrapper(const BitWrapper& other);

        BitWrapper& operator=(const BitWrapper& other);
        
        ~BitWrapper();

        template <typename T>
        static BitWrapper fromInteger(T x)
        {
            unsigned char * x_ptr = reinterpret_cast<unsigned char*>(&x);

            //If system is little-endian, swap bytes to big-endian representation
            int n = 1;
            if(*((char *)&n) == 1)
                toggleEndianness(x_ptr, sizeof(x));
            
            return BitWrapper(x_ptr, sizeof(x), true);
        }

        const char* const data() const;
        void fill(unsigned char value);

        void wrap(char* buffer, bool copy = true);
        void wrap(unsigned char* buffer, bool copy = true);

        const unsigned get_size() const;
        bool is_copied() const;

        //Operators
        BitWrapper& operator<<=(unsigned shift);
        BitWrapper& operator>>=(unsigned shift);
        BitWrapper operator>>(unsigned shift) const;
        BitWrapper operator<<(unsigned shift) const;

        BitWrapper& operator^=(const BitWrapper& other);
        BitWrapper& operator&=(const BitWrapper& other);
        BitWrapper& operator|=(const BitWrapper& other);
        BitWrapper operator^(const BitWrapper& other) const;
        BitWrapper operator&(const BitWrapper& other) const;
        BitWrapper operator|(const BitWrapper& other) const;
        char operator[](unsigned index) const;

    private:
        unsigned size; //See later
        unsigned char* bytes = nullptr;
        bool copy = true;

        static void toggleEndianness(unsigned char* buffer, const std::size_t SIZE);
};

#endif