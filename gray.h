#ifndef GRAY_H
#define GRAY_H

#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <bytewrapper.h>

void encodeGray(ByteWrapper& wrapped_buffer);

void decodeGray(ByteWrapper& wrapped_buffer);

#endif