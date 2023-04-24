#include "dc_client.hpp"

#include <string>
#include <thread>

#include "capsule.pb.h"
#include "request.pb.h"
#include "dc_config.hpp"

#include "util/logging.hpp"
#include "util/encode.hpp"


DCClient::DCClient(const int64_t client_id) : 
    client_comm_(ClientComm(NET_DC_SERVER_IP, client_id, this))
{}


int DCClient::RunListenServer(const std::atomic<bool> *end_signal)
{
    client_comm_.run_dc_client_listen_server(end_signal);

    return 0;
}

void DCClient::Put(const std::string hash, const std::string &srl_pdu) {
    Logger::log(LDEBUG, "[DCClient] Put called, " + Util::binary_to_hex_string(hash.c_str(), hash.size()));

    std::shared_ptr<struct put_status> pops(new struct put_status);
    pops->done = false;
    pops->ret = 0;
    pops->timestamp = 0;

    const auto res = put_status_.insert({hash, pops}); 

    if (res.second) { // new key is inserted
        //capsule::CapsulePDU pdu;
        //std::string out_msg;

        //client_comm_.CreatePdu(hash, srl_payload, pdu);
        //pdu.SerializeToString(&out_msg);
        client_comm_.mcast_dc(srl_pdu);
    }
    
    {
        std::unique_lock<std::mutex> lk(put_status_[hash]->m);
        const bool &d = put_status_[hash]->done;
        put_status_[hash]->cv.wait(lk, [&d]{return d;});
    }

    if (put_status_[hash]->ret) // something's wrong if ret = non-zero
		Logger::log(ERROR, "[DCClient] Put error");

    put_status_.erase(hash);

    return;
}

/* can be improved to MT version using parallel hashmap and modify_if ...*/
std::string* DCClient::Get(const std::string hash, DCGetOptions opt)
{
    assert(opt.is_fresh_req == false || hash.size() == 0);
    assert(!(opt.is_fresh_req && opt.is_metaonly_req));

    
    if (opt.is_fresh_req)
        Logger::log(LogLevel::LDEBUG, "[DCClient] Freshness Service called");
    else
        Logger::log(LogLevel::LDEBUG, "[DCClient] Get called, " + Util::binary_to_hex_string(hash.c_str(), hash.size()));

    std::shared_ptr<struct get_status> gops(new struct get_status);
    gops->done = false;
    gops->ret = 0;
    gops->timestamp = 0;
    gops->srl_pdu = NULL;
    gops->is_fresh_req = opt.is_fresh_req;
    gops->is_metaonly_req = opt.is_metaonly_req;


    gs_key gk(hash, opt.is_metaonly_req);
    const auto res = get_status_.insert({gk, gops}); 

    if (res.second) { // new key is inserted   
        capsule::ClientGetRequest out_req;
        out_req.set_hash(hash);
        out_req.set_replyaddr(client_comm_.m_recv_get_resp_addr);
        out_req.set_fresh_req(gops->is_fresh_req);
        out_req.set_metaonly_req(gops->is_metaonly_req);

        std::string out_msg;
        out_req.SerializeToString(&out_msg);
    
        client_comm_.send_get_req(out_msg);
    }

    {
        std::unique_lock<std::mutex> lk(get_status_[gk]->m);
        const bool &d = get_status_[gk]->done;
        get_status_[gk]->cv.wait(lk, [&d]{return d;});
    }

    if (!get_status_[gk]->ret && get_status_[gk]->srl_pdu) // something's wrong if ret = non-zero
        return get_status_[gk]->srl_pdu;
    else
        return NULL;
}

bool DCClient::CommitAck(const std::string &hash) {
    auto it = put_status_.find(hash);
    if (it == put_status_.end())
        return false;
    
    {
        std::unique_lock<std::mutex> lk(it->second->m);
        it->second->done = true;
        it->second->ret = 0;
    }
    it->second->cv.notify_all();

    return true;
}

bool DCClient::CommitGetResp(const std::string &hash, const capsule::CapsulePDU &pdu) {
    bool is_metaonly_req = (pdu.payload_in_transit().size() == 0);
    gs_key gk(hash, is_metaonly_req);

    auto it = get_status_.find(gk);
    if (it == get_status_.end())
        return false;
    {
        std::unique_lock<std::mutex> lk(it->second->m);
        it->second->done = true;
        it->second->ret = 0;
        it->second->srl_pdu = new std::string();

        /* TODO: change inteface to remove this reserialization */
        pdu.SerializeToString(it->second->srl_pdu);    
    }
    it->second->cv.notify_all();

    return true;
}

bool DCClient::CommitFreshResp(const std::string &hash, const capsule::FreshHashesContainer &fhc) {
    gs_key gk(hash, false);

    auto it = get_status_.find(gk);
    if (it == get_status_.end())
        return false;
    {
        std::unique_lock<std::mutex> lk(it->second->m);
        it->second->done = true;
        it->second->ret = 0;
        it->second->srl_pdu = new std::string();

        fhc.SerializeToString(it->second->srl_pdu);
        //it->second->srl_pdu = new std::string(fresh_hashes_str.c_str(), fresh_hashes_str.size());
    }
    it->second->cv.notify_all();

    return true;
}