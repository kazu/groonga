/* -*- c-basic-offset: 2; coding: utf-8 -*- */

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/sen-assertions.h"

void test_simple_exec(void);
void test_simple_exec_euc_jp(void);
void test_simple_exec_sjis(void);
void test_simple_exec_utf8(void);
void test_exec_with_invalid_argument(void);
void test_exec_with_normalize(void);
void test_exec_with_many_results(void);
void test_customized_tag(void);
void test_multi_conditions(void);
void test_invalid_result_index(void);
void test_html_mapping(void);
void test_html_mapping_escape(void);
void test_close_with_null(void);
void test_open_with_invalid_max_results(void);
void test_add_cond_with_invalid_argument(void);
void test_add_cond_with_too_large_keyword(void);
void test_add_cond_with_copy_tag_flag(void);

static sen_snip *snip;
static gchar *keyword;
static gchar *result;

static sen_encoding default_encoding;
static int default_flags;
static unsigned int default_width;
static unsigned int default_max_results;
static gchar *default_open_tag;
static unsigned int default_open_tag_len;
static gchar *default_close_tag;
static unsigned int default_close_tag_len;
static sen_snip_mapping *default_mapping;

static const gchar text[] =
  "Groonga is an embeddable fulltext search engine, which you can use in\n"
  "conjunction with various scripting languages and databases. Groonga is\n"
  "an inverted index based engine, & combines the best of n-gram\n"
  "indexing and word indexing to achieve fast, precise searches. While\n"
  "groonga codebase is rather compact it is scalable enough to handle large\n"
  "amounts of data and queries.";
static const gchar text_ja_utf8[] =
  "Groongaは組み込み型の全文検索エンジンです。DBMSやスクリプト言語処理系等に\n"
  "組み込むことによって、その全文検索機能を強化することができます。n-gram\n"
  "インデックスと単語インデックスの特徴を兼ね備えた、高速かつ高精度な転置\n"
  "インデックスタイプのエンジンです。コンパクトな実装ですが、大規模な文書\n"
  "量と検索要求を処理できるように設計されています。また、純粋なn-gramイン\n"
  "デックスの作成も可能です。";
static gchar *text_ja_euc;
static gchar *text_ja_sjis;
static const gchar html_text[] =
  "<div class=\"day\">\n"
  "  <h2 id=\"Requirements\">Requirements</h2>\n"
  "  <div class=\"body\">\n"
  "    <div class=\"section\">\n"
  "      <ul>\n"
  "        <li>OS</li>\n"
  "        <ul>\n"
  "          <li>Linux, FreeBSD, MacOS X</li>\n"
  "        </ul>\n"
  "        <li>Requirements</li>\n"
  "        <ul>\n"
  "          <li>MeCab-0.80 or later "
  /*          */"(for japanese-word indexing. normally not required.)</li>\n"
  "          <li>Ruby 1.8.1 or later (for Ruby binding.)"
  /*          */"<a class=\"external\" href=\"http://www.ruby-lang.org/\">"
  /*            */"http://www.ruby-lang.org/"
  /*          */"</a>"
  /*      */"</li>\n"
  "        </ul>\n"
  "      </ul>\n"
  "    </div><!--section-->\n"
  "  </div><!--body-->\n"
  "</div><!--day-->\n";

static gchar *
convert(const gchar *string, const gchar *from, const gchar *to, GError **error)
{
  return g_convert(string, -1, to, from, NULL, NULL, error);
}

static gchar *
utf8_to_euc_jp(const gchar *utf8, GError **error)
{
  return convert(utf8, "utf-8", "eucJP", error);
}

static gchar *
euc_jp_to_utf8(const gchar *euc_jp, GError **error)
{
  return convert(euc_jp, "eucJP", "utf-8", error);
}

static gchar *
utf8_to_sjis(const gchar *utf8, GError **error)
{
  return convert(utf8, "utf-8", "CP932", error);
}

static gchar *
sjis_to_utf8(const gchar *sjis, GError **error)
{
  return convert(sjis, "CP932", "utf-8", error);
}

