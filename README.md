# dpi_library

Emerging network technologies will have a significant impact on data-intensive applications, but how will applications make best use of them?    
Making use of RDMA, which is already seeing significant adoption, is still complex due to missing higher-level abstractions.
This problem is even worse for emerging technologies, like smart NICs and switches for in-network processing.    
Data processing systems, such as distributed database systems or analytics engines (Spark, Flink) can exploit these technologies, but
doing this on a system by system basis demands repeated reinvention of the wheel.   

In this paper, we propose DPI, an interface that provides a set of useful abstractions for network-aware data-intensive processing.
The aim of DPI is to provide simple yet powerful abstractions that are flexible enough to enable exploitation of RDMA and in-network processing. 
Like MPI, DPI is just an interface that can have multiple implementations for different networking technologies.
To that end, a concrete DPI implementation can serve as a toolkit for implementing networked data-intensive applications, such as analytics engines or distributed database systems.
