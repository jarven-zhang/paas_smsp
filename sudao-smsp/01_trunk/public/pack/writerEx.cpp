#include "writerEx.h"

#include "endianEx.h"
#include "packetEx.h"

namespace pack
{

    Writer::Writer(OutputBuffer *buffer)
    {
        buffer_ = buffer;
        good_ = true;;
    }

    Writer::~Writer()
    {
        buffer_->Flush();
    }

    Writer::operator bool() const
    {
        return good_;
    }

    Writer &Writer::operator()(const Packet &value)
    {
        value.Pack(*this);
        return *this;
    }

    Writer &Writer::operator()(bool value)
    {
        this->operator()((UInt8) value);
        return *this;
    }

    Writer &Writer::operator()(UInt64 value, bool bigEndian)
    {
        if (bigEndian)
        {
            value = Endian::ToBig(value);
        }
        else
        {
            value = Endian::ToLittle(value);
        }
        buffer_->Write(reinterpret_cast<const char *>(&value), 8);
        return *this;
    }

    Writer &Writer::operator()(Int64 value, bool bigEndian)
    {
        if (bigEndian)
        {
            value = Endian::ToBig((UInt64 &) value);
        }
        else
        {
            value = Endian::ToLittle((UInt64 &) value);
        }
        buffer_->Write(reinterpret_cast<const char *>(&value), 8);
        return *this;
    }

    Writer &Writer::operator()(UInt32 value, bool bigEndian)
    {
        if (bigEndian)
        {
            value = Endian::ToBig(value);
        }
        else
        {
            value = Endian::ToLittle(value);
        }
        buffer_->Write(reinterpret_cast<const char *>(&value), 4);
        return *this;
    }

    Writer &Writer::operator()(Int32 value, bool bigEndian)
    {
        if (bigEndian)
        {
            value = Endian::ToBig((UInt32 &) value);
        }
        else
        {
            value = Endian::ToLittle((UInt32 &) value);
        }
        buffer_->Write(reinterpret_cast<const char *>(&value), 4);
        return *this;
    }

    Writer &Writer::operator()(UInt16 value, bool bigEndian)
    {
        if (bigEndian)
        {
            value = Endian::ToBig(value);
        }
        else
        {
            value = Endian::ToLittle(value);
        }
        buffer_->Write(reinterpret_cast<const char *>(&value), 2);
        return *this;
    }

    Writer &Writer::operator()(Int16 value, bool bigEndian)
    {
        if (bigEndian)
        {
            value = Endian::ToBig((UInt16 &) value);
        }
        else
        {
            value = Endian::ToLittle((UInt16 &) value);
        }
        buffer_->Write(reinterpret_cast<const char *>(&value), 2);
        return *this;
    }

    Writer &Writer::operator()(UInt8 value)
    {
        buffer_->Write(reinterpret_cast<const char *>(&value), 1);
        return *this;
    }

    Writer &Writer::operator()(Int8 value)
    {
        buffer_->Write(reinterpret_cast<const char *>(&value), 1);
        return *this;
    }

    Writer &Writer::operator()(const std::string &value)
    {
        UInt16 size = static_cast<UInt16>(value.size());
        if (this->operator()(size))
        {
            buffer_->Write(value.data(), size);
        }
        return *this;
    }

    Writer &Writer::operator()(const std::string &value, bool)
    {
        UInt32 size = static_cast<UInt32>(value.size());
        if (this->operator()(size))
        {
            buffer_->Write(value.data(), size);
        }
        return *this;
    }

    Writer &Writer::operator()(UInt32 size, const std::string &value)
    {
        buffer_->Write(value.data(), size);
        return *this;
    }

    Writer &Writer::operator()(UInt32 size, const char *value)
    {
        buffer_->Write(value, size);
        return *this;
    }

    Writer &Writer::operator()(UInt32 size, const std::string &value, bool bigEndian)
    {
        if (this->operator()(size, bigEndian))
        {
            if (size == 0)
            {
                return *this;
            }
            buffer_->Write(value.data(), size);
        }
        return *this;
    }

    Writer &Writer::operator()(UInt16 size, const std::string &value, bool bigEndian)
    {
        if (this->operator()(size, bigEndian))
        {
            if (size == 0)
            {
                return *this;
            }
            buffer_->Write(value.data(), size);
        }
        return *this;
    }

}