static const gchar *
take_euc_jp_to_utf8(const gchar *euc_jp)
{
  gchar *utf8;
  GError *error = NULL;

  utf8 = euc_jp_to_utf8(euc_jp, &error);
  cut_assert_g_error(error);
  return cut_take_string(utf8);
}

static const gchar *
take_sjis_to_utf8(const gchar *sjis)
{
  gchar *utf8;
  GError *error = NULL;

  utf8 = sjis_to_utf8(sjis, &error);
  cut_assert_g_error(error);
  return cut_take_string(utf8);
}

#define cut_check_g_error(error) do                                     \
{                                                                       \
  GError *_error = (error);                                             \
  if (_error) {                                                         \
    const gchar *message;                                               \
    message = cut_take_printf("%s: %d: %s",                             \
                              g_quark_to_string(_error->domain),        \
                              _error->code,                             \
                              _error->message);                         \
    g_error_free(_error);                                               \
    cut_error("%s", message);                                           \
  }                                                                     \
} while (0)

void
startup(void)
{
  GError *error = NULL;

  text_ja_euc = NULL;
  text_ja_sjis = NULL;

  text_ja_euc = utf8_to_euc_jp(text_ja_utf8, &error);
  cut_check_g_error(error);

  text_ja_sjis = utf8_to_sjis(text_ja_utf8, &error);
  cut_check_g_error(error);
}

void
shutdown(void)
{
  if (text_ja_euc) {
    g_free(text_ja_euc);
  }

  if (text_ja_sjis) {
    g_free(text_ja_sjis);
  }
}

void
setup(void)
{
  snip = NULL;
  keyword = NULL;
  result = NULL;

  default_encoding = sen_enc_default;
  default_flags = 0;
  default_width = 100;
  default_max_results = 10;
  default_open_tag = g_strdup("[[");
  default_open_tag_len = strlen(default_open_tag);
  default_close_tag = g_strdup("]]");
  default_close_tag_len = strlen(default_close_tag);
  default_mapping = NULL;
}

void
teardown(void)
{
  if (snip) {
    sen_snip_close(snip);
  }
  if (keyword) {
    g_free(keyword);
  }
  if (result) {
    g_free(result);
  }

  if (default_open_tag) {
    g_free(default_open_tag);
  }
  if (default_close_tag) {
    g_free(default_close_tag);
  }
}

static sen_snip *
open_snip(void)
{
  if (snip) {
    sen_snip_close(snip);
  }
  snip = sen_snip_open(default_encoding, default_flags,
                       default_width,  default_max_results,
                       default_open_tag, default_open_tag_len,
                       default_close_tag, default_close_tag_len,
                       default_mapping);
  return snip;
}

#define cut_assert_open_snip() do                                       \
{                                                                       \
  open_snip();                                                          \
  cut_assert(snip);                                                     \
} while (0)

void
test_simple_exec(void)
{
  unsigned int n_results;
  unsigned int max_tagged_len;
  unsigned int result_len;
  const gchar keyword[] = "Groonga";

  cut_assert_open_snip();
  sen_test_assert(sen_snip_add_cond(snip, keyword, strlen(keyword),
                                   NULL, 0, NULL, 0));

  sen_test_assert(sen_snip_exec(snip, text, strlen(text),
                               &n_results, &max_tagged_len));
  cut_assert_equal_uint(2, n_results);
  cut_assert_equal_uint(105, max_tagged_len);
  result = g_new(gchar, max_tagged_len);

  sen_test_assert(sen_snip_get_result(snip, 0, result, &result_len));
  cut_assert_equal_string("[[Groonga]] is an embeddable fulltext search engine, "
                          "which you can use in\n"
                          "conjunction with various scripti",
                          result);
  cut_assert_equal_uint(104, result_len);

  sen_test_assert(sen_snip_get_result(snip, 1, result, &result_len));
  cut_assert_equal_string("ng languages and databases. [[Groonga]] is\n"
                          "an inverted index based engine, & combines "
                          "the best of n-gram\ni",
                          result);
  cut_assert_equal_uint(104, result_len);
}

