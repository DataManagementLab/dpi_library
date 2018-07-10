#include "NodeServer.h"

NodeServer::NodeServer(): RDMAServer("NodeServer", Config::DPI_NODE_PORT){

};

NodeServer::NodeServer(uint16_t port): RDMAServer("NodeServer",port){

};

NodeServer::~NodeServer(){

};