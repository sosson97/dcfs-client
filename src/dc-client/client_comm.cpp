#include "client_comm.hpp"

#include <cstdlib>

#include "capsule.pb.h"
#include "request.pb.h"
#include "dc_config.hpp"
#include "dc_client.hpp"

#include "util/logging.hpp"
#include "util/encode.hpp"
//#include "crypto.hpp"
//#include "crypto_util.hpp"

ClientComm::ClientComm(std::string ip, int64_t client_id, DCClient *dc_client)
    : m_context(1)
{
	if (ip.empty()) {
		Logger::log(ERROR, "[ClientComm] Client IP is empty. Exiting...");
		exit(1);
	}

    m_ip = ip;
    m_recv_ack_port = std::to_string(NET_CLIENT_RECV_ACK_PORT + client_id);
    m_recv_ack_addr = m_ip + ":" + m_recv_ack_port;
    m_recv_get_resp_port = std::to_string(NET_CLIENT_RECV_GET_RESP_PORT + client_id);
    m_recv_get_resp_addr = m_ip + ":" + m_recv_get_resp_port;
    m_dc_client = dc_client;

    // initialize server addrs and sockets
    std::string server_ips = NET_PAIRING_DC_SERVER_IPs; // use pairing ips
	if (server_ips.empty()) {
		Logger::log(ERROR, "[ClientComm] Server IPs is empty. Exiting...");
		exit(1);
	}

    std::cout << "TEST: " << server_ips << std::endl;
    std::string ip_delim = ",";
    std::string count_delim = ":";
    std::vector<std::pair<std::string, int> > server_ip_count;
    int last = 0;
    int next = 0;
    while ((next = server_ips.find(ip_delim, last)) != std::string::npos)
    {
        std::string ip_count = server_ips.substr(last, next - last);
        int pos = ip_count.find(count_delim);
        server_ip_count.push_back(std::make_pair(ip_count.substr(0, pos), std::stoi(ip_count.substr(pos + 1))));
        last = next + ip_delim.length();
    }
    std::string ip_count = server_ips.substr(last);
    int pos = ip_count.find(count_delim);
    server_ip_count.push_back(std::make_pair(ip_count.substr(0, pos), std::stoi(ip_count.substr(pos + 1))));

#if OUTGOING_MODE == 1 or OUTGOING_MODE == 2
    // initialize dc server dc sockets
    for (auto &p : server_ip_count)
    {
        for (int i = INIT_DC_SERVER_ID; i < p.second + INIT_DC_SERVER_ID; i++)
        {
            std::string server_addr = p.first + ":" + std::to_string(NET_DC_SERVER_BASE_PORT + i);
            zmq::socket_t *socket_send_dc = new zmq::socket_t(m_context, ZMQ_PUSH);
            socket_send_dc->connect("tcp://" + server_addr);

            m_dc_server_dc_sockets.emplace(server_addr, socket_send_dc);
            Logger::log(LogLevel::LDEBUG, "[DC CLIENT] connected to server for dc: " + server_addr);
        }
    }
    Logger::log(LogLevel::LDEBUG, "[DC CLIENT] Number of server destinations: " + std::to_string(m_dc_server_dc_sockets.size()));
#endif

    // initialize dc server serve sockets
    for (auto &p : server_ip_count)
    {
        for (int i = INIT_DC_SERVER_ID; i < p.second + INIT_DC_SERVER_ID; i++)
        {
            std::string server_addr = p.first + ":" + std::to_string(NET_SERVE_PORT + i);
            zmq::socket_t *socket_send_dc = new zmq::socket_t(m_context, ZMQ_PUSH);
            socket_send_dc->connect("tcp://" + server_addr);

            m_dc_server_serve_sockets.emplace(server_addr, socket_send_dc);
            Logger::log(LogLevel::LDEBUG, "[DC CLIENT] connected to server for serve get: " + server_addr);
        }
    }

#if OUTGOING_MODE == 3
    // initialize proxy write socket
    std::string proxy_write_addr = (std::string) NET_PROXY_IP + ":" + std::to_string(NET_PROXY_RECV_WRITE_REQ_PORT);
    m_proxy_write_socket = new zmq::socket_t(m_context, ZMQ_PUSH);
    m_proxy_write_socket->connect("tcp://" + proxy_write_addr);
    Logger::log(LogLevel::LDEBUG, "[DC CLIENT] connected to proxy for writes: " + proxy_write_addr);
#endif
}

void ClientComm::mcast_dc(const std::string &msg) 
{
    for (auto &p : m_dc_server_dc_sockets)
    {
        send_string(msg, p.second);
        Logger::log(LogLevel::LDEBUG, "[DC CLIENT] Sent dc to server: " + p.first);
    }
}