void
test_simple_exec_euc_jp(void)
{
  GError *error = NULL;
  unsigned int n_results;
  unsigned int max_tagged_len;
  unsigned int result_len;

  keyword = utf8_to_euc_jp("検索", &error);
  cut_assert_g_error(error);

  default_encoding = sen_enc_euc_jp;

  cut_assert_open_snip();
  sen_test_assert(sen_snip_add_cond(snip, keyword, strlen(keyword),
                                   NULL, 0, NULL, 0));

  sen_test_assert(sen_snip_exec(snip,
                               text_ja_euc, strlen(text_ja_euc),
                               &n_results, &max_tagged_len));
  cut_assert_equal_uint(2, n_results);
  cut_assert_equal_uint(108, max_tagged_len);
  result = g_new(gchar, max_tagged_len);

  sen_test_assert(sen_snip_get_result(snip, 0, result, &result_len));
  cut_assert_equal_string("型の全文[[検索]]エンジンです。"
                          "DBMSやスクリプト言語処理系等に\n"
                          "組み込むことによって、その全文[[検索]]機能を強",
                          take_euc_jp_to_utf8(result));
  cut_assert_equal_uint(107, result_len);

  sen_test_assert(sen_snip_get_result(snip, 1, result, &result_len));
  cut_assert_equal_string("です。コンパクトな実装ですが、大規模な文書\n"
                          "量と[[検索]]要求を処理できるように設計されて"
                          "います。また、純",
                          take_euc_jp_to_utf8(result));
  cut_assert_equal_uint(103, result_len);
}

void
test_simple_exec_sjis(void)
{
  GError *error = NULL;
  unsigned int n_results;
  unsigned int max_tagged_len;
  unsigned int result_len;

  keyword = utf8_to_sjis("処理", &error);
  cut_assert_g_error(error);

  default_encoding = sen_enc_sjis;

  cut_assert_open_snip();
  sen_test_assert(sen_snip_add_cond(snip, keyword, strlen(keyword),
                                   NULL, 0, NULL, 0));

  sen_test_assert(sen_snip_exec(snip,
                               text_ja_sjis, strlen(text_ja_sjis),
                               &n_results, &max_tagged_len));
  cut_assert_equal_uint(2, n_results);
  cut_assert_equal_uint(104, max_tagged_len);
  result = g_new(gchar, max_tagged_len);

  sen_test_assert(sen_snip_get_result(snip, 0, result, &result_len));
  cut_assert_equal_string("み型の全文検索エンジンです。"
                          "DBMSやスクリプト言語[[処理]]系等に\n"
                          "組み込むことによって、その全文検索機能を",
                          take_sjis_to_utf8(result));
  cut_assert_equal_uint(103, result_len);

  sen_test_assert(sen_snip_get_result(snip, 1, result, &result_len));
  cut_assert_equal_string("パクトな実装ですが、大規模な文書\n"
                          "量と検索要求を[[処理]]できるように設計"
                          "されています。また、純粋なn-gram",
                          take_sjis_to_utf8(result));
  cut_assert_equal_uint(103, result_len);
}

void
test_simple_exec_utf8(void)
{
  unsigned int n_results;
  unsigned int max_tagged_len;
  unsigned int result_len;
  const gchar keyword[] = "エンジン";

  default_encoding = sen_enc_utf8;

  cut_assert_open_snip();
  sen_test_assert(sen_snip_add_cond(snip, keyword, strlen(keyword),
                                   NULL, 0, NULL, 0));

  sen_test_assert(sen_snip_exec(snip,
                               text_ja_utf8, strlen(text_ja_utf8),
                               &n_results, &max_tagged_len));
  cut_assert_equal_uint(2, n_results);
  cut_assert_equal_uint(105, max_tagged_len);
  result = g_new(gchar, max_tagged_len);

  sen_test_assert(sen_snip_get_result(snip, 0, result, &result_len));
  cut_assert_equal_string("Groongaは組み込み型の全文検索[[エンジン]]です。"
                          "DBMSやスクリプト言語処理系等",
                          result);
  cut_assert_equal_uint(103, result_len);

  sen_test_assert(sen_snip_get_result(snip, 1, result, &result_len));
  cut_assert_equal_string("度な転置\n"
                          "インデックスタイプの[[エンジン]]です。"
                          "コンパクトな実装ですが、",
                          result);
  cut_assert_equal_uint(104, result_len);
}

