#include "fw_flowmanager.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>  //for memcpy
#include <rte_ethdev.h>

#include "libvig/verified/double-chain.h"
#include "libvig/verified/map.h"
#include "libvig/verified/vector.h"
#include "libvig/verified/expirator.h"
#include "nf-log.h"

#include "state.h"

struct FlowManager {
  struct State *state;
  vigor_time_t expiration_time; /*seconds*/
};

struct FlowManager *flow_manager_allocate(uint16_t fw_device,
                                          vigor_time_t expiration_time,
                                          uint64_t max_flows,
                                          uint32_t capacity, uint32_t dev_count) {
  struct FlowManager *manager =
      (struct FlowManager *)malloc(sizeof(struct FlowManager));
  if (manager == NULL) {
    return NULL;
  }
  manager->state = alloc_state(max_flows, fw_device, capacity, dev_count);
  if (manager->state == NULL) {
    return NULL;
  }

  manager->expiration_time = expiration_time;

  return manager;
}

void flow_manager_allocate_or_refresh_flow(struct FlowManager *manager,
                                           struct FlowId *id,
                                           uint32_t internal_device,
                                           vigor_time_t time,
                                           uint16_t *external_port) {
  int index;
  if (map_get(manager->state->fm, id, &index)) {
    dchain_rejuvenate_index(manager->state->heap, index, time);
    return;
  }
  if (!dchain_allocate_new_index(manager->state->heap, &index, time)) {
    // No luck, the flow table is full, but we can at least let the
    // outgoing traffic out.
    return;
  }

  //TODO: add spec to start->start_port
  *external_port = 10000 + index;

  struct FlowId *key = 0;
  vector_borrow(manager->state->fv, index, (void **)&key);
  memcpy((void *)key, (void *)id, sizeof(struct FlowId));
  map_put(manager->state->fm, key, index);
  vector_return(manager->state->fv, index, key);
  uint32_t *int_dev;
  vector_borrow(manager->state->int_devices, index, (void **)&int_dev);
  *int_dev = internal_device;
  vector_return(manager->state->int_devices, index, int_dev);
}

void flow_manager_expire(struct FlowManager *manager, vigor_time_t time) {
  assert(time >= 0);  // we don't support the past
  assert(sizeof(vigor_time_t) <= sizeof(uint64_t));
  uint64_t time_u = (uint64_t)time;  // OK because of the two asserts
  vigor_time_t last_time =
      time_u - manager->expiration_time * 1000;  // us to ns
  expire_items_single_map(manager->state->heap, manager->state->fv,
                          manager->state->fm, last_time);
}

bool flow_manager_get_refresh_flow(struct FlowManager *manager,
                                   struct FlowId *id, vigor_time_t time,
                                   uint32_t *internal_device,
                                   uint16_t external_port) {
  
  //TODO add to spec state->external_port
  int index = external_port - 10000;
  if (dchain_is_index_allocated(manager->state->heap, index) == 0) {
    return false;
  }


  uint32_t *int_dev;
  vector_borrow(manager->state->int_devices, index, (void **)&int_dev);
  *internal_device = *int_dev;
  NF_INFO("Its a known flow on device %u", *internal_device);
  vector_return(manager->state->int_devices, index, int_dev);

  struct FlowId *key = 0;
  vector_borrow(manager->state->fv, index, (void **)&key);
  memcpy((void *)id, (void *)key, sizeof(struct FlowId));
  vector_return(manager->state->fv, index, key);

  dchain_rejuvenate_index(manager->state->heap, index, time);
  return true;
}


int policer_expire_entries(vigor_time_t time, uint16_t burst, uint16_t rate, struct FlowManager* flow_manager) {
  assert(time >= 0); // we don't support the past
  vigor_time_t exp_time =
      VIGOR_TIME_SECONDS_MULTIPLIER * (burst / rate);
  uint64_t time_u = (uint64_t)time;
  // OK because time >= config.burst / config.rate >= 0
  vigor_time_t min_time = time_u - exp_time;

  return expire_items_single_map(flow_manager->state->dyn_heap, flow_manager->state->dyn_keys,
                                 flow_manager->state->dyn_map, min_time);
}

