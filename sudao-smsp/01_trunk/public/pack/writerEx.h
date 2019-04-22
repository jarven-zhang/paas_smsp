#ifndef PACK_WRITER_H_
#define PACK_WRITER_H_


#include "outputbuffer.h"
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

    class Writer
    {
    public:
        Writer(OutputBuffer *buffer);
        virtual ~Writer();

        operator bool() const;

    public:
        Writer &operator()(const Packet &value);

        Writer &operator()(bool value);

        Writer &operator()(UInt64 value, bool bigEndian = true);
        Writer &operator()(Int64 value, bool bigEndian = true);

        Writer &operator()(UInt32 value, bool bigEndian = true);
        Writer &operator()(Int32 value, bool bigEndian = true);

        Writer &operator()(UInt16 value, bool bigEndian = true);
        Writer &operator()(Int16 value, bool bigEndian = true);

        Writer &operator()(UInt8 value);
        Writer &operator()(Int8 value);

        Writer &operator()(const std::string &value);
        Writer &operator()(const std::string &value, bool);

        Writer &operator()(UInt32 size, const std::string &value);
        Writer &operator()(UInt32 size, const char *value);

        Writer &operator()(UInt32 size, const std::string &value, bool bigEndian);
        Writer &operator()(UInt16 size, const std::string &value, bool bigEndian);

        template <typename ValueT>
        Writer &operator()(const std::list<ValueT> &container, bool bigEndian = true);

        template <typename KeyT, typename ValueT>
        Writer &operator()(const std::map<KeyT, ValueT> &container, bool bigEndian = true);

        template <typename ValueT>
        Writer &operator()(const std::set<ValueT> &container);

    private:
        bool good_;
        OutputBuffer *buffer_;
    };

    template <typename ValueT>
    Writer &Writer::operator()(const std::list<ValueT> &container, bool bigEndian)
    {
        UInt32 size = container.size();
        if (this->operator()(size, bigEndian))
        {
            for (typename std::list<ValueT>::const_iterator i = container.begin(); i != container.end(); ++i)
            {
                if (!this->operator()(*i))
                {
                    break;
                }
            }
        }
        return *this;
    }

    template <typename KeyT, typename ValueT>
    Writer &Writer::operator()(const std::map<KeyT, ValueT> &container, bool bigEndian)
    {
        UInt32 size = container.size();
        if (this->operator()(size,  bigEndian))
        {
            for (typename std::map<KeyT, ValueT>::const_iterator i = container.begin(); i != container.end(); ++i)
            {
                if (!this->operator()(i->first))
                {
                    break;
                }
                if (!this->operator()(i->second))
                {
                    break;
                }
            }
        }
        return *this;
    }

    template <typename ValueT>
    Writer &Writer::operator()(const std::set<ValueT> &container)
    {
        UInt32 size = container.size();
        if (this->operator()(size))
        {
            for (typename std::set<ValueT>::const_iterator i = container.begin(); i != container.end(); ++i)
            {
                if (!this->operator()(*i))
                {
                    break;
                }
            }
        }
        return *this;
    }

}

#endif /* PACK_WRITER_H_ */

