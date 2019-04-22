#ifndef PACK_READER_H_
#define PACK_READER_H_

///#include "endian.h"
///#include "packet.h"
#include "inputbuffer.h"
#include "packetEx.h"

///ndef API
#include <string>
#include <list>
#include <map>
#include <set>

///clude "net/api.h"
///dif /* API */

namespace pack
{

///class Packet;

    class Reader
    {
    public:
        Reader(InputBuffer *buffer);
        virtual ~Reader();

    public:
        operator bool() const;

    public:
        UInt32 GetSize() const;
		bool GetGood();
		void SetGood(bool bgood);

    public:
        Reader &operator()(Packet &value);

        Reader &operator()(bool &value);

        Reader &operator()(UInt64 &value, bool bigEndian = true);
        Reader &operator()(Int64 &value, bool bigEndian = true);

        Reader &operator()(UInt32 &value, bool bigEndian = true);
        Reader &operator()(Int32 &value, bool bigEndian = true);

        Reader &operator()(UInt16 &value, bool bigEndian = true);
        Reader &operator()(Int16 &value, bool bigEndian = true);

        Reader &operator()(UInt8 &value);
        Reader &operator()(Int8 &value);

        Reader &operator()(std::string &value);
        Reader &operator()(std::string &value, bool);

        Reader &operator()(UInt32 size, std::string &value);
        Reader &operator()(UInt32 size, char *value);

        Reader &operator()(UInt32 size, std::string &value, bool bigEndian);
        Reader &operator()(UInt16 size, std::string &value, bool bigEndian);

        template <typename ValueT>
        Reader &operator()(std::list<ValueT> &container, bool bigEndian = true);

        template <typename KeyT, typename ValueT>
        Reader &operator()(std::map<KeyT, ValueT> &container, bool bigEndian = true);

        template <typename ValueT>
        Reader &operator()(std::set<ValueT> &container);

    private:
        bool good_;
        InputBuffer *buffer_;
    };

    template <typename ValueT>
    Reader &Reader::operator()(std::list<ValueT> &container, bool bigEndian)
    {
        container.clear();
        UInt32 size;
        if (this->operator()(size, bigEndian))
        {
            for (UInt32 i = 0; i < size; ++i)
            {
                ValueT value;
                if (!this->operator()(value))
                {
                    good_ = false;
                    break;
                }
                container.push_back(value);
            }
        }
        else
        {
            good_ = false;
        }
        return *this;
    }

    template <typename KeyT, typename ValueT>
    Reader &Reader::operator()(std::map<KeyT, ValueT> &container, bool bigEndian)
    {
        container.clear();
        UInt32 size;
        if (this->operator()(size, bigEndian))
        {
            for (UInt32 i = 0; i < size; ++i)
            {
                std::pair<KeyT, ValueT> pair;
                if (!this->operator()(pair.first))
                {
                    good_ = false;
                    break;
                }
                if (!this->operator()(pair.second))
                {
                    good_ = false;
                    break;
                }
                container.insert(pair);
            }
        }
        else
        {
            good_ = false;
        }
        return *this;
    }

    template <typename ValueT>
    Reader &Reader::operator()(std::set<ValueT> &container)
    {
        UInt32 size;

        container.clear();
        if (this->operator()(size))
        {
            for (UInt32 i = 0; i < size; ++i)
            {
                ValueT v;
                if (!this->operator()(v))
                {
                    good_ = false;
                    break;
                }
                container.insert(v);
            }
        }
        else
        {
            good_ = false;
        }
        return *this;
    }
}

#endif /* PACK_READER_H_ */

