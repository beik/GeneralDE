#include <assert.h>
#include <ctype.h>
#include "cpe/dr/dr_ctypes_info.h"
#include "cpe/dr/dr_metalib_builder.h"
#include "cpe/dr/dr_metalib_manage.h"
#include "generate_ops.h"

static void cpe_dr_generate_h_includes(write_stream_t stream, dr_metalib_source_t source, cpe_dr_generate_ctx_t ctx) {
    struct dr_metalib_source_it include_source_it;
    dr_metalib_source_t include_source;

    dr_metalib_source_includes(&include_source_it, source);
    while((include_source = dr_metalib_source_next(&include_source_it))) {
        stream_printf(stream, "#include \"%s.h\"\n", dr_metalib_source_name(include_source));
    }

    stream_printf(stream, "\n");
}

static void cpe_dr_generate_h_macros(write_stream_t stream, dr_metalib_source_t source, cpe_dr_generate_ctx_t ctx) {
    struct dr_metalib_source_element_it element_it;
    dr_metalib_source_element_t element;
    int macro_value;

    dr_metalib_source_elements(&element_it, source);
    while((element = dr_metalib_source_element_next(&element_it))) {
       if (dr_metalib_source_element_type(element) != dr_metalib_source_element_type_macro) continue;

       if (dr_lib_find_macro_value(&macro_value, ctx->m_metalib, dr_metalib_source_element_name(element)) == 0) {
           stream_printf(
               stream, "\n#define %s (%d)",
               dr_metalib_source_element_name(element),
               macro_value);
       }
    }

    stream_printf(stream, "\n");
}

static void cpe_dr_generate_h_print_type(write_stream_t stream, LPDRMETAENTRY entry) {
    switch(dr_entry_type(entry)) {
    case CPE_DR_TYPE_UNION:
    case CPE_DR_TYPE_STRUCT: {
        LPDRMETA ref_meta;
        ref_meta = dr_entry_ref_meta(entry);
        assert(ref_meta);
        stream_toupper(stream, dr_meta_name(ref_meta));
        break;
    }
    case CPE_DR_TYPE_STRING: {
        stream_printf(stream, "char *");
        break;
    }
    case CPE_DR_TYPE_CHAR: {
        stream_printf(stream, "char");
        break;
    }
    case CPE_DR_TYPE_UCHAR: {
        stream_printf(stream, "unsigned char");
        break;
    }
    case CPE_DR_TYPE_FLOAT: {
        stream_printf(stream, "float");
        break;
    }
    case CPE_DR_TYPE_DOUBLE: {
        stream_printf(stream, "double");
        break;
    }
    default:
        stream_printf(stream, "%s_t", dr_entry_type_name(entry));
        break;
    }
}

static void cpe_dr_generate_h_metas(write_stream_t stream, dr_metalib_source_t source, cpe_dr_generate_ctx_t ctx) {
    struct dr_metalib_source_element_it element_it;
    dr_metalib_source_element_t element;
    int curent_pack;
    int packed;

    curent_pack = __WORDSIZE / 8;
    packed = 0;
    dr_metalib_source_elements(&element_it, source);
    while((element = dr_metalib_source_element_next(&element_it))) {
        LPDRMETA meta;
        int entry_pos;
        const char * meta_name;

        if (dr_metalib_source_element_type(element) != dr_metalib_source_element_type_meta) continue;

        meta = dr_lib_find_meta_by_name(ctx->m_metalib, dr_metalib_source_element_name(element));
        if (meta == NULL) continue;

        if (dr_meta_align(meta) != curent_pack) {
            stream_printf(stream, "\n#pragma pack(%d)\n", dr_meta_align(meta));
            curent_pack = dr_meta_align(meta);
            packed = 1;
        }

        meta_name = dr_meta_name(meta);
        stream_printf(stream, "\ntypedef %s _%s {", dr_type_name(dr_meta_type(meta)), meta_name);

        for(entry_pos = 0; entry_pos < dr_meta_entry_num(meta); ++entry_pos) {
            LPDRMETAENTRY entry = dr_meta_entry_at(meta, entry_pos);

            stream_printf(stream, "\n");
            stream_printf(stream, "    ");

            switch(dr_entry_type(entry)) {
            case CPE_DR_TYPE_UNION:
            case CPE_DR_TYPE_STRUCT: {
                LPDRMETA ref_meta;
                ref_meta = dr_entry_ref_meta(entry);
                if (ref_meta == NULL) continue;

                stream_toupper(stream, dr_meta_name(ref_meta));
                stream_printf(stream, " %s", dr_entry_name(entry));
                break;
            }
            case CPE_DR_TYPE_STRING: {
                stream_printf(stream, "char %s[%d]", dr_entry_name(entry), dr_entry_size(entry));
                break;
            }
            case CPE_DR_TYPE_CHAR: {
                stream_printf(stream, "char %s", dr_entry_name(entry));
                break;
            }
            case CPE_DR_TYPE_UCHAR: {
                stream_printf(stream, "unsigned char %s", dr_entry_name(entry));
                break;
            }
            case CPE_DR_TYPE_FLOAT: {
                stream_printf(stream, "float %s", dr_entry_name(entry));
                break;
            }
            case CPE_DR_TYPE_DOUBLE: {
                stream_printf(stream, "double %s", dr_entry_name(entry));
                break;
            }
            default: {
                stream_printf(stream, "%s_t %s", dr_type_name(dr_entry_type(entry)), dr_entry_name(entry));
                break;
            }
            }

            if (dr_entry_array_count(entry) != 1) {
                stream_printf(stream, "[%d]", dr_entry_array_count(entry) < 1 ? 1 : dr_entry_array_count(entry));
            }

            stream_printf(stream, ";");
        }

        stream_printf(stream, "\n} ");
        stream_toupper(stream, meta_name);
        stream_printf(stream, ";\n");
    }

    if (packed) {
        stream_printf(stream, "\n#pragma pack()\n\n");
    }
}

