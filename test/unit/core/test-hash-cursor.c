/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "test-hash.h"

void data_next_with_no_entry(void);
void test_next_with_no_entry(gconstpointer data);
void data_next_with_one_entry(void);
void test_next_with_one_entry(gconstpointer data);
void data_next_with_multi_entries(void);
void test_next_with_multi_entries(gconstpointer data);
void data_value(void);
void test_value(gconstpointer data);
void data_delete(void);
void test_delete(gconstpointer data);

static GList *keys;
static GList *keys_and_values;

void
startup(void)
{
  startup_hash_common();
}

void
shutdown(void)
{
  shutdown_hash_common();
}

void
setup(void)
{
  setup_hash_common("hash-cursor");

  keys = NULL;
  keys_and_values = NULL;

  sen_test_hash_factory_set_flags(factory, SEN_OBJ_KEY_VAR_SIZE);
  sen_test_hash_factory_set_key_size(factory, SEN_HASH_MAX_KEY_SIZE);

  sample_value = NULL;
}

static void
keys_free(void)
{
  if (keys) {
    gcut_list_string_free(keys);
    keys = NULL;
  }
}

static void
keys_and_values_free(void)
{
  if (keys_and_values) {
    gcut_list_string_free(keys_and_values);
    keys_and_values = NULL;
  }
}

void
teardown(void)
{
  keys_free();
  keys_and_values_free();
  teardown_hash_common();
}

static GList *
retrieve_all_keys(void)
{
  sen_id id;

  keys_free();

  id = sen_hash_cursor_next(context, cursor);
  while (id != SEN_ID_NIL) {
    void *key;
    GString *null_terminated_key;
    int size;

    size = sen_hash_cursor_get_key(context, cursor, &key);
    null_terminated_key = g_string_new_len(key, size);
    keys = g_list_append(keys, g_string_free(null_terminated_key, FALSE));
    id = sen_hash_cursor_next(context, cursor);
  }

  return keys;
}

static GList *
retrieve_all_keys_and_values(void)
{
  sen_id id;

  keys_and_values_free();

  id = sen_hash_cursor_next(context, cursor);
  while (id != SEN_ID_NIL) {
    int length;
    void *key, *value;
    GString *null_terminated_key, *null_terminated_value;

    length = sen_hash_cursor_get_key(context, cursor, &key);
    null_terminated_key = g_string_new_len(key, length);
    keys_and_values = g_list_append(keys_and_values,
                                    g_string_free(null_terminated_key, FALSE));

    length = sen_hash_cursor_get_value(context, cursor, &value);
    null_terminated_value = g_string_new_len(value, length);
    keys_and_values = g_list_append(keys_and_values,
                                    g_string_free(null_terminated_value, FALSE));

    id = sen_hash_cursor_next(context, cursor);
  }

  return keys_and_values;
}

typedef struct cursor_test_data
{
  GList *expected_strings;
  GList *set_parameters_funcs;
} cursor_test_data;

static cursor_test_data *test_data_new(GList *expected_strings,
                                       sen_test_set_parameters_func set_parameters,
                                       ...) G_GNUC_NULL_TERMINATED;
static cursor_test_data *
test_data_new(GList *expected_strings,
              sen_test_set_parameters_func set_parameters,
              ...)
{
  cursor_test_data *test_data;
  va_list args;

  test_data = g_new0(cursor_test_data, 1);
  test_data->expected_strings = expected_strings;

  va_start(args, set_parameters);
  test_data->set_parameters_funcs = NULL;
  while (set_parameters) {
    test_data->set_parameters_funcs =
      g_list_append(test_data->set_parameters_funcs, set_parameters);
    set_parameters = va_arg(args, sen_test_set_parameters_func);
  }
  va_end(args);

  return test_data;
}

static void
test_data_set_parameters(const cursor_test_data *test_data)
{
  GList *node;

  for (node = test_data->set_parameters_funcs;
       node;
       node = g_list_next(node)) {
    sen_test_set_parameters_func set_parameters_func = node->data;
    set_parameters_func();
  }
}

static void
test_data_free(cursor_test_data *test_data)
{
  gcut_list_string_free(test_data->expected_strings);
  g_list_free(test_data->set_parameters_funcs);
  g_free(test_data);
}

