#ifndef _FLOWMANAGER_H_INCLUDED_
#define _FLOWMANAGER_H_INCLUDED_

#include "flow.h.gen.h"
#include "libvig/verified/vigor-time.h"

#include <stdbool.h>
#include <stdint.h>

struct FlowManager;

struct FlowManager *flow_manager_allocate(uint16_t fw_device,
                                          vigor_time_t expiration_time,
                                          uint64_t max_flows,
                                          uint32_t capacity, uint32_t dev_count);

void flow_manager_allocate_or_refresh_flow(struct FlowManager *manager,
                                           struct FlowId *id,
                                           uint32_t internal_device,
                                           vigor_time_t time,
                                           uint16_t* external_port);
void flow_manager_expire(struct FlowManager *manager, vigor_time_t time);
bool flow_manager_get_refresh_flow(struct FlowManager *manager,
                                   struct FlowId *id, vigor_time_t time,
                                   uint32_t *internal_device,
                                   uint16_t external_port);

int policer_expire_entries(vigor_time_t time, uint16_t burst, uint16_t rate,
                           struct FlowManager *flow_manager);

bool policer_check_tb(uint32_t dst, uint16_t size, vigor_time_t time,
                      struct FlowManager *flow_manager, uint16_t burst,
                      uint16_t rate);

bool flow_manager_allocate_flow(struct FlowManager *manager, struct FlowId *id,
                                uint16_t internal_device, vigor_time_t time,
                                uint16_t *external_port);
void flow_manager_expire(struct FlowManager *manager, vigor_time_t time);
bool flow_manager_get_internal(struct FlowManager *manager, struct FlowId *id,
                               vigor_time_t time, uint16_t *external_port);
bool flow_manager_get_external(struct FlowManager *manager,
                               uint16_t external_port, vigor_time_t time,
                               struct FlowId *out_flow);

#endif //_FLOWMANAGER_H_INCLUDED_
