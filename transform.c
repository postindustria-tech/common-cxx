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

#define initStaticKey(x) {x, sizeof(x) - 1}

#define NotExpectSymbol(json, ch, exit)                       \
  if (*json == ch) {                                          \
    exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA; \
    exit;                                                     \
  }

#define ExpectSymbol(json, ch, exit)                          \
  if (*json != ch) {                                          \
    exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA; \
    exit;                                                     \
  }

#define ValuePtr                                                           \
  (exception->status == FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY ? NULL \
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

typedef Key (*readKeyCallback)(const char**, fiftyoneDegreesException* const);
typedef char* (*readValueCallback)(const char** json, char* begin,
                                   const char* const end,
                                   fiftyoneDegreesKeyValuePair* cache, Key key,
                                   fiftyoneDegreesException* const exception);

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

static inline char* safe_write_to_buffer(
    char* begin, const char* const end, char symbol,
    fiftyoneDegreesException* const exception) {
  if (begin < end) {
    *begin = symbol;
  } else {
    exception->status = FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY;
  }

  return ++begin;
}

static inline uint32_t base64_char_to_value(
    char c, fiftyoneDegreesException* const exception) {
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
    exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
  }

  return base64_lookup_table[(uint8_t)c];
}

static size_t base64_decode(const char* base64_input, char* const buffer,
                            size_t length,
                            fiftyoneDegreesException* const exception) {
  size_t input_length = strlen(base64_input);
  if (input_length % 4 != 0) {
    exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
    return 0;  // Invalid base64 input length
  }

  char* begin = buffer;
  char* end = buffer + length;

  for (size_t i = 0; i < input_length;) {
    uint32_t sextet_a = GET_SEXTET(base64_input, i);
    uint32_t sextet_b = GET_SEXTET(base64_input, i);
    uint32_t sextet_c = GET_SEXTET(base64_input, i);
    uint32_t sextet_d = GET_SEXTET(base64_input, i);

    if (exception->status == FIFTYONE_DEGREES_STATUS_CORRUPT_DATA) {
      return 0;
    }

    uint32_t triple =
        (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;

    begin = safe_write_to_buffer(begin, end, (triple >> 16) & 0xFF, exception);
    begin = safe_write_to_buffer(begin, end, (triple >> 8) & 0xFF, exception);
    begin = safe_write_to_buffer(begin, end, triple & 0xFF, exception);
  }

  begin = safe_write_to_buffer(begin, end, '\0', exception);

  return begin - buffer;
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

static inline char* init_keys(char* const begin, const char* const end,
                              fiftyoneDegreesKeyValuePair* cache,
                              fiftyoneDegreesException* const exception) {
  char* ptr = begin;

  for (size_t k = 0; k < KEY_UNDEFINED; ++k) {
    cache[k].key = ptr;
    cache[k].keyLength = key_map[k].len;

    for (size_t i = 0; i < key_map[k].len; ++i) {
      ptr = safe_write_to_buffer(ptr, end, key_map[k].key[i], exception);
    }
  }

  return ptr;
}

static const char* init_parsing(const char* json, char** begin,
                                const char* const end,
                                fiftyoneDegreesKeyValuePair* cache,
                                fiftyoneDegreesException* const exception) {
  exception->status = FIFTYONE_DEGREES_STATUS_SUCCESS;

  *begin = init_keys(*begin, end, cache, exception);

  json = skip_whitespaces(json);
  ExpectSymbol(json, '{', return json);
  return skip_to_next_char(json, '"');
}

static Key read_ghev_key(const char** json,
                         fiftyoneDegreesException* const exception) {
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
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, '"');
                       return KEY_UNDEFINED);
        }

        return ARCHITECTURE;
      } break;

      case FULL_VERSION_LIST: {
        for (const char* i = "ullVersionList"; *i != '\0'; ++(*json), ++i) {
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, '"');
                       return KEY_UNDEFINED);
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
        ExpectSymbol(*json, 'o', *json = skip_to_next_char(*json, '"');
                     return KEY_UNDEFINED);

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
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, '"');
                       return KEY_UNDEFINED);
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
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, '"');
                       return KEY_UNDEFINED);
        }

        return BRANDS;
      } break;

      case READ_KEY_BITNESS: {
        for (const char* i = "tness"; *i; ++(*json), ++i) {
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, '"');
                       return KEY_UNDEFINED);
        }

        return BITNESS;
      } break;

      case READ_KEY_MOBILE: {
        for (const char* i = "ile"; *i; ++(*json), ++i) {
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, '"');
                       return KEY_UNDEFINED);
        }

        return MOBILE;
      } break;

      case READ_KEY_MODEL: {
        for (const char* i = "el"; *i; ++(*json), ++i) {
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, '"');
                       return KEY_UNDEFINED);
        }

        return MODEL;
      } break;

      case READ_KEY_PLATFORMVERSION: {
        for (const char* i = "ersion"; *i; ++(*json), ++i) {
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, '"');
                       return KEY_UNDEFINED);
        }

        return PLATFORMVERSION;
      } break;
    }
  }

  return KEY_UNDEFINED;
}

