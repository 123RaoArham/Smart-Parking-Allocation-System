# Smart Parking Allocation and Reservation Platform

DAA Project 46 | C11 | Rao Arham Aziz | SAP ID 590022632 | Batch 14

A menu-driven parking simulator that sorts reservation requests with merge sort, finds shortest road distances using Dijkstra, and allocates compatible parking spaces without overlapping reservations.

## Run (Windows PowerShell)

```powershell
gcc -std=c11 -O2 Phase_II/src/Smart_Parking.c -o Smart_Parking.exe
.\Smart_Parking.exe
```

On Linux/macOS: `gcc -std=c11 -O2 Phase_II/src/Smart_Parking.c -o smart_parking && ./smart_parking`.

## Algorithms

- Merge sort: sort reservation start times, O(R log R).
- Nearest-compatible greedy allocation: checks zone, type, interval overlaps, and Dijkstra distance; worst-case scan O(R S B).
- Heap Dijkstra: shortest paths with nonnegative edge weights.
- First-fit allocation: comparison baseline.
- Array Dijkstra: comparison baseline.

## Program menu

1. View spaces
2. Add reservation
3. Show allocations and routes
4. Cancel reservation
5. Compare algorithms / benchmark
6. Validate allocations
0. Exit

## Project layout

- `Phase_II/src/Smart_Parking.c`: final single-file C11 program (primary submission).
- `Phase_I/src/Smart_Parking_Phase_I.c`: C11 reconstruction of the core source for phase documentation; **not** the original historical Phase I implementation. The final feature set is retained to avoid implying an unverified development timeline.
- `tests/test_cli.py`: repeatable command-line smoke and edge-case checks.
- `screenshots/`: C outputs plus an explicitly labelled historical C++ reference screenshot.
- `presentation/`: C11 presentation.
- `docs/`: design and complexity notes.

## Important notes

Booking times are integer minutes after midnight, intervals are [start,end), and back-to-back reservations are permitted. Session changes are not persisted. Benchmarks are environment-dependent. The original multi-file C++ project is not included as C code and is not represented as an original C-language development history.
