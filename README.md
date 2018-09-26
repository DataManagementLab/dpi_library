# dpi_library

As data processing evolves towards large scale, distributed plat-
forms, the network will necessarily play a substantial role in achiev-
ing efficiency and performance. Increasingly, switches, network
cards, and protocols are becoming more flexible while programma-
bility at all levels (aka, software defined networks) opens up many
possibilities to tailor the network to data processing applications
and to push processing down to the network elements.
In this paper, we propose DPI, an interface providing a set of
simple yet powerful abstractions flexible enough to exploit features
of modern networks (e.g., RDMA or in-network processing) suit-
able for data processing. Mirroring the concept behind the Message
Passing Interface (MPI) used extensively in high-performance com-
puting, DPI is an interface definition rather than an implementation
so as to be able to bridge different networking technologies and
to evolve with them. In the paper we motivate and discuss key
primitives of the interface and present a number of use cases that
show the potential of DPI for data-intensive applica
