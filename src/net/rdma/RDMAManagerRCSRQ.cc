

#include "RDMAManagerRCSRQ.h"

namespace dpi {

RDMAManagerRCSRQ::RDMAManagerRCSRQ()
        : RDMAManagerRC(Config::EXP_STORAGE_RDMA_MEMSIZE) {
    // TODO Auto-generated constructor stub

    // No nice solution -> first attempt to make map thread safe
    for (int i = 0; i < Config::MAX_NUM_SRQ; ++i) {
        m_srqs[i] = sharedrq_t();
    }

}

RDMAManagerRCSRQ::~RDMAManagerRCSRQ() {
    // TODO Auto-generated destructor stub
}

vector<ib_addr_t> RDMAManagerRCSRQ::getIbAddrs(size_t srq_id) {
    return m_connectedQPs.at(srq_id);
}

bool RDMAManagerRCSRQ::createQP(size_t srq_id, struct ib_qp_t& qp) {
    Logging::debug(__FILE__, __LINE__, "RDMAManagerRCSRQ::createQP: Method Called");
    // initialize QP attributes
    struct ibv_exp_qp_init_attr qp_init_attr;
    memset(&qp_init_attr, 0, sizeof(qp_init_attr));
    memset(&(m_res.device_attr), 0, sizeof(m_res.device_attr));
    m_res.device_attr.comp_mask |= IBV_EXP_DEVICE_ATTR_EXT_ATOMIC_ARGS
            | IBV_EXP_DEVICE_ATTR_EXP_CAP_FLAGS;

    if (ibv_exp_query_device(m_res.ib_ctx, &(m_res.device_attr))) {
        Logging::error(__FILE__, __LINE__, "Error, ibv_exp_query_device() failed");
        return false;
    }

    //send queue
    if (!(qp.send_cq = ibv_create_cq(m_res.ib_ctx, Config::RDMA_MAX_WR + 1, nullptr, nullptr, 0))) {
        Logging::error(__FILE__, __LINE__, "Cannot create send CQ!");
        return false;
    }

    qp_init_attr.pd = m_res.pd;
    qp_init_attr.send_cq = qp.send_cq;
    qp_init_attr.recv_cq = m_srqs[srq_id].recv_cq;
    qp.recv_cq = m_srqs[srq_id].recv_cq;
    qp_init_attr.sq_sig_all = 0;  // In every WR, it must be decided whether to generate a WC or not
    qp_init_attr.cap.max_inline_data = 0;
    qp_init_attr.max_atomic_arg = 32;
    qp_init_attr.exp_create_flags = IBV_EXP_QP_CREATE_ATOMIC_BE_REPLY;
    qp_init_attr.comp_mask = IBV_EXP_QP_INIT_ATTR_CREATE_FLAGS | IBV_EXP_QP_INIT_ATTR_PD;
    qp_init_attr.comp_mask |= IBV_EXP_QP_INIT_ATTR_ATOMICS_ARG;
    qp_init_attr.srq = m_srqs[srq_id].shared_rq;  // Shared receive queue
    qp_init_attr.qp_type = m_qpType;

    qp_init_attr.cap.max_send_wr = Config::RDMA_MAX_WR;
    qp_init_attr.cap.max_recv_wr = Config::RDMA_MAX_WR;
    qp_init_attr.cap.max_send_sge = Config::RDMA_MAX_SGE;
    qp_init_attr.cap.max_recv_sge = Config::RDMA_MAX_SGE;

    // create queue pair
    if (!(qp.qp = ibv_exp_create_qp(m_res.ib_ctx, &qp_init_attr))) {
        Logging::error(__FILE__, __LINE__, "Cannot create queue pair!");
        return false;
    }

    return true;
}

bool RDMAManagerRCSRQ::initQP(size_t srq_id, struct ib_addr_t& retIbAddr) {

    Logging::debug(__FILE__, __LINE__, "RDMAManagerRCSRQ::initQP: Method Called with SRQID" + to_string(srq_id));
    struct ib_qp_t qp;
    //create queues
    if (!createQP(srq_id, qp)) {
        Logging::error(__FILE__, __LINE__, "Failed to create QP");
        return false;
    }
    uint64_t connKey = nextConnKey();  //qp.qp->qp_num;
    retIbAddr.conn_key = connKey;

    // create local connection data
    struct ib_conn_t localConn;
    union ibv_gid my_gid;
    memset(&my_gid, 0, sizeof my_gid);

    localConn.buffer = (uint64_t) m_res.buffer;
    localConn.rc.rkey = m_res.mr->rkey;
    localConn.qp_num = qp.qp->qp_num;
    localConn.lid = m_res.port_attr.lid;
    memcpy(localConn.gid, &my_gid, sizeof my_gid);

    // init queue pair
    if (!modifyQPToInit(qp.qp)) {
        Logging::error(__FILE__, __LINE__, "Failed to initialize QP");
        return false;
    }

    //done
    setQP(retIbAddr, qp);
    setLocalConnData(retIbAddr, localConn);
    m_connectedQPs[srq_id].push_back(retIbAddr);
    Logging::debug(__FILE__, __LINE__, "Created RC queue pair");

    return true;
}

bool RDMAManagerRCSRQ::receive(size_t srq_id, const void* memAddr, size_t size) {

    struct ibv_sge sge;
    struct ibv_recv_wr wr;
    struct ibv_recv_wr *bad_wr;
    memset(&sge, 0, sizeof(sge));
    sge.addr = (uintptr_t) memAddr;
    sge.length = size;
    sge.lkey = m_res.mr->lkey;

    memset(&wr, 0, sizeof(wr));
    wr.wr_id = 0;
    wr.sg_list = &sge;
    wr.num_sge = 1;

    if ((errno = ibv_post_srq_recv(m_srqs.at(srq_id).shared_rq, &wr, &bad_wr))) {
        Logging::errorNo(__FILE__, __LINE__, std::strerror(errno), errno);
        Logging::error(__FILE__, __LINE__, "RECV has not been posted successfully! ");
        return false;
    }

    return true;
}

bool RDMAManagerRCSRQ::createSharedReceiveQueue(size_t& ret_srq_id) {

    Logging::debug(__FILE__, __LINE__, "RDMAManagerRCSRQ::createSharedReceiveQueue: Method Called");

    struct ibv_srq_init_attr srq_init_attr;
    sharedrq_t srq;
    memset(&srq_init_attr, 0, sizeof(srq_init_attr));

    srq_init_attr.attr.max_wr = Config::RDMA_MAX_WR;
    srq_init_attr.attr.max_sge = Config::RDMA_MAX_SGE;

    srq.shared_rq = ibv_create_srq(m_res.pd, &srq_init_attr);
    if (!srq.shared_rq) {
        std::cerr << "Error, ibv_create_srq() failed\n" << std::endl;
        return false;
    }

    //receive queue
//    if (!(srq.recv_cq = ibv_create_cq(m_res.ib_ctx, Config::RDMA_MAX_WR + 1, nullptr,

    if (!(srq.recv_cq = ibv_create_cq(srq.shared_rq->context, Config::RDMA_MAX_WR + 1, nullptr, nullptr, 0))) {
        Logging::error(__FILE__, __LINE__, "Cannot create receive CQ!");
        return false;
    }

    Logging::debug(__FILE__, __LINE__, "Created send and receive CQs!");

    ret_srq_id = m_srqCounter;
    m_srqs[ret_srq_id] = srq;
    m_srqCounter++;
    return true;
}

bool RDMAManagerRCSRQ::pollReceive(size_t srq_id, ib_addr_t& ret_ibaddr, bool& doPoll) {
    int ne;
    struct ibv_wc wc;

    do {
        wc.status = IBV_WC_SUCCESS;
        ne = ibv_poll_cq(m_srqs.at(srq_id).recv_cq, 1, &wc);
        if (wc.status != IBV_WC_SUCCESS) {
            Logging::error(
            __FILE__,
                           __LINE__,
                           "RDMA completion event in CQ with error! " + to_string(wc.status));
            return false;
        }

    } while (ne == 0 && doPoll);

    if (doPoll) {
        if (ne < 0) {
            Logging::error(__FILE__, __LINE__, "RDMA polling from CQ failed!");
            return false;
        }
        uint64_t qp = wc.qp_num;
        ret_ibaddr = m_qpNum2connKey.at(qp);
        return true;
    } else if (ne > 0) {
        return true;
    }

    return false;
}

} /* namespace dpi */

void RDMAManagerRCSRQ::destroyQPs() {
    Logging::debug(__FILE__, __LINE__, "RDMAManagerRCSRQ::destroyQPs: DESTROY");
    for (auto& qp : m_qps) {
        if (qp.qp != nullptr) {
            if (ibv_destroy_qp(qp.qp) != 0) {
                Logging::error(__FILE__, __LINE__, "Error, ibv_destroy_qp() failed");
            }

            if (ibv_destroy_cq(qp.send_cq) != 0) {
              Logging::error(__FILE__, __LINE__, "Cannot delete send CQ");
            }

            if (ibv_destroy_cq(qp.recv_cq) != 0) {
              Logging::error(__FILE__, __LINE__, "Cannot delete receive CQ");
            }
        }

        for(auto& kv : m_srqs){
            if (ibv_destroy_srq(kv.second.shared_rq)) {
              Logging::error(__FILE__, __LINE__, "RDMAManagerRCSRQ::destroyQPs: ibv_destroy_srq() failed ");
        }
        }
    }
}
