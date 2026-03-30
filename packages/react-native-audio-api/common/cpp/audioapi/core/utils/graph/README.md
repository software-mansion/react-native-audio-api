# Graph structure
If you are reading this, you are probably curious about how the graph structure works (or something broke in this case you are fu**ed). This is the right place to start. This document will give you a high level overview of the graph structure, how it works, how it is used, tested and provide all decisions made during the development of it.

# Structure & design

## Graph
`Graph.hpp` is the main entrypoint to the graph structure and provides public API for it. It is the only file that should be included by the users of the graph structure. It contains the main `Graph` class which is used to create and manipulate the graph structure. It also contains some helper classes and functions.

Graph class is an orhestrator that provides high level API and manages internal structures like `AudioGraph`, `HostGraph`, `SPSC`, `Disposer` and pool capacity.

## HostGraph
`HostGraph.hpp` contains the `HostGraph` class which is responsible for managing the graph structure and providing low level API for it. It keeps track of nodes, edges, their connections, cycles and other internal metadata. It also provides API for adding, removing and connecting nodes. Each modification returns a `AGEvent` which is a closure that can be executed to apply the same modification on corelated `AudioGraph`. This move allows us to keep `HostGraph` in a state after update while sending async event to Audio thread, as we know if events are applied in order, we can be sure that the state of `AudioGraph` in time frame T1 will be the same as `HostGraph` in the same time frame T1.

On each operation `HostGraph` checks if any of the `NodeHandles` is orphaned by `AudioGraph` if so it can be disposed and removed. This way we can be sure there will never be false positive to add an edge that would create a cycle, because lifetime of the nodes is managed fully on Audio thread so `HostGraph` can only be sure that the node is not being processed when its handle is orphaned.
> So in worst case HostGraph can silent out edge creation if it thinks it would create a cycle, this is very rare case and false negative is acceptable compared to false negative. I don't even know if this is reachable in any realistic scenario, but it is worth to mention.

## AudioGraph
`AudioGraph.hpp` contains the `AudioGraph` class which is responsible for processing the graph structure on the audio thread. It keeps track of nodes, edges, their inputs and has topological order. It minimizes memory usage, speed of processing and limits memory fragmentation by using `InputPool` to manage edges. It also provides API for processing the graph structure and applying events from `HostGraph`.

It has very limited task that needs to be performed very fast, so it is designed to be as efficient as possible. It is also designed to be as simple as possible, so it is easy to understand and maintain.

## InputPool
`InputPool.hpp` contains the `InputPool` class which is responsible for managing the pool of edges. It is designed to be as efficient as possible, so it uses a free list to manage the pool of edges. It also provides API for allocating and deallocating edges. It is designed to be used on the audio thread, so it is not thread safe and should be used within the `AudioGraph` class. Or its events.

## NodeHandle
`NodeHandle.hpp` contains the `NodeHandle` class which is responsible for managing node index in AudioGraph. Indices are 32 bit to minimize memory usage and maximize cache efficiency. So it is only to be used in `AGEvent` closures to reference nodes in `AudioGraph` only having a handle to it.

## Disposer
`Disposer.hpp` contains the `Disposer` class which is responsible for managing the disposal of nodes and edges. It is designed to be used on single thread. It has a worker thread that we can offload destruction onto through SPSC queue. It is more robust then previous implementation allowing wide range of payloads to be disposed.
