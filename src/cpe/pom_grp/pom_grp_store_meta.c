#include <assert.h>
#include "cpe/pal/pal_stdio.h"
#include "cpe/dr/dr_metalib_init.h"
#include "cpe/dr/dr_metalib_build.h"
#include "cpe/dr/dr_metalib_manage.h"
#include "cpe/pom_grp/pom_grp_meta.h"
#include "cpe/pom_grp/pom_grp_store.h"
#include "pom_grp_internal_ops.h"

static int pom_grp_meta_build_store_meta_on_entry_normal(
    struct DRInBuildMetaLib * builder,
    struct DRInBuildMeta * meta,
    pom_grp_entry_meta_t entry,
    mem_buffer_t str_buffer,
    error_monitor_t em)
{
    LPDRMETA entry_meta = pom_grp_entry_meta_normal_meta(entry);
    assert(entry_meta);

    return dr_inbuild_meta_copy_entrys(meta, entry_meta);
}

static int pom_grp_meta_build_store_meta_on_entry_ba(
    struct DRInBuildMeta * meta,
    pom_grp_entry_meta_t entry,
    mem_buffer_t str_buffer,
    error_monitor_t em)
{
    struct DRInBuildMetaEntry * new_entry = dr_inbuild_meta_add_entry(meta);
    if (new_entry == NULL) {
        CPE_ERROR(em, "pom_grp_meta_build_store_meta: create new entry %s fail!", entry->m_name);
        return -1;
    }

    dr_inbuild_entry_set_name(new_entry, entry->m_name);
    dr_inbuild_entry_set_id(new_entry, -1);
    dr_inbuild_entry_set_type(new_entry, "uint8");
    dr_inbuild_entry_set_array_count(new_entry, pom_grp_entry_meta_ba_bytes(entry));

    return 0;
}

static int pom_grp_meta_build_store_meta_on_entry_binary(
    struct DRInBuildMeta * meta,
    pom_grp_entry_meta_t entry,
    mem_buffer_t str_buffer,
    error_monitor_t em)
{
    struct DRInBuildMetaEntry * new_entry = dr_inbuild_meta_add_entry(meta);
    if (new_entry == NULL) {
        CPE_ERROR(em, "pom_grp_meta_build_store_meta: create new entry %s fail!", entry->m_name);
        return -1;
    }

    dr_inbuild_entry_set_name(new_entry, entry->m_name);
    dr_inbuild_entry_set_id(new_entry, -1);
    dr_inbuild_entry_set_type(new_entry, "uint8");
    dr_inbuild_entry_set_array_count(new_entry, pom_grp_entry_meta_binary_capacity(entry));

    return 0;
}

static int pom_grp_meta_build_store_meta_on_entry_list(
    struct DRInBuildMetaLib * builder,
    struct DRInBuildMeta * main_meta,
    LPDRMETAENTRY key_entry,
    pom_grp_entry_meta_t entry,
    mem_buffer_t str_buffer,
    error_monitor_t em)
{
    LPDRMETA entry_meta = pom_grp_entry_meta_list_meta(entry);
    assert(entry_meta);

    if (entry->m_data.m_list.m_standalone) {
        struct DRInBuildMeta * meta;

        if (dr_inbuild_metalib_find_meta(builder, dr_meta_name(entry_meta))) return 0;

        meta = dr_inbuild_metalib_add_meta(builder);
        if (meta == NULL) {
            CPE_ERROR(em, "pom_grp_meta_build_store_meta: create meta for %s fail!", entry->m_name);
            return -1;
        }

        dr_inbuild_meta_init(meta, entry_meta);

        if (dr_inbuild_meta_copy_entry(meta, key_entry) == NULL) return -1;

        dr_inbuild_meta_copy_entrys(meta, entry_meta);
    }
    else {
        struct DRInBuildMetaEntry * new_entry;
        size_t countNameLen;
        char * countNameBuf;
        /*count*/
        new_entry = dr_inbuild_meta_add_entry(main_meta);
        if (new_entry == NULL) {
            CPE_ERROR(em, "pom_grp_meta_build_store_meta: create new entry %sCount fail!", entry->m_name);
            return -1;
        }

        countNameLen = strlen(entry->m_name) + 1 + strlen("Count");
        countNameBuf = mem_buffer_alloc(str_buffer, countNameLen);
        if (countNameBuf == NULL) {
            CPE_ERROR(em, "pom_grp_meta_build_store_meta: create new entry %sCount fail!", entry->m_name);
            return -1;
        }
        snprintf(countNameBuf, countNameLen, "%sCount", entry->m_name);
        dr_inbuild_entry_set_name(new_entry, countNameBuf);
        dr_inbuild_entry_set_id(new_entry, -1);
        dr_inbuild_entry_set_type(new_entry, "uint32");
        dr_inbuild_entry_set_array_count(new_entry, 1);


        /*entry*/
        if (dr_inbuild_metalib_find_meta(builder, dr_meta_name(entry_meta)) == NULL) {
            if (dr_inbuild_metalib_copy_meta(builder, entry_meta) == NULL) {
                CPE_ERROR(em, "pom_grp_meta_build_store_meta: create meta for %s fail!", entry->m_name);
                return -1;
            }
        }


        new_entry = dr_inbuild_meta_add_entry(main_meta);
        if (new_entry == NULL) {
            CPE_ERROR(em, "pom_grp_meta_build_store_meta: create new entry %s fail!", entry->m_name);
            return -1;
        }

        dr_inbuild_entry_set_name(new_entry, entry->m_name);
        dr_inbuild_entry_set_id(new_entry, -1);
        dr_inbuild_entry_set_type(new_entry, dr_meta_name(entry_meta));
        dr_inbuild_entry_set_array_count(new_entry, pom_grp_entry_meta_list_capacity(entry));
        dr_inbuild_entry_set_array_refer(new_entry, countNameBuf);
    }

    return 0;
}

