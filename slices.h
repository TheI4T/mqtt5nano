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

#pragma once

namespace knotfree
{

    // Slice represents a read only sequence of bytes. The size of the slice, and the underlaying array,
    // are limited to 64k.
    // Some of the methods increment the 'start' as they parse.
    // We pass them around by value most times. They are *not* null terminated.

    // We don't malloc or free or new these ever. They live embedded in structs
    // or as local variables.

    struct sink; // sink is the not-read-only version of slice. defined below

    struct slice
    {
        const char *base;
        unsigned short int start;
        unsigned short int end;

        slice() // returns an empty slice
        {
            base = 0;
        }

        // init with c string
        slice(const char *str)
        {
            int len = 0; // calc strlen(str);
            for (; str[len]; len++)
                ;
            base = str;
            start = 0;
            end = len;
        }
        slice(const char *str, int i1, int i2)
        {
            base = str;
            start = i1;
            end = i2;
        }

        // Init a slice with the sink ptr,start,end
        slice(sink s);
        // {
        //     base = s.base;
        //     start = s.start;
        //     end = s.end;
        // }

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
        int length()
        {
            return size();
        }
        static const char *emptyStr; // in the cpp
        const char *charPointer()    // a pointer to the first char. not null terminated
        {
            if (size())
            {
                return base + start;
            }
            return emptyStr; // pointer to 0
        }

        // return the first byte and shrink the size. aka pop.
        unsigned char readByte() // advances start
        {
            if (empty())
            {
                return 0;
            }
            return base[start++];
        };

        // getBigEndianVarLenInt will parse a variable length int where it's big-endian and only the last, least
        // significant, byte is <128. So, it's going to be 7 usable bits per byte.
        // 3 will be enough here.
        int getBigEndianVarLenInt() // advances start
        {
            int val = 0;
            int i = 0;
            unsigned char tmp;
            do
            {
                if (empty())
                {
                    return -1;
                }
                tmp = readByte();
                i++;
                val = (val << 7) | (tmp & 0x7F);
                if (i == 3)
                {
                    break;
                }
            } while (tmp >= 128);
            return val;
        };
        int getLittleEndianVarLenInt() // advances start
        {
            int val = 0;
            int i = 0;
            unsigned char tmp;
            do
            {
                if (empty())
                {
                    return -1;
                }
                tmp = readByte();
                val |= int(tmp & 0x7F) << (i * 7);
                i++;
                if (i == 3)
                {
                    break;
                }
            } while (tmp >= 128);
            return val;
        };

        // getFixLenInt pops two bytes big-endian and returns an int.
        int getBigFixLenInt() // advances start
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

        int getLittleFixLenInt() // advances start
        {
            if (empty())
            {
                return 0;
            }
            int val = readByte();
            val |= (readByte() << 8);
            return val;
        };

        // read a 2 byte length and then that many chars.
        // little-endian
        // unless there are not enough then there's a silent fail.
        slice getLittleFixedLenString() // advances start
        {
            slice result;
            int len = getLittleFixLenInt();
            if (len > size())
            {
                len = size(); // never go past end
            }
            result.base = base;
            result.start = start;
            result.end = start + len;
            start = result.end;
            return result;
        };

        slice getBigFixedLenString() // advances start
        {
            slice result;
            int len = getBigFixLenInt();
            if (len > size())
            {
                len = size(); // never go past end
            }
            result.base = base;
            result.start = start;
            result.end = start + len;
            start = result.end;
            return result;
        };

        // return true if this slice has the same bytes as the null terminated str.
        bool equals(const char *str)
        {
            if (empty()) // never ever iterate an empty slice.
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

        // for debugging.
        // Copy out the slice into the buffer.
        char *getCstr(char *buffer, int max); // in the cpp
        // copy the slice into the buffer while expanding into hex.
        char *gethexstr(char *buffer, int max); // in the cpp
        void gethexstr(sink dest);              // defined in the cpp

    }; // slice

    // sink is like a slice except that we can write to it.
    // All the write or put functions you might expect in slice are in here.
    // We write at the start and then increment start until start
    // gets to end and then we're empty,
    struct sink
    {
        char *base;
        unsigned short int start;
        unsigned short int end;

