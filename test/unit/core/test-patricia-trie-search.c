/* -*- c-file-style: "gnu"; coding: utf-8 -*- */

#include "test-patricia-trie.h"

void data_lcp_search(void);
void test_lcp_search(gconstpointer data);
void data_prefix_search(void);
void test_prefix_search(gconstpointer data);
void data_suffix_search(void);
void test_suffix_search(gconstpointer data);

static GList *keys;

static sen_hash *hash;

void
setup(void)
{
  setup_trie_common("patricia-trie-search");

  hash = NULL;

  keys = NULL;

  default_encoding = sen_enc_utf8;
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
hash_free(void)
{
  if (context && hash) {
    sen_hash_close(context, hash);
    hash = NULL;
  }
}

void
teardown(void)
{
  keys_free();
  hash_free();
  teardown_trie_common();
}

#define create_hash()                                   \
  hash = sen_hash_create(context, NULL, sizeof(sen_id), \
                         0, SEN_HASH_TINY,              \
                         sen_enc_default)

#define cut_assert_create_hash() do                     \
{                                                       \
  clear_messages();                                     \
  hash_free();                                          \
  create_hash();                                        \
  cut_assert_equal_g_list_string(NULL, messages());     \
  cut_assert(hash);                                     \
} while (0)


static GList *
retrieve_all_keys(void)
{
  sen_id hash_id;
  sen_hash_cursor *cursor;

  keys_free();

  cursor = sen_hash_cursor_open(context, hash,
                                NULL, 0, NULL, 0,
                                SEN_CURSOR_DESCENDING);
  hash_id = sen_hash_cursor_next(context, cursor);
  while (hash_id != SEN_ID_NIL) {
    sen_id *trie_id;
    void *hash_key;
    GString *null_terminated_key;
    gchar key[SEN_PAT_MAX_KEY_SIZE];
    int size;

    sen_hash_cursor_get_key(context, cursor, &hash_key);
    trie_id = hash_key;
    size = sen_pat_get_key(context, trie, *trie_id, key, sizeof(key));
    null_terminated_key = g_string_new_len(key, size);
    keys = g_list_append(keys, g_string_free(null_terminated_key, FALSE));
    hash_id = sen_hash_cursor_next(context, cursor);
  }
  sen_hash_cursor_close(context, cursor);

  return keys;
}

static sen_trie_test_data *lcp_test_data_new(const gchar *expected_key,
                                             const gchar *search_key,
                                             sen_test_set_parameters_func set_parameters,
                                             ...) G_GNUC_NULL_TERMINATED;
static sen_trie_test_data *
lcp_test_data_new(const gchar *expected_key, const gchar *search_key,
                  sen_test_set_parameters_func set_parameters, ...)
{
  sen_trie_test_data *test_data;
  va_list args;

  va_start(args, set_parameters);
  test_data = trie_test_data_newv(NULL, search_key, expected_key,
                                  sen_success, NULL, NULL,
                                  set_parameters, &args);
  va_end(args);

  return test_data;
}

static void
lcp_test_data_free(sen_trie_test_data *test_data)
{
  trie_test_data_free(test_data);
}

void
data_lcp_search(void)
{
  cut_add_data("default - nonexistence",
               lcp_test_data_new(NULL, "カッター", NULL, NULL),
               lcp_test_data_free,
               "default - short",
               lcp_test_data_new(NULL, "セ", NULL, NULL),
               lcp_test_data_free,
               "default - exact",
               lcp_test_data_new("セナ", "セナ", NULL, NULL),
               lcp_test_data_free,
               "default - long",
               lcp_test_data_new("セナセナ", "セナセナセナ", NULL, NULL),
               lcp_test_data_free,
               "sis - nonexistence",
               lcp_test_data_new(NULL, "カッター", set_sis, NULL),
               lcp_test_data_free,
               "sis - short",
               lcp_test_data_new("セ", "セ", set_sis, NULL),
               lcp_test_data_free,
               "sis - exact",
               lcp_test_data_new("セナ", "セナ", set_sis, NULL),
               lcp_test_data_free,
               "sis - long",
               lcp_test_data_new("セナセナ", "セナセナセナ",
                                 set_sis, NULL),
               lcp_test_data_free);
}

void
test_lcp_search(gconstpointer data)
{
  const sen_trie_test_data *test_data = data;
  gchar key[SEN_PAT_MAX_KEY_SIZE];
  const gchar key1[] = "セナ";
  const gchar key2[] = "ナセナセ";
  const gchar key3[] = "Senna";
  const gchar key4[] = "セナ + Ruby";
  const gchar key5[] = "セナセナ";

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();

  cut_assert_lookup_add(key1);
  cut_assert_lookup_add(key2);
  cut_assert_lookup_add(key3);
  cut_assert_lookup_add(key4);
  cut_assert_lookup_add(key5);

  id = sen_pat_lcp_search(context, trie,
                          test_data->search_key,
                          strlen(test_data->search_key));
  if (test_data->expected_key) {
    int size;
    gchar *null_terminated_key;

    sen_test_assert_not_nil(id);
    size = sen_pat_get_key(context, trie, id, key, sizeof(key));
    null_terminated_key = g_string_free(g_string_new_len(key, size), FALSE);
    cut_assert_equal_string(test_data->expected_key, null_terminated_key);
  } else {
    sen_test_assert_nil(id);
  }
}

static sen_trie_test_data *xfix_test_data_new(sen_rc expected_rc,
                                              GList *expected_keys,
                                              gchar *search_key,
                                              sen_test_set_parameters_func set_parameters,
                                              ...) G_GNUC_NULL_TERMINATED;
static sen_trie_test_data *
xfix_test_data_new(sen_rc expected_rc, GList *expected_keys, gchar *search_key,
                   sen_test_set_parameters_func set_parameters, ...)
{
  sen_trie_test_data *test_data;
  va_list args;

  va_start(args, set_parameters);
  test_data = trie_test_data_newv(NULL, search_key, NULL, expected_rc,
                                  expected_keys, NULL,
                                  set_parameters, &args);
  va_end(args);

  return test_data;
}

static void
xfix_test_data_free(sen_trie_test_data *test_data)
{
  trie_test_data_free(test_data);
}

void
data_prefix_search(void)
{
  cut_add_data("default - nonexistence",
               xfix_test_data_new(sen_end_of_data, NULL, "カッター", NULL, NULL),
               xfix_test_data_free,
               "default - short",
               xfix_test_data_new(sen_success,
                                  gcut_list_string_new("セナ", "セナ + Ruby",
                                                       "セナセナ", NULL),
                                  "セ", NULL, NULL),
               xfix_test_data_free,
               "default - exact",
               xfix_test_data_new(sen_success,
                                  gcut_list_string_new("セナ", "セナ + Ruby",
                                                       "セナセナ", NULL),
                                  "セナ", NULL, NULL),
               xfix_test_data_free,
               "default - long",
               xfix_test_data_new(sen_end_of_data, NULL, "セナセナセナ",
                                  NULL, NULL),
               xfix_test_data_free,
               "sis - nonexistence",
               xfix_test_data_new(sen_end_of_data, NULL, "カッター",
                                  set_sis, NULL),
               xfix_test_data_free,
               "sis - short",
               xfix_test_data_new(sen_success,
                                  gcut_list_string_new("セ", "セナ",
                                                       "セナ + Ruby",
                                                       "セナセ", "セナセナ",
                                                       NULL),
                                  "セ", set_sis, NULL),
               xfix_test_data_free,
               "sis - exact",
               xfix_test_data_new(sen_success,
                                  gcut_list_string_new("セナ",
                                                       "セナ + Ruby",
                                                       "セナセ", "セナセナ",
                                                       NULL),
                                  "セナ", set_sis, NULL),
               xfix_test_data_free,
               "sis - long",
               xfix_test_data_new(sen_end_of_data, NULL, "セナセナセナ",
                                  set_sis, NULL),
               xfix_test_data_free);
}

void
test_prefix_search(gconstpointer data)
{
  const sen_trie_test_data *test_data = data;
  const gchar key1[] = "セナ";
  const gchar key2[] = "ナセナセ";
  const gchar key3[] = "Senna";
  const gchar key4[] = "セナ + Ruby";
  const gchar key5[] = "セナセナ";

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();

  cut_assert_lookup_add(key1);
  cut_assert_lookup_add(key2);
  cut_assert_lookup_add(key3);
  cut_assert_lookup_add(key4);
  cut_assert_lookup_add(key5);

  cut_assert_create_hash();
  sen_test_assert_equal_rc(test_data->expected_rc,
                           sen_pat_prefix_search(context, trie,
                                                 test_data->search_key,
                                                 strlen(test_data->search_key),
                                                 hash));
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys());
}