static void cpe_dr_generate_h_traits(write_stream_t stream, dr_metalib_source_t source, cpe_dr_generate_ctx_t ctx) {
    struct dr_metalib_source_element_it element_it;
    dr_metalib_source_element_t element;

    stream_printf(stream, "\n#ifdef __cplusplus\n\n");

    stream_printf(stream, "namespace Cpe { namespace Dr {\n");
    stream_printf(stream, "\n");
    stream_printf(stream, "template<class T> struct MetaTraits;\n");
    stream_printf(stream, "\n");
    stream_printf(stream, "class Meta;\n");

    dr_metalib_source_elements(&element_it, source);
    while((element = dr_metalib_source_element_next(&element_it))) {
        LPDRMETA meta;
        struct dr_meta_dyn_info dyn_info;

        const char * meta_name;

        if (dr_metalib_source_element_type(element) != dr_metalib_source_element_type_meta) continue;

        meta = dr_lib_find_meta_by_name(ctx->m_metalib, dr_metalib_source_element_name(element));
        if (meta == NULL) continue;

        if (dr_meta_type(meta) != CPE_DR_TYPE_STRUCT) continue;

        meta_name = dr_meta_name(meta);
        stream_printf(stream, "template<> struct MetaTraits<");
        stream_toupper(stream, meta_name);
        stream_printf(stream, "> {\n");
        if (dr_meta_id(meta) != -1) {
            stream_printf(stream, "    static const int ID = %d;\n", dr_meta_id(meta));
        }
        stream_printf(stream, "    static Meta const & META;\n");
        stream_printf(stream, "    static const char * const NAME;\n");

        if (dr_meta_find_dyn_info(meta, &dyn_info) == 0) {
            stream_printf(stream, "    typedef ");
            cpe_dr_generate_h_print_type(stream, dyn_info.m_array_entry);
            stream_printf(stream, " dyn_element_type;\n");

            stream_printf(stream, "    static const int dyn_data_start_pos = %d;\n", dyn_info.m_array_start);
            stream_printf(stream, "    static const int dyn_count = %d;\n", dr_entry_array_count(dyn_info.m_array_entry));

            if (dyn_info.m_refer_entry) {
                char buf[256];

                stream_printf(stream, "    typedef ");
                cpe_dr_generate_h_print_type(stream, dyn_info.m_refer_entry);
                stream_printf(stream, " dyn_size_type;\n");

                stream_printf(stream, "    static const int dyn_refer_start_pos = %d;\n", dyn_info.m_refer_start);

                stream_printf(stream, "    static size_t data_size( ");
                stream_toupper(stream, meta_name);
                stream_printf(
                    stream, " const & o) { return sizeof(o) - sizeof(dyn_element_type) + sizeof(dyn_element_type) * o.%s; }\n",
                    dr_meta_off_to_path(meta, dyn_info.m_refer_start, buf, sizeof(buf)));
            }
        }
        else {
            stream_printf(stream, "    static size_t data_size( ");
            stream_toupper(stream, meta_name);
            stream_printf(stream, " const & o) { return sizeof(o); }\n");
        }

        stream_printf(stream, "};\n\n");
    }

    stream_printf(stream, "}}\n");
    stream_printf(stream, "\n#endif\n");
}

int cpe_dr_generate_h(write_stream_t stream, dr_metalib_source_t source, int with_traits, cpe_dr_generate_ctx_t ctx) {
    const char * lib_name;

    assert(source);
    assert(stream);

    if ((lib_name = dr_metalib_source_libname(source))) {
        stream_printf(stream, "#ifndef DR_GENERATED_H_%s_%s_INCLEDED\n", lib_name, dr_metalib_source_name(source));
        stream_printf(stream, "#define DR_GENERATED_H_%s_%s_INCLEDED\n", lib_name, dr_metalib_source_name(source));
    }
    else {
        stream_printf(stream, "#ifndef DR_GENERATED_H_%s_INCLEDED\n", dr_metalib_source_name(source));
        stream_printf(stream, "#define DR_GENERATED_H_%s_INCLEDED\n", dr_metalib_source_name(source));
    }
    stream_printf(stream, "#include \"cpe/pal/pal_types.h\"\n");
    cpe_dr_generate_h_includes(stream, source, ctx);

    cpe_dr_generate_h_macros(stream, source, ctx);

    stream_printf(
        stream, 
        "\n"
        "#ifdef __cplusplus\n"
        "extern \"C\" {\n"
        "#endif\n");

    cpe_dr_generate_h_metas(stream, source, ctx);

    stream_printf(
        stream, 
        "#ifdef __cplusplus\n"
        "}\n"
        "#endif\n");

    if (with_traits) {
        cpe_dr_generate_h_traits(stream, source, ctx);
    }

    stream_printf(stream, "\n#endif\n");

    return 0;
}
