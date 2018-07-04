

#include "RDMAServerSRQ.h"

namespace dpi {

RDMAServerSRQ::RDMAServerSRQ() {
    // TODO Auto-generated constructor stub

}

RDMAServerSRQ::~RDMAServerSRQ() {
    // TODO Auto-generated destructor stub
}

RDMAServerSRQ::RDMAServerSRQ(string name, int port) : RDMAServer(name, port,true) {
    m_countWR = 0;

    m_rdmaManager = new RDMAManagerRCSRQ();

    if (m_rdmaManager == nullptr) {
      throw invalid_argument("RDMAManager could not be created");
    }
}

void RDMAServerSRQ::handle(Any* anyReq, Any* anyResp) {
  if (anyReq->Is<RDMAConnRequest>()) {
    RDMAConnResponse connResp;
    RDMAConnRequest connReq;
    anyReq->UnpackTo(&connReq);
    connectQueueSRQ(&connReq, &connResp);
    Logging::debug(__FILE__, __LINE__, "RDMAServerSRQ::handle: after connectQueue");
    anyResp->PackFrom(connResp);
  } else {
    RDMAServer::handle(anyReq,anyResp);
  }
}


bool RDMAServerSRQ::connectQueueSRQ(RDMAConnRequest* connRequest, RDMAConnResponse* connResponse) {
    Logging::debug(__FILE__, __LINE__, "RDMAServerSRQ::connectQueue: SRQ connect queues");
    unique_lock<mutex> lck(m_connLock);

      //create local QP
      struct ib_addr_t ibAddr;
      if (!initQP(getCurrentSRQ(),ibAddr)) {
        lck.unlock();
        return false;
      }
      m_addr.push_back(ibAddr);

      // set remote connection data
      struct ib_conn_t remoteConn;
      remoteConn.buffer = connRequest->buffer();
      remoteConn.rc.rkey = connRequest->rkey();
      remoteConn.qp_num = connRequest->qp_num();
      remoteConn.lid = connRequest->lid();
      for (int i = 0; i < 16; ++i) {
        remoteConn.gid[i] = connRequest->gid(i);
      }
      remoteConn.ud.psn = connRequest->psn();
      m_rdmaManager->setRemoteConnData(ibAddr, remoteConn);

      //connect QPs
      if (!m_rdmaManager->connectQP(ibAddr)) {
        lck.unlock();
        return false;
      }

      //create response
      ib_conn_t localConn = m_rdmaManager->getLocalConnData(ibAddr);
      connResponse->set_buffer(localConn.buffer);
      connResponse->set_rkey(localConn.rc.rkey);
      connResponse->set_qp_num(localConn.qp_num);
      connResponse->set_lid(localConn.lid);
      connResponse->set_psn(localConn.ud.psn);
      connResponse->set_server_connkey(ibAddr.conn_key);
      for (int i = 0; i < 16; ++i) {
        connResponse->add_gid(localConn.gid[i]);
      }

      Logging::debug(
          __FILE__, __LINE__,
          "RDMAServer: connected to client!" + to_string(ibAddr.conn_key));

      lck.unlock();
      return true;
}

} /* namespace dpi */
