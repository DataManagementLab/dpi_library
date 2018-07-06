/**
 * @brief 
 * 
 * @file NodeClient.h
 * @author cbinnig, lthostrup, tziegler
 * @date 2018-07-06
 */


#pragma once

#include "../utils/Config.h"
#include "BufferWriter.h"


namespace dpi
{

class NodeClient
{

public:
    NodeClient(/* args */);
    ~NodeClient();
        
     template <class T> bool dpi_append(BufferWriter<T>* writer, void* data, size_t size){
          return writer->append(data,size);
     };

};
 
}