void
test_exec_with_invalid_argument(void)
{
  unsigned int n_results;
  unsigned int max_tagged_len;

  cut_assert_open_snip();

  sen_test_assert(sen_snip_exec(snip, text, strlen(text),
                               &n_results, &max_tagged_len));

  sen_test_assert_equal_rc(sen_invalid_argument,
                          sen_snip_exec(NULL, text, strlen(text),
                                        &n_results, &max_tagged_len));
  sen_test_assert_equal_rc(sen_invalid_argument,
                          sen_snip_exec(snip, NULL, strlen(text),
                                        &n_results, &max_tagged_len));
  sen_test_assert_equal_rc(sen_invalid_argument,
                          sen_snip_exec(snip, text, strlen(text),
                                        NULL, &max_tagged_len));
  sen_test_assert_equal_rc(sen_invalid_argument,
                          sen_snip_exec(snip, text, strlen(text),
                                        &n_results, NULL));
}

void
test_exec_with_normalize(void)
{
  unsigned int n_results;
  unsigned int max_tagged_len;
  unsigned int result_len;
  const gchar keyword[] = "転置インデックス";

  default_encoding = sen_enc_utf8;

  cut_assert_open_snip();
  sen_test_assert(sen_snip_add_cond(snip, keyword, strlen(keyword),
                                   NULL, 0, NULL, 0));

  sen_test_assert(sen_snip_exec(snip,
                               text_ja_utf8, strlen(text_ja_utf8),
                               &n_results, &max_tagged_len));
  cut_assert_equal_uint(0, n_results);

  sen_snip_close(snip);
  snip = NULL;


  default_flags = SEN_SNIP_NORMALIZE;

  cut_assert_open_snip();
  sen_test_assert(sen_snip_add_cond(snip, keyword, strlen(keyword),
                                   NULL, 0, NULL, 0));

  sen_test_assert(sen_snip_exec(snip,
                               text_ja_utf8, strlen(text_ja_utf8),
                               &n_results, &max_tagged_len));
  cut_assert_equal_uint(1, n_results);
  cut_assert_equal_uint(105, max_tagged_len);
  result = g_new(gchar, max_tagged_len);

  sen_test_assert(sen_snip_get_result(snip, 0, result, &result_len));
  cut_assert_equal_string("備えた、高速かつ高精度な[[転置\n"
                          "インデックス]]タイプのエンジンです。コン",
                          result);
  cut_assert_equal_uint(104, result_len);
}

void
test_exec_with_one_length_keyword(void)
{
  unsigned int n_results;
  unsigned int max_tagged_len;
  unsigned int result_len;
  const gchar keyword[] = "x";

  cut_assert_open_snip();
  sen_test_assert(sen_snip_add_cond(snip, keyword, strlen(keyword),
                                   NULL, 0, NULL, 0));

  sen_test_assert(sen_snip_exec(snip,
                               text, strlen(text),
                               &n_results, &max_tagged_len));
  cut_assert_equal_uint(2, n_results);
  cut_assert_equal_uint(113, max_tagged_len);
  result = g_new(gchar, max_tagged_len);

  sen_test_assert(sen_snip_get_result(snip, 0, result, &result_len));
  cut_assert_equal_string("Groonga is an embeddable fullte[[x]]t search "
                          "engine, which you can use in\n"
                          "conjunction with various scripti",
                          result);
  cut_assert_equal_uint(104, result_len);

  sen_test_assert(sen_snip_get_result(snip, 1, result, &result_len));
  cut_assert_equal_string("an inverted inde[[x]] based engine, & "
                          "combines the best of n-gram\n"
                          "inde[[x]]ing and word inde[[x]]ing to achieve ",
                          result);
  cut_assert_equal_uint(112, result_len);
}

