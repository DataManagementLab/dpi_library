

#ifndef NET_PROTOSERVER_H
#define NET_PROTOSERVER_H

#include <string>
#include <iostream>
#include <sstream>
#include <mutex>

#include "../../utils/Config.h"
#include "zmq.hpp"
#include "ProtoSocket.h"
#include "../message/MessageTypes.h"
#include "../../thread/Thread.h"
#include "../../utils/Network.h"

using google::protobuf::Any;

namespace istore2 {

class ProtoServer : public Thread {
 public:
  ProtoServer(string name, int port);
  virtual ~ProtoServer();
  virtual bool startServer();
  virtual void stopServer();
  void run();bool isRunning();
  virtual void handle(Any* sendMsg, Any* respMsg) = 0;

  int getPort() {
    return m_port;
  }

 private:
  string m_name;
  int m_port;bool m_running;
  ProtoSocket* m_pSocket;
  mutex m_handleLock;
};

}  // end namespace istore2

#endif /* NET_PROTOSERVER_H_ */