void
data_suffix_search(void)
{
  cut_add_data("default - nonexistence",
               xfix_test_data_new(sen_end_of_data, NULL, "カッター", NULL, NULL),
               xfix_test_data_free,
               "default - short",
               xfix_test_data_new(sen_end_of_data, NULL, "ナ", NULL, NULL),
               xfix_test_data_free,
               "default - exact",
               xfix_test_data_new(sen_success,
                                  gcut_list_string_new("セナ", NULL),
                                  "セナ", NULL, NULL),
               xfix_test_data_free,
               "default - long",
               xfix_test_data_new(sen_end_of_data, NULL, "セナセナセナ",
                                  NULL, NULL),
               xfix_test_data_free,
               "sis - nonexistence",
               xfix_test_data_new(sen_end_of_data, NULL, "カッター",
                                  set_sis, NULL),
               xfix_test_data_free,
               "sis - short",
               xfix_test_data_new(sen_success,
                                  gcut_list_string_new("セナセナ",
                                                       "ナセナ",
                                                       "セナ",
                                                       "ナ",
                                                       NULL),
                                  "ナ", set_sis, NULL),
               xfix_test_data_free,
               "sis - exact",
               xfix_test_data_new(sen_success,
                                  gcut_list_string_new("セナセナ",
                                                       "ナセナ",
                                                       "セナ",
                                                       NULL),
                                  "セナ", set_sis, NULL),
               xfix_test_data_free,
               "sis - long",
               xfix_test_data_new(sen_end_of_data, NULL, "セナセナセナ",
                                  set_sis, NULL),
               xfix_test_data_free);
}

void
test_suffix_search(gconstpointer data)
{
  const sen_trie_test_data *test_data = data;
  const gchar key1[] = "セナ";
  const gchar key2[] = "ナセナセ";
  const gchar key3[] = "Senna";
  const gchar key4[] = "セナ + Ruby";
  const gchar key5[] = "セナセナ";

  trie_test_data_set_parameters(test_data);

  cut_assert_create_trie();

  cut_assert_lookup_add(key1);
  cut_assert_lookup_add(key2);
  cut_assert_lookup_add(key3);
  cut_assert_lookup_add(key4);
  cut_assert_lookup_add(key5);

  cut_assert_create_hash();
  sen_test_assert_equal_rc(test_data->expected_rc,
                           sen_pat_suffix_search(context, trie,
                                                 test_data->search_key,
                                                 strlen(test_data->search_key),
                                                 hash));
  gcut_assert_equal_list_string(test_data->expected_strings,
                                retrieve_all_keys());
}