        sink()
        {
            base = 0;
        }

        sink(char *cP, int amt)
        {
            base = cP;
            start = 0;
            end = amt;
        }

        int size()
        {
            return end - start;
        };

        // empty means no remaining space to write
        // confusingly it's the same as 'full'.
        bool empty()
        {
            if (start >= end || base == 0)
            {
                return true;
            }
            return false;
        }

        // writeByte copies the char into the buffer and advances the start index.
        // return true if failed.
        bool writeByte(char c)
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

        bool writeBytes(const char *cP, int amt)
        {
            bool fail = false;
            for (int i = 0; i < amt; i++)
            {
                fail = writeByte(cP[i]);
                if (fail)
                    return fail;
            }
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
            return write(slice(s));
        };

        bool writeFixedLenStr(slice s)
        {
            bool fail = false;
            int len = s.size();
            if (len + 2 > end)
            {
                fail = true;
                return fail;
            }
            writeByte(len >> 8);
            writeByte(len);
            for (int i = 0; i < len; i++)
            {
                writeByte(s.base[s.start + i]);
            }
            end += len;
            return fail;
        }

        // the int needs to be less than 2^21
        // mqtt needs litle endian.
        bool writeBigEndianVarLenInt(int val)
        {
            if (val >= 128)
            {
                if (val > 128 * 128)
                {
                    writeByte((val >> 14) | 0x80);
                }
                writeByte((val >> 7) | 0x80);
            }
            writeByte(val & 0x7F);
            return empty();
        }
        bool writeLittleEndianVarLenInt(int val)
        {
            while (true)
            {
                if (val < 128)
                {
                    writeByte(val);
                    break;
                }
                else
                {
                    writeByte((val & 0x7F) | 0x80);
                }
                val = val >> 7;
            }
            return empty();
        }
        // getWritten return the slice BEFORE the start.  It's what's *been* written.
        slice getWritten()
        {
            slice s;
            s.base = base;
            s.start = 0;
            s.end = start;
            return s;
        }
        void reset()
        {
            start = 0;
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
        int getBigEndianVarLenInt()
        {
            int val = 0;
            int i = 0;
            unsigned char tmp;
            do
            {
                tmp = readByte();
                i++;
                val = (val << 7) | (tmp & 0x7F);
                if (i == 3)
                {
                    break;
                }
            } while (tmp >= 128);
            return val;
        };
        int getLittleEndianVarLenInt()
        {
            int val = 0;
            int i = 0;
            unsigned char tmp;
            do
            {
                tmp = readByte();
                val += int(tmp & 0x7F) << (i * 7);
                i++;
                if (i == 3)
                {
                    break;
                }
            } while (tmp >= 128);
            return val;
        };
    };

    // drain can act as a place to send a stream of bytes.
    // Like a network connection.
    // Return true when things are failed.
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
        bool write(const char *cP) // c string
        {
            for (; *cP != 0; cP++)
            {
                writeByte(*cP);
            }
            bool fail = false;
            return fail;
        }

        bool write(sink s)
        { // is this tested?
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

        // writeFixedLenStr writes a 2 byte length big endian
        // and then the string bytes
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

    // a fount that is a slice.
    struct sliceFount : fount
    {
        slice src;
        sliceFount(slice src) : src(src) {}
        sliceFount() {}
        virtual unsigned char readByte()
        {
            return src.readByte();
        }
        virtual bool empty()
        {
            return src.empty();
        }
    };

    // a drain that is a sink.
    // writes to this drain are written to the drain's buffer.
    struct sinkDrain : drain
    {
        sink dest;
        bool writeByte(char c) override
        {
            return dest.writeByte(c);
        };
    };

    // sliceResult is for when we want to return slice,char
    // which requires a struct in C++
    struct sliceResult
    {
        slice s;
        char error;
        sliceResult()
        {
            error = 0;
        }
    };

    // intResult is for when we want to return int,char
    struct intResult
    {
        int i;
        unsigned char error;
        intResult()
        {
            error = 0;
        }
    };

} // namespace knotfree