void
test_customized_tag(void)
{
  const gchar open_tag[] = "((*";
  const gchar close_tag[] = "*))";
  unsigned int n_results;
  unsigned int max_tagged_len;
  unsigned int result_len;
  const gchar keyword[] = "engine";

  cut_assert_open_snip();
  sen_test_assert(sen_snip_add_cond(snip, keyword, strlen(keyword),
                                   open_tag, strlen(open_tag),
                                   close_tag, strlen(close_tag)));

  sen_test_assert(sen_snip_exec(snip, text, strlen(text),
                               &n_results, &max_tagged_len));
  cut_assert_equal_uint(2, n_results);
  cut_assert_equal_uint(107, max_tagged_len);
  result = g_new(gchar, max_tagged_len);

  sen_test_assert(sen_snip_get_result(snip, 0, result, &result_len));
  cut_assert_equal_string("Groonga is an embeddable fulltext search "
                          "((*engine*)), which you can use in\n"
                          "conjunction with various scripti",
                          result);
  cut_assert_equal_uint(106, result_len);

  sen_test_assert(sen_snip_get_result(snip, 1, result, &result_len));
  cut_assert_equal_string("nd databases. Groonga is\n"
                          "an inverted index based ((*engine*)), "
                          "& combines the best of n-gram\n"
                          "indexing and wo",
                          result);
  cut_assert_equal_uint(106, result_len);
}

void
test_multi_conditions(void)
{
  const gchar open_tag[] = "((*";
  const gchar close_tag[] = "*))";
  unsigned int n_results;
  unsigned int max_tagged_len;
  unsigned int result_len;
  const gchar keyword1[] = "fulltext";
  const gchar keyword2[] = "groonga";

  cut_assert_open_snip();
  sen_test_assert(sen_snip_add_cond(snip, keyword1, strlen(keyword1),
                                   open_tag, strlen(open_tag),
                                   close_tag, strlen(close_tag)));
  sen_test_assert(sen_snip_add_cond(snip, keyword2, strlen(keyword2),
                                   NULL, 0, NULL, 0));

  sen_test_assert(sen_snip_exec(snip, text, strlen(text),
                               &n_results, &max_tagged_len));
  cut_assert_equal_uint(2, n_results);
  cut_assert_equal_uint(107, max_tagged_len);
  result = g_new(gchar, max_tagged_len);

  sen_test_assert(sen_snip_get_result(snip, 0, result, &result_len));
  cut_assert_equal_string("Groonga is an embeddable ((*fulltext*)) search "
                          "engine, which you can use in\n"
                          "conjunction with various scripti",
                          result);
  cut_assert_equal_uint(106, result_len);

  sen_test_assert(sen_snip_get_result(snip, 1, result, &result_len));
  cut_assert_equal_string("dexing to achieve fast, precise searches. While\n"
                          "[[groonga]] codebase is rather compact it is "
                          "scalable enou",
                          result);
  cut_assert_equal_uint(104, result_len);
}

void
test_invalid_result_index(void)
{
  unsigned int n_results;
  unsigned int max_tagged_len;
  unsigned int result_len;
  const gchar keyword[] = "index";

  cut_assert_open_snip();
  sen_test_assert(sen_snip_add_cond(snip, keyword, strlen(keyword),
                                   NULL, 0, NULL, 0));

  sen_test_assert(sen_snip_exec(snip, text, strlen(text),
                               &n_results, &max_tagged_len));
  cut_assert_equal_uint(1, n_results);
  cut_assert_equal_uint(113, max_tagged_len);
  result = g_new(gchar, max_tagged_len);

  sen_test_assert(sen_snip_get_result(snip, 0, result, &result_len));
  sen_test_assert_equal_rc(sen_invalid_argument,
                          sen_snip_get_result(snip, 1, result, &result_len));
}

