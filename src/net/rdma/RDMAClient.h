

#ifndef RDMAClient_H_
#define RDMAClient_H_

#include "../../utils/Config.h"
#include "./RDMAManager.h"
#include "./RDMAManagerUD.h"
#include "./RDMAManagerRC.h"
#include "../proto/ProtoClient.h"

#include <unordered_map>
#include <list>

namespace dpi {

class RDMAClient {
 public:
#if RDMA_TRANSPORT==0
  RDMAClient(rdma_transport_t transport = rc);
#elif RDMA_TRANSPORT==1
  RDMAClient(rdma_transport_t transport = ud );
#endif

  ~RDMAClient();

  // connection
  bool connect(const string& connection, bool managementQueue = false);
// use whith nodeIDs: improves performance since nodeids are used
  bool connect(const string& connection, const NodeID nodeID ,bool managementQueue = false);
  bool connect(const string& connection, struct ib_addr_t& retIbAddr,
               bool managementQueue = false);

  // memory management
  void* localAlloc(const size_t& size);
  bool localFree(const void* ptr);
  bool remoteAlloc(const string& connection, const size_t size, size_t& offset);
  bool remoteAlloc(const NodeID nodeID, const size_t size, size_t& offset);
  bool remoteFree(const string& connection, const size_t size,
                  const size_t offset);
  void* getBuffer(const size_t offset = 0);

  // one-sided
  // ip interface
  bool write(const string& connection, size_t remoteOffset, void* localData,
             size_t size, bool signaled);
  bool read(const string& connection, size_t remoteOffset, void* localData,
            size_t size, bool signaled);
  bool requestRead(const string& connection, size_t remoteOffset,
                   void* localData, size_t size);

  bool fetchAndAdd(const string& connection, size_t remoteOffset,
                   void* localData, size_t size, bool signaled);

  bool fetchAndAdd(const string& connection, size_t remoteOffset,
                   void* localData, size_t value_to_add ,size_t size, bool signaled);

  bool compareAndSwap(const string& connection, size_t remoteOffset,
                      void* localData, int toCompare, int toSwap, size_t size,
                      bool signaled);

  // ib_addr_t interface
  bool write(ib_addr_t& ibAddr, size_t remoteOffset, void* localData,
             size_t size, bool signaled);
  bool requestRead(ib_addr_t& ibAddr, size_t remoteOffset, void* localData,
                   size_t size);
  bool read(ib_addr_t& ibAddr, size_t remoteOffset, void* localData,
            size_t size, bool signaled);
  bool fetchAndAdd(ib_addr_t& ibAddr, size_t remoteOffset, void* localData,
                   size_t size, bool signaled);
  bool compareAndSwap(ib_addr_t& ibAddr, size_t remoteOffset, void* localData,
                      int toCompare, int toSwap, size_t size, bool signaled);


  //onesided
  // nodeid interface

  bool fetchAndAdd(const NodeID& nodeid, size_t remoteOffset,
                   void* localData, size_t value_to_add ,size_t size, bool signaled);
  bool requestRead(const NodeID& nodeid, size_t remoteOffset, void* localData,
                   size_t size);
  bool write(const NodeID& nodeid, size_t remoteOffset, void* localData,
             size_t size, bool signaled);
  bool writeRC(const NodeID& nodeid, size_t remoteOffset, void* localData,
             size_t size, bool signaled);
  bool read(const NodeID& nodeid, size_t remoteOffset, void* localData,
            size_t size, bool signaled);

  bool fetchAndAdd(const NodeID& nodeid, size_t remoteOffset,
                   void* localData, size_t size, bool signaled);
  bool compareAndSwap(const NodeID& nodeid, size_t remoteOffset,
                      void* localData, int toCompare, int toSwap, size_t size,
                      bool signaled);



  //two-sided
  // nodeid

  bool receive(const NodeID& nodeid, void* localData, size_t size);
  bool send(const NodeID& nodeid, void* localData, size_t size, bool signaled);
  bool pollReceive(const NodeID& nodeid);
  bool pollSend(const NodeID& nodeid);

  // two-sided
  // ip interface
  bool receive(const string& connection, void* localData, size_t size);
  bool pollReceive(const string& connection);
  bool send(const string& connection, void* localData, size_t size,
            bool signaled);
  bool pollSend(const string& connection);

  // ib_addr_t interface
  bool receive(ib_addr_t& ib_addr, void* localData, size_t size);
  bool send(ib_addr_t& ib_addr, void* localData, size_t size, bool signaled);
  bool pollReceive(ib_addr_t &ib_addr);
  bool pollSend(ib_addr_t &ib_addr);

  ib_addr_t getMgmtQueue(const string& connection) {
    return m_mgmt_addr.at(connection);
  }

  // multicast
  bool joinMCastGroup(string mCastAddress);
  bool joinMCastGroup(string mCastAddress, struct ib_addr_t& retIbAddr);

  bool leaveMCastGroup(string mCastAddress);
  bool sendMCast(string mCastAddress, const void* memAddr, size_t size,
                 bool signaled);
  bool receiveMCast(string mCastAddress, const void* memAddr, size_t size);
  bool pollReceiveMCast(string mCastAddress);

  bool leaveMCastGroup(struct ib_addr_t ibAddr);
  bool sendMCast(struct ib_addr_t ibAddr, const void* memAddr, size_t size,
                 bool signaled);
  bool receiveMCast(struct ib_addr_t ibAddr, const void* memAddr, size_t size);
  bool pollReceiveMCast(struct ib_addr_t ibAddr);



 protected:
  bool createManagementQueue(const string& connection,
                             const uint64_t qpNumServer);

  inline bool __attribute__((always_inline)) checkSignaled(bool signaled) {
    ++m_countWR;
    if (m_countWR == Config::RDMA_MAX_WR) {
      signaled = true;
      m_countWR = 0;
    }
    return signaled;
  }

  //check if client is connected to data node
  inline bool isConnected(const string& connection) {
    if (m_clients.count(connection) != 0) {
      return true;
    }
    return false;
  }

  RDMAManager* m_rdmaManager;
  unordered_map<string, ProtoClient*> m_clients;
  unordered_map<string, ib_addr_t> m_addr;
  unordered_map<string, ib_addr_t> m_mgmt_addr;
  unordered_map<string, ib_addr_t> m_mcast_addr;
  size_t m_countWR;

  // Mapping from nodeID to ibaddr
  vector<ib_addr_t> m_nodeIDsIBaddr;

};
}

#endif /* RDMAClient_H_ */
