#ifndef MQTHREAD_ENUM_H
#define MQTHREAD_ENUM_H

enum MqThreadPoolType
{
    PRODUCE_TO_MQ_C2S_IO    = 0,
    PRODUCE_TO_MQ_SEND_IO   = 1,
    PRODUCE_TO_MQ_C2S_DB    = 2,
    PRODUCE_TO_MQ_SEND_DB   = 3,
    CONSUME_FROM_MQ_C2S_IO  = 4,
    CONSUME_FROM_MQ_SEND_IO = 5,
    CONSUME_FROM_MQ_C2S_DB  = 6,
    CONSUME_FROM_MQ_SEND_DB = 7,
};

#endif // MQTHREAD_ENUM_H