static Key read_sua_key(const char** json,
                        fiftyoneDegreesException* const exception) {
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
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, '"');
                       return KEY_UNDEFINED);
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
        ExpectSymbol(*json, 'o', *json = skip_to_next_char(*json, '"');
                     return KEY_UNDEFINED);

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
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, ':');
                       return KEY_UNDEFINED);
        }

        return **json == '\"' ? PLATFORM : KEY_UNDEFINED;
      } break;

      case READ_KEY_BROWSERS: {
        for (const char* i = "owsers"; *i; ++(*json), ++i) {
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, '"');
                       return KEY_UNDEFINED);
        }

        return FULLVERSIONLIST;
      } break;

      case READ_KEY_BITNESS: {
        for (const char* i = "tness"; *i; ++(*json), ++i) {
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, '"');
                       return KEY_UNDEFINED);
        }

        return BITNESS;
      } break;

      case READ_KEY_MOBILE: {
        for (const char* i = "ile"; *i; ++(*json), ++i) {
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, '"');
                       return KEY_UNDEFINED);
        }

        return MOBILE;
      } break;

      case READ_KEY_MODEL: {
        for (const char* i = "el"; *i; ++(*json), ++i) {
          ExpectSymbol(*json, *i, *json = skip_to_next_char(*json, '"');
                       return KEY_UNDEFINED);
        }

        return MODEL;
      } break;
    }
  }

  return KEY_UNDEFINED;
}

static char* read_string_value(const char** json, char* begin,
                               const char* const end,
                               fiftyoneDegreesException* const exception) {
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

  for (begin = safe_write_to_buffer(begin, end, '"', exception);; ++(*json)) {
    NotExpectSymbol(*json, '\0', return begin);

    begin = safe_write_to_buffer(begin, end, **json, exception);

    switch (**json) {
      case '\"': {
        ++(*json);
        return begin;
      }

      case '\\': {
        if ((*json)[1] == '\\' || (*json)[1] == '"') {
          ++(*json);

          NotExpectSymbol(*json, '\0', return begin);
          begin = safe_write_to_buffer(begin, end, **json, exception);
        }
      } break;
    }
  }
}

static char* read_bool_ghev_value(const char** json, char* begin,
                                  const char* const end,
                                  fiftyoneDegreesKeyValuePair* cache, Key key,
                                  fiftyoneDegreesException* const exception) {
  char* ptr = begin;

  switch (**json) {
    case 't': {
      ++(*json);
      for (const char* i = "rue"; *i != '\0'; ++(*json), ++i) {
        ExpectSymbol(*json, *i, return begin);
      }

      ptr = safe_write_to_buffer(ptr, end, '?', exception);
      ptr = safe_write_to_buffer(ptr, end, '1', exception);
    } break;

    case 'f': {
      ++(*json);
      for (const char* i = "alse"; *i != '\0'; ++(*json), ++i) {
        ExpectSymbol(*json, *i, return begin);
      }

      ptr = safe_write_to_buffer(ptr, end, '?', exception);
      ptr = safe_write_to_buffer(ptr, end, '0', exception);
    } break;

    default: {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return begin;
    } break;
  }

  cache[key].value = ValuePtr;
  cache[key].valueLength = ptr - begin;

  return ptr;
}

static char* read_bool_sua_value(const char** json, char* begin,
                                 const char* const end,
                                 fiftyoneDegreesKeyValuePair* cache, Key key,
                                 fiftyoneDegreesException* const exception) {
  switch (**json) {
    case '0':
    case '1': {
    } break;

    default: {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return begin;
    } break;
  }

  char* ptr = safe_write_to_buffer(begin, end, '?', exception);
  ptr = safe_write_to_buffer(ptr, end, **json, exception);

  ++(*json);

  cache[key].value = ValuePtr;
  cache[key].valueLength = ptr - begin;

  return ptr;
}

