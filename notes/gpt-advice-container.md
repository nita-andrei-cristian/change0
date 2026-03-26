To efficiently store and traverse a large graph in C, use a compact adjacency list representation based on contiguous arrays (CSR-like format).

All edges are stored in a single global array, while each node stores only two integers: the index of its first outgoing edge and the number of edges it has.

During construction, first count how many edges each node has, then compute prefix sums to determine where each node’s edges begin in the global edge array.

Next, insert each edge into its correct position using a cursor array.

To find all connections (neighbors) of a node, simply iterate linearly over the segment of the edge array defined by [first_edge, first_edge + edge_count).

This approach avoids pointer overhead, minimizes memory fragmentation, and provides excellent cache locality, making traversals such as BFS or DFS very fast even for graphs with up to millions of nodes.

