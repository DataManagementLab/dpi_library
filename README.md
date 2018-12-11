# dpi_library
## Abstract
As data processing evolves towards large scale, distributed platforms, the network will necessarily play a substantial role in achieving efficiency and performance. Increasingly, switches, network cards, and protocols are becoming more flexible while programmability at all levels (aka, software defined networks) opens up many
possibilities to tailor the network to data processing applications
and to push processing down to the network elements.
In this paper, we propose DPI, an interface providing a set of
simple yet powerful abstractions flexible enough to exploit features
of modern networks (e.g., RDMA or in-network processing) suitable for data processing. Mirroring the concept behind the Message
Passing Interface (MPI) used extensively in high-performance computing, DPI is an interface definition rather than an implementation
so as to be able to bridge different networking technologies and
to evolve with them. In the paper we motivate and discuss key
primitives of the interface and present a number of use cases that
show the potential of DPI for data-intensive applications, such as
analytic engines and distributed database systems.



## Examples
Examples can be found in the `src/test/examples/` folder.

Simple example of appending to a buffer:
```
  string buffer_name = "buffer";
  char data1[] = "Hello ";
  char data2[] = "World!";
  int rcv_node_id = 1; //ID is mapped to a concrete node in cluster spec
  DPI_Context context;
  DPI_Init(context); 
  DPI_Create_buffer(buffer_name, rcv_node_id, context);
  DPI_Append(buffer_name, (void*)data1, sizeof(data1), context);
  DPI_Append(buffer_name, (void*)data2, sizeof(data2), context);
  DPI_Close_buffer(buffer_name, context);

  size_t buffer_size;
  void* buffer_ptr;
  DPI_Get_buffer(buffer_name, buffer_size, buffer_ptr, context);

  for(size_t i = 0; i < buffer_size; i++)
  {
    cout << ((char*)buffer_ptr)[i];
  }
  DPI_Finalize(context);
```
