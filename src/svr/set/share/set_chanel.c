#include <assert.h>
#include "cpe/pal/pal_platform.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/stream_buffer.h"
#include "cpe/dp/dp_request.h"
#include "cpe/dr/dr_pbuf.h"
#include "svr/set/share/set_pkg.h"
#include "svr/set/share/set_chanel.h"
#include "protocol/svr/set/set_share_chanel.h"
#include "protocol/svr/set/set_share_pkg.h"

#define SET_SHARE_SAVE_HEAD_SIZE (sizeof(uint32_t) + sizeof(SET_PKG_HEAD) + 1) /*长度计数以及一个协议头*/

static void set_chanel_pipe_save_ignore_pkg(void * buf, size_t capacity);
static int set_chanel_pipe_save_pkg(dp_req_t body, dp_req_t head, dp_req_t carry, void * buf, size_t capacity);
static int set_chanel_pipe_load_pkg(dp_req_t body, dp_req_t head, dp_req_t carry, void * buf, size_t capacity);
static int set_chanel_pipe_write(SVR_SET_CHANEL * chanel, SVR_SET_PIPE * pipe, dp_req_t body, size_t * size);
static int set_chanel_pipe_peak(SVR_SET_CHANEL * chanel, SVR_SET_PIPE * pipe, dp_req_t body);
static int set_chanel_pipe_erase(SVR_SET_CHANEL * chanel, SVR_SET_PIPE * pipe);

int set_chanel_r_write(set_chanel_t input_chanel, dp_req_t body, size_t * size) {
    SVR_SET_CHANEL * chanel = (SVR_SET_CHANEL *)input_chanel;
    return set_chanel_pipe_write(chanel, &chanel->r, body, size);
}

int set_chanel_r_peak(set_chanel_t input_chanel, dp_req_t body) {
    SVR_SET_CHANEL * chanel = (SVR_SET_CHANEL *)input_chanel;
    return set_chanel_pipe_peak(chanel, &chanel->r, body);
}

int set_chanel_r_erase(set_chanel_t input_chanel) {
    SVR_SET_CHANEL * chanel = (SVR_SET_CHANEL *)input_chanel;
    return set_chanel_pipe_erase(chanel, &chanel->r);
}

int set_chanel_w_write(set_chanel_t input_chanel, dp_req_t body, size_t * size) {
    SVR_SET_CHANEL * chanel = (SVR_SET_CHANEL *)input_chanel;
    return set_chanel_pipe_write(chanel, &chanel->w, body, size);
}

int set_chanel_w_peak(set_chanel_t input_chanel, dp_req_t body) {
    SVR_SET_CHANEL * chanel = (SVR_SET_CHANEL *)input_chanel;
    return set_chanel_pipe_peak(chanel, &chanel->w, body);
}

int set_chanel_w_erase(set_chanel_t input_chanel) {
    SVR_SET_CHANEL * chanel = (SVR_SET_CHANEL *)input_chanel;
    return set_chanel_pipe_erase(chanel, &chanel->w);
}

static int set_chanel_pipe_erase(SVR_SET_CHANEL * chanel, SVR_SET_PIPE * pipe) {
    char * buf = ((char *)chanel) + pipe->begin;
    SET_PKG_HEAD * head;
    uint32_t total_size;
    uint32_t capacity;

TRY_AGAIN:
    if (pipe->wp >= pipe->rp) {
        capacity = pipe->wp - pipe->rp;

        if (capacity < SET_SHARE_SAVE_HEAD_SIZE) {
            pipe->rp = pipe->wp;
            return set_chanel_error_chanel_empty;
        }

        total_size = *((uint32_t*)(buf + pipe->rp));

        if (capacity < total_size) {
            pipe->rp = pipe->wp;
            return set_chanel_error_chanel_empty;
        }

        if (total_size < SET_SHARE_SAVE_HEAD_SIZE) {
            pipe->rp += total_size;
            goto TRY_AGAIN;
        }

        head = (SET_PKG_HEAD *)(buf + pipe->rp + sizeof(uint32_t));
        if (head->to_svr_id == 0 && head->to_svr_type == 0) {
            pipe->rp += total_size;
            goto TRY_AGAIN;
        }

        pipe->rp += total_size;
        return 0;
    }
    else {
        assert(pipe->wp < pipe->rp);
        assert(pipe->rp <= pipe->capacity);

        capacity = pipe->capacity - pipe->rp;
        if (capacity < SET_SHARE_SAVE_HEAD_SIZE) {
            pipe->rp = 0;
            goto TRY_AGAIN;
        }

        total_size = *((uint32_t*)(buf + pipe->rp));
        if (capacity < total_size) {
            pipe->rp = 0;
            goto TRY_AGAIN;
        }

        if (total_size < SET_SHARE_SAVE_HEAD_SIZE) {
            pipe->rp += total_size;
            goto TRY_AGAIN;
        }

        head = (SET_PKG_HEAD *)(buf + pipe->rp + sizeof(uint32_t));
        if (head->to_svr_id == 0 && head->to_svr_type == 0) {
            pipe->rp += total_size;
            goto TRY_AGAIN;
        }

        pipe->rp += total_size;
        return 0;
    }
}

