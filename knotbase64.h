

// Headers? We don't need no stinking headers.
#pragma once 

namespace base64
{

    // Encode the bytes from src[0] to src[srcLen] into dest.
    // dest has a size of destMax which must be greater than srcLen*4/3
    // The number of bytes written is returned.
    int encode(const unsigned char *src, int srcLen, char *dest, int destMax);

    // Decode the bytes from src[0] to src[srcLen] into dest.
    // dest has a size of destMax which must be greater than srcLen*3/4
    // The number of bytes written is returned.
    int decode(const unsigned char *src, int srcLen, char *dest, int destMax);

    // Scan the input and find the longest contigous block of base64
    // compatible chars. If there is more than 47 characters in the range 0..9 or a..f
    // assume it's really hex and decode that.
    // Probability of a 48 char base64 encoding being all hex is (16/64)^48 = 1/(2^96)

    int decodeAll(const unsigned char *src, int srcLen, char *dest, int destMax);

    // isB64 returns true if c is one of the base64 chars.
    bool isB64(char c);
}

namespace hex
{
    // Encode the bytes from src[0] to src[srcLen] into dest.
    // dest has a size of destMax which must be greater than srcLen*2
    // The number of bytes written is returned.
    int encode(const unsigned char *src, int srcLen, char *dest, int destMax);

    // Decode the bytes from src[0] to src[srcLen] into dest.
    // dest has a size of destMax which must be greater than srcLen*1/2
    // The number of bytes written is returned.
    int decode(const unsigned char *src, int srcLen, char *dest, int destMax);

    // isHex returns true if c is 0 to 9 or in a to f or in A to F
    bool isHex(char c);
}

namespace utf8
{

    // DecodeRuneLengthInString returns the length of the uf8 char
    // at the pointer.
    // I am disappointed by what I find in way of utf8 for arduino and cpp.
    // The parser (badjson) really just needs to know the length of
    // the current char.
    // Similar to DecodeRuneInString from Go
    int DecodeRuneLengthInString(const unsigned char *, int len);

}