static char* read_version_sua(const char** json, char* begin,
                              const char* const end,
                              fiftyoneDegreesException* const exception) {
  enum version_state {
    version_read,
    version_skip,
    version_exit,
  } state = version_skip;

  for (char* ptr = begin;; ++(*json)) {
    NotExpectSymbol(*json, '\0', return begin);

    switch (state) {
      case version_read: {
        switch (**json) {
          case '\"': {
            state = version_skip;
          } break;

          case '\\': {
            ptr = safe_write_to_buffer(ptr, end, **json, exception);

            if ((*json)[1] == '\\' || (*json)[1] == '"') {
              ++(*json);
              NotExpectSymbol(*json, '\0', return begin);

              ptr = safe_write_to_buffer(ptr, end, **json, exception);
            }
          } break;

          default: {
            ptr = safe_write_to_buffer(ptr, end, **json, exception);
          } break;
        }
      } break;

      case version_skip: {
        switch (**json) {
          case '"': {
            state = version_read;
          } break;

          case ',': {
            ptr = safe_write_to_buffer(ptr, end, '.', exception);
          } break;

          case ']': {
            state = version_exit;
          } break;
        }
      } break;

      case version_exit: {
        ptr = safe_write_to_buffer(ptr, end, '"', exception);
        return ptr;
      } break;
    }
  }
}

static char* read_brands_ghev_value(const char** json, char* begin,
                                    const char* const end,
                                    fiftyoneDegreesKeyValuePair* cache, Key key,
                                    fiftyoneDegreesException* const exception) {
  *json = skip_to_next_char(*json, '[');
  ExpectSymbol(*json, '[', return begin);

  ++*json;

  for (char* ptr = begin;; ++*json) {
    NotExpectSymbol(*json, '\0', return begin);

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
    NotExpectSymbol(*json, '\0', return begin);

    char* ptr2 = read_string_value(json, ptr, end, exception);
    if (ptr2 != NULL) {
      ptr = safe_write_to_buffer(ptr2, end, ';', exception);
      ptr = safe_write_to_buffer(ptr, end, 'v', exception);
      ptr = safe_write_to_buffer(ptr, end, '=', exception);

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

      ++*json;

      ptr2 = read_string_value(json, ptr, end, exception);
      if (ptr2 == NULL) {
        ptr2 = ptr;

        ptr = safe_write_to_buffer(ptr, end, 'n', exception);
        ptr = safe_write_to_buffer(ptr, end, 'u', exception);
        ptr = safe_write_to_buffer(ptr, end, 'l', exception);
        ptr = safe_write_to_buffer(ptr, end, 'l', exception);
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
          ptr = safe_write_to_buffer(ptr, end, ',', exception);
          ptr = safe_write_to_buffer(ptr, end, ' ', exception);
        }
      } break;

      default: {
        exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
        return begin;
      } break;
    }
  }
}

static char* read_brands_sua_value(const char** json, char* begin,
                                   const char* const end,
                                   fiftyoneDegreesKeyValuePair* cache, Key key,
                                   fiftyoneDegreesException* const exception) {
  *json = skip_to_next_char(*json, '[');
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

    char* ptr2 = read_string_value(json, ptr, end, exception);
    if (ptr2 != NULL) {
      ptr = safe_write_to_buffer(ptr2, end, ';', exception);
      ptr = safe_write_to_buffer(ptr, end, 'v', exception);
      ptr = safe_write_to_buffer(ptr, end, '=', exception);

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

      ptr = safe_write_to_buffer(ptr, end, '"', exception);
      ptr = read_version_sua(json, ptr, end, exception);
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
          ptr = safe_write_to_buffer(ptr, end, ',', exception);
          ptr = safe_write_to_buffer(ptr, end, ' ', exception);
        }
      } break;

      default: {
        exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
        return begin;
      } break;
    }
  }
}

static char* read_pure_string_value(const char** json, char* begin,
                                    const char* const end,
                                    fiftyoneDegreesKeyValuePair* cache, Key key,
                                    fiftyoneDegreesException* const exception) {
  char* ptr = read_string_value(json, begin, end, exception);

  if (ptr != NULL) {
    cache[key].value = ValuePtr;
    cache[key].valueLength = ptr - begin;
  }

  return ptr;
}

