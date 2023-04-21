## Current Status
- Currently, dcfs supports single 16KB block map, which has 512 entries. Max file size = 512 * 16KB = 8MB.
- Networked version is working. Client-DataCapsule is connected.

## To-do
- Merge middleware branch to main.
- Add public key list management.
- Write evaluation scenario.


## Engineering Notes
- We want to use our own access control scheme based on public key. Therfore, "default_permission" option must be disabled. Other fuse-level access control check also should be bypassed. 
