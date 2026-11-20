# Design and Complexity

R: reservation requests; S: spaces; B: maximum prior bookings scanned per space; V: graph vertices; E: graph edges.

| Algorithm | Time | Auxiliary memory / storage |
|---|---|---|
| Merge sort | O(R log R) | O(R) |
| Greedy nearest-space allocation scan | O(R S B) worst case | O(R+S) for bookings/results, excluding static capacities and graph |
| First-fit allocation scan | O(R S B) worst case; may stop early | Similar booking/result storage |
| Binary heap Dijkstra | Common O((V+E) log V) for sparse/simple graphs | O(V+E) with lazy queue entries |
| Array Dijkstra | O(V^2+E) | O(V) excluding graph |

Correctness conditions: intervals are [start,end), and two reservations conflict iff `a.start < b.end && b.start < a.end`. Assignments only select matching zones/types, reachable vertices, and nonconflicting intervals. Dijkstra requires nonnegative road lengths. Greedy allocation is locally nearest but is not guaranteed globally optimal.

The 7-node sample yields four accepted bookings, 325m total, and matching Dijkstra distances. Actual benchmark timings vary with compiler, hardware, and system load.