void
test_html_mapping(void)
{
  const gchar open_tag[] = "<<";
  unsigned int n_results;
  unsigned int max_tagged_len;
  unsigned int result_len;
  const gchar keyword[] = "indexing";

  default_mapping = (sen_snip_mapping *)-1;
  cut_assert_open_snip();
  sen_test_assert(sen_snip_add_cond(snip, keyword, strlen(keyword),
                                   open_tag, strlen(open_tag), NULL, 0));

  sen_test_assert(sen_snip_exec(snip, text, strlen(text),
                               &n_results, &max_tagged_len));
  cut_assert_equal_uint(1, n_results);
  cut_assert_equal_uint(113, max_tagged_len);
  result = g_new(gchar, max_tagged_len);

  sen_test_assert(sen_snip_get_result(snip, 0, result, &result_len));
  cut_assert_equal_string("ngine, &amp; combines the best of n-gram\n"
                          "<<indexing]] and word <<indexing]] to achieve fast, "
                          "precise searches. W",
                          result);
  cut_assert_equal_uint(112, result_len);
}

void
test_html_mapping_escape(void)
{
  const gchar close_tag[] = ">&>";
  unsigned int n_results;
  unsigned int max_tagged_len;
  unsigned int result_len;
  const gchar keyword[] = "Ruby";
  const gchar expected[] =
    "y not required.)&lt;/li&gt;\n"
    "          &lt;li&gt;[[Ruby>&> 1.8.1 or later "
    /*                */"(for [[Ruby>&> binding.)"
    /*                */"&lt;a class=&quot;external&quot; "
    /*                      */"href=";

  default_mapping = (sen_snip_mapping *)-1;
  cut_assert_open_snip();
  sen_test_assert(sen_snip_add_cond(snip, keyword, strlen(keyword),
                                   NULL, 0, close_tag, strlen(close_tag)));

  sen_test_assert(sen_snip_exec(snip, html_text, strlen(html_text),
                               &n_results, &max_tagged_len));
  cut_assert_equal_uint(1, n_results);
  cut_assert_equal_uint(strlen(expected) + 1, max_tagged_len);
  result = g_new(gchar, max_tagged_len);

  sen_test_assert(sen_snip_get_result(snip, 0, result, &result_len));
  cut_assert_equal_string(expected, result);
  cut_assert_equal_uint(strlen(expected), result_len);
}

void
test_close_with_null(void)
{
  sen_test_assert_equal_rc(sen_invalid_argument, sen_snip_close(NULL));
}

void
test_open_with_invalid_max_results(void)
{
  default_max_results = -1;
  cut_assert_null(open_snip());

  default_max_results = 0;
  cut_assert_null(open_snip());

  default_max_results = 16;
  cut_assert_not_null(open_snip());

  default_max_results = 17;
  cut_assert_null(open_snip());
}

void
test_open_with_copy_tag(void)
{
  const gchar *original_default_open_tag, *original_default_close_tag;
  unsigned int original_default_open_tag_len, original_default_close_tag_len;

  default_flags = SEN_SNIP_COPY_TAG;

  cut_assert_not_null(open_snip());

  original_default_open_tag = cut_take_string(default_open_tag);
  original_default_open_tag_len = default_open_tag_len;
  original_default_close_tag = cut_take_string(default_close_tag);
  original_default_close_tag_len = default_close_tag_len;

  default_open_tag = NULL;
  default_open_tag_len = 0;
  default_close_tag = NULL;
  default_close_tag_len = 0;
  cut_assert_not_null(open_snip());

  default_open_tag = g_strdup(original_default_open_tag);
  default_open_tag_len = original_default_open_tag_len;
  cut_assert_not_null(open_snip());
  g_free(default_open_tag);
  default_open_tag = NULL;
  default_open_tag_len = 0;

  default_close_tag = g_strdup(original_default_open_tag);
  default_close_tag_len = original_default_open_tag_len;
  cut_assert_not_null(open_snip());
}