void ClientComm::send_dc_proxy(std::string &msg) 
{
    send_string(msg, m_proxy_write_socket);
    Logger::log(LogLevel::LDEBUG, "[DC CLIENT] Sent dc to proxy, dc: " + msg);
}

void ClientComm::send_get_req(std::string &msg) 
{
    auto random_it = std::next(
        std::begin(m_dc_server_serve_sockets), 
        rand() % m_dc_server_serve_sockets.size()
    );

    send_string(msg, random_it->second);
    Logger::log(LogLevel::LDEBUG, "[DC CLIENT] Sent get req to server: " + random_it->first);
}

void ClientComm::run_dc_client_listen_server(const std::atomic<bool> *end_signal)
{   
    // to receive ack msg from dc servers
    zmq::socket_t socket_for_ack(m_context, ZMQ_PULL);
    socket_for_ack.bind("tcp://*:" + m_recv_ack_port);
    // to receive get response from dc servers
    zmq::socket_t socket_for_get_resp(m_context, ZMQ_PULL);
    socket_for_get_resp.bind("tcp://*:" + m_recv_get_resp_port);

    // poll for new messages
    std::vector<zmq::pollitem_t> pollitems = {
        {static_cast<void *>(socket_for_ack), 0, ZMQ_POLLIN, 0},
        {static_cast<void *>(socket_for_get_resp), 0, ZMQ_POLLIN, 0},
    };

    Logger::log(LogLevel::LDEBUG, "[DC CLIENT] run_dc_client_listen_server() start polling.");
    while (true)
    {
        zmq::poll(pollitems.data(), pollitems.size(), 10);
        /* ack */
        if (pollitems[0].revents & ZMQ_POLLIN)
        {
            // Received an ack
            std::string msg = this->recv_string(&socket_for_ack);
            this->handle_ack(msg);
        }       
	    /* get response */
        if (pollitems[1].revents & ZMQ_POLLIN) 
        {
            // Received a get response
            std::string msg = this->recv_string(&socket_for_get_resp);
            this->handle_get_resp(msg);
        }

        if (end_signal->load()) {
            Logger::log(LogLevel::LDEBUG, "[DC CLIENT] listen server finished");
            return;
        }
    }
}

void ClientComm::handle_ack(std::string &msg) {
    capsule::CapsulePDU ack_dc;
    ack_dc.ParseFromString(msg);
    
    assert (ack_dc.header().msgtype() == REPLICATION_ACK);

    //if (verify_dc) // skip for now
    
    // receive acks directly from dc servers
    m_recv_ack_map[ack_dc.header_hash()] += 1;
    if (m_recv_ack_map[ack_dc.header_hash()] == WRITE_THRESHOLD) {
        Logger::log(LogLevel::INFO, "[DC CLIENT] ack message reached quorum for signature: " + Util::binary_to_hex_string(ack_dc.header_hash().c_str(), 32));
        
        if(!this->m_dc_client->CommitAck(ack_dc.header_hash())) {
            Logger::log(LogLevel::LDEBUG, "[DC CLIENT] COMMIT ACK failed");
        } 
    }
}
void ClientComm::handle_get_resp(std::string &msg) {
    capsule::ClientGetResponse resp;
    resp.ParseFromString(msg);

    std::string succ = resp.success() ? "true" : "false";

    if (resp.fresh_resp()) {
        Logger::log(LogLevel::INFO, "[DC CLIENT] Received freshness response");

        if (resp.success()) {
	    capsule::FreshHashesContainer fhc;
	    *fhc.mutable_fresh_hashes() = {resp.fresh_hashes().begin(), resp.fresh_hashes().end()};
            if(!this->m_dc_client->CommitFreshResp(resp.hash(), fhc))
                Logger::log(LogLevel::LDEBUG, "[DC CLIENT] COMMIT FRESHNESS failed");
        }
    } else {
        std::string dots("...");
        int print_len = 16;
        if (resp.hash().size() < print_len) {
            print_len = resp.hash().size();
            dots = std::string("");
        }                                                                                                            
        Logger::log(LogLevel::INFO, "[DC CLIENT] Received get response for hash: [" + Util::binary_to_hex_string(resp.hash().c_str(), print_len) + dots + "], succ: " + succ);

        if (resp.success()) {
            if(!this->m_dc_client->CommitGetResp(resp.hash(), resp.record()))
                Logger::log(LogLevel::LDEBUG, "[DC CLIENT] COMMIT GET RESP failed");
        }
    }
}
