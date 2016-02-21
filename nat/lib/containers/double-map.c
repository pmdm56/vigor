#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "double-map.h"
#include "double-map-impl.h"

/*
#if DMAP_IMPL_CAPACITY < capacity
#  error "The implementation dmap capacity is "
"insufficient for the declared interface capacity."
#endif
*/

struct DoubleMap {
  int key_a_offset;
  int key_a_size;
  int key_b_offset;
  int key_b_size;
  int value_size;

  uint8_t *values;

  int *bbs_a;
  void **kps_a;
  int *khs_a;
  int *inds_a;
  map_keys_equality *eq_a;

  int *bbs_b;
  void **kps_b;
  int *khs_b;
  int *inds_b;
  map_keys_equality *eq_b;

  int n_vals;
  int capacity;
};

static
uint32_t hash(void* keyp, int key_size)
{
  uint32_t* slice = (uint32_t*)keyp;
  int n = key_size*sizeof(uint8_t)/sizeof(uint32_t);
  uint32_t rez = 0;
  for (--n; n >= 0; --n)
    {
      rez ^= slice[n];
    }
  return rez;
}

int dmap_allocate(int key_a_size, int key_a_offset, map_keys_equality* eq_a,
                  int key_b_size, int key_b_offset, map_keys_equality* eq_b,
                  int value_size, int capacity,
                  struct DoubleMap** map_out) {

  *map_out = malloc(sizeof(struct DoubleMap));
  (**map_out).key_a_size = key_a_size;
  (**map_out).key_a_offset = key_a_offset;
  (**map_out).eq_a = eq_a;
  (**map_out).key_b_size = key_b_size;
  (**map_out).key_b_offset = key_b_offset;
  (**map_out).eq_b = eq_b;
  (**map_out).value_size = value_size;
  (**map_out).capacity = capacity;

  if (NULL == ((**map_out).values = malloc(value_size   *capacity))) return 0;
  if (NULL == ((**map_out).bbs_a  = malloc(sizeof(int)  *capacity))) return 0;
  if (NULL == ((**map_out).kps_a  = malloc(sizeof(void*)*capacity))) return 0;
  if (NULL == ((**map_out).khs_a  = malloc(sizeof(int)  *capacity))) return 0;
  if (NULL == ((**map_out).inds_a = malloc(sizeof(int)  *capacity))) return 0;
  if (NULL == ((**map_out).bbs_b  = malloc(sizeof(int)  *capacity))) return 0;
  if (NULL == ((**map_out).kps_b  = malloc(sizeof(void*)*capacity))) return 0;
  if (NULL == ((**map_out).khs_b  = malloc(sizeof(int)  *capacity))) return 0;
  if (NULL == ((**map_out).inds_b = malloc(sizeof(int)  *capacity))) return 0;

  dmap_impl_init((**map_out).bbs_a, (**map_out).eq_a,
                 (**map_out).bbs_b, (**map_out).eq_b,
                 (**map_out).capacity);

  (**map_out).n_vals = 0;
  return 1;
}

int dmap_get_a(struct DoubleMap* map, void* key, int* index) {
  return dmap_impl_get(map->bbs_a, map->kps_a, map->khs_a, map->inds_a, key,
                       hash(key, map->key_a_size), map->eq_a, index,
                       map->capacity);
}

int dmap_get_b(struct DoubleMap* map, void* key, int* index) {
  return dmap_impl_get(map->bbs_b, map->kps_b, map->khs_b, map->inds_b, key,
                       hash(key, map->key_b_size), map->eq_b, index,
                       map->capacity);
}

int dmap_put(struct DoubleMap* map, void* value, int index) {
  void* key_a = (uint8_t*)value + map->key_a_offset;
  void* key_b = (uint8_t*)value + map->key_b_offset;
  memcpy(map->values + index, value, map->value_size);
  int ret = dmap_impl_put(map->bbs_a, map->kps_a, map->khs_a,
                          map->inds_a, key_a,
                          hash(key_a, map->key_a_size),
                          map->bbs_b, map->kps_b, map->khs_b,
                          map->inds_b, key_b,
                          hash(key_b, map->key_b_size),
                          index, map->capacity);
  if (ret) ++map->n_vals;
  return ret;
}

int dmap_erase(struct DoubleMap* map, void* key_a, void* key_b) {
  int ret = dmap_impl_erase(map->bbs_a, map->kps_a, map->khs_a, key_a,
                            map->eq_a, hash(key_a, map->key_a_size),
                            map->bbs_b, map->kps_b, map->khs_b, key_b,
                            map->eq_b, hash(key_b, map->key_b_size),
                            map->capacity);
  if (ret) --map->n_vals;
  return ret;
}

void dmap_get_value(struct DoubleMap* map, int index, void* value_out) {
  memcpy(value_out, map->values + index, map->value_size);
}

void dmap_set_value(struct DoubleMap* map, int index, void* value) {
  memcpy(map->values + index, value, map->value_size);
}

int dmap_size(struct DoubleMap* map) {
  return map->n_vals;
}

