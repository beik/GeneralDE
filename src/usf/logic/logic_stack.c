#include <assert.h>
#include "gd/app/app_context.h"
#include "usf/logic/logic_context.h"
#include "usf/logic/logic_manage.h"
#include "usf/logic/logic_executor.h"
#include "logic_internal_ops.h"

#define LOGIC_STACK_INLINE_ITEM_COUNT \
    ( sizeof(((struct logic_stack*)0)->m_inline_items) \
      / sizeof(((struct logic_stack*)0)->m_inline_items[0]) )

void logic_stack_init(struct logic_stack * stack) {
    stack->m_item_pos = -1;
    stack->m_extern_items = NULL;
    stack->m_extern_items_capacity = 0;
}

void logic_stack_fini(struct logic_stack * stack, logic_context_t context) {
    if (stack->m_extern_items) {
        mem_free(context->m_mgr->m_alloc, stack->m_extern_items);
    }
}

void logic_stack_push(struct logic_stack * stack, logic_context_t context, logic_executor_t executor) {
    struct logic_stack_item * stack_item;

REINTER:
    if (stack->m_item_pos + 1 < LOGIC_STACK_INLINE_ITEM_COUNT) {
        stack_item = &stack->m_inline_items[++stack->m_item_pos];
    }
    else {
        int32_t writePos = stack->m_item_pos + 1 - LOGIC_STACK_INLINE_ITEM_COUNT;
        if (writePos >= stack->m_extern_items_capacity) {
            int32_t new_capacity;
            struct logic_stack_item * new_buf;

            new_capacity = stack->m_extern_items_capacity + 16;
            new_buf = (struct logic_stack_item *)mem_alloc(context->m_mgr->m_alloc, sizeof(struct logic_stack_item) * new_capacity);
            if (new_buf == NULL) {
                context->m_errno = -1;
                context->m_state = logic_context_state_error;
                return;
            }

            if (stack->m_extern_items) {
                memcpy(new_buf, stack->m_extern_items, sizeof(struct logic_stack_item) * stack->m_extern_items_capacity);
                mem_free(context->m_mgr->m_alloc, stack->m_extern_items);
            }

            stack->m_extern_items = new_buf;
            stack->m_extern_items_capacity = new_capacity;
        }

        assert(writePos < stack->m_extern_items_capacity);
        stack_item = &stack->m_extern_items[writePos]; 
        ++stack->m_item_pos;
    }

    assert(stack_item);
    stack_item->m_executr = executor;

    if (executor->m_category == logic_executor_category_composite) {
        struct logic_executor_composite * composite = (struct logic_executor_composite *)executor;
        if (!TAILQ_EMPTY(&composite->m_members)) {
            executor = TAILQ_FIRST(&composite->m_members);
            goto REINTER;
        }
    }
}

#define logic_stack_item_at(stack, pos)                                 \
    ((pos) < LOGIC_STACK_INLINE_ITEM_COUNT                              \
     ? &stack->m_inline_items[(pos)]                                    \
     : &stack->m_extern_items[(pos) - LOGIC_STACK_INLINE_ITEM_COUNT])   \

logic_op_exec_result_t logic_stack_exec(struct logic_stack * stack, int32_t stop_stack_pos, logic_context_t ctx) {
    struct logic_stack_item * last_stack_item;
    last_stack_item = NULL;

    while(ctx->m_state == logic_context_state_idle
          && stack->m_item_pos > stop_stack_pos
          && ctx->m_require_waiting_count == 0)
    {
        struct logic_stack_item * stack_item = logic_stack_item_at(stack, stack->m_item_pos);

        if (stack_item->m_executr) {
            if (stack_item->m_executr->m_category == logic_executor_category_action) {
                struct logic_executor_action * action = (struct logic_executor_action *)stack_item->m_executr;
                if (action->m_type->m_op) {
                    stack_item->m_rv =
                        ((logic_op_fun_t)action->m_type->m_op)(ctx, stack_item->m_executr, action->m_type->m_ctx, action->m_args);
                    last_stack_item = stack_item;

                    if (stack_item->m_rv == logic_op_exec_result_redo) {
                        if (ctx->m_require_waiting_count == 0) {
                            CPE_ERROR(
                                gd_app_em(ctx->m_mgr->m_app), "logic_stack_exec: action logic op %s return redo, but no pending require!",
                                logic_executor_name(stack_item->m_executr));
                            ctx->m_errno = -1;
                            ctx->m_state = logic_context_state_error;
                        }
                        break;
                    }
                }
                else {
                    CPE_ERROR(gd_app_em(ctx->m_mgr->m_app), "logic_stack_exec: action logic op %s have no op!", logic_executor_name(stack_item->m_executr));
                }
            }
            else if (stack_item->m_executr->m_category == logic_executor_category_decorator) {
                struct logic_executor_decorator * decorator = (struct logic_executor_decorator *)stack_item->m_executr;
                logic_stack_push(stack, ctx, decorator->m_inner);
                continue;
            }
        }
        else {
            CPE_ERROR(gd_app_em(ctx->m_mgr->m_app), "stack item have no executor!");
        }

        --stack->m_item_pos;

        while(stack->m_item_pos > stop_stack_pos) {
            struct logic_stack_item * stack_item = logic_stack_item_at(stack, stack->m_item_pos);
            struct logic_stack_item * pre_stack_item = logic_stack_item_at(stack, stack->m_item_pos + 1);
            assert(pre_stack_item->m_rv != logic_op_exec_result_redo);

            if (stack_item->m_executr->m_category == logic_executor_category_composite) {
                struct logic_executor_composite * composite = (struct logic_executor_composite *)stack_item->m_executr;
                logic_executor_t next = TAILQ_NEXT(pre_stack_item->m_executr, m_next);

                switch(composite->m_composite_type) {
                case logic_executor_composite_selector:
                    break;
                case logic_executor_composite_sequence: {
                    if (pre_stack_item->m_rv == logic_op_exec_result_true) {
                        if (next) {
                            pre_stack_item->m_executr = next;
                            pre_stack_item->m_rv = logic_op_exec_result_false;
                            ++stack->m_item_pos;
                        }
                    }
                    else {
                        assert(pre_stack_item->m_rv == logic_op_exec_result_false);
                        stack_item->m_rv = logic_op_exec_result_false;
                    }
                    break;
                }
                case logic_executor_composite_parallel:
                    break;
                default:
                    break;
                }
            }
            else if (stack_item->m_executr->m_category == logic_executor_category_decorator) {
                struct logic_executor_decorator * decorator = (struct logic_executor_decorator *)stack_item->m_executr;
                switch(decorator->m_decorator_type) {
                case logic_executor_decorator_protect:
                    stack_item->m_rv = logic_op_exec_result_true;
                    last_stack_item = stack_item;
                    break;
                case logic_executor_decorator_not:
                    if (pre_stack_item->m_rv == logic_op_exec_result_true) {
                        stack_item->m_rv = logic_op_exec_result_false;
                    }
                    else {
                        assert(pre_stack_item->m_rv == logic_op_exec_result_false);
                        stack_item->m_rv = logic_op_exec_result_true;
                    }                        
                    last_stack_item = stack_item;
                    break;
                }
                --stack->m_item_pos;
                continue;
            }

            break;
        }
    }

    return last_stack_item ? last_stack_item->m_rv : logic_op_exec_result_false;
}