#ifndef PACK_PACKET_H_
#define PACK_PACKET_H_

#include <string>

namespace pack
{

    class Reader;
    class Writer;

    class Packet
    {
    public:
        virtual bool Pack(Writer &writer) const = 0;
        virtual bool Unpack(Reader &reader) = 0;

        virtual std::string Dump() const = 0;
    };

}

#endif /* PACK_PACKET_H_ */