static int set_chanel_pipe_peak(SVR_SET_CHANEL * chanel, SVR_SET_PIPE * pipe, dp_req_t body) {
    dp_req_t head = set_pkg_head_check_create(body);
    dp_req_t carry = set_pkg_carry_check_create(body, 0);
    char * buf = ((char *)chanel) + pipe->begin;
    int rv;

    if (head == NULL) return set_chanel_error_no_memory;

    if (pipe->wp >= pipe->rp) {
        rv = set_chanel_pipe_load_pkg(body, head, carry, buf + pipe->rp, pipe->wp - pipe->rp);
    }
    else {
        assert(pipe->wp < pipe->rp);

        rv = set_chanel_pipe_load_pkg(body, head, carry, buf + pipe->rp, pipe->capacity - pipe->rp);
        if (rv == set_chanel_error_chanel_empty || rv == -1) {
            pipe->rp = 0;
            rv = set_chanel_pipe_load_pkg(body, head, carry, buf + pipe->rp, pipe->wp - pipe->rp);
        }
    }

    if (rv == -1) {/*internal error, should not occure*/
        pipe->rp = pipe->wp; /*清空数据*/
        return set_chanel_error_chanel_empty;
    }
    else {
        return rv;
    }
}

static int set_chanel_pipe_write(SVR_SET_CHANEL * chanel, SVR_SET_PIPE * pipe, dp_req_t body, size_t * size) {
    dp_req_t head = set_pkg_head_find(body);
    dp_req_t carry = set_pkg_carry_find(body);
    char * buf = ((char *)chanel) + pipe->begin;
    int write_size;

    assert(head);

    if (pipe->wp >= pipe->capacity) {
        if (pipe->rp == 0) return set_chanel_error_chanel_full;
        pipe->wp = 0;
    }

    if (pipe->wp >= pipe->rp) {
        assert(pipe->wp != pipe->capacity);

        write_size = set_chanel_pipe_save_pkg(body, head, carry, buf + pipe->wp, pipe->capacity - pipe->wp);
        if (write_size > 0) {
            pipe->wp += write_size;
        }
        else if (write_size == set_chanel_error_chanel_full) {
            if (pipe->rp <= 0) return set_chanel_error_chanel_full;

            write_size = set_chanel_pipe_save_pkg(body, head, carry, buf, pipe->rp - 1);
            if (write_size > 0) {
                set_chanel_pipe_save_ignore_pkg(buf + pipe->wp, pipe->capacity - pipe->wp);
                pipe->wp = write_size;
            }
            else {
                return set_chanel_error_chanel_full;
            }
        }
        else {
            return write_size;
        }
    }
    else {
        write_size = set_chanel_pipe_save_pkg(body, head, carry, buf + pipe->wp, pipe->rp - pipe->wp - 1);
        if (write_size > 0) {
            pipe->wp += write_size;
        }
        else {
            return write_size;
        }
    }

    if (size) *size = write_size;

    return 0;
}

static void set_chanel_pipe_save_ignore_pkg(void * buf, size_t capacity) {
    SET_PKG_HEAD * head;

    if (capacity < SET_SHARE_SAVE_HEAD_SIZE) return;
    
    *((uint32_t*)buf) = (uint32_t)capacity;
    head = (SET_PKG_HEAD *)(((uint32_t*)buf) + 1);
    head->to_svr_id = 0;
    head->to_svr_type = 0;
}

