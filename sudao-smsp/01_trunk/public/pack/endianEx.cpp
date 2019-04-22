#include "endianEx.h"

namespace pack
{

    EndianChooser endian;

    UInt64 Endian::FromBig(UInt64 value)
    {
        return endian.endian_->FromBig(value);
    }

    UInt32 Endian::FromBig(UInt32 value)
    {
        return endian.endian_->FromBig(value);
    }

    UInt16 Endian::FromBig(UInt16 value)
    {
        return endian.endian_->FromBig(value);
    }

    UInt64 Endian::FromLittle(UInt64 value)
    {
        return endian.endian_->FromLittle(value);
    }

    UInt32 Endian::FromLittle(UInt32 value)
    {
        return endian.endian_->FromLittle(value);
    }

    UInt16 Endian::FromLittle(UInt16 value)
    {
        return endian.endian_->FromLittle(value);
    }

    UInt64 Endian::ToBig(UInt64 value)
    {
        return endian.endian_->ToBig(value);
    }

    UInt32 Endian::ToBig(UInt32 value)
    {
        return endian.endian_->ToBig(value);
    }

    UInt16 Endian::ToBig(UInt16 value)
    {
        return endian.endian_->ToBig(value);
    }

    UInt64 Endian::ToLittle(UInt64 value)
    {
        return endian.endian_->ToLittle(value);
    }

    UInt32 Endian::ToLittle(UInt32 value)
    {
        return endian.endian_->ToLittle(value);
    }

    UInt16 Endian::ToLittle(UInt16 value)
    {
        return endian.endian_->ToLittle(value);
    }

}

