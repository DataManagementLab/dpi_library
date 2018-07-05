#pragma once

#include "../utils/Config.h"
#include "BufferWriter.h"


namespace dpi
{

class NodeClient
{

public:
    NodeClient(/* args */){};
    ~NodeClient(){};
        
    //tempalte fucntion
     template <class T> bool dpi_append(BufferWriter<T>* writer, void* data, size_t size){
          return writer->append(data,size);
     };

};
 
}
