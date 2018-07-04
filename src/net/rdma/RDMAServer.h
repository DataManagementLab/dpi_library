

#ifndef RDMAServer_H_
#define RDMAServer_H_

#include "../../utils/Config.h"
#include "../proto/ProtoServer.h"
#include "../rdma/RDMAManager.h"
#include "./RDMAManagerUD.h"
#include "./RDMAManagerRC.h"
#include "../message/MessageTypes.h"
#include "../message/MessageErrors.h"


#include <unordered_map>
#include <list>
#include <mutex>

namespace istore2 {

class RDMAServer : public ProtoServer {
 public:
#if RDMA_TRANSPORT==0
    RDMAServer(int port = Config::RDMA_PORT, rdma_transport_t transport = rc);
    RDMAServer(string name, int port = Config::RDMA_PORT, rdma_transport_t transport = rc);
    RDMAServer(string name, int port, bool newFunc);
#elif RDMA_TRANSPORT==1
    RDMAServer(int port = Config::RDMA_PORT, rdma_transport_t transport = ud );
    RDMAServer(string name, int port = Config::RDMA_PORT, rdma_transport_t transport = ud );
#endif

    ~RDMAServer();

    // server methods
    bool startServer();
    void stopServer();
    virtual void handle(Any* sendMsg, Any* respMsg);

    // memory management
    bool localFree(const void* ptr);
    bool localFree(const size_t& offset) const;
    void* localAlloc(const size_t& size);
    rdma_mem_t remoteAlloc(const size_t& size);
    void* getBuffer(const size_t offset = 0);

    //connection management
    ib_addr_t getMgmtQueue(const ib_addr_t& serverQueue);
    vector<ib_addr_t> getQueues();

    //data transfer
    bool receive(ib_addr_t& ibAddr, void* localData, size_t size);
    bool send(ib_addr_t& ibAddr, void* localData, size_t size, bool signaled);
    bool pollReceive(ib_addr_t& ibAddr, bool doPoll = true);
    bool pollReceive(ib_addr_t& ibAddr, uint32_t& ret_qp_num);

    // multicast
    bool joinMCastGroup(string mCastAddress);
    bool joinMCastGroup(string mCastAddress, struct ib_addr_t& retIbAddr);

    bool leaveMCastGroup(string mCastAddress);
    bool sendMCast(string mCastAddress, const void* memAddr, size_t size, bool signaled);
    bool receiveMCast(string mCastAddress, const void* memAddr, size_t size);
    bool pollReceiveMCast(string mCastAddress);

    bool leaveMCastGroup(struct ib_addr_t ibAddr);
    bool sendMCast(struct ib_addr_t ibAddr, const void* memAddr, size_t size, bool signaled);
    bool receiveMCast(struct ib_addr_t ibAddr, const void* memAddr, size_t size);
    bool pollReceiveMCast(struct ib_addr_t ibAddr);

// protected:
    // memory management
    bool addMemoryResource(size_t size);
    MessageErrors requestMemoryResource(size_t size, size_t& offset);
    MessageErrors releaseMemoryResource(size_t& offset);

    virtual bool connectQueue(RDMAConnRequest* connRequest, RDMAConnResponse* connResponse);

    bool connectMgmtQueue(RDMAConnRequestMgmt* connRequest, RDMAConnResponseMgmt* connResponse);

    // helper
    bool checkSignaled(bool signaled) {
        m_countWR++;
        if (m_countWR % Config::RDMA_MAX_WR == 0 && !signaled) {
            signaled = true;
        }
        if (signaled) {
            m_countWR = 0;
        }
        return signaled;
    }

    // RDMA management
    RDMAManager* m_rdmaManager;
    size_t m_countWR;
    vector<ib_addr_t> m_addr;
    unordered_map<uint64_t, ib_addr_t> m_addr2mgmtAddr;  //queue 2 mgmtQueue
    unordered_map<string, ib_addr_t> m_mcastAddr;  //mcast_string to ibaddr

    // Locks for multiple clients accessing server
    mutex m_connLock;
    mutex m_memLock;
};

}

#endif /* RDMAServer_H_ */
