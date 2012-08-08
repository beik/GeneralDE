#include "cpe/utils/error.h"
#include "cpe/dp/dp_manage.h"
#include "usf/bpg_pkg/bpg_pkg.h"
#include "usf/bpg_bind/bpg_bind_manage.h"
#include "bpg_bind_internal_ops.h"

int bpg_bind_manage_rsp(dp_req_t req, void * ctx, error_monitor_t em) {
    bpg_bind_manage_t mgr = (bpg_bind_manage_t)ctx;
    bpg_pkg_t pkg;

    /* if (req->type == "control") { */
    /*     if (connect) {// */
    /*     } */
    /*     else if (disconnect) { */
    /*         /\*close queue*\/ */
    /*     } */
    /* } */
    /* else { */
        pkg = bpg_pkg_from_dp_req(req);
        if (pkg == NULL) {
            CPE_ERROR(mgr->m_em, "%s: cast to bpg_pkg fail!", bpg_bind_manage_name(mgr));
            return -1;
        }

		////��bind��ϣ���в����Ƿ�ǰclientid��connection�Ѿ�����
		struct bpg_bind_binding * binding;

		binding = mem_alloc(mgr->m_alloc, sizeof(struct bpg_bind_binding));
		if (binding == NULL) return -1;

		binding->m_client_id = bpg_pkg_client_id(pkg);
		binding->m_connection_id = bpg_pkg_connection_id(pkg);

		if (binding->m_client_id != 0)
		{
			void * pUserFound = cpe_hash_table_find(&mgr->m_cliensts, binding);
			if (pUserFound != NULL)
			{
				struct bpg_bind_binding * found_binding = (struct bpg_bind_binding *) pUserFound;

				if (found_binding->m_connection_id != binding->m_connection_id)
				{
					//����ҵ��˳�ͻ���û���������Ϣ����֪ͨ��ͻ���û����ߣ�ͬʱɾ�����û�
					bpg_pkg_t kickoff_packet = bpg_pkg_create(mgr->m_pkg_manage, 4096, NULL, 0);
					if (kickoff_packet != NULL)
					{
						bpg_pkg_set_connection_id(kickoff_packet, found_binding->m_connection_id);
						bpg_pkg_set_cmd(kickoff_packet, 10413);
						bpg_pkg_set_client_id(kickoff_packet, found_binding->m_client_id);
						//RESKICKOFF* pbody = (RESKICKOFF*)bpg_pkg_pkg_data(kickoff_packet);
						//if (pbody)
						//{
						//	pbody->result = 1;
						//}
						dp_req_t resreq = bpg_pkg_to_dp_req(kickoff_packet);
						dp_dispatch_by_string(mgr->m_reply_to, resreq, mgr->m_em);
						cpe_hash_table_remove_by_key(&mgr->m_cliensts, found_binding);
						cpe_hash_table_remove_by_key(&mgr->m_connections, found_binding);

						bpg_pkg_free(kickoff_packet);
					}
				}
			}

			bpg_bind_process_binding(mgr, binding->m_client_id, binding->m_connection_id);
		}

		mem_free(mgr->m_alloc, binding);

        if (dp_dispatch_by_numeric(bpg_pkg_cmd(pkg), bpg_pkg_to_dp_req(pkg), mgr->m_em) != 0) {
            CPE_ERROR(
                mgr->m_em, "%s: dispatch cmd %d error!",
                bpg_bind_manage_name(mgr), bpg_pkg_cmd(pkg));
            return -1;
        }

    return 0;
}
