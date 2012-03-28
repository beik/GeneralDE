#include "cpe/pal/pal_external.h"
#include "cpe/net/net_manage.h"
#include "gd/app/app_context.h"

static int app_net_run_main(gd_app_context_t ctx) {
    return net_mgr_run(gd_app_net_mgr(ctx));
}

static int app_net_run_stop(gd_app_context_t ctx) {
    net_mgr_stop(gd_app_net_mgr(ctx));
    return 0;
}

EXPORT_DIRECTIVE
int app_net_runner_app_init(gd_app_context_t app, gd_app_module_t module, cfg_t cfg) {
    gd_app_set_main(app, app_net_run_main, app_net_run_stop);
    return 0;
}

EXPORT_DIRECTIVE
void app_net_runner_app_fini(gd_app_context_t app, gd_app_module_t module) {
}