static int set_chanel_pipe_load_pkg(dp_req_t body, dp_req_t head, dp_req_t carry, void * buf, size_t capacity) {
    uint32_t total_size;
    uint32_t left_size;
    SET_PKG_HEAD * head_buf;
    char * read_buf = buf;
    uint32_t read_pos;

TRY_AGAIN:
    if (capacity < SET_SHARE_SAVE_HEAD_SIZE) return set_chanel_error_chanel_empty;

    total_size = *((uint32_t*)read_buf);

    if (capacity < total_size) return -1;

    if (total_size < SET_SHARE_SAVE_HEAD_SIZE) {
        read_buf += total_size;
        capacity -= total_size;
        goto TRY_AGAIN;
    }

    read_pos = sizeof(uint32_t);
    left_size = total_size - sizeof(uint32_t);

    head_buf = (SET_PKG_HEAD *)(read_buf + read_pos);
    read_pos += sizeof(SET_PKG_HEAD);
    left_size -= sizeof(SET_PKG_HEAD);

    if (head_buf->to_svr_id == 0 && head_buf->to_svr_type == 0) {
        read_buf += total_size;
        capacity -= total_size;
        goto TRY_AGAIN;
    }

    dp_req_set_buf(head, head_buf, sizeof(SET_PKG_HEAD));
    dp_req_set_size(head, sizeof(SET_PKG_HEAD));

    if (set_pkg_carry_set_buf(carry, read_buf + read_pos, left_size) != 0) {
        assert(0);
        return -1;
    }

    read_pos += dp_req_size(carry);
    left_size -= dp_req_size(carry);

    dp_req_set_buf(body, read_buf + read_pos, left_size);
    dp_req_set_size(body, left_size);

    return 0;
}

static int set_chanel_pipe_save_pkg(dp_req_t body, dp_req_t head, dp_req_t carry, void * input_buf, size_t capacity) {
    char * buf = input_buf;
    uint32_t total_size;
    size_t head_size;
    uint8_t carry_size = 0;
    SET_PKG_HEAD * pkg_head_buf;

    if (carry) {
        carry_size = set_pkg_carry_size(carry);
        assert(carry_size + 1 == dp_req_size(carry));
    }

    head_size = sizeof(uint32_t) + sizeof(SET_PKG_HEAD) + 1 + carry_size;

    if (capacity < head_size) return set_chanel_error_chanel_full;

    if (set_pkg_pack_state(head) == set_pkg_packed) {
        size_t body_capacity;
        int decode_size;

        body_capacity = capacity - head_size;

        decode_size = 
            dr_pbuf_read(
                buf + head_size, body_capacity,
                dp_req_data(body), dp_req_size(body), dp_req_meta(body), NULL);
        if (decode_size < 0) {
            if (decode_size == dr_code_error_not_enough_output) {
                return set_chanel_error_chanel_full;
            }
            else {
                return set_chanel_error_decode;
            }
        }

        total_size = head_size + decode_size;
    }
    else {
        total_size = head_size + dp_req_size(body);
        
        if (capacity < total_size) return set_chanel_error_chanel_full;

        /*填写包体 */
        memcpy(buf + head_size, dp_req_data(body), dp_req_size(body));
    }

    /*填写总大小 */
    memcpy(buf, &total_size, sizeof(total_size));

    /*填写包头 */
    pkg_head_buf = (SET_PKG_HEAD*)(buf + sizeof(total_size));
    memcpy(pkg_head_buf, dp_req_data(head), sizeof(SET_PKG_HEAD));
    pkg_head_buf->flags &= ~((uint16_t)(1 << 2));

    /*填写carry_data */
    *(uint8_t*)(buf + (sizeof(total_size) + sizeof(SET_PKG_HEAD))) = carry_size;
    if (carry_size) {
        memcpy(buf + sizeof(total_size) + sizeof(SET_PKG_HEAD) + 1, set_pkg_carry_data(carry), carry_size);
    }

    return total_size;
}

const char * set_chanel_str_error(int err) {
    switch(err) {
    case set_chanel_error_chanel_full:
        return "chanel_full";
    case set_chanel_error_chanel_empty:
        return "chanel_empty";
    case set_chanel_error_carry_overflow:
        return "carry_overflow";
    case set_chanel_error_no_memory:
        return "no_memory";
    case set_chanel_error_decode:
        return "decode_error";
    default:
        return "unknown set_chanel_error";
    }
}

const char * set_chanel_dump(set_chanel_t input_chanel, mem_buffer_t buffer) {
    SVR_SET_CHANEL * chanel = (SVR_SET_CHANEL *)input_chanel;
    struct write_stream_buffer stream;

    write_stream_buffer_init(&stream, buffer);
    stream_printf(
        (write_stream_t)&stream, "chanel: rq(capacity=%d, wp=%d, rp=%d) wq(capacity=%d, wp=%d, rp=%d)",
        chanel->r.capacity, chanel->r.wp, chanel->r.rp,
        chanel->w.capacity, chanel->w.wp, chanel->w.rp);

    return mem_buffer_make_continuous(buffer, 0);
}