// Copyright 2020 Alan Tracey Wootton
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef NANO_SLICES_H // include guard
#define NANO_SLICES_H

#include <stdint.h>
#include <string.h>

namespace nano
{

// Slice represents a read only sequence of bytes. The size of the slice, and the underlaying array,
// are limited to 64k.
// Some of the methods increment the 'start' as they parse.
struct slice
{
    const char *base;
    uint16_t start;
    uint16_t end;

    slice()
    {
        base = 0;
    }

    slice(const char *str)
    {
        int len = strlen(str);
        base = str;
        start = 0;
        end = len;
    }

    bool empty()
    {
        if (!base || start >= end)
        {
            return true;
        }
        return false;
    };

    int size()
    {
        if (empty())
        {
            return 0;
        }
        return end - start;
    }

    // return the first byte and change the size.
    // aka pop.
    unsigned char readByte() // advances start
    {
        if (empty())
        {
            return 0;
        }
        return base[start++];
    };

    // Parse a variable length int where it's big-endian and only the last, least
    // significant, byte is <128. So, it's going to be 7 usable bits per byte.
    // 3 will be enough here.
    int getVarLenInt()
    {
        int val = 0;
        unsigned char tmp;
        do
        {
            if (empty())
            {
                return 0;
            }
            tmp = readByte();
            val = (val << 7) | (tmp & 0x7F);
        } while (tmp >= 128);
        return val;
    };

    // getFixLenInt pops two bytes big-endian and returns an int.
    int getFixLenInt() // advances start
    {
        if (empty())
        {
            return 0;
        }
        int val = readByte();
        val = (val << 8) & 0xFF00;
        val |= readByte();
        return val;
    };

    slice getFixedLenString() // advances start
    {
        slice result;
        int len = getFixLenInt();
        if (len > size())
        {
            len = size(); // don't go off end
        }

        result.base = base;
        result.start = start;
        result.end = start + len;
        start = result.end;
        return result;
    };

    // return true if this slice has the same bytes as the nil
    // terminated str.
    bool equals(const char *str)
    {
        if (empty()) // never iterate an empty slice.
        {
            if (str[0] == 0)
            {
                return true;
            }
            return false;
        }
        int i = start;
        for (; i < end; i++)
        {
            char c = str[i - start];
            if (c == 0)
            {
                return false;
            }
            if (c != base[i])
            {
                return false;
            }
        }
        char c = str[i - start];
        if (c != 0)
        { // it needs to be null now.
            return false;
        }
        return true;
    }

    // for debugging
    char *getCstr(char buffer[1024])
    {
        buffer[0] = 0;
        if (empty() == false)
        {
            int amount = size();
            if (amount + 1 > 1024)
            {
                amount = 1024 - 1;
            }
            int i = 0;
            for (; i < amount; i++)
            {
                (buffer)[i] = base[start + i];
            }
            buffer[i] = 0;
        }
        return &buffer[0];
    }

    // for test
    char *gethexstr(char buffer[1024])
    {
        buffer[0] = 0;
        if (empty())
        {
            return &buffer[0];
        }
        char *cP = &buffer[0];
        for (int i = start; i < end; i++)
        {
            char c = base[i];
            char ch = c >> 4 & 0x0F;
            c = c & 0x0F;
            if (ch <= 9)
            {
                *cP++ = char('0' + ch);
            }
            else
            {
                *cP++ = char('a' + ch - 10);
            }
            if (c <= 9)
            {
                *cP++ = char('0' + c);
            }
            else
            {
                *cP++ = char('a' + c - 10);
            }
        }
        *cP++ = 0;
        return &buffer[0];
    };

}; // slice

// sink is like a slice except that we can write to it.
// all the write or put functions you might expect in slice
// are in here.
// we write at the start and then increment start until start
// gets to end and then we're done,
struct sink
{
    char *base;
    uint16_t start;
    uint16_t end;

    sink()
    {
        base = 0;
    }

    int size()
    {
        return end - start;
    };