static void
set_ascending(void)
{
  sen_test_hash_factory_add_cursor_flags(factory, SEN_CURSOR_ASCENDING);
}

static void
set_descending(void)
{
  sen_test_hash_factory_add_cursor_flags(factory, SEN_CURSOR_DESCENDING);
}

static void
add_next_with_no_entry_data(const gchar *additional_label,
                            sen_test_set_parameters_func set_parameters_funcs)
{
  cut_add_data(cut_take_printf("ascending%s", additional_label),
               test_data_new(NULL, set_ascending, set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("descending%s", additional_label),
               test_data_new(NULL, set_descending, set_parameters_funcs, NULL),
               test_data_free);
}

void
data_next_with_no_entry(void)
{
  add_next_with_no_entry_data("", NULL);
  add_next_with_no_entry_data(" - tiny", set_tiny_flags);
}

void
test_next_with_no_entry(gconstpointer data)
{
  const cursor_test_data *test_data = data;

  test_data_set_parameters(test_data);

  cut_assert_create_hash();
  cut_assert_open_cursor();
  gcut_assert_equal_list_string(NULL, retrieve_all_keys());
}

static void
add_next_with_one_entry_data(const gchar *additional_label,
                             sen_test_set_parameters_func set_parameters_funcs)
{
  cut_add_data(cut_take_printf("ascending%s", additional_label),
               test_data_new(gcut_list_string_new("セナ", NULL),
                             set_ascending, set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("descending%s", additional_label),
               test_data_new(gcut_list_string_new("セナ", NULL),
                             set_descending, set_parameters_funcs, NULL),
               test_data_free);
}

void
data_next_with_one_entry(void)
{
  add_next_with_one_entry_data("", NULL);
  add_next_with_one_entry_data(" - tiny", set_tiny_flags);
}

void
test_next_with_one_entry(gconstpointer data)
{
  const cursor_test_data *test_data = data;
  const gchar key[] = "セナ";

  test_data_set_parameters(test_data);

  cut_assert_create_hash();

  cut_assert_lookup_add(key);

  cut_assert_open_cursor();
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys());
}

static void
set_max(void)
{
  const gchar max[] = "セナ + Ruby";
  sen_test_hash_factory_set_cursor_max(factory, max, strlen(max));
}

static void
set_max_low(void)
{
  const gchar max[] = "ナセナセ";
  sen_test_hash_factory_set_cursor_max(factory, max, strlen(max));
}

static void
set_max_nonexistence(void)
{
  const gchar max[] = "Hyper Estraier";
  sen_test_hash_factory_set_cursor_max(factory, max, strlen(max));
}

static void
set_min(void)
{
  const gchar min[] = "ナセナセ";
  sen_test_hash_factory_set_cursor_min(factory, min, strlen(min));
}

static void
set_min_high(void)
{
  const gchar min[] = "セナ + Ruby";
  sen_test_hash_factory_set_cursor_min(factory, min, strlen(min));
}

static void
set_min_nonexistence(void)
{
  const gchar min[] = "Hyper Estraier";
  sen_test_hash_factory_set_cursor_min(factory, min, strlen(min));
}

static void
set_gt(void)
{
  sen_test_hash_factory_add_cursor_flags(factory, SEN_CURSOR_GT);
}

static void
set_lt(void)
{
  sen_test_hash_factory_add_cursor_flags(factory, SEN_CURSOR_LT);
}

static void
add_data_ascending(const gchar *additional_label,
                   sen_test_set_parameters_func set_parameters_funcs)
{
  cut_add_data(cut_take_printf("ascending%s", additional_label),
               test_data_new(gcut_list_string_new("セナ",
                                                  "ナセナセ",
                                                  "Senna",
                                                  "セナ + Ruby",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_parameters_funcs, NULL),
               test_data_free);

  cut_add_data(cut_take_printf("ascending - max%s", additional_label),
               test_data_new(gcut_list_string_new("セナ",
                                                  "ナセナセ",
                                                  "Senna",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_ascending, set_max, set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("ascending - max - gt%s", additional_label),
               test_data_new(gcut_list_string_new("セナ",
                                                  "ナセナセ",
                                                  "Senna",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_ascending, set_max, set_gt,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("ascending - max - lt%s", additional_label),
               test_data_new(gcut_list_string_new("セナ",
                                                  "ナセナセ",
                                                  "Senna",
                                                  NULL),
                             set_ascending, set_max, set_lt,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("ascending - max - gt - lt%s", additional_label),
               test_data_new(gcut_list_string_new("セナ",
                                                  "ナセナセ",
                                                  "Senna",
                                                  NULL),
                             set_ascending, set_max, set_gt, set_lt,
                             set_parameters_funcs, NULL),
               test_data_free);

  cut_add_data(cut_take_printf("ascending - min%s", additional_label),
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "Senna",
                                                  "セナ + Ruby",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_min,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("ascending - min - gt%s", additional_label),
               test_data_new(gcut_list_string_new("Senna",
                                                  "セナ + Ruby",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_min, set_gt,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("ascending - min - lt%s", additional_label),
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "Senna",
                                                  "セナ + Ruby",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_min, set_lt,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("ascending - min - gt - lt%s", additional_label),
               test_data_new(gcut_list_string_new("Senna",
                                                  "セナ + Ruby",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_min, set_gt, set_lt,
                             set_parameters_funcs, NULL),
               test_data_free);

  cut_add_data(cut_take_printf("ascending - max - min%s", additional_label),
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "Senna",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_ascending, set_max, set_min,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("ascending - max - min - gt%s", additional_label),
               test_data_new(gcut_list_string_new("Senna",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_ascending, set_max, set_min, set_gt,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("ascending - max - min - lt%s", additional_label),
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "Senna",
                                                  NULL),
                             set_ascending, set_max, set_min, set_lt,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("ascending - max - min - gt - lt%s",
                               additional_label),
               test_data_new(gcut_list_string_new("Senna",
                                                  NULL),
                             set_ascending, set_max, set_min, set_gt,
                             set_lt, set_parameters_funcs, NULL),
               test_data_free);

  cut_add_data(cut_take_printf("ascending - high-min%s", additional_label),
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "セナセナ",
                                                  NULL),
                             set_ascending, set_min_high, set_parameters_funcs,
                             NULL),
               test_data_free,
               cut_take_printf("ascending - low-max%s", additional_label),
               test_data_new(gcut_list_string_new("セナ",
                                                  "ナセナセ",
                                                  NULL),
                             set_ascending, set_max_low, set_parameters_funcs,
                             NULL),
               test_data_free,
               cut_take_printf("ascending - high-min - low-max%s",
                               additional_label),
               test_data_new(NULL,
                             set_ascending, set_min_high, set_max_low,
                             set_parameters_funcs, NULL),
               test_data_free);

  cut_add_data(cut_take_printf("ascending - nonexistence-min%s",
                               additional_label),
               test_data_new(NULL,
                             set_ascending, set_min_nonexistence,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("ascending - nonexistence-max%s",
                               additional_label),
               test_data_new(NULL,
                             set_ascending, set_max_nonexistence,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("ascending - nonexistence-min - "
                               "nonexistence-max%s", additional_label),
               test_data_new(NULL,
                             set_ascending, set_min_nonexistence,
                             set_max_nonexistence, set_parameters_funcs, NULL),
               test_data_free);
}

static void
add_data_descending(const gchar *additional_label,
                    sen_test_set_parameters_func set_parameters_funcs)
{
  cut_add_data(cut_take_printf("descending%s", additional_label),
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナ + Ruby",
                                                  "Senna",
                                                  "ナセナセ",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_parameters_funcs, NULL),
               test_data_free);

  cut_add_data(cut_take_printf("descending - max%s", additional_label),
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "Senna",
                                                  "ナセナセ",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_max, set_parameters_funcs,
                             NULL),
               test_data_free,
               cut_take_printf("descending - max - gt%s", additional_label),
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "Senna",
                                                  "ナセナセ",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_max, set_gt,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("descending - max - lt%s", additional_label),
               test_data_new(gcut_list_string_new("Senna",
                                                  "ナセナセ",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_max, set_lt,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("descending - max - gt - lt%s", additional_label),
               test_data_new(gcut_list_string_new("Senna",
                                                  "ナセナセ",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_max, set_gt, set_lt,
                             set_parameters_funcs, NULL),
               test_data_free);

  cut_add_data(cut_take_printf("descending - min%s", additional_label),
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナ + Ruby",
                                                  "Senna",
                                                  "ナセナセ",
                                                  NULL),
                             set_descending, set_min, set_parameters_funcs,
                             NULL),
               test_data_free,
               cut_take_printf("descending - min - gt%s", additional_label),
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナ + Ruby",
                                                  "Senna",
                                                  NULL),
                             set_descending, set_min, set_gt,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("descending - min - lt%s", additional_label),
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナ + Ruby",
                                                  "Senna",
                                                  "ナセナセ",
                                                  NULL),
                             set_descending, set_min, set_lt,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("descending - min - gt - lt%s", additional_label),
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナ + Ruby",
                                                  "Senna",
                                                  NULL),
                             set_descending, set_min, set_gt, set_lt,
                             set_parameters_funcs, NULL),
               test_data_free);

  cut_add_data(cut_take_printf("descending - max - min%s", additional_label),
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "Senna",
                                                  "ナセナセ",
                                                  NULL),
                             set_descending, set_max, set_min,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("descending - max - min - gt%s",
                               additional_label),
               test_data_new(gcut_list_string_new("セナ + Ruby",
                                                  "Senna",
                                                  NULL),
                             set_descending, set_max, set_min, set_gt,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("descending - max - min - lt%s",
                               additional_label),
               test_data_new(gcut_list_string_new("Senna",
                                                  "ナセナセ",
                                                  NULL),
                             set_descending, set_max, set_min, set_lt,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("descending - max - min - gt - lt%s",
                               additional_label),
               test_data_new(gcut_list_string_new("Senna",
                                                  NULL),
                             set_descending, set_max, set_min, set_gt,
                             set_lt, set_parameters_funcs, NULL),
               test_data_free);

  cut_add_data(cut_take_printf("descending - high-min%s",
                               additional_label),
               test_data_new(gcut_list_string_new("セナセナ",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_descending, set_min_high,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("descending - low-max%s",
                               additional_label),
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "セナ",
                                                  NULL),
                             set_descending, set_max_low,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("descending - high-min - low-max%s",
                               additional_label),
               test_data_new(NULL,
                             set_descending, set_min_high, set_max_low,
                             set_parameters_funcs, NULL),
               test_data_free);

  cut_add_data(cut_take_printf("descending - nonexistence-min%s",
                               additional_label),
               test_data_new(NULL,
                             set_descending, set_min_nonexistence,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("descending - nonexistence-max%s",
                               additional_label),
               test_data_new(NULL,
                             set_descending, set_max_nonexistence,
                             set_parameters_funcs, NULL),
               test_data_free,
               cut_take_printf("descending - nonexistence-min - "
                               "nonexistence-max%s", additional_label),
               test_data_new(NULL,
                             set_descending, set_min_nonexistence,
                             set_max_nonexistence, set_parameters_funcs, NULL),
               test_data_free);
}

void
data_next_with_multi_entries(void)
{
  add_data_ascending("", NULL);
  add_data_descending("", NULL);

  add_data_ascending(" - tiny", set_tiny_flags);
  add_data_descending(" - tiny", set_tiny_flags);
}

void
test_next_with_multi_entries(gconstpointer data)
{
  const cursor_test_data *test_data = data;
  const gchar key1[] = "セナ";
  const gchar key2[] = "ナセナセ";
  const gchar key3[] = "Senna";
  const gchar key4[] = "セナ + Ruby";
  const gchar key5[] = "セナセナ";

  test_data_set_parameters(test_data);

  cut_assert_create_hash();

  cut_assert_lookup_add(key1);
  cut_assert_lookup_add(key2);
  cut_assert_lookup_add(key3);
  cut_assert_lookup_add(key4);
  cut_assert_lookup_add(key5);

  cut_assert_open_cursor();
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys());
}

static void
set_value_size(void)
{
  sen_test_hash_factory_set_value_size(factory, strlen("上書きされた値 -"));
}

static void
add_value_data(const gchar *additional_label,
               sen_test_set_parameters_func set_parameters_funcs)
{
  cut_add_data(cut_take_printf("default%s", additional_label),
               test_data_new(gcut_list_string_new("セナ",
                                                  "",
                                                  "ナセナセ",
                                                  "VALUE2",
                                                  "Senna",
                                                  "",
                                                  "セナ + Ruby",
                                                  "",
                                                  /* should be set two values */
                                                  "セナセナ",
                                                  "上書きされた値 -",
                                                  NULL),
                             set_ascending, set_value_size,
                             set_parameters_funcs, NULL),
               test_data_free);
}

void
data_value(void)
{
  add_value_data("", NULL);
  add_value_data(" - tiny", set_tiny_flags);
}

void
test_value(gconstpointer data)
{
  const cursor_test_data *test_data = data;
  const gchar key1[] = "セナ";
  const gchar key2[] = "ナセナセ";
  const gchar key3[] = "Senna";
  const gchar key4[] = "セナ + Ruby";
  const gchar key5[] = "セナセナ";
  gchar value2[] = "VALUE2";
  gchar value4_1[] = "Senna";
  gchar value4_2[] = "るびい";
  gchar value5_1[] = "上書きされる値 - overridden value";
  gchar value5_2[] = "上書きされた値 - override value";

  test_data_set_parameters(test_data);

  cut_assert_create_hash();

  cut_assert_lookup_add(key1);
  cut_assert_lookup_add(key2);
  cut_assert_lookup_add(key3);
  cut_assert_lookup_add(key4);
  cut_assert_lookup_add(key5);

  cut_assert_open_cursor();
  while (sen_hash_cursor_next(context, cursor) != SEN_ID_NIL) {
    void *key;
    gchar *null_terminated_key;
    int size;

    size = sen_hash_cursor_get_key(context, cursor, &key);
    null_terminated_key = g_string_free(g_string_new_len(key, size), FALSE);
    if (g_str_equal(null_terminated_key, key2)) {
      sen_hash_cursor_set_value(context, cursor, value2, SEN_OBJ_SET);
    } else if (g_str_equal(null_terminated_key, key4)) {
      sen_hash_cursor_set_value(context, cursor, value4_1, SEN_OBJ_INCR);
      sen_hash_cursor_set_value(context, cursor, value4_2, SEN_OBJ_INCR);
    } else if (g_str_equal(null_terminated_key, key5)) {
      sen_hash_cursor_set_value(context, cursor, value5_1, SEN_OBJ_SET);
      sen_hash_cursor_set_value(context, cursor, value5_2, SEN_OBJ_SET);
    }
  }

  cut_assert_open_cursor();
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys_and_values());
}

static void
add_delete_data(const gchar *additional_label,
                sen_test_set_parameters_func set_parameters_funcs)
{
  cut_add_data(cut_take_printf("default%s", additional_label),
               test_data_new(gcut_list_string_new("ナセナセ",
                                                  "Senna",
                                                  "セナ + Ruby",
                                                  NULL),
                             set_ascending, set_parameters_funcs, NULL),
               test_data_free);
}

void
data_delete(void)
{
  add_delete_data("", NULL);

  add_delete_data(" - tiny", set_tiny_flags);
}

void
test_delete(gconstpointer data)
{
  const cursor_test_data *test_data = data;
  const gchar key1[] = "セナ";
  const gchar key2[] = "ナセナセ";
  const gchar key3[] = "Senna";
  const gchar key4[] = "セナ + Ruby";
  const gchar key5[] = "セナセナ";

  test_data_set_parameters(test_data);

  cut_assert_create_hash();

  cut_assert_lookup_add(key1);
  cut_assert_lookup_add(key2);
  cut_assert_lookup_add(key3);
  cut_assert_lookup_add(key4);
  cut_assert_lookup_add(key5);

  cut_assert_open_cursor();
  while (sen_hash_cursor_next(context, cursor) != SEN_ID_NIL) {
    void *key;
    gchar *null_terminated_key;
    int size;

    size = sen_hash_cursor_get_key(context, cursor, &key);
    null_terminated_key = g_string_free(g_string_new_len(key, size), FALSE);
    if (g_str_equal(null_terminated_key, key1) ||
        g_str_equal(null_terminated_key, key5)) {
      sen_hash_cursor_delete(context, cursor, NULL);
    }
  }

  cut_assert_open_cursor();
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys());
}