bool policer_check_tb(uint32_t dst, uint16_t size, vigor_time_t time, struct FlowManager* flow_manager, uint16_t burst, uint16_t rate) {
  int index = -1;
  int present = map_get(flow_manager->state->dyn_map, &dst, &index);
  if (present) {
    dchain_rejuvenate_index(flow_manager->state->dyn_heap, index, time);

    struct DynamicValue *value = 0;
    vector_borrow(flow_manager->state->dyn_vals, index, (void **)&value);

    assert(0 <= time);
    uint64_t time_u = (uint64_t)time;
    assert(sizeof(vigor_time_t) == sizeof(int64_t));
    assert(value->bucket_time >= 0);
    assert(value->bucket_time <= time_u);
    uint64_t time_diff = time_u - value->bucket_time;
    if (time_diff <
        burst * VIGOR_TIME_SECONDS_MULTIPLIER / rate) {
      uint64_t added_tokens =
          time_diff * rate / VIGOR_TIME_SECONDS_MULTIPLIER;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtautological-compare"
      vigor_note(0 <= time_diff * rate / VIGOR_TIME_SECONDS_MULTIPLIER);
#pragma GCC diagnostic pop
      assert(value->bucket_size <= burst);
      value->bucket_size += added_tokens;
      if (value->bucket_size > burst) {
        value->bucket_size = burst;
      }
    } else {
      value->bucket_size = burst;
    }
    value->bucket_time = time_u;

    bool fwd = false;
    if (value->bucket_size > size) {
      value->bucket_size -= size;
      fwd = true;
    }

    vector_return(flow_manager->state->dyn_vals, index, value);

    return fwd;
  } else {
    if (size > burst) {
      NF_DEBUG("  Unknown flow with packet larger than burst size. Dropping.");
      return false;
    }

    int allocated =
        dchain_allocate_new_index(flow_manager->state->dyn_heap, &index, time);
    if (!allocated) {
      NF_DEBUG("No more space in the policer table");
      return false;
    }
    uint32_t *key;
    struct DynamicValue *value = 0;
    vector_borrow(flow_manager->state->dyn_keys, index, (void **)&key);
    vector_borrow(flow_manager->state->dyn_vals, index, (void **)&value);
    *key = dst;
    value->bucket_size = burst - size;
    value->bucket_time = time;
    map_put(flow_manager->state->dyn_map, key, index);
    // the other half of the key is in the map
    vector_return(flow_manager->state->dyn_keys, index, key);
    vector_return(flow_manager->state->dyn_vals, index, value);

    NF_DEBUG("  New flow. Forwarding.");
    return true;
  }
}

bool flow_manager_allocate_flow(struct FlowManager *manager, struct FlowId *id,
                                uint16_t internal_device, vigor_time_t time,
                                uint16_t *external_port) {
  int index;
  if (dchain_allocate_new_index(manager->state->heap, &index, time) == 0) {
    return false;
  }

  *external_port = manager->state->start_port + index;

  struct FlowId *key = 0;
  vector_borrow(manager->state->fv, index, (void **)&key);
  memcpy((void *)key, (void *)id, sizeof(struct FlowId));
  map_put(manager->state->fm, key, index);
  vector_return(manager->state->fv, index, key);
  return true;
}

void flow_manager_expire(struct FlowManager *manager, vigor_time_t time) {
  assert(time >= 0);  // we don't support the past
  assert(sizeof(vigor_time_t) <= sizeof(uint64_t));
  uint64_t time_u = (uint64_t)time;  // OK because of the two asserts
  vigor_time_t vigor_time_expiration = (vigor_time_t)manager->expiration_time;
  vigor_time_t last_time = time_u - vigor_time_expiration * 1000;  // us to ns
  expire_items_single_map(manager->state->heap, manager->state->fv,
                          manager->state->fm, last_time);
}

bool flow_manager_get_internal(struct FlowManager *manager, struct FlowId *id,
                               vigor_time_t time, uint16_t *external_port) {
  int index;
  if (map_get(manager->state->fm, id, &index) == 0) {
    return false;
  }
  *external_port = index + manager->state->start_port;
  dchain_rejuvenate_index(manager->state->heap, index, time);
  return true;
}

bool flow_manager_get_external(struct FlowManager *manager,
                               uint16_t external_port, vigor_time_t time,
                               struct FlowId *out_flow) {
  int index = external_port - manager->state->start_port;
  if (dchain_is_index_allocated(manager->state->heap, index) == 0) {
    return false;
  }

  struct FlowId *key = 0;
  vector_borrow(manager->state->fv, index, (void **)&key);
  memcpy((void *)out_flow, (void *)key, sizeof(struct FlowId));
  vector_return(manager->state->fv, index, key);

  dchain_rejuvenate_index(manager->state->heap, index, time);

  return true;
}
