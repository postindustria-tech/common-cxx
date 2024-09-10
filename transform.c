/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2023 51 Degrees Mobile Experts Limited, Davidson House,
 * Forbury Square, Reading, Berkshire, United Kingdom RG1 3EU.
 *
 * This Original Work is licensed under the European Union Public Licence
 * (EUPL) v.1.2 and is subject to its terms as set out below.
 *
 * If a copy of the EUPL was not distributed with this file, You can obtain
 * one at https://opensource.org/licenses/EUPL-1.2.
 *
 * The 'Compatible Licences' set out in the Appendix to the EUPL (as may be
 * amended by the European Commission) shall be deemed incompatible for
 * the purposes of the Work and the provisions of the compatibility
 * clause in Article 5 of the EUPL shall not apply.
 *
 * If using the Work as, or as part of, a network application, by
 * including the attribution notice(s) required under Article 5 of the EUPL
 * in the end user terms of the application under an appropriate heading,
 * such notice(s) shall fulfill the requirements of that article.
 * ********************************************************************* */

#include "transform.h"
#include "fiftyone.h"

#define initStaticKey(x) {x, sizeof(x) - 1}

#define NotExpectSymbol(json, ch, exit) \
  if (*json == ch) {                    \
	EXCEPTION_SET(CORRUPT_DATA);   \
    exit;                               \
  }

#define ExpectSymbol(json, ch, exit)  \
  if (*json != ch) {                  \
	EXCEPTION_SET(CORRUPT_DATA); \
    exit;                             \
  }

#define ExpectKeySymbol(json, ch)        \
  if (*json != ch) {                     \
    json = skip_to_next_char(json, '"'); \
    return KEY_UNDEFINED;                \
  }

#define ValuePtr \
  (EXCEPTION_CHECK(INSUFFICIENT_MEMORY) ? NULL \
                                                  : begin)

#define GET_SEXTET(str, i) \
  (str[i] == '=' ? 0 & i++ : base64_char_to_value(str[i++], exception))

typedef enum {
  ARCHITECTURE,     // sec-ch-ua-arch
  BRANDS,           // sec-ch-ua
  BITNESS,          // sec-ch-ua-bitness
  FULLVERSIONLIST,  // sec-ch-ua-full-version-list
  MOBILE,           // sec-ch-ua-mobile
  MODEL,            // sec-ch-ua-model
  PLATFORM,         // sec-ch-ua-platform
  PLATFORMVERSION,  // sec-ch-ua-platform-version
  KEY_UNDEFINED,    //
} Key;

typedef Key (*readKeyCallback)(const char**, Exception* const);
typedef char* (*readValueCallback)(const char** json, StringBuilder *builder,
                                   KeyValuePair* cache, Key key,
                                   Exception* const exception);

static struct {
  const char* key;
  size_t len;
} key_map[] = {
    initStaticKey("sec-ch-ua-arch"),
    initStaticKey("sec-ch-ua"),
    initStaticKey("sec-ch-ua-bitness"),
    initStaticKey("sec-ch-ua-full-version-list"),
    initStaticKey("sec-ch-ua-mobile"),
    initStaticKey("sec-ch-ua-model"),
    initStaticKey("sec-ch-ua-platform"),
    initStaticKey("sec-ch-ua-platform-version"),
};

// ----

static inline char* safe_write_to_buffer(StringBuilder *builder,
                                         char symbol,
                                         Exception* const exception) {
    StringBuilderAddChar(builder, symbol);
    if (builder->full) {
        EXCEPTION_SET(INSUFFICIENT_MEMORY);
    }
    return builder->current;
}

static inline uint32_t base64_char_to_value(
    char c, Exception* const exception) {
  static const uint32_t base64_lookup_table[256] = {
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,  255,
      255, 255, 63,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255,
      255, 255, 255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
      10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
      25,  255, 255, 255, 255, 255, 255, 26,  27,  28,  29,  30,  31,  32,  33,
      34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
      49,  50,  51,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255};

  if (base64_lookup_table[(uint8_t)c] == 255) {
    EXCEPTION_SET(CORRUPT_DATA);
  }

  return base64_lookup_table[(uint8_t)c];
}

