#ifndef SVR_CONN_BPG_INTERNAL_OPS_H
#define SVR_CONN_BPG_INTERNAL_OPS_H
#include "conn_bpg_internal_types.h"

conn_bpg_chanel_t
conn_bpg_chanel_create(
    gd_app_context_t app,
    const char * name,
    bpg_pkg_manage_t bpg_pkg_manage,
    conn_cli_t conn_cli,
    mem_allocrator_t alloc,
    error_monitor_t em);
void conn_bpg_chanel_free(conn_bpg_chanel_t mgr);

conn_bpg_chanel_t
conn_bpg_chanel_find(gd_app_context_t app, cpe_hash_string_t name);
conn_bpg_chanel_t
conn_bpg_chanel_find_nc(gd_app_context_t app, const char * name);

const char * conn_bpg_chanel_name(conn_bpg_chanel_t mgr);

int conn_bpg_chanel_set_incoming_recv_at(conn_bpg_chanel_t sp, const char * recv_at);
int conn_bpg_chanel_set_incoming_dispatch_to(conn_bpg_chanel_t sp, const char * dispatch_to);
int conn_bpg_chanel_set_outgoing_recv_at(conn_bpg_chanel_t sp, const char * recv_at);
int conn_bpg_chanel_set_outgoing_dispatch_to(conn_bpg_chanel_t sp, const char * dispatch_to);

int conn_bpg_chanel_incoming_recv(dp_req_t req, void * ctx, error_monitor_t em);
int conn_bpg_chanel_outgoing_recv(dp_req_t req, void * ctx, error_monitor_t em);

#endif