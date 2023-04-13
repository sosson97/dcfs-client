## Current Status
- Currently, dcfs supports single 16KB block map, which has 512 entries. Max file size = 512 * 16KB = 8MB.
- Working on Client - DataCapsule server connection
- Middleware is currently simulated locally.

### Note on Access Control
We want to use our own access control scheme based on public key. Therfore, "default_permission" option must be disabled. Other fuse-level access control check also should be bypassed. 

### Benchmark Description
The benchmark will measure the time to take each step. 
0. Receive test directory as argument. The file system to which the directory belongs will affect the performance.  
----Test1----(in-memory ops and backup)
1. Open file X.
2. Prepare a buffer of size n MB.
2. Write the buffer to X.
3. Read X.
4. Check correctness.
5. Close X.
----Test2----(read from backend)
1. Open file X.
2. Read X.
3. Check correctness
4. Close X.
----Test3----(overwrite)
1. Open file X.
2. Overwrite some parts + Append.
3. Check correctness
4. Close X.
5. Reopen X
6. Read and check correctness.
