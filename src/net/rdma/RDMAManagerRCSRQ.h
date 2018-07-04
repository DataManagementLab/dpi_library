

#ifndef SRC_NET_RDMA_RDMAMANAGERRCSRQ_H_
#define SRC_NET_RDMA_RDMAMANAGERRCSRQ_H_

#include "RDMAManagerRC.h"

namespace dpi {

struct sharedrq_t{
    ibv_srq* shared_rq;
    ibv_cq* recv_cq;
};

class RDMAManagerRCSRQ : public RDMAManagerRC {
 public:
    RDMAManagerRCSRQ();
    ~RDMAManagerRCSRQ();


    bool initQP(size_t srq_id, struct ib_addr_t& reIbAddr) override;

    bool receive(size_t srq_id, const void* memAddr,
                 size_t size) override;
    bool pollReceive(size_t srq_id, ib_addr_t& ret_ibaddr, bool& doPoll);
    bool createSharedReceiveQueue(size_t& ret_srq_id) override;

    vector<ib_addr_t> getIbAddrs(size_t srq_id);


 private:
    void destroyQPs() override;

    bool createQP(size_t srq_id, struct ib_qp_t& qp) ;

    map<size_t,sharedrq_t> m_srqs;
    // srq id ? vector qpnums
    map<size_t,vector<ib_addr_t>> m_connectedQPs;
    size_t m_srqCounter = 0;
};

} /* namespace dpi */

#endif /* SRC_NET_RDMA_RDMAMANAGERRCSRQ_H_ */
