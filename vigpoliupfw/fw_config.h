#pragma once

#include <stdint.h>

#include <rte_ether.h>

#include "nf.h"

struct nf_config {
  // WAN device, i.e. external
  uint16_t wan_device;

  // MAC addresses of devices
  struct rte_ether_addr *device_macs;

  // MAC addresses of the endpoints the devices are linked to
  struct rte_ether_addr *endpoint_macs;

  // Expiration time of flows, in microseconds
  uint32_t expiration_time;

  // Size of the flow table
  uint32_t max_flows;

    // LAN (i.e. internal) device
  uint16_t lan_device;

  // Policer rate in B/s
  uint64_t rate;

  // Policer burst size in B
  uint64_t burst;

  // Size of the dynamic filtering table
  uint32_t dyn_capacity;
};
