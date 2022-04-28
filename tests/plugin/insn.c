/*
 * Copyright (C) 2018, Emilio G. Cota <cota@braap.org>
 *
 * License: GNU GPL, version 2 or later.
 *   See the COPYING file in the top-level directory.
 */
#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <glib.h>

#include <qemu-plugin.h>

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

// static uint64_t insn_count;
static bool do_inline;
static bool do_size;
static GArray *sizes;

GHashTable *hash;
uint64_t count[1024];
int ind;
int flag;

// static void vcpu_insn_exec_before(unsigned int cpu_index, void *udata)
// {
//     static uint64_t last_pc;
//     uint64_t this_pc = GPOINTER_TO_UINT(udata);
//     if (this_pc == last_pc) {
//         g_autofree gchar *out = g_strdup_printf("detected repeat execution @ 0x%"
//                                                 PRIx64 "\n", this_pc);
//         qemu_plugin_outs(out);
//     }
//     last_pc = this_pc;
//     insn_count++;
// }

// static void vcpu_tb_exec_before(unsigned int vcpu_index, void *userdata)
// {
//     struct qemu_plugin_tb *tb = userdata;
//     size_t n = qemu_plugin_tb_n_insns(tb);

//     printf("exec on addr %lx (%lu)\n", qemu_plugin_tb_vaddr(tb), n);

//     // for (int i = 0; i < n; i++) {
//     //     struct qemu_plugin_insn *insn = qemu_plugin_tb_get_insn(tb, i);

//     //     char *s = qemu_plugin_insn_disas(insn);

//     //     printf("\t>> %s\n", s);

//     //     free(s);
//     // }
// }

static void vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    size_t n = qemu_plugin_tb_n_insns(tb);
    size_t i;

    if (!flag) {
        if (qemu_plugin_tb_vaddr(tb) == 0xc00029c8) {
            flag = 1;
        } else {
            return;
        }
    }

    printf("trans on addr %lx (%lu)\n", qemu_plugin_tb_vaddr(tb), n);

    // qemu_plugin_register_vcpu_tb_exec_cb(tb, vcpu_tb_exec_before, QEMU_PLUGIN_CB_RW_REGS, tb);

    for (i = 0; i < n; i++) {
        struct qemu_plugin_insn *insn = qemu_plugin_tb_get_insn(tb, i);
        struct {
            GByteArray *data;
        } *ar = (void *)insn;

        char *s = qemu_plugin_insn_disas(insn);

        printf("\t\t %2x %2x %2x %2x\n",
            ar->data->data[0], ar->data->data[1], ar->data->data[2], ar->data->data[3]);

        free(s);

        // if (strncmp(s, ".byte", 5)) {
        //     char *space = strchr(s, ' ');

        //     if (space != NULL) {
        //         *space = '\0';
        //     }
        // }

        // uint64_t *val = g_hash_table_lookup(hash, s);

        // if (val == NULL) {
        //     val = &count[ind++];
        //     *val = 1;
        //     g_hash_table_insert(hash, s, val);
        // }

        // qemu_plugin_register_vcpu_insn_exec_inline(
        //         insn, QEMU_PLUGIN_INLINE_ADD_U64, val, 1);

        // ищем в таблице указатель на
        // если нет, то добавляем

        // освобождаем строку
        // free(s);

        // if (do_inline) {
        //     qemu_plugin_register_vcpu_insn_exec_inline(
        //         insn, QEMU_PLUGIN_INLINE_ADD_U64, &insn_count, 1);
        // } else {
        //     uint64_t vaddr = qemu_plugin_insn_vaddr(insn);
        //     qemu_plugin_register_vcpu_insn_exec_cb(
        //         insn, vcpu_insn_exec_before, QEMU_PLUGIN_CB_NO_REGS,
        //         GUINT_TO_POINTER(vaddr));
        // }

        // if (do_size) {
        //     size_t sz = qemu_plugin_insn_size(insn);
        //     if (sz > sizes->len) {
        //         g_array_set_size(sizes, sz);
        //     }
        //     unsigned long *cnt = &g_array_index(sizes, unsigned long, sz);
        //     (*cnt)++;
        // }
    }
}

static void plugin_exit(qemu_plugin_id_t id, void *p)
{
    // g_autoptr(GString) out = g_string_new(NULL);

    // if (do_size) {
    //     int i;
    //     for (i = 0; i <= sizes->len; i++) {
    //         unsigned long *cnt = &g_array_index(sizes, unsigned long, i);
    //         if (*cnt) {
    //             g_string_append_printf(out,
    //                                    "len %d bytes: %ld insns\n", i, *cnt);
    //         }
    //     }
    // } else {
    //     g_string_append_printf(out, "insns: %" PRIu64 "\n", insn_count);
    // }
    // printf("%s\n", (char *)out->str);

    // GList *list = g_hash_table_get_keys(hash);
    // list = g_list_first(list);

    // while (list->next != NULL) {
    //     uint64_t *val = g_hash_table_lookup(hash, list->data);
    //     printf(">> %10lu \t %s\n", *val, (char *)list->data);
    //     list = list->next;
    // }

}

QEMU_PLUGIN_EXPORT int qemu_plugin_install(qemu_plugin_id_t id,
                                           const qemu_info_t *info,
                                           int argc, char **argv)
{
    for (int i = 0; i < argc; i++) {
        char *opt = argv[i];
        g_autofree char **tokens = g_strsplit(opt, "=", 2);
        if (g_strcmp0(tokens[0], "inline") == 0) {
            if (!qemu_plugin_bool_parse(tokens[0], tokens[1], &do_inline)) {
                fprintf(stderr, "boolean argument parsing failed: %s\n", opt);
                return -1;
            }
        } else if (g_strcmp0(tokens[0], "sizes") == 0) {
            if (!qemu_plugin_bool_parse(tokens[0], tokens[1], &do_size)) {
                fprintf(stderr, "boolean argument parsing failed: %s\n", opt);
                return -1;
            }
        } else {
            fprintf(stderr, "option parsing failed: %s\n", opt);
            return -1;
        }
    }

    if (do_size) {
        sizes = g_array_new(true, true, sizeof(unsigned long));
    }

hash = g_hash_table_new(g_str_hash, g_str_equal);

    qemu_plugin_register_vcpu_tb_trans_cb(id, vcpu_tb_trans);
    qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);
    return 0;
}
