## Benchmark Description
Test 1. Create file, write something, read it, and close (flush to storage).     
Test 2. Reopen the file, read it, and close.     
Test 3. Reopen the file, partially modify it, and close.      

## Questions we want to answer
- What is the source of slowdown in performance?

- How does the data block size (default=16KB) affects the performance?
  - What overhead is proportional to the block size?
  - What overhead is fixed when the block size changes?


### Details behind the questions
- Source of Slowdown
    - The performance penalty of DCFS can be attributed to the two factos: the increased network traffic and cryptographic operations.
    - DCFS increases the network traffic required for reading/writing files in two-folds. First, DCFS file format is composed of multiple records to enforce the hash chain structure of datacapsule. Second, every record should be formatted to DataCaspule format, being larger than its payload.
    - DCFS uses many cryptographic operations in reading/writing files, and, obviously, they increase latency by far. 
 
- Factoring out the implementation quality.
    - This is a naive prototype, so the measusred overhead may not be inherenet but just attrbiuted to the quality of the implementation. We have to factor out QoI as much as possible to get meaningful intutions from the measurement.
    - One way to measure the ideal performance is use simulated versions. For example, we can compare full implementation vs. NFS-file simulation version to factor out QoI of the networking stack (dc_comm, dc_client, ...).

## Overhead Breakdown (WIP)
### Write Process (flushing)
Disclaimer: steps are not strictly ordered. I focused on the overhead factors.

   ---Client---    
    1. Create a vector of buffer desecriptor.    
    2. Create a write request and sign it.    
    ---MDW---    
    3. Verfiy the signature.    
    4. Encrypt payload.    
    5. Compute the hash of the encrypted payload.    
    6. Composing Inode/BlockMapRecords and serialize them.    
    7. Composing DataRecords and serialize them.    
    8. Sign requests and pass them to networking stack.   
    ---DCServer---     
    9. Verify the signature.     
    10. Store and Ack.     
    
### Read Process
   ---Client---    
    1. Create a read request and sign it.     
    ---MDW---    
    2. Verfiy the signature.     
    3. Let client knows the list of recordnames.    
    ---Client---
    4. Create a real read request and sign it.
    5. Pass it to the networking stack.
    ---DCServer---     
    6. Verify the signature.     
    7. Read storage and return.
    ---Client---
    8. Deserialize record.
    9. Decrypt payload.
    10. Load it in local cache
    