static size_t base64_decode(const char* base64_input, StringBuilder *builder,
                            Exception* const exception) {
    size_t before = builder->added;
  size_t input_length = strlen(base64_input);
  if (input_length % 4 != 0) {
	  EXCEPTION_SET(CORRUPT_DATA);
    return 0;  // Invalid base64 input length
  }

  for (size_t i = 0; i < input_length;) {
    uint32_t sextet_a = GET_SEXTET(base64_input, i);
    uint32_t sextet_b = GET_SEXTET(base64_input, i);
    uint32_t sextet_c = GET_SEXTET(base64_input, i);
    uint32_t sextet_d = GET_SEXTET(base64_input, i);

    if (EXCEPTION_CHECK(CORRUPT_DATA)) {
      return 0;
    }

    uint32_t triple =
        (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;

    safe_write_to_buffer(builder, (triple >> 16) & 0xFF, exception);
    safe_write_to_buffer(builder, (triple >> 8) & 0xFF, exception);
    safe_write_to_buffer(builder, triple & 0xFF, exception);
  }

  safe_write_to_buffer(builder, '\0', exception);
    size_t after = builder->added;
  return after - before;
}

static inline const char* skip_whitespaces(const char* json) {
  for (;; ++json) {
    switch (*json) {
      case ' ':
      case '\t':
      case '\n':
      case '\v':
      case '\r':
      case '\f':
        break;

      default:
        return json;
    }
  }
}

static inline const char* skip_to_next_char(const char* json,
                                            const char target) {
  for (; *json != target; ++json) {
    switch (*json) {
      case '\0': {
        return json;
      } break;

      case '\\': {
        if (json[1] == target || json[1] == '\\') {
          ++json;
        }
      } break;
    }
  }

  return json;
}

static const char* skip_value(const char* json) {
  json = skip_to_next_char(json, ':');
  if (*json == '\0') {
    return json;
  }

  json = skip_whitespaces(json + 1);

  switch (*json) {
    case '\0': {
      return json;
    } break;

    case '{': {  // skip nested object
      ++json;

      for (int nesting_level = 1; nesting_level > 0; ++json) {
        switch (*json) {
          case '\0': {
            return json;
          } break;

          case '{': {
            ++nesting_level;
          } break;

          case '}': {
            --nesting_level;
          } break;

          case '"': {
            json = skip_to_next_char(json + 1, '"');
            if (*json == '\0') {
              return json;
            }
          } break;
        }
      }
    } break;

    case '[': {
      ++json;

      for (int nesting_level = 1; nesting_level > 0; ++json) {
        switch (*json) {
          case '\0': {
            return json;
          } break;

          case '[': {
            ++nesting_level;
          } break;

          case ']': {
            --nesting_level;
          } break;

          case '"': {
            json = skip_to_next_char(json + 1, '"');
            if (*json == '\0') {
              return json;
            }
          } break;
        }
      }
    } break;

    case '"': {
      json = skip_to_next_char(json + 1, '"');
    } break;

    default: {
      for (int flag = 1; flag;) {
        switch (*json) {
          case '\0': {
            return json;
          } break;

          case ',':
          case '}':
          case ']': {
            flag = 0;
          } break;

          default: {
            ++json;
          } break;
        }
      }
    } break;
  }

  if (*json == '\0') {
    return json;
  }

  return skip_to_next_char(json + 1, '"');
}

static inline void init_keys(StringBuilder *builder,
                              KeyValuePair* cache,
                              Exception* const exception) {
  for (size_t k = 0; k < KEY_UNDEFINED; ++k) {
    cache[k].key = builder->current;
    cache[k].keyLength = key_map[k].len;

    for (size_t i = 0; i < key_map[k].len; ++i) {
      safe_write_to_buffer(builder, key_map[k].key[i], exception);
    }
  }
}

static const char* init_parsing(const char* json, 
                                StringBuilder *builder,
                                KeyValuePair* cache,
                                Exception* const exception) {

  init_keys(builder, cache, exception);

  json = skip_whitespaces(json);
  ExpectSymbol(json, '{', return json);
  return skip_to_next_char(json, '"');
}

static Key read_ghev_key(const char** json,
                         Exception* const exception) {
  enum ReadKeyState {
    READ_KEY_INIT,
    ARCH,
    BRANDS_OR_BITNESS,
    FULL_VERSION_LIST,
    MOBILE_OR_MODEL,
    PLATFORM_OR_VERSION,
    READ_KEY_BRANDS,
    READ_KEY_BITNESS,
    READ_KEY_MOBILE,
    READ_KEY_MODEL,
    READ_KEY_PLATFORMVERSION,
  };

  ++*json;
  NotExpectSymbol(*json, '\0', return KEY_UNDEFINED);

  for (enum ReadKeyState state = READ_KEY_INIT; **json != '\0'; ++(*json)) {
    switch (state) {
      case READ_KEY_INIT: {
        switch (**json) {
          case 'a': {
            state = ARCH;
          } break;

          case 'f': {
            state = FULL_VERSION_LIST;
          } break;

          case 'b': {
            state = BRANDS_OR_BITNESS;
          } break;

          case 'm': {
            state = MOBILE_OR_MODEL;
          } break;

          case 'p': {
            state = PLATFORM_OR_VERSION;
          } break;

          default: {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          } break;

          case '"': {
            return KEY_UNDEFINED;
          } break;
        }
      } break;

      case ARCH: {
        for (const char* i = "rchitecture"; *i != '\0'; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        return ARCHITECTURE;
      } break;

      case FULL_VERSION_LIST: {
        for (const char* i = "ullVersionList"; *i != '\0'; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        return FULLVERSIONLIST;
      } break;

      case BRANDS_OR_BITNESS: {
        switch (**json) {
          case 'r': {
            state = READ_KEY_BRANDS;
          } break;

          case 'i': {
            state = READ_KEY_BITNESS;
          } break;

          default: {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          } break;
        }
      } break;

      case MOBILE_OR_MODEL: {
        ExpectKeySymbol(*json, 'o');

        ++(*json);
        NotExpectSymbol(*json, '\0', return KEY_UNDEFINED);

        switch (**json) {
          case 'b': {
            state = READ_KEY_MOBILE;
          } break;

          case 'd': {
            state = READ_KEY_MODEL;
          } break;

          default: {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          } break;
        }
      } break;

      case PLATFORM_OR_VERSION: {
        for (const char* i = "latform"; *i; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        switch (**json) {
          case '"': {
            return PLATFORM;
          } break;

          case 'V': {
            state = READ_KEY_PLATFORMVERSION;
          } break;

          default: {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          } break;
        }
      } break;

      case READ_KEY_BRANDS: {
        for (const char* i = "ands"; *i; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        return BRANDS;
      } break;

      case READ_KEY_BITNESS: {
        for (const char* i = "tness"; *i; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        return BITNESS;
      } break;

      case READ_KEY_MOBILE: {
        for (const char* i = "ile"; *i; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        return MOBILE;
      } break;

      case READ_KEY_MODEL: {
        for (const char* i = "el"; *i; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        return MODEL;
      } break;

      case READ_KEY_PLATFORMVERSION: {
        for (const char* i = "ersion"; *i; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        return PLATFORMVERSION;
      } break;
    }
  }

  return KEY_UNDEFINED;
}

static Key read_sua_key(const char** json,
                        Exception* const exception) {
  enum ReadKeyState {
    READ_KEY_INIT,
    BROWSERS_OR_BITNESS,
    MOBILE_OR_MODEL,
    READ_KEY_BITNESS,
    ARCH,
    READ_KEY_MOBILE,
    READ_KEY_MODEL,
    READ_KEY_PLATFORM,
    READ_KEY_BROWSERS,
  };

  ++*json;
  NotExpectSymbol(*json, '\0', return KEY_UNDEFINED);

  for (enum ReadKeyState state = READ_KEY_INIT; **json != '\0'; ++(*json)) {
    switch (state) {
      case READ_KEY_INIT: {
        switch (**json) {
          case 'a': {
            state = ARCH;
          } break;

          case 'b': {
            state = BROWSERS_OR_BITNESS;
          } break;

          case 'm': {
            state = MOBILE_OR_MODEL;
          } break;

          case 'p': {
            state = READ_KEY_PLATFORM;
          } break;

          default: {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          } break;

          case '"': {
            return KEY_UNDEFINED;
          } break;
        }
      } break;

      case ARCH: {
        for (const char* i = "rchitecture"; *i; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        return ARCHITECTURE;
      } break;

      case BROWSERS_OR_BITNESS: {
        switch (**json) {
          case 'r': {
            state = READ_KEY_BROWSERS;
          } break;

          case 'i': {
            state = READ_KEY_BITNESS;
          } break;

          default: {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          } break;
        }
      } break;

      case MOBILE_OR_MODEL: {
        ExpectKeySymbol(*json, 'o');

        ++(*json);
        NotExpectSymbol(*json, '\0', return KEY_UNDEFINED);

        switch (**json) {
          case 'b': {
            state = READ_KEY_MOBILE;
          } break;

          case 'd': {
            state = READ_KEY_MODEL;
          } break;

          default: {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          } break;
        }
      } break;

      case READ_KEY_PLATFORM: {
        for (const char* i = "latform"; *i; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        ExpectKeySymbol(*json, '"');

        return PLATFORM;
      } break;

      case READ_KEY_BROWSERS: {
        for (const char* i = "owsers"; *i; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        return FULLVERSIONLIST;
      } break;

      case READ_KEY_BITNESS: {
        for (const char* i = "tness"; *i; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        return BITNESS;
      } break;

      case READ_KEY_MOBILE: {
        for (const char* i = "ile"; *i; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        return MOBILE;
      } break;

      case READ_KEY_MODEL: {
        for (const char* i = "el"; *i; ++(*json), ++i) {
          ExpectKeySymbol(*json, *i);
        }

        return MODEL;
      } break;
    }
  }

  return KEY_UNDEFINED;
}

static char* read_string_value(const char** json, StringBuilder *builder,
                               Exception* const exception) {
    char *begin = builder->current;
  *json = skip_whitespaces(*json);
  if (**json == 'n') {
    ++(*json);
    NotExpectSymbol(*json, '\0', return begin);

    for (const char* i = "ull"; *i; ++(*json), ++i) {
      ExpectSymbol(*json, *i, return begin);
    }

    return NULL;
  }

  ExpectSymbol(*json, '"', return begin);

  ++*json;

  for (begin = safe_write_to_buffer(builder, '"', exception);; ++(*json)) {
    NotExpectSymbol(*json, '\0', return begin);

    begin = safe_write_to_buffer(builder, **json, exception);

    switch (**json) {
      case '\"': {
        ++(*json);
        return begin;
      }

      case '\\': {
        if ((*json)[1] == '\\' || (*json)[1] == '"') {
          ++(*json);

          safe_write_to_buffer(builder, **json, exception);
        }
      } break;
    }
  }
}

static char* read_bool_ghev_value(const char** json,
                                  StringBuilder *builder,
                                  KeyValuePair* cache, Key key,
                                  Exception* const exception) {
    char *begin = builder->current;
    char *ptr = begin;
    size_t before = builder->added;
  switch (**json) {
    case 't': {
      ++(*json);
      for (const char* i = "rue"; *i != '\0'; ++(*json), ++i) {
        ExpectSymbol(*json, *i, return begin);
      }

      ptr = safe_write_to_buffer(builder, '?', exception);
      ptr = safe_write_to_buffer(builder, '1', exception);
    } break;

    case 'f': {
      ++(*json);
      for (const char* i = "alse"; *i != '\0'; ++(*json), ++i) {
        ExpectSymbol(*json, *i, return begin);
      }

      ptr = safe_write_to_buffer(builder, '?', exception);
      ptr = safe_write_to_buffer(builder, '0', exception);
    } break;

    default: {
		EXCEPTION_SET(CORRUPT_DATA);
      return begin;
    } break;
  }

    size_t after = builder->added;
  cache[key].value = ValuePtr;
  cache[key].valueLength = after - before;
    return ptr;
}

static char* read_bool_sua_value(const char** json,
                                 StringBuilder *builder,
                                 KeyValuePair* cache, Key key,
                                 Exception* const exception) {
    char *begin = builder->current;
    size_t before = builder->added;
  switch (**json) {
    case '0':
    case '1': {
    } break;

    default: {
		EXCEPTION_SET(CORRUPT_DATA);
      return begin;
    } break;
  }

  char* ptr = safe_write_to_buffer(builder, '?', exception);
  ptr = safe_write_to_buffer(builder, **json, exception);

  ++(*json);

    size_t after = builder->added;
  cache[key].value = ValuePtr;
  cache[key].valueLength = after - before;

  return ptr;
}

static char* read_version_sua(const char** json,
                              StringBuilder *builder,
                              Exception* const exception) {
  enum version_state {
    version_read,
    version_skip,
    version_exit,
  } state = version_skip;

    char *begin = builder->current;
  for (char* ptr = begin;; ++(*json)) {
    NotExpectSymbol(*json, '\0', return begin); //rollback

    switch (state) {
      case version_read: {
        switch (**json) {
          case '\"': {
            state = version_skip;
          } break;

          case '\\': {
            ptr = safe_write_to_buffer(builder, **json, exception);

            if ((*json)[1] == '\\' || (*json)[1] == '"') {
              ++(*json);
              ptr = safe_write_to_buffer(builder, **json, exception);
            }
          } break;

          default: {
            ptr = safe_write_to_buffer(builder, **json, exception);
          } break;
        }
      } break;

      case version_skip: {
        switch (**json) {
          case '"': {
            state = version_read;
          } break;

          case ',': {
            ptr = safe_write_to_buffer(builder, '.', exception);
          } break;

          case ']': {
            state = version_exit;
          } break;
        }
      } break;

      case version_exit: {
        ptr = safe_write_to_buffer(builder, '"', exception);
          return ptr;
      } break;
    }
  }
}

static char* read_brands_ghev_value(const char** json, StringBuilder *builder,
                                    KeyValuePair* cache, Key key,
                                    Exception* const exception) {
    char *begin = builder->current;
  *json = skip_to_next_char(*json, '[');
  ExpectSymbol(*json, '[', return begin);

  ++*json;

  for (char* ptr = begin;; ++*json) {
    *json = skip_to_next_char(*json, '{');
    ExpectSymbol(*json, '{', return begin);

    *json = skip_to_next_char(*json + 1, '"');
    ExpectSymbol(*json, '"', return begin);

    ++*json;

    for (const char* k = "brand\""; *k != '\0'; ++k, ++*json) {
      ExpectSymbol(*json, *k, return begin);
    }

    *json = skip_to_next_char(*json, ':');
    ExpectSymbol(*json, ':', return begin);

    ++*json;

      char* ptr2 = read_string_value(json, builder, exception);
    if (ptr2 != NULL) {
      ptr = safe_write_to_buffer(builder, ';', exception);
      ptr = safe_write_to_buffer(builder, 'v', exception);
      ptr = safe_write_to_buffer(builder, '=', exception);

      *json = skip_to_next_char(*json, ',');
      ExpectSymbol(*json, ',', return begin); //rollback

      *json = skip_to_next_char(*json + 1, '"');
      ExpectSymbol(*json, '"', return begin); //rollback

      ++*json;

      for (const char* k = "version\""; *k != '\0'; ++k, ++*json) {
        ExpectSymbol(*json, *k, return begin); //rollback
      }

      *json = skip_to_next_char(*json, ':');
      ExpectSymbol(*json, ':', return begin); //rollback

      ++*json;

      ptr2 = read_string_value(json, builder, exception);
      if (ptr2 == NULL) {
        ptr2 = ptr;

        ptr = safe_write_to_buffer(builder, 'n', exception);
        ptr = safe_write_to_buffer(builder, 'u', exception);
        ptr = safe_write_to_buffer(builder, 'l', exception);
        ptr = safe_write_to_buffer(builder, 'l', exception);
      } else {
        ptr = ptr2;
      }
    }

    *json = skip_to_next_char(*json, '}');
    ExpectSymbol(*json, '}', return begin);

    *json = skip_whitespaces(*json + 1);
    NotExpectSymbol(*json, '\0', return begin);

    switch (**json) {
      case ']': {
        if (ptr != begin) {
          cache[key].value = ValuePtr;
          cache[key].valueLength = ptr - begin;
          return ptr;
        } else {
          return NULL;
        }

      } break;

      case ',': {
        if (ptr2 != NULL) {
          ptr = safe_write_to_buffer(builder, ',', exception);
          ptr = safe_write_to_buffer(builder, ' ', exception);
        }
      } break;

      default: {
		EXCEPTION_SET(CORRUPT_DATA);
        return begin;
      } break;
    }
  }
}

static char* read_brands_sua_value(const char** json, StringBuilder *builder,
                                   KeyValuePair* cache, Key key,
                                   Exception* const exception) {
  *json = skip_to_next_char(*json, '[');
    char *begin = builder->current;
  ExpectSymbol(*json, '[', return begin);

  for (char* ptr = begin;; ++*json) {
    NotExpectSymbol(*json, '\0', return begin);

    *json = skip_to_next_char(*json, '{');
    ExpectSymbol(*json, '{', return begin);

    *json = skip_to_next_char(*json, '"');
    ExpectSymbol(*json, '"', return begin);

    ++*json;

    for (const char* k = "brand\""; *k != '\0'; ++k, ++*json) {
      ExpectSymbol(*json, *k, return begin);
    }

    *json = skip_to_next_char(*json, ':');
    ExpectSymbol(*json, ':', return begin);

    ++*json;

    char* ptr2 = read_string_value(json, builder, exception);
    if (ptr2 != NULL) {
      ptr = safe_write_to_buffer(builder, ';', exception);
      ptr = safe_write_to_buffer(builder, 'v', exception);
      ptr = safe_write_to_buffer(builder, '=', exception);

      *json = skip_to_next_char(*json, ',');
      ExpectSymbol(*json, ',', return begin);

      *json = skip_to_next_char(*json + 1, '"');
      ExpectSymbol(*json, '"', return begin);

      ++*json;

      for (const char* k = "version\""; *k != '\0'; ++k, ++*json) {
        ExpectSymbol(*json, *k, return begin);
      }

      *json = skip_to_next_char(*json, ':');
      ExpectSymbol(*json, ':', return begin);

      *json = skip_to_next_char(*json, '[');
      ExpectSymbol(*json, '[', return begin);

      ++*json;

      ptr = safe_write_to_buffer(builder, '"', exception);
      ptr = read_version_sua(json, builder, exception);
    }

    *json = skip_to_next_char(*json, '}');
    ExpectSymbol(*json, '}', return begin);

    *json = skip_whitespaces(*json + 1);
    NotExpectSymbol(*json, '\0', return begin);

    switch (**json) {
      case ']': {
        if (ptr != begin) {
          cache[key].value = ValuePtr;
          cache[key].valueLength = ptr - begin;
          return ptr;
        } else {
          return NULL;
        }
      } break;

      case ',': {
        if (ptr != begin) {
          ptr = safe_write_to_buffer(builder, ',', exception);
          ptr = safe_write_to_buffer(builder, ' ', exception);
        }
      } break;

      default: {
		EXCEPTION_SET(CORRUPT_DATA);
        return begin;
      } break;
    }
  }
}

static char* read_pure_string_value(const char** json, StringBuilder *builder,
                                    KeyValuePair* cache, Key key,
                                    Exception* const exception) {
    char *begin = builder->current;
  char* ptr = read_string_value(json, builder, exception);

  if (ptr != NULL) {
    cache[key].value = ValuePtr;
    cache[key].valueLength = ptr - begin;
  }

  return ptr;
}

static char* read_platform_sua_value(
    const char** json, StringBuilder *builder,
    KeyValuePair* cache, Key key,
    Exception* const exception) {
    char *begin = builder->current;
  *json = skip_to_next_char(*json, '{');
  ExpectSymbol(*json, '{', return begin);

  *json = skip_to_next_char(*json + 1, '"');
  ExpectSymbol(*json, '"', return begin);

  ++*json;

  for (const char* k = "brand\""; *k != '\0'; ++k, ++*json) {
    ExpectSymbol(*json, *k, return begin);
  }

  *json = skip_to_next_char(*json, ':');
  ExpectSymbol(*json, ':', return begin);

  ++*json;

  char* ptr = read_string_value(json, builder, exception);
  if (ptr == NULL) {
    return NULL;
  }

  cache[key].value = ValuePtr;
  cache[key].valueLength = ptr - begin;

  cache[PLATFORMVERSION].value = NULL;
  cache[PLATFORMVERSION].valueLength = 0;

  begin = ptr;

  *json = skip_whitespaces(*json);

  if (**json == '}') {
    return begin;
  }

  ExpectSymbol(*json, ',', return begin);

  *json = skip_to_next_char(*json + 1, '"');
  ExpectSymbol(*json, '"', return begin);

  ++*json;

  for (const char* k = "version\""; *k != '\0'; ++k, ++*json) {
    ExpectSymbol(*json, *k, return begin);
  }

  *json = skip_to_next_char(*json, ':');
  ExpectSymbol(*json, ':', return begin);

  *json = skip_to_next_char(*json + 1, '[');
  ExpectSymbol(*json, '[', return begin);

  ++*json;
  NotExpectSymbol(*json, '\0', return begin);

  ptr = safe_write_to_buffer(builder, '"', exception);
  ptr = read_version_sua(json, builder, exception);

  cache[PLATFORMVERSION].value = ValuePtr;
  cache[PLATFORMVERSION].valueLength = ptr - begin;

  return ptr;
}

static inline readValueCallback read_value_switch(Key key, int isSua) {
  readValueCallback res = NULL;

  switch (key) {
    case ARCHITECTURE: {
      res = read_pure_string_value;
    } break;

    case BITNESS: {
      res = read_pure_string_value;
    } break;

    case BRANDS: {
      res = isSua ? NULL : read_brands_ghev_value;
    } break;

    case FULLVERSIONLIST: {
      res = isSua ? read_brands_sua_value : read_brands_ghev_value;
    } break;

    case MOBILE: {
      res = isSua ? read_bool_sua_value : read_bool_ghev_value;
    } break;

    case MODEL: {
      res = read_pure_string_value;
    } break;

    case PLATFORM: {
      res = isSua ? read_platform_sua_value : read_pure_string_value;
    } break;

    case PLATFORMVERSION: {
      res = isSua ? NULL : read_pure_string_value;
    } break;

    case KEY_UNDEFINED: {
      res = NULL;
    } break;
  }

  return res;
}

static bool pushToHeaders(void* ctx, KeyValuePair header) {
  KeyValuePairArray* const headers = (KeyValuePairArray* const)ctx;

  if (headers->count < headers->capacity) {
    KeyValuePair* pair = headers->items + headers->count++;

    pair->key = header.key;
    pair->keyLength = header.keyLength;
    pair->value = header.value;
    pair->valueLength = header.valueLength;
  }
  return (headers->count < headers->capacity);
}
// ------------------------------------------------------------------------------------------------
static uint32_t main_parsing_body(const char* json,
                                StringBuilder *builder,
                                Exception* const exception,
                                int isSua,
                                TransformCallback callback,
                                void* ctx) {
  KeyValuePair cache[KEY_UNDEFINED];

    char *begin = builder->current;
  // define buffer range

  // write keys to buffer, init cache and skip to the first key
  json = init_parsing(json, builder, cache, exception);
  if (EXCEPTION_CHECK(CORRUPT_DATA)) {
    return 0;
  }

	uint32_t iterations = 0;  // total number of parsed key-value pairs

  // main reading loop
  readKeyCallback read_key = isSua ? read_sua_key : read_ghev_key;
  while (*json != '\0') {
    Key key = read_key(&json, exception);
    ExpectSymbol(json, '"', break);

    readValueCallback read_value = read_value_switch(key, isSua);
    if (key == KEY_UNDEFINED || read_value == NULL) {
      json = skip_value(json + 1);
      continue;
    }

    json = skip_to_next_char(json, ':');
    ExpectSymbol(json, ':', break);

    json = skip_whitespaces(json + 1);
    NotExpectSymbol(json, '\0', break);

    char* ptr = read_value(&json, builder, cache, key, exception);
    if (EXCEPTION_CHECK(CORRUPT_DATA)) {
      break;
    }

    if (ptr != NULL) {
      begin = ptr;

      ++iterations;
      if (!callback(ctx, cache[key])) {
		  break;
      }

      if (key == PLATFORM && isSua && cache[PLATFORMVERSION].valueLength != 0) {
        ++iterations;
        if (!callback(ctx, cache[PLATFORMVERSION])) {
			break;
        }
      }
    }

    json = skip_to_next_char(json, '"');
  }

  return iterations;
}

// The difference of this function is that it does not initialize the builder - and assumes
// it has been initialized outside - useful for base64 and then JSON from the same buffer

uint32_t TransformIterateGhevFromJsonPrivate(const char* json, StringBuilder *builder,
                                                          fiftyoneDegreesTransformCallback callback, void* ctx,
                                                          fiftyoneDegreesException* const exception) {
    return main_parsing_body(json, builder, exception, 0, callback, ctx);
}
// ------------------------------------------------------------------------------------------------
fiftyoneDegreesTransformIterateResult fiftyoneDegreesTransformIterateGhevFromJson(
    const char* json, char *buffer, size_t length,
    fiftyoneDegreesTransformCallback callback, void* ctx,
    fiftyoneDegreesException* const exception) {
	StringBuilder builder = {buffer, length};
    StringBuilderInit(&builder);
	uint32_t iterations = TransformIterateGhevFromJsonPrivate(json, &builder, callback, ctx, exception);
    StringBuilderComplete(&builder);
	fiftyoneDegreesTransformIterateResult result = {iterations, builder.added, builder.added > builder.length};
	return result;
}

fiftyoneDegreesTransformIterateResult fiftyoneDegreesTransformIterateGhevFromBase64(
    const char* base64, char *buffer, size_t length,
    fiftyoneDegreesTransformCallback callback, void* ctx,
    fiftyoneDegreesException* const exception) {
	StringBuilder builder = {buffer, length};
    StringBuilderInit(&builder);
    base64_decode(base64, &builder, exception);
	fiftyoneDegreesTransformIterateResult result = {0, builder.added, builder.added > builder.length};
  if (EXCEPTION_CHECK(CORRUPT_DATA) || EXCEPTION_CHECK(INSUFFICIENT_MEMORY)) {
    return result;
  }
    char *json = builder.ptr;
    //note we are calling a private function to reuse the initialized stringbuilder
	uint32_t iterations = TransformIterateGhevFromJsonPrivate(json, &builder, callback, ctx, exception);
    StringBuilderComplete(&builder);
	result.iterations = iterations;
	result.written = builder.added;
	result.bufferTooSmall = builder.added > builder.length;
	return result;
}

fiftyoneDegreesTransformIterateResult fiftyoneDegreesTransformIterateSua(
    const char* json, char *buffer, size_t length,
    fiftyoneDegreesTransformCallback callback, void* ctx,
    fiftyoneDegreesException* const exception) {
	StringBuilder builder = {buffer, length};
    StringBuilderInit(&builder);
	uint32_t iterations = main_parsing_body(json, &builder, exception, 1, callback, ctx);
    StringBuilderComplete(&builder);
	fiftyoneDegreesTransformIterateResult result = {iterations, builder.added, builder.added > builder.length};
    return result;
}

// Array methods internally relay on iterative methods
fiftyoneDegreesTransformIterateResult fiftyoneDegreesTransformGhevFromJson(
    const char* json, char *buffer, size_t length,
    fiftyoneDegreesKeyValuePairArray* const headers,
    fiftyoneDegreesException* const exception) {
  uint32_t initial = headers->count;
	fiftyoneDegreesTransformIterateResult result = TransformIterateGhevFromJson(
      json, buffer, length, pushToHeaders, headers, exception);

  if (result.iterations != headers->count - initial) {
    EXCEPTION_SET(INSUFFICIENT_CAPACITY);
  }
  return result;
}

fiftyoneDegreesTransformIterateResult fiftyoneDegreesTransformGhevFromBase64(
    const char* base64, char *buffer, size_t length,
    fiftyoneDegreesKeyValuePairArray* const headers,
    fiftyoneDegreesException* const exception) {
  uint32_t initial = headers->count;
  fiftyoneDegreesTransformIterateResult result = TransformIterateGhevFromBase64(
      base64, buffer, length, pushToHeaders, headers, exception);

  if (result.iterations != headers->count - initial) {
    EXCEPTION_SET(INSUFFICIENT_CAPACITY);
  }

  return result;
}

fiftyoneDegreesTransformIterateResult fiftyoneDegreesTransformSua(
    const char* json, char *buffer, size_t length,
    fiftyoneDegreesKeyValuePairArray* const headers,
    fiftyoneDegreesException* const exception) {
  uint32_t initial = headers->count;
  fiftyoneDegreesTransformIterateResult result = TransformIterateSua(
      json, buffer, length, pushToHeaders, headers, exception);

  if (result.iterations != headers->count - initial) {
    EXCEPTION_SET(INSUFFICIENT_CAPACITY);
  }

  return result;
}
