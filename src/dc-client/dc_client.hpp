#ifndef DCCLIENT_H
#define DCCLIENT_H

/** dc_client
 * Modified from Hanming's DC client.
 * Manages a background thread for receiving Ack/Get response.
*/

#include <atomic>
#include <mutex>
#include <condition_variable>

//#include "crypto.hpp"
#include "client_comm.hpp"
#include "capsule.pb.h"
#include "request.pb.h"
/**
 * only one special operation can be true
 */
struct DCGetOptions {
    bool is_fresh_req;
    bool is_metaonly_req;
};

class DCClient
{
public:
    DCClient(const int64_t client_id);

    /** ListenServer takes 3 roles
     * 1. Receive and parse Ack
     * 2. Receive and parse Get resp
     * 3. Periodically check ops_status_ and clear record cache 
    */

    int RunListenServer(const std::atomic<bool> *end_signal);

    bool CommitAck(const std::string &hash);
    bool CommitGetResp(const std::string &hash, const capsule::CapsulePDU &pdu);
    bool CommitFreshResp(const std::string &hash, const capsule::FreshHashesContainer &fhc);

    /**
     * current implementation supports synchronous only
    */
    void Put(const std::string hash, const std::string &srl_pdu);  
    std::string* Get(const std::string hash, const DCGetOptions opt);

private:
    /** status of an outstanding operation
    * ppdu is a pointer to CapsulePDU in Get resp.
    * ppdu also takes a role of host-side cache.
    */
    struct put_status { 
        bool done;
        int ret;
        uint64_t timestamp; // for cache eviction
        std::mutex m;
        std::condition_variable cv;
    };
    struct get_status { 
        bool done;
        int ret;
        uint64_t timestamp; // for cache eviction
        std::string* srl_pdu;
        std::mutex m;
        std::condition_variable cv;

        bool is_fresh_req;
        bool is_metaonly_req;
    };
    //Crypto crypto;
    //std::string m_prev_hash = "init";

    /* Assuming single thread */
    ClientComm client_comm_; // communication implementation
    
    std::map<std::string, std::shared_ptr<struct put_status>> put_status_;
    
    typedef std::pair<std::string, bool> gs_key; // <hash, metaonly>
    std::map<gs_key, std::shared_ptr<struct get_status>> get_status_;
};

#endif // DCCLIENT_H