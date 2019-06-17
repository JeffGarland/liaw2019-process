# Boost.Process proposal

## Scope

In Scope:
1. Multiprocessing on a single machine
2. Pipe communication
3. `<iostream>` interfaces
4. Environment variables
5. Definition of processes
6. Path searching (depends on `std::filesystem`) (fixing unportability)
7. Process groups (use case: when a server/process spawns worker processes, we'd like to treat them as one entity) (the parent process isn't automatically included in any group)
8. Children should die with the parent (if possible)
  - on windows this is posssible using [Job Objects](https://docs.microsoft.com/en-us/windows/desktop/procthread/job-objects) [example](https://github.com/CobaltFusion/DebugViewPP/blob/master/Win32Lib/Win32Lib.cpp)
9. Parents survive their children when they die.

Not in scope:

1. Acquiring a handle to an existing process
	+ Depends on the privileges, doesn't always work
2. Named pipes
	+ Boost process uses them for asynchronous IO in windows
	+ Not fully portable across the board
3. Listing / enumerating processes
4. Setting up privileges for the child process (UID/GID)
	+ None of the languages we've looked at works
	+ we should put this to text
5. Asynchronous communication
6. Signals (although could be useful for implementation)
7. Other IPC primitives (no `mmap`)

## Questions

### Error handling

Boost process takes all arguments over a constructor call, so can't return an error code or expected like object. If the spawning fails, it throws an exception.

Moving the spawn to a free function could fix these problems.

