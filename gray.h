#ifndef GRAY_H
#define GRAY_H

#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <bitwrapper.h>

void encodeGray(BitWrapper& wrapped_buffer);

void decodeGray(BitWrapper& wrapped_buffer);

#endif