

#ifndef SRC_NET_RDMA_RDMASERVERSRQ_H_
#define SRC_NET_RDMA_RDMASERVERSRQ_H_

#include "RDMAServer.h"
#include "RDMAManagerRCSRQ.h"
#include "string"

namespace dpi {

class RDMAServerSRQ : public RDMAServer {
 public:
    RDMAServerSRQ();
    RDMAServerSRQ(string name, int port);

    ~RDMAServerSRQ();

    bool receive(size_t srq_id, const void* memAddr,
                 size_t size){
        return m_rdmaManager->receive(srq_id,memAddr,size);
    };
    bool pollReceive(size_t srq_id,  ib_addr_t& ret_ib_addr) {
        return m_rdmaManager->pollReceive(srq_id,ret_ib_addr);
    };

    bool createSRQ(size_t& ret_srq_id){
        return m_rdmaManager->createSharedReceiveQueue(ret_srq_id);
    };

    bool initQP(size_t srq_id, struct ib_addr_t& reIbAddr) {
        return m_rdmaManager->initQP(srq_id,reIbAddr);
    };

    void setCurrentSRQ(size_t srqID){
        Logging::debug(__FILE__, __LINE__, "setCurrentSRQ: assigned to " + to_string(srqID));
        m_currentSRQ = srqID;
    }

    size_t getCurrentSRQ(){
        return m_currentSRQ;
    }

    void handle(Any* anyReq, Any* anyResp) override;

 protected:

    bool connectQueueSRQ(RDMAConnRequest* connRequest,
                                  RDMAConnResponse* connResponse);

    size_t m_currentSRQ = 0;

};

} /* namespace dpi */

#endif /* SRC_NET_RDMA_RDMASERVERSRQ_H_ */
