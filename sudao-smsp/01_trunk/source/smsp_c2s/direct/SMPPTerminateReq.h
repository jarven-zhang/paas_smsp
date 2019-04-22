#ifndef SMPPTERMINATEREQ_H_
#define SMPPTERMINATEREQ_H_
#include "SMPPBase.h"
namespace smsp
{

    class SMPPTerminateReq : public SMPPBase
    {
    public:
        SMPPTerminateReq();
        virtual ~SMPPTerminateReq();
    };

} /* namespace smsp */
#endif /* SMPPTERMINATEREQ_H_ */
