#include "cpepp/cfg/Node.hpp"
#include "cpepp/nm/Object.hpp"
#include "cpepp/nm/Manager.hpp"
#include "gdpp/app/Application.hpp"
#include "gdpp/app/ModuleDef.hpp"
#include "gdpp/app/Log.hpp"
#include "usf/bpg_pkg/bpg_pkg.h"
#include "usf/bpg_pkg/bpg_pkg_data.h"
#include "usf/bpg_rsp/bpg_rsp_manage.h"
#include "usf/bpg_rsp/bpg_rsp_addition.h"
#include "usfpp/bpg_rsp/RspManager.hpp"
#include "usfpp/mongo_cli/CliProxy.hpp"
#include "AccountSvrLogic.hpp"

namespace Svr { namespace Account {

class AccountSvrLogicImpl : public AccountSvrLogic {
public:
    static cpe_hash_string_t NAME;

    AccountSvrLogicImpl(Gd::App::Application & app, Gd::App::Module & module, Cpe::Cfg::Node & cfg)
        : m_app(app)
        , m_rspManager(Usf::Bpg::RspManager::instance(app, cfg["rsp-manage"].asString(NULL)))
        , m_db_name(cfg["db-connection"].asString().c_str())
    {
        init_rsp_mgr(app, module, cfg);
    }

    ~AccountSvrLogicImpl() {
        fini_rsp_manage();
    }

    virtual Usf::Mongo::CliProxy & db(void) {
        return Usf::Mongo::CliProxy::instance(m_app, m_db_name.c_str());
    }

private:
    void init_rsp_mgr(gd_app_context_t app, gd_app_module_t module, cfg_t cfg) {
        bpg_rsp_manage_set_context_op(
            m_rspManager,
            0,
            NULL,
            NULL,
            AccountSvr_pkg_init,
            NULL);
    }

    void fini_rsp_manage(void) {
        bpg_rsp_manage_set_context_op(
            m_rspManager,
            0,
            NULL,
            NULL,
            NULL,
            NULL);
    }

    static int AccountSvr_pkg_init(logic_context_t context, dp_req_t pkg, void * ctx) {
        bpg_pkg_set_cmd(pkg, bpg_pkg_cmd(pkg) + 1);
        bpg_rsp_addition_data_add_all(context);
        return 0;
    }

    Gd::App::Application & m_app;
    Usf::Bpg::RspManager & m_rspManager;
    ::std::string m_db_name;
};

AccountSvrLogic::~AccountSvrLogic() {
}

AccountSvrLogic & AccountSvrLogic::instance(Gd::App::Application & app) {
    AccountSvrLogic * r =
        dynamic_cast<AccountSvrLogic *>(
            &app.nmManager().object(AccountSvrLogic::NAME));
    if (r == NULL) {
        APP_THROW_EXCEPTION(::std::runtime_error, "AccountSvrLogic cast fail!");
    }

    return *r;
}

GDPP_APP_MODULE_DEF(AccountSvrLogic, AccountSvrLogicImpl);

}}