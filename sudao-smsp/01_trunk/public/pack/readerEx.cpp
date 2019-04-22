#include "readerEx.h"

#include "endianEx.h"
#include "packetEx.h"

namespace pack
{

    namespace
    {
        const UInt32 CONST_BUFFER_SIZE = 65535;
        //char CONST_BUFFER[CONST_BUFFER_SIZE];
    }

    Reader::Reader(InputBuffer *buffer)
    {
        buffer_ = buffer;
        good_ = true;
    }

    Reader::~Reader()
    {
        if (good_)
        {
            buffer_->Mark();
        }
        else
        {
            buffer_->Reset();
        }
    }

    Reader::operator bool() const
    {
        return good_;
    }

    UInt32 Reader::GetSize() const
    {
        return buffer_->Available();
    }

    bool Reader::GetGood()
    {
    	return good_;
    }
    
	void Reader::SetGood(bool bgood)
	{
		good_ = bgood;
	}

    Reader &Reader::operator()(Packet &value)
    {
        good_ = good_ && value.Unpack(*this);
        return *this;
    }

    Reader &Reader::operator()(bool &value)
    {
        UInt8 v = (UInt8)value;

        good_ = good_ && this->operator()(v);
        value = v;
        return *this;
    }

    Reader &Reader::operator()(UInt64 &value, bool bigEndian)
    {
        good_ = good_ && buffer_->Read(reinterpret_cast<char *>(&value), 8);
        if (bigEndian)
        {
            value = Endian::FromBig(value);
        }
        else
        {
            value = Endian::FromLittle(value);
        }
        return *this;
    }

    Reader &Reader::operator()(Int64 &value, bool bigEndian)
    {
        good_ = good_ && buffer_->Read(reinterpret_cast<char *>(&value), 8);
        if (bigEndian)
        {
            value = Endian::FromBig((UInt64 &) value);
        }
        else
        {
            value = Endian::FromLittle((UInt64 &) value);
        }
        return *this;
    }

    Reader &Reader::operator()(UInt32 &value, bool bigEndian)
    {
        good_ = good_ && buffer_->Read(reinterpret_cast<char *>(&value), 4);
        if (bigEndian)
        {
            value = Endian::FromBig(value);
        }
        else
        {
            value = Endian::FromLittle(value);
        }
        return *this;
    }

    Reader &Reader::operator()(Int32 &value, bool bigEndian)
    {
        good_ = good_ && buffer_->Read(reinterpret_cast<char *>(&value), 4);
        if (bigEndian)
        {
            value = Endian::FromBig((UInt32 &) value);
        }
        else
        {
            value = Endian::FromLittle((UInt32 &) value);
        }
        return *this;
    }

    Reader &Reader::operator()(UInt16 &value, bool bigEndian)
    {
        good_ = good_ && buffer_->Read(reinterpret_cast<char *>(&value), 2);
        if (bigEndian)
        {
            value = Endian::FromBig(value);
        }
        else
        {
            value = Endian::FromLittle(value);
        }
        return *this;
    }

    Reader &Reader::operator()(Int16 &value, bool bigEndian)
    {
        good_ = good_ && buffer_->Read(reinterpret_cast<char *>(&value), 2);
        if (bigEndian)
        {
            value = Endian::FromBig((UInt16 &) value);
        }
        else
        {
            value = Endian::FromLittle((UInt16 &) value);
        }
        return *this;
    }

    Reader &Reader::operator()(UInt8 &value)
    {
        good_ = good_ && buffer_->Read(reinterpret_cast<char *>(&value), 1);
        return *this;
    }

    Reader &Reader::operator()(Int8 &value)
    {
        good_ = good_ && buffer_->Read(reinterpret_cast<char *>(&value), 1);
        return *this;
    }

    Reader &Reader::operator()(std::string &value)
    {
        if (good_)
        {
            UInt16 size;
            if (this->operator()(size))
            {
                if (buffer_->Available() >= size)
                {
                    //char *buffer = CONST_BUFFER;
                    char *buffer = new char[size];
                    good_ = buffer_->Read(buffer, size);
                    value.assign(buffer, size);
					delete[] buffer;
                }
                else
                {
                    good_ = false;
                }
            }
            else
            {
                good_ = false;
            }
        }
        return *this;
    }

    Reader &Reader::operator()(std::string &value, bool)
    {
        if (good_)
        {
            UInt32 size;
            if (this->operator()(size))
            {
                if (buffer_->Available() >= size)
                {
                    /*char *buffer = CONST_BUFFER;
                    if (size > CONST_BUFFER_SIZE)
                    {
                        buffer = new char[size];
                    }*/
                    char *buffer = new char[size];
                    good_ = buffer_->Read(buffer, size);
                    value.assign(buffer, size);
                    /*if (size > CONST_BUFFER_SIZE)
                    {
                        delete[] buffer;
                    }*/
                    delete[] buffer;
                }
                else
                {
                    good_ = false;
                }
            }
            else
            {
                good_ = false;
            }
        }
        return *this;
    }

    Reader &Reader::operator()(UInt32 size, std::string &value)
    {
        if (buffer_->Available() >= size)
        {
        	#if 0
            char *buffer = CONST_BUFFER;
            if (size > CONST_BUFFER_SIZE)
            {
                buffer = new char[size];
            }
 			#else
 			char *buffer = new char[size];
			#endif
			
            good_ = good_ && buffer_->Read(buffer, size);
            value.assign(buffer, size);
			#if 0
            if (size > CONST_BUFFER_SIZE)
            {
                delete[] buffer;
            }
			#else			
			delete[] buffer;
			#endif

        }
        else
        {
            good_ = false;
        }
        return *this;
    }

    Reader &Reader::operator()(UInt32 size, char *value)
    {
        if (buffer_->Available() >= size)
        {
            good_ = good_ && buffer_->Read(value, size);
        }
        else
        {
            good_ = false;
        }
        return *this;
    }

    Reader &Reader::operator()(UInt32 size, std::string &value, bool bigEndian)
    {
        if (good_)
        {
            if (this->operator()(size, bigEndian))
            {
                if (size == 0)
                {
                    return *this;
                }

                if (buffer_->Available() >= size)
                {
                    //char *buffer = CONST_BUFFER;
                    //if (size > CONST_BUFFER_SIZE)
                    //{
                    //    buffer = new char[size];
                    //}
                    char *buffer = new char[size];
                    good_ = buffer_->Read(buffer, size);
                    value.assign(buffer, size);
                    //if (size > CONST_BUFFER_SIZE)
                    //{
                    //    delete[] buffer;
                    //}
                    delete[] buffer;
                }
                else
                {
                    good_ = false;
                }
            }
            else
            {
                good_ = false;
            }
        }
        return *this;
    }

    Reader &Reader::operator()(UInt16 size, std::string &value, bool bigEndian)
    {
        if (good_)
        {
            if (this->operator()(size, bigEndian))
            {
                if (size == 0)
                {
                    return *this;
                }

                if (buffer_->Available() >= size)
                {
                    //char *buffer = CONST_BUFFER;
                    char *buffer = new char[size];
                    good_ = buffer_->Read(buffer, size);
                    value.assign(buffer, size);
					delete[] buffer;
                }
                else
                {
                    good_ = false;
                }
            }
            else
            {
                good_ = false;
            }
        }
        return *this;
    }

}