static char* read_platform_sua_value(
    const char** json, char* begin, const char* const end,
    fiftyoneDegreesKeyValuePair* cache, Key key,
    fiftyoneDegreesException* const exception) {
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

  char* ptr = read_string_value(json, begin, end, exception);
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

  ptr = safe_write_to_buffer(ptr, end, '"', exception);
  ptr = read_version_sua(json, ptr, end, exception);

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

static bool pushToHeaders(void* ctx, fiftyoneDegreesKeyValuePair header,
                          fiftyoneDegreesException* const exception) {
  fiftyoneDegreesKeyValuePairArray* const headers =
      (fiftyoneDegreesKeyValuePairArray* const)ctx;

  if (headers->count < headers->capacity) {
    fiftyoneDegreesKeyValuePair* pair = headers->items + headers->count++;

    pair->key = header.key;
    pair->keyLength = header.keyLength;
    pair->value = header.value;
    pair->valueLength = header.valueLength;
  }
  else {
      // TODO set exception in the else clause
      exception;
  }
  
  exception;
  return (headers->count < headers->capacity);
}
// ------------------------------------------------------------------------------------------------
static size_t main_parsing_body(const char* json, char* const buffer,
                                size_t length,
                                fiftyoneDegreesException* const exception,
                                int isSua,
                                fiftyoneDegreesTransformCallback callback,
                                void* ctx) {
  fiftyoneDegreesKeyValuePair cache[KEY_UNDEFINED];

  // define buffer range
  char* begin = buffer;
  const char* const end = buffer + length;

  // write keys to buffer, init cache and skip to the first key
  json = init_parsing(json, &begin, end, cache, exception);
  if (exception->status == FIFTYONE_DEGREES_STATUS_CORRUPT_DATA ||
      exception->status == FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY) {
    return 0;
  }

  size_t iterations = 0;  // total number of parsed key-value pairs

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

    char* ptr = read_value(&json, begin, end, cache, key, exception);
    if (exception->status == FIFTYONE_DEGREES_STATUS_CORRUPT_DATA) {
      break;
    }

    if (ptr != NULL) {
      begin = ptr;

      ++iterations;
      if (!callback(ctx, cache[key], exception)) {
        return iterations;
      }

      if (key == PLATFORM && isSua && cache[PLATFORMVERSION].valueLength != 0) {
        ++iterations;
        if (!callback(ctx, cache[PLATFORMVERSION], exception)) {
          return iterations;
        }
      }
    }

    json = skip_to_next_char(json, '"');
  }

  return iterations;
}

// ------------------------------------------------------------------------------------------------
size_t fiftyoneDegreesTransformIterateGhevFromJson(
    const char* json, char* const buffer, size_t length,
    fiftyoneDegreesException* const exception,
    fiftyoneDegreesTransformCallback callback, void* ctx) {
  return main_parsing_body(json, buffer, length, exception, 0, callback, ctx);
}

size_t fiftyoneDegreesTransformIterateGhevFromBase64(
    const char* base64, char* buffer, size_t length,
    fiftyoneDegreesException* const exception,
    fiftyoneDegreesTransformCallback callback, void* ctx) {
  size_t offset = base64_decode(base64, buffer, length, exception);

  if (exception->status == FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY ||
      exception->status == FIFTYONE_DEGREES_STATUS_CORRUPT_DATA) {
    return 0;
  }

  return fiftyoneDegreesTransformIterateGhevFromJson(
      buffer, buffer + offset, length > offset ? length - offset : 0, exception,
      callback, ctx);
}

size_t fiftyoneDegreesTransformIterateSua(
    const char* json, char* const buffer, size_t length,
    fiftyoneDegreesException* const exception,
    fiftyoneDegreesTransformCallback callback, void* ctx) {
  return main_parsing_body(json, buffer, length, exception, 1, callback, ctx);
}

size_t fiftyoneDegreesTransformGhevFromJson(
    const char* json, char* buffer, size_t length,
    fiftyoneDegreesException* const exception,
    fiftyoneDegreesKeyValuePairArray* const headers) {
  size_t calls = fiftyoneDegreesTransformIterateGhevFromJson(
      json, buffer, length, exception, pushToHeaders, headers);

  if (calls != headers->count) {
    exception->status = FIFTYONE_DEGREES_STATUS_INSUFFICIENT_CAPACITY;
  }

  return calls;
}

size_t fiftyoneDegreesTransformGhevFromBase64(
    const char* base64, char* buffer, size_t length,
    fiftyoneDegreesException* const exception,
    fiftyoneDegreesKeyValuePairArray* const headers) {
  size_t calls = fiftyoneDegreesTransformIterateGhevFromBase64(
      base64, buffer, length, exception, pushToHeaders, headers);

  if (calls != headers->count) {
    exception->status = FIFTYONE_DEGREES_STATUS_INSUFFICIENT_CAPACITY;
  }

  return calls;
}

size_t fiftyoneDegreesTransformSua(
    const char* json, char* buffer, size_t length,
    fiftyoneDegreesException* const exception,
    fiftyoneDegreesKeyValuePairArray* const headers) {
  size_t calls = fiftyoneDegreesTransformIterateSua(
      json, buffer, length, exception, pushToHeaders, headers);

  if (calls != headers->count) {
    exception->status = FIFTYONE_DEGREES_STATUS_INSUFFICIENT_CAPACITY;
  }

  return calls;
}
