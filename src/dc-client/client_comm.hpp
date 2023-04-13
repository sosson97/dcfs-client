#ifndef CLIENT_COMM_H
#define CLIENT_COMM_H

#include <string>
#include <unordered_map>
#include <zmq.hpp>
#include <atomic>

#include "capsule.pb.h"


class DCClient; // Forward Declaration to avoid circular dependency

class ClientComm
{
public:
    ClientComm(std::string ip, int64_t client_id, DCClient *dc_client);

    void mcast_dc(const std::string &msg);
    void send_dc_proxy(std::string &msg);
    void send_get_req(std::string &msg);
    void run_dc_client_listen_server(const std::atomic<bool> *end_signal);

    DCClient *m_dc_client;
    std::string m_ip;
    std::string m_recv_ack_port;
    std::string m_recv_ack_addr;
    std::string m_recv_get_resp_port;
    std::string m_recv_get_resp_addr;
    zmq::context_t m_context;
    std::unordered_map<std::string, zmq::socket_t *> m_dc_server_dc_sockets;
    std::unordered_map<std::string, zmq::socket_t *> m_dc_server_serve_sockets;
    zmq::socket_t *m_proxy_write_socket;
    std::unordered_map<std::string, int> m_recv_ack_map;

    /* for comm test? */
    /*void CreatePdu(
        std::string hash_str, 
        std::string record_str, 
        capsule::CapsulePDU &out_dc);*/

    zmq::message_t string_to_message(const std::string &s)
    {
        zmq::message_t msg(s.size());
        memcpy(msg.data(), s.c_str(), s.size());
        return msg;
    }

    std::string message_to_string(const zmq::message_t &message)
    {
        return std::string(static_cast<const char *>(message.data()), message.size());
    }
    std::string recv_string(zmq::socket_t *socket)
    {
        zmq::message_t message;
        socket->recv(&message);
        return this->message_to_string(message);
    }
    void send_string(const std::string &s, zmq::socket_t *socket)
    {
        socket->send(string_to_message(s));
    }
private:
    void handle_ack(std::string &msg);
    void handle_get_resp(std::string &msg);
};
#endif // CLIENT_COMM_H