static int pom_grp_meta_build_calc_version(
    struct DRInBuildMetaLib * builder)
{
    return 1;
}

static int pom_grp_meta_build_store_meta_i(
    struct DRInBuildMetaLib * builder,
    pom_grp_meta_t meta,
    const char * main_entry,
    const char * key,
    mem_buffer_t str_buffer,
    error_monitor_t em)
{
    struct DRInBuildMeta * main_meta;
    LPDRMETALIB src_metalib;
    LPDRMETAENTRY key_entry;
    pom_grp_entry_meta_t main_entry_meta;
    uint16_t i, count;

    main_entry_meta = pom_grp_entry_meta_find(meta, main_entry);
    if (main_entry_meta == NULL) {
        CPE_ERROR(em, "pom_grp_meta_build_store_meta: create metalib builder fail!");
        return -1;
    }

    if (main_entry_meta->m_type != pom_grp_entry_type_normal) {
        CPE_ERROR(
            em, "pom_grp_meta_build_store_meta: main entry %s is not type normal!",
            main_entry_meta->m_name);
        return -1;
    }

    key_entry = dr_meta_find_entry_by_name(pom_grp_entry_meta_normal_meta(main_entry_meta), key);
    if (key_entry == NULL) {
        CPE_ERROR(
            em, "pom_grp_meta_build_store_meta: key %s not exsit in %s(type=%s)!",
            key, main_entry_meta->m_name, dr_meta_name(pom_grp_entry_meta_normal_meta(main_entry_meta)));
        return -1;
    }

    main_meta = dr_inbuild_metalib_add_meta(builder);
    if (main_meta == NULL) {
        CPE_ERROR(em, "pom_grp_meta_build_store_meta: create main meta fail!");
        return -1;
    }
    dr_inbuild_meta_set_current_version(
        main_meta, dr_meta_current_version(dr_entry_self_meta(key_entry)));

    src_metalib = dr_meta_owner_lib(dr_entry_self_meta(key_entry));

    dr_inbuild_metalib_set_tagsetversion(builder, dr_lib_tag_set_version(src_metalib));
    dr_inbuild_metalib_set_name(builder, pom_grp_meta_name(meta));

    if (dr_inbuild_meta_init(main_meta, pom_grp_entry_meta_normal_meta(main_entry_meta)) != 0) return -1;

    if (pom_grp_meta_build_store_meta_on_entry_normal(builder, main_meta, main_entry_meta, str_buffer, em) != 0) return -1;

    count = pom_grp_meta_entry_count(meta);
    for(i = 0; i < count; ++i) {
        pom_grp_entry_meta_t entry_meta = pom_grp_meta_entry_at(meta, i);
        if (entry_meta == main_entry_meta) continue;

        switch(entry_meta->m_type) {
        case pom_grp_entry_type_normal:
            if (pom_grp_meta_build_store_meta_on_entry_normal(builder, main_meta, entry_meta, str_buffer, em) != 0) return -1;
            break;
        case pom_grp_entry_type_list:
            if (pom_grp_meta_build_store_meta_on_entry_list(builder, main_meta, key_entry, entry_meta, str_buffer, em) != 0) return -1;
            break;
        case pom_grp_entry_type_ba:
            if (pom_grp_meta_build_store_meta_on_entry_ba(main_meta, entry_meta, str_buffer, em) != 0) return -1;
            break;
        case pom_grp_entry_type_binary:
            if (pom_grp_meta_build_store_meta_on_entry_binary(main_meta, entry_meta, str_buffer, em) != 0) return -1;
            break;
        }
    }

    dr_inbuild_metalib_set_version(builder, pom_grp_meta_build_calc_version(builder));

    return 0;
}

int pom_grp_meta_build_store_meta(
    mem_buffer_t buffer,
    pom_grp_meta_t meta,
    const char * main_entry,
    const char * key,
    error_monitor_t em)
{
    struct mem_buffer str_buffer;

    struct DRInBuildMetaLib * builder = dr_inbuild_create_lib();
    if (builder == NULL) {
        CPE_ERROR(em, "pom_grp_meta_build_store_meta: create metalib builder fail!");
        return -1;
    }
    mem_buffer_init(&str_buffer, NULL);

    if (pom_grp_meta_build_store_meta_i(builder, meta, main_entry, key, &str_buffer, em) != 0) {
        goto ERROR;
    }
    
    if (dr_inbuild_tsort(builder, em) != 0) {
        CPE_ERROR(em, "pom_grp_meta_build_store_meta: sort lib fail!");
        goto ERROR;
    }

    if (dr_inbuild_build_lib(buffer, builder, em) != 0) {
        CPE_ERROR(em, "pom_grp_meta_build_store_meta: build lib to buffer fail!");
        goto ERROR;
    }

    dr_inbuild_free_lib(builder);
    mem_buffer_clear(&str_buffer);
    return 0;

ERROR:
    dr_inbuild_free_lib(builder);
    mem_buffer_clear(&str_buffer);
    return -1;
}