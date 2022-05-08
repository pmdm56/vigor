#include <stdlib.h>

#include "flow.h.gen.h"
#include "fw_flowmanager.h"
#include "fw_config.h"
#include "nf.h"
#include "nf-log.h"
#include "nf-util.h"
#include "assert.h"

#include "libvig/verified/double-chain.h"
#include "libvig/verified/map.h"
#include "libvig/verified/vector.h"
#include "libvig/verified/expirator.h"

struct nf_config config;

struct FlowManager *flow_manager;

bool nf_init(void) {
  flow_manager = flow_manager_allocate(
      config.wan_device, config.expiration_time, config.max_flows,config.dyn_capacity, rte_eth_dev_count_avail());
  return flow_manager != NULL;
}

int nf_process_firewall(uint16_t device, struct rte_ipv4_hdr* rte_ipv4_header, struct tcpudp_hdr * tcpudp_header, vigor_time_t now) {

  flow_manager_expire(flow_manager, now);

  NF_INFO("[NAT] Received packet");

  uint16_t dst_device;
  if (device == config.wan_device) {

    struct FlowId id;
    uint32_t dst_device_long;
    if (!flow_manager_get_refresh_flow(flow_manager, &id, now,
                                       &dst_device_long, tcpudp_header->dst_port)) {
      NF_INFO("[NAT] Unknown external flow, dropping");
      return device;
    }

    if (id.dst_ip != rte_ipv4_header->src_addr |
        id.dst_port != tcpudp_header->src_port |
        id.protocol != rte_ipv4_header->next_proto_id) {
      NF_DEBUG("[NAT]Spoofing attempt, dropping.");
      return device;
    }

    NF_DEBUG("[NAT] Found internal flow.");

    rte_ipv4_header->dst_addr = id.src_ip;
    tcpudp_header->dst_port = id.src_port;
    dst_device = dst_device_long;

  } else {

    uint16_t external_port;

    struct FlowId id = {.src_port = tcpudp_header->src_port,
                        .dst_port = tcpudp_header->dst_port,
                        .src_ip = rte_ipv4_header->src_addr,
                        .dst_ip = rte_ipv4_header->dst_addr,
                        .protocol = rte_ipv4_header->next_proto_id, };

    flow_manager_allocate_or_refresh_flow(flow_manager, &id, device, now, &external_port);
    NF_INFO("Internal flow:");
    LOG_FLOWID(&id, NF_INFO);

    //TODO: add external ip to config
    rte_ipv4_header->src_addr = 0;
    tcpudp_header->src_port = external_port;
    dst_device = config.wan_device;
    
  }

  return dst_device;

}

int nf_process_policer(uint16_t device, struct rte_ipv4_hdr* rte_ipv4_header, struct tcpudp_hdr * tcpudp_header, vigor_time_t now, uint16_t packet_length) {

  NF_INFO("[Policer] Received packet");

  policer_expire_entries(now, config.burst, config.rate, flow_manager);

    if (device == config.wan_device) {
    // Simply forward outgoing packets.
    NF_INFO("[Policer] Incoming packet. Not policing.");
    return config.lan_device;
  } else if (device == config.lan_device) {
    // Police incoming packets.
    bool fwd = policer_check_tb(rte_ipv4_header->dst_addr, packet_length, now, flow_manager, config.burst, config.rate);

    if (fwd) {
      NF_INFO("[Policer] Outcoming packet within policed rate. Forwarding.");
      return config.wan_device;
    } else {
      NF_INFO("[Policer] Outcoming packet outside of policed rate. Dropping.");
      return config.lan_device;
    }
  } else {
    // Drop any other packets.
    NF_INFO("[Policer] Unknown port. Dropping.");
    return device;
  }
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length,
               vigor_time_t now, struct rte_mbuf *mbuf) {
            
  struct rte_ether_hdr *rte_ether_header = nf_then_get_rte_ether_header(buffer);
  assert(rte_ether_header != NULL);

  uint8_t *ip_options;
  struct rte_ipv4_hdr *rte_ipv4_header =
      nf_then_get_rte_ipv4_header(rte_ether_header, buffer, &ip_options);
  if (rte_ipv4_header == NULL) {
    NF_INFO("Not IPv4, dropping");
    return device;
  }

  struct tcpudp_hdr *tcpudp_header =
      nf_then_get_tcpudp_header(rte_ipv4_header, buffer);
  if (tcpudp_header == NULL) {
    NF_INFO("Not TCP/UDP, dropping");
    return device;
  }
  
  uint16_t dst_device_fw =
      nf_process_firewall(device, rte_ipv4_header, tcpudp_header, now);

  uint16_t dst_device_pol =
      nf_process_policer(device, rte_ipv4_header, tcpudp_header, now, packet_length);

  //if at least 1 drop
  if((dst_device_pol == device) | (dst_device_fw == device))
    return device;

  // if no one drops they will forward the packet to the same device
  assert(dst_device_pol == dst_device_fw);

  nf_set_rte_ipv4_udptcp_checksum(rte_ipv4_header, tcpudp_header, buffer);

  rte_ether_header->s_addr = config.device_macs[dst_device_pol];
  rte_ether_header->d_addr = config.endpoint_macs[dst_device_pol];

  return dst_device_pol;
}
