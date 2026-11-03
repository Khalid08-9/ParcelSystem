# Agent Instructions — ParcelSystem

This is a shared group project (APU Smart Campus Parcel System assignment). Each
teammate owns one module/task, and the assignment is assessed individually, so it
is important that no one's work gets overwritten or altered by someone else.

## Rules for any AI coding agent working in this repo

1. **Do not modify another teammate's task/module unless explicitly asked to.**
   - Task 1 (Parcel Intake, Sorting, Processing) and Task 5 (Parcel Category /
     tree organisation) are Ali's responsibility: `include/Parcel.hpp`,
     `include/Status.hpp`, `src/Status.cpp`, `include/ParcelQueue.hpp`,
     `src/ParcelQueue.cpp`, `include/ParcelRegistry.hpp`,
     `src/ParcelRegistry.cpp`, `include/ParcelTree.hpp`, `src/tree.cpp`,
     `src/main.cpp`.
   - Tasks 2 (Service Requests), 3 (Resource Management) and 4 (Activity
     History) belong to other teammates once implemented. Treat any files they
     add for those tasks as off-limits for edits — read-only unless the user
     directs otherwise.
   - If integration requires touching shared files (e.g. `Interface.hpp`,
     `CMakeLists.txt`), make the minimal change needed and call it out instead
     of silently restructuring someone else's code.

2. **Only push to the `aliBranch` branch.** Never push directly to `master` or
   any other teammate's branch. Open work as commits on `aliBranch` and let the
   user decide when/how to merge or open a PR.

3. Keep changes scoped to what was asked — don't refactor or "clean up" other
   people's modules as a side effect of an unrelated task.
