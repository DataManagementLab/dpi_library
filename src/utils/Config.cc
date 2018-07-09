

#include "Config.h"
#include <cmath>

using namespace dpi;

//TEST
int Config::HELLO_PORT = 4001;
vector<string> Config::PTEST_MCAST_NODES = { "192.168.1.1:"
    + to_string(Config::RDMA_PORT) };
size_t Config::PTEST_SCAN_PREFETCH = 10;


//DPI
string Config::DPI_REGISTRY_SERVER = "127.0.0.1";
uint32_t Config::DPI_REGISTRY_PORT = 5300;
vector<string> Config::DPI_NODES = {  "127.0.0.1:"
    + to_string(Config::RDMA_PORT) };

//RDMA
size_t Config::RDMA_MEMSIZE = 1024ul * 1024 * 1024 * 1;  //1GB
uint32_t Config::RDMA_NUMAREGION = 1;
uint32_t Config::RDMA_DEVICE = 1;
uint32_t Config::RDMA_IBPORT = 1;
uint32_t Config::RDMA_PORT = 5200;
uint32_t Config::RDMA_MAX_WR = 4096;


//THREADING
vector<int> Config::THREAD_CPUS = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 20, 21, 22,
    23, 24, 25, 26, 27, 28, 29 };

//LOGGING
int Config::LOGGING_LEVEL = 1;


inline string trim(string str) {
  str.erase(0, str.find_first_not_of(' '));
  str.erase(str.find_last_not_of(' ') + 1);
  return str;
}

void Config::init_vector(vector<string>& values, string csv_list) {
  values.clear();
  char* csv_clist = new char[csv_list.length() + 1];
  strcpy(csv_clist, csv_list.c_str());
  char* token = strtok(csv_clist, ",");

  while (token) {
    values.push_back(token);
    token = strtok(nullptr, ",");
  }

  delete[] csv_clist;
}

void Config::init_vector(vector<int>& values, string csv_list) {
  values.clear();
  char* csv_clist = new char[csv_list.length() + 1];
  strcpy(csv_clist, csv_list.c_str());
  char* token = strtok(csv_clist, ",");

  while (token) {
    string value(token);
    values.push_back(stoi(value));
    token = strtok(nullptr, ",");
  }

  delete[] csv_clist;
}

void Config::unload() {
  google::protobuf::ShutdownProtobufLibrary();
}

void Config::load() {
  string configFile = "./conf/DPI.conf";
  ifstream file(configFile.c_str());

  string line;
  string key;
  string value;
  int posEqual;
  while (getline(file, line)) {

    if (line.length() == 0)
      continue;

    if (line[0] == '#')
      continue;
    if (line[0] == ';')
      continue;

    posEqual = line.find('=');
    key = line.substr(0, posEqual);
    value = line.substr(posEqual + 1);
    set(trim(key), trim(value));
  }
}

void Config::set(string key, string value) {
  //config
  if (key.compare("RDMA_PORT") == 0) {
    Config::RDMA_PORT = stoi(value);
  } else if (key.compare("RDMA_MEMSIZE") == 0) {
    Config::RDMA_MEMSIZE = strtoul(value.c_str(), nullptr, 0);
  } else if (key.compare("RDMA_NUMAREGION") == 0) {
    Config::RDMA_NUMAREGION = stoi(value);
  } else if (key.compare("RDMA_DEVICE") == 0) {
    Config::RDMA_DEVICE = stoi(value);
  } else if (key.compare("RDMA_IBPORT") == 0) {
    Config::RDMA_IBPORT = stoi(value);
  } else if (key.compare("THREAD_CPUS") == 0) {
    init_vector(Config::THREAD_CPUS, value);
  } else if (key.compare("DPI_REGISTRY_SERVER") == 0) {
    Config::DPI_REGISTRY_SERVER = string(value.c_str());
  } else if (key.compare("DPI_REGISTRY_PORT") == 0) {
    Config::DPI_REGISTRY_PORT = stoi(value);
  }else if (key.compare("DPI_NODES") == 0) {
    init_vector(Config::DPI_NODES, value);
  }
}

string dpi::Config::getIP() {
  int fd;
  struct ifreq ifr;
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  /* I want to get an IPv4 IP address */
  ifr.ifr_addr.sa_family = AF_INET;
  /* I want an IP address attached to "em1" */
  strncpy(ifr.ifr_name, "em1", IFNAMSIZ-1);

  ioctl(fd, SIOCGIFADDR, &ifr);
  close(fd);

  return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}
