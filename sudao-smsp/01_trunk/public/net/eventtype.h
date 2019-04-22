#ifndef NET_EVENTTYPE_H_
#define NET_EVENTTYPE_H_



class EventType {
public:
    enum {
        Connected,
        Timeout,
        Closed,
        Error,
        Read,
        Write,
        Readable,
        Writable,
        Accept
    };
};



#endif /* NET_EVENTTYPE_H_ */

