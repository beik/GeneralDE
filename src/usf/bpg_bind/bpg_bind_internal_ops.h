#ifndef USF_BPG_BIND_INTERNAL_OPS_H
#define USF_BPG_BIND_INTERNAL_OPS_H
#include "cpe/net/net_types.h"
#include "bpg_bind_internal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*binding operations*/
int bpg_bind_binding_create(
    bpg_bind_manage_t mgr,
    uint64_t client_id,
    uint32_t connection_id);

void bpg_bind_binding_free(bpg_bind_manage_t mgr, struct bpg_bind_binding * binding);

struct bpg_bind_binding *
bpg_bind_binding_find_by_client_id(bpg_bind_manage_t mgr, uint64_t client_id);

struct bpg_bind_binding *
bpg_bind_binding_find_by_connection_id(bpg_bind_manage_t mgr, uint32_t connection_id);

//net_ep_t bpg_bind_process_reply(bpg_bind_manage_t mgr, uint64_t client_id, uint32_t connection_id);
bpg_net_pkg_next_step_t bpg_bind_process_recv(bpg_bind_manage_t mgr, uint64_t client_id, uint32_t connection_id);

uint32_t bpg_bind_binding_client_id_hash(
    const struct bpg_bind_binding * binding);
int bpg_bind_binding_client_id_cmp(
    const struct bpg_bind_binding * l,
    const struct bpg_bind_binding * r);
uint32_t bpg_bind_binding_connection_id_hash(
    const struct bpg_bind_binding * binding);
int bpg_bind_binding_connection_id_cmp(
    const struct bpg_bind_binding * l,
    const struct bpg_bind_binding * r);
void bpg_bind_binding_free_all(bpg_bind_manage_t mgr);

#ifdef __cplusplus
}
#endif

#endif