void
test_add_cond_with_invalid_argument(void)
{
  unsigned int n_conds = 0, max_n_conds = 32U;
  const gchar keyword[] = "Groonga";
  unsigned int keyword_len;
  const gchar open_tag[] = "<<";
  const gchar close_tag[] = ">>";
  unsigned int open_tag_len, close_tag_len;

  keyword_len = strlen(keyword);
  open_tag_len = strlen(open_tag);
  close_tag_len = strlen(close_tag);

  cut_assert_open_snip();

  sen_test_assert(sen_snip_add_cond(snip,
                                   keyword, keyword_len,
                                   open_tag, open_tag_len,
                                   close_tag, close_tag_len));
  n_conds++;

  sen_test_assert_equal_rc(sen_invalid_argument,
                          sen_snip_add_cond(NULL,
                                            keyword, keyword_len,
                                            open_tag, open_tag_len,
                                            close_tag, close_tag_len));
  sen_test_assert_equal_rc(sen_invalid_argument,
                          sen_snip_add_cond(snip,
                                            NULL, keyword_len,
                                            open_tag, open_tag_len,
                                            close_tag, close_tag_len));
  sen_test_assert_equal_rc(sen_invalid_argument,
                          sen_snip_add_cond(snip,
                                            keyword, 0,
                                            open_tag, open_tag_len,
                                            close_tag, close_tag_len));

  while (n_conds < max_n_conds) {
    sen_test_assert(sen_snip_add_cond(snip,
                                      keyword, keyword_len,
                                      open_tag, open_tag_len,
                                      close_tag, close_tag_len),
                    "cond #%d", n_conds);
    n_conds++;
  }

  sen_test_assert_equal_rc(sen_invalid_argument,
                          sen_snip_add_cond(snip,
                                            keyword, keyword_len,
                                            open_tag, open_tag_len,
                                            close_tag, close_tag_len),
                          "cond #%d", n_conds);
}

void
test_add_cond_with_too_large_keyword(void)
{
  const gchar *sub_text;

  cut_assert_open_snip();

  cut_assert_operator_int(strlen(text), >, default_width);
  sen_test_assert_equal_rc(sen_invalid_argument,
                          sen_snip_add_cond(snip,
                                            text, strlen(text),
                                            NULL, 0, NULL, 0));

  sub_text = text + strlen(text) - default_width;
  sen_test_assert(sen_snip_add_cond(snip,
                                    sub_text, strlen(sub_text),
                                    NULL, 0, NULL, 0));

  sub_text--;
  sen_test_assert_equal_rc(sen_invalid_argument,
                          sen_snip_add_cond(snip,
                                            sub_text, strlen(sub_text),
                                            NULL, 0, NULL, 0));
}

void
test_add_cond_with_copy_tag(void)
{
  const gchar keyword[] = "Groonga";
  unsigned int keyword_len;
  const gchar open_tag[] = "<<";
  const gchar close_tag[] = ">>";
  unsigned int open_tag_len, close_tag_len;

  keyword_len = strlen(keyword);
  open_tag_len = strlen(open_tag);
  close_tag_len = strlen(close_tag);

  default_flags = SEN_SNIP_COPY_TAG;

  cut_assert_open_snip();

  sen_test_assert(sen_snip_add_cond(snip,
                                    keyword, keyword_len,
                                    open_tag, open_tag_len,
                                    close_tag, close_tag_len));
  sen_test_assert(sen_snip_add_cond(snip,
                                    keyword, keyword_len,
                                    open_tag, open_tag_len,
                                    NULL, 0));
  sen_test_assert(sen_snip_add_cond(snip,
                                    keyword, keyword_len,
                                    NULL, 0,
                                    close_tag, close_tag_len));
  sen_test_assert(sen_snip_add_cond(snip,
                                    keyword, keyword_len,
                                    NULL, 0,
                                    NULL, 0));
}