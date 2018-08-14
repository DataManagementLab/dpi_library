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
#include "BufferReader.h"

namespace dpi
{

class NodeClient
{

  public:
    NodeClient(/* args */);
    ~NodeClient();

    template <class T>
    bool dpi_append(BufferWriter<T> *writer, void *data, size_t size)
    {
        return writer->append(data, size);
    };

    void *dpi_read_buffer(BufferReader *reader, size_t &read_buffer_size)
    {
        return reader->read(read_buffer_size);
    };
};

} // namespace dpi
