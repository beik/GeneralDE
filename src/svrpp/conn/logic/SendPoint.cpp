#include <stdexcept>
#include "cpe/dr/dr_metalib_manage.h"
#include "cpepp/dp/Request.hpp"
#include "gdpp/app/Log.hpp"
#include "gdpp/app/Application.hpp"
#include "svrpp/conn/logic/SendPoint.hpp" 

namespace Svr { namespace Conn {

Cpe::Dp::Request & SendPoint::outgoingPkgBuf(size_t size) {
    dp_req_t req = conn_logic_sp_outgoing_buf(*this, size);

    if (req == NULL) {
        APP_CTX_THROW_EXCEPTION(
            app(),
            ::std::runtime_error,
            "center_logic_sp %s: get outgoing pkg buf fail, size=%d", name(), (int)size);
    }

    return *(Cpe::Dp::Request *)req;
}

void SendPoint::sendReq(
    LPDRMETA meta, void const * data, size_t size,
    logic_require_t require)
{
    if (conn_logic_sp_send_request(*this, meta, data, size, require) != 0) {
        APP_CTX_THROW_EXCEPTION(
            app(),
            ::std::runtime_error,
            "center_logic_sp %s: send %s(len=%d) fail!",
            name(), dr_meta_name(meta), (int)size);
    }
}

SendPoint & SendPoint::instance(gd_app_context_t app, const char * name) {
    conn_logic_sp_t sp = conn_logic_sp_find_nc(app, name);
    if (sp == NULL) {
        APP_CTX_THROW_EXCEPTION(
            app,
            ::std::runtime_error,
            "center_logic_sp %s not exist!", name);
    }

    return *(SendPoint*)sp;
}

}}