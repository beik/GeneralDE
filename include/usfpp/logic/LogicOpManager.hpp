#ifndef USFPP_LOGIC_OPMANAGER_H
#define USFPP_LOGIC_OPMANAGER_H
#include "cpepp/utils/ClassCategory.hpp"
#include "gdpp/app/Application.hpp"
#include "usf/logic/logic_manage.h"
#include "System.hpp"

namespace Usf { namespace Logic {

class LogicOpManager : public Cpe::Utils::SimulateObject {
public:
    operator logic_manage_t() const { return (logic_manage_t)this; }

    const char * name(void) const { return logic_manage_name(*this); }
    cpe_hash_string_t name_hs(void) const { return logic_manage_name_hs(*this); }

    Gd::App::Application & app(void) { return Gd::App::Application::_cast(logic_manage_app(*this)); }
    Gd::App::Application const & app(void) const { return Gd::App::Application::_cast(logic_manage_app(*this)); }

    LogicOpContext & createContext(
        size_t capacity = 0,
        const void * data = 0,
        logic_context_id_t id = INVALID_LOGIC_CONTEXT_ID);

    template<typename T>
    LogicOpContext &
    createContext(
        T const & data,
        logic_context_id_t id = INVALID_LOGIC_CONTEXT_ID)
    {
        return createContext(sizeof(data), &data, id);
    }

    void destory(void) { logic_manage_free(*this); }

    static LogicOpManager & instance(gd_app_context_t app = 0, cpe_hash_string_t name = 0);
    static LogicOpManager & instance(gd_app_context_t app, const char * name);

    static LogicOpManager * find(gd_app_context_t app = 0, cpe_hash_string_t name = 0);
    static LogicOpManager * find(gd_app_context_t app, const char * name);

    static LogicOpManager & install(gd_app_context_t app, mem_allocrator_t alloc = 0, const char * name = 0);
    static void uninstall(gd_app_context_t app, cpe_hash_string_t name = 0);
};

}}

#endif