#ifndef __PACK_ENDIAN_H_
#define __PACK_ENDIAN_H_

#include "platform.h"

namespace pack
{
    class CommonEndian
    {
    public:
        virtual UInt64 FromBig(UInt64 value) = 0;
        virtual UInt32 FromBig(UInt32 value) = 0;
        virtual UInt16 FromBig(UInt16 value) = 0;

        virtual UInt64 FromLittle(UInt64 value) = 0;
        virtual UInt32 FromLittle(UInt32 value) = 0;
        virtual UInt16 FromLittle(UInt16 value) = 0;

        virtual UInt64 ToBig(UInt64 value) = 0;
        virtual UInt32 ToBig(UInt32 value) = 0;
        virtual UInt16 ToBig(UInt16 value) = 0;

        virtual UInt64 ToLittle(UInt64 value) = 0;
        virtual UInt32 ToLittle(UInt32 value) = 0;
        virtual UInt16 ToLittle(UInt16 value) = 0;

        virtual void Destroy() = 0;
    };

    class BigEndian  : public CommonEndian
    {
    public:
        virtual ~BigEndian() {}
        virtual UInt64 FromBig(UInt64 value)
        {
            return value;
        };

        virtual UInt32 FromBig(UInt32 value)
        {
            return value;
        }

        virtual UInt16 FromBig(UInt16 value)
        {
            return value;
        }

        virtual UInt64 FromLittle(UInt64 value)
        {
            return
                ((value & 0x00000000000000FFULL) << 0x38) |
                ((value & 0x000000000000FF00ULL) << 0x28) |
                ((value & 0x0000000000FF0000ULL) << 0x18) |
                ((value & 0x00000000FF000000ULL) << 0x08) |
                ((value & 0x000000FF00000000ULL) >> 0x08) |
                ((value & 0x0000FF0000000000ULL) >> 0x18) |
                ((value & 0x00FF000000000000ULL) >> 0x28) |
                ((value & 0xFF00000000000000ULL) >> 0x38);
        }

        virtual UInt32 FromLittle(UInt32 value)
        {
            return
                ((value & 0xFF000000) >> 0x18) |
                ((value & 0x00FF0000) >> 0x08) |
                ((value & 0x0000FF00) << 0x08) |
                ((value & 0x000000FF) << 0x18);
        }

        virtual UInt16 FromLittle(UInt16 value)
        {
            return
                ((value & 0xFF00) >> 0x08) |
                ((value & 0x00FF) << 0x08);
        }

        virtual UInt64 ToBig(UInt64 value)
        {
            return value;
        }

        virtual UInt32 ToBig(UInt32 value)
        {
            return value;
        }

        virtual UInt16 ToBig(UInt16 value)
        {
            return value;
        }

        virtual UInt64 ToLittle(UInt64 value)
        {
            return
                ((value & 0x00000000000000FFULL) << 0x38) |
                ((value & 0x000000000000FF00ULL) << 0x28) |
                ((value & 0x0000000000FF0000ULL) << 0x18) |
                ((value & 0x00000000FF000000ULL) << 0x08) |
                ((value & 0x000000FF00000000ULL) >> 0x08) |
                ((value & 0x0000FF0000000000ULL) >> 0x18) |
                ((value & 0x00FF000000000000ULL) >> 0x28) |
                ((value & 0xFF00000000000000ULL) >> 0x38);
        }

        virtual UInt32 ToLittle(UInt32 value)
        {
            return
                ((value & 0xFF000000) >> 0x18) |
                ((value & 0x00FF0000) >> 0x08) |
                ((value & 0x0000FF00) << 0x08) |
                ((value & 0x000000FF) << 0x18);
        }

        virtual UInt16 ToLittle(UInt16 value)
        {
            return
                ((value & 0xFF00) >> 0x08) |
                ((value & 0x00FF) << 0x08);
        }

        virtual void Destroy()
        {
            delete this;
        }
    };

    class LittleEndian  : public CommonEndian
    {
    public:
        virtual ~LittleEndian() {}
        virtual UInt64 FromBig(UInt64 value)
        {
            return
                ((value & 0x00000000000000FFULL) << 0x38) |
                ((value & 0x000000000000FF00ULL) << 0x28) |
                ((value & 0x0000000000FF0000ULL) << 0x18) |
                ((value & 0x00000000FF000000ULL) << 0x08) |
                ((value & 0x000000FF00000000ULL) >> 0x08) |
                ((value & 0x0000FF0000000000ULL) >> 0x18) |
                ((value & 0x00FF000000000000ULL) >> 0x28) |
                ((value & 0xFF00000000000000ULL) >> 0x38);
        };

        virtual UInt32 FromBig(UInt32 value)
        {
            return
                ((value & 0xFF000000) >> 0x18) |
                ((value & 0x00FF0000) >> 0x08) |
                ((value & 0x0000FF00) << 0x08) |
                ((value & 0x000000FF) << 0x18);
        }

        virtual UInt16 FromBig(UInt16 value)
        {
            return
                ((value & 0xFF00) >> 0x08) |
                ((value & 0x00FF) << 0x08);
        }

        virtual UInt64 FromLittle(UInt64 value)
        {
            return value;
        }

        virtual UInt32 FromLittle(UInt32 value)
        {
            return value;
        }

        virtual UInt16 FromLittle(UInt16 value)
        {
            return value;
        }

        virtual UInt64 ToBig(UInt64 value)
        {
            return
                ((value & 0x00000000000000FFULL) << 0x38) |
                ((value & 0x000000000000FF00ULL) << 0x28) |
                ((value & 0x0000000000FF0000ULL) << 0x18) |
                ((value & 0x00000000FF000000ULL) << 0x08) |
                ((value & 0x000000FF00000000ULL) >> 0x08) |
                ((value & 0x0000FF0000000000ULL) >> 0x18) |
                ((value & 0x00FF000000000000ULL) >> 0x28) |
                ((value & 0xFF00000000000000ULL) >> 0x38);
        }

        virtual UInt32 ToBig(UInt32 value)
        {
            return
                ((value & 0xFF000000) >> 0x18) |
                ((value & 0x00FF0000) >> 0x08) |
                ((value & 0x0000FF00) << 0x08) |
                ((value & 0x000000FF) << 0x18);
        }

        virtual UInt16 ToBig(UInt16 value)
        {
            return
                ((value & 0xFF00) >> 0x08) |
                ((value & 0x00FF) << 0x08);
        }

        virtual UInt64 ToLittle(UInt64 value)
        {
            return value;
        }

        virtual UInt32 ToLittle(UInt32 value)
        {
            return value;
        }

        virtual UInt16 ToLittle(UInt16 value)
        {
            return value;
        }

        virtual void Destroy()
        {
            delete this;
        }
    };


    class EndianChooser
    {
    public:
        EndianChooser()
        {
            unsigned short i = 1;
            if (*((char *) (&i)))
            {
                endian_ = new LittleEndian();
            }
            else
            {
                endian_ = new BigEndian();
            }
        }

        ~EndianChooser()
        {
            endian_->Destroy();
        }
        CommonEndian* endian_;
    };

    class Endian
    {
    public:
        static UInt64 FromBig(UInt64 value);
        static UInt32 FromBig(UInt32 value);
        static UInt16 FromBig(UInt16 value);

        static UInt64 FromLittle(UInt64 value);
        static UInt32 FromLittle(UInt32 value);
        static UInt16 FromLittle(UInt16 value);

        static UInt64 ToBig(UInt64 value);
        static UInt32 ToBig(UInt32 value);
        static UInt16 ToBig(UInt16 value);

        static UInt64 ToLittle(UInt64 value);
        static UInt32 ToLittle(UInt32 value);
        static UInt16 ToLittle(UInt16 value);
    };

}

///using pack::Endian;

#endif ///__PACK_ENDIAN_H_ 