    bool empty() // of remaining space to write
    {
        if (start >= end || base == 0)
        {
            return true;
        }
        return false;
    }

    bool writeByte(char c) // advances start, return true
    {
        bool fail = false;
        if (empty() == false)
        {
            base[start] = c;
            start++;
            return false;
        }
        fail = true;
        return fail;
    }

    bool write(slice s)
    {
        bool fail = false;
        if (s.empty())
        {
            fail = true;
            return fail;
        }
        for (int i = s.start; i < s.end; i++)
        {
            fail |= writeByte(s.base[i]);
        }
        return fail;
    };

    bool write(sink s)
    {
        bool fail = false;
        if (s.empty())
        {
            fail = true;
            return fail;
        }

        for (int i = s.start; i < s.end; i++)
        {
            fail |= writeByte(s.base[i]);
        }
        return fail;
    };

    bool writeFixedLenStr(slice s)
    {
        bool fail = false;
        if (s.empty())
        {
            fail = true;
            return fail;
        }
        int len = s.size();
        if (len + 2 > end)
        {
            return true;
        }
        writeByte(len >> 8);
        writeByte(len);
        for (int i = 0; i < len; i++)
        {
            writeByte(s.base[s.start + i]);
        }
        end += len;
        fail |= empty();
        return fail;
    }

    // the int needs to be less that 2^21
    bool writeVarLenInt(int val)
    {
        if (val >= 128)
        {
            if (val > 128 * 128)
            {
                writeByte((val >> 14) | 0x80);
            }
            writeByte((val >> 7) | 0x80);
        }
        writeByte(val);
        return empty();
    }

    // getWritten return the slice BEFORE the start.  It's what's been written.
    slice getWritten()
    {
        slice s;
        s.base = base;
        s.start = 0;
        s.end = start;
        return s;
    }
};

// fount can act as an incoming stream of bytes.
// like a network connection.
// outside of test it may block.
// there is an implementation where the bytes come from a slice. See sliceFount
struct fount
{
    virtual unsigned char readByte()
    {
        return 0;
    }
    virtual bool empty()
    {
        return true;
    }

    int getVarLenInt()
    {
        int val = 0;
        unsigned char tmp;
        do
        {
            tmp = readByte();
            val = (val << 7) | (tmp & 0x7F);
        } while (tmp >= 128);
        return val;
    };

    // unsigned char readByte()
    // {
    //       return 0;
    //   };
};

// drain can act as a place to send a stream of bytes.
// like a network connection
// return true when things are not ok.
// There is an implementation where the drain is a sink.
struct drain
{
    virtual bool writeByte(char c) = 0;

    // write the bytes of s down the drain.
    bool write(slice s)
    {
        if (s.empty())
        {
            return false;
        }
        for (int i = 0; i < s.size(); i++)
        {
            bool fail = writeByte(s.base[s.start + i]);
            if (fail)
            {
                return fail;
            }
        }
        return false;
    }

    bool write(sink s)
    {
        if (s.empty())
        {
            return false;
        }
        for (int i = 0; i < s.size(); i++)
        {
            bool fail = writeByte(s.base[s.start + i]);
            if (fail)
            {
                return fail;
            }
        }
        return false;
    }

    bool writeFixedLenStr(slice str)
    {
        int len = str.size();
        bool fail = writeByte(len >> 8);
        if (fail)
        {
            return fail;
        }
        fail = writeByte(len);
        if (fail)
        {
            return fail;
        }
        for (int i = 0; i < len; i++)
        {
            fail = writeByte(str.base[str.start + i]);
            if (fail)
            {
                return fail;
            }
        }
        return false;
    }
};

struct sliceFount : fount
{
    slice src;
    virtual unsigned char readByte()
    {
        return src.readByte();
    }
    virtual bool empty()
    {
        return src.empty();
    }
};

// writes to this drain are written to the drain's buffer.
struct sinkDrain : drain
{
    sink src;
    virtual bool writeByte(char c)
    {
        return src.writeByte(c);
    };
};

} // namespace nano

#endif