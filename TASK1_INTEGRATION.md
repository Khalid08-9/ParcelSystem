# Task 1 integration handoff

Task 1 implements registration, lookup/display/search, FIFO intake routing,
detail/status updates, removal and exceptional-condition checks. Recipient IDs
must be uppercase RC followed by six digits. All four parcel types are supported.
Cancellation is permitted from every non-final status. Special handling and
return-to-sender parcels can be reported missing. Completed records stay locked.

## Shared ownership and API

Create one `ParcelRegistry` in the integrated controller and pass it by reference
to the modules. Include `ParcelRegistry.hpp` directly, or use the implemented
Task 1 wrappers in `Interface.hpp` (which take that same registry reference).
The Task 2–4 declarations in `Interface.hpp` remain proposals for their owners;
they are not implemented by Task 1.

- `registerParcel(parcel, assignedID)`: validates, generates ID, timestamps and queues.
- `findParcel(id)`: returns a borrowed pointer, or null when absent.
- `peekNextParcel(status)`: returns a read-only pointer to the FIFO head of the
  waiting, collection, delivery, special, return or hold queue. Returns null for
  an empty queue, invalid status or status without a queue.
- `routeNextParcel(action)`: routes the oldest waiting parcel.
- `updateParcelStatus(id, status)`: validates transition and updates queues.
- `updateParcelDetails(...)`: validates edits before applying any changes.
- `removeParcel(id)`: refuses removal while request/resource links are present.

Never delete borrowed pointers, overwrite the `next` link or change `status`
directly. Pointers expire when their parcel is removed or the registry is cleared
or destroyed. Store Parcel IDs in long-lived requests/history, not pointers.
Registry and queue copying is disabled to avoid double deletion.

## Controller responsibilities when connecting the other tasks

1. **Task 2:** Inspect the appropriate route queue, create a request referencing
   its Parcel ID, and fill `requestID` only after successful request creation.
   Check the request module for duplicate requests: peeking does not dequeue.
2. **Task 3:** Allocate an appropriate resource, then fill `resourceID` only after
   successful allocation. `OutForDelivery` currently means the delivery route has
   been chosen; it is not proof that a resource has already been assigned.
3. **Completion/cancellation:** Call `updateParcelStatus` and check its result.
   The controller must also complete/cancel the associated request and release
   the resource. Keep the parcel/request/resource consistent on failure; Task 1
   cannot update other modules automatically.
4. **Task 4:** Capture the old details/status and relevant request/resource IDs
   before a successful operation; record the event afterwards. Task 1 does not
   automatically call history functions. Undo eligibility and cross-module
   rollback must be agreed with the history owner. Do not bypass final-status
   locking to implement undo.
5. **Task 5:** The existing tree stores copies, not references. Insert after
   registration, call `updateParcelRecord` after each successful change, and
   coordinate deletion in both structures. Tree-only updates do not update the
   registry. The registry remains authoritative for live processing. Failed
   multi-module operations need rollback or reconciliation in the controller.
6. Clear request/resource links only after their associated records have been
   released or detached. Clear the registry only at shutdown/reset after dependent
   modules have stopped using its records.

The integrated application still needs the real Task 2–4 implementations and a
controller connecting these operations. The existing `src/main.cpp` is a Task 1
demonstration, not the completed integrated application.

## Verification

`tests/RegistryTests.cpp` checks recipient/type validation, atomic failed edits,
cancellation, missing transitions and queue consistency. `tests/StatusTests.cpp`
checks every status pair and invalid status values. `tests/Task1IntegrationTests.cpp`
exercises the shared API, simulated request/resource linkage, completion and
explicit tree synchronization. It does not test real Task 2–4 implementations.

When CMake is available:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Data structures to explain during evaluation

The registry is a manually implemented singly linked list; intake and destination
queues are manually implemented FIFO linked queues. Queue enqueue/dequeue/peek
are O(1). Parcel lookup, duplicate checks, searches, updates and removal are O(n).
Routing the next parcel is O(1); routing n waiting parcels is O(n). Total storage
is O(n). FIFO preserves arrival order without shifting an array.
