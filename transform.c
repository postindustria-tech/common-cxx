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

static const char keys_src[] =
    "sec-ch-ua-arch"
    "sec-ch-ua"
    "sec-ch-ua-bitness"
    "sec-ch-ua-full-version-list"
    "sec-ch-ua-mobile"
    "sec-ch-ua-model"
    "sec-ch-ua-platform"
    "sec-ch-ua-platform-version";

enum Key {
  ARCHITECTURE,     //
  BRANDS,           //
  BITNESS,          //
  FULLVERSIONLIST,  //
  MOBILE,           //
  MODEL,            //
  PLATFORM,         //
  PLATFORMVERSION,  //
  KEY_UNDEFINED,    //
};

// ----

static inline size_t safe_write_to_buffer(
    char* buffer, size_t length, size_t offset, char symbol,
    fiftyoneDegreesException* const exception) {
  if (offset < length) {
    buffer[offset] = symbol;
  } else {
    exception->status = FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY;
  }

  return ++offset;
}

static inline int base64_char_to_value(char c) {
  switch (c) {
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z': {
      return c - 'A';
    } break;

    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z': {
      return c - 'a' + 26;
    } break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      return c - '0' + 52;
    } break;

    case '+': {
      return 62;
    } break;

    case '/': {
      return 63;
    } break;

    default: {
      return -1;
    } break;
  }
}

static size_t base64_decode(const char* base64_input, char* const buffer,
                            size_t length,
                            fiftyoneDegreesException* const exception) {
  size_t input_length = strlen(base64_input);
  if (input_length % 4 != 0) {
    exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
    return 0;  // Invalid base64 input length
  }

  size_t res = input_length / 4 * 3;

  if (base64_input[input_length - 1] == '=') {
    res--;
  }

  if (base64_input[input_length - 2] == '=') {
    res--;
  }

  for (size_t i = 0, j = 0; i < input_length;) {
    int sextet_a = base64_input[i] == '='
                       ? 0 & i++
                       : base64_char_to_value(base64_input[i++]);

    int sextet_b = base64_input[i] == '='
                       ? 0 & i++
                       : base64_char_to_value(base64_input[i++]);

    int sextet_c = base64_input[i] == '='
                       ? 0 & i++
                       : base64_char_to_value(base64_input[i++]);

    int sextet_d = base64_input[i] == '='
                       ? 0 & i++
                       : base64_char_to_value(base64_input[i++]);

    int triple =
        (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;

    j = safe_write_to_buffer(buffer, length, j, (triple >> 16) & 0xFF,
                             exception);
    j = safe_write_to_buffer(buffer, length, j, (triple >> 8) & 0xFF,
                             exception);
    j = safe_write_to_buffer(buffer, length, j, triple & 0xFF, exception);
  }

  return safe_write_to_buffer(buffer, length, res, '\0', exception);
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

static int skip_value(const char** json) {
  *json = skip_whitespaces(*json);

  switch (**json) {
    case '\0': {
      return 0;
    } break;

    case '{': {  // skip nested object
      ++(*json);

      for (int nesting_level = 1; nesting_level > 0; ++(*json)) {
        switch (**json) {
          case '\0': {
            return 0;
          } break;

          case '{': {
            ++nesting_level;
          } break;

          case '}': {
            --nesting_level;
          } break;

          case '"': {
            *json = skip_to_next_char(*json + 1, '"');
            if (**json == '\0') {
              return 0;
            }
          } break;
        }
      }

      *json = skip_to_next_char(*json, ',');
    } break;

    case '[': {
      ++(*json);

      for (int nesting_level = 1; nesting_level > 0; ++(*json)) {
        switch (**json) {
          case '\0': {
            return 0;
          } break;

          case '[': {
            ++nesting_level;
          } break;

          case ']': {
            --nesting_level;
          } break;

          case '"': {
            *json = skip_to_next_char(*json + 1, '"');
            if (**json == '\0') {
              return 0;
            }
          } break;
        }
      }

      *json = skip_to_next_char(*json, ',');
    } break;

    case '"': {
      *json = skip_to_next_char(*json + 1, '"');
      *json = skip_to_next_char(*json, ',');
    } break;

    default: {
      for (int flag = 1; flag; ++(*json)) {
        switch (**json) {
          case '\0': {
            return 0;
          } break;

          case ',': {
            *json = skip_to_next_char(*json + 1, '"');
            return **json != '\0';
          } break;

          case '}':
          case ']': {
            flag = 0;
          } break;
        }
      }
    } break;
  }

  if (**json == '\0') {
    return 0;
  }

  *json = skip_to_next_char(*json + 1, '"');

  return **json != '\0';
}

static enum Key read_ghev_key(const char** json) {
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

  for (enum ReadKeyState state = READ_KEY_INIT; *(*json); ++(*json)) {
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
        for (const char* i = "rchitecture"; *i; ++(*json), ++i) {
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
        }

        return ARCHITECTURE;
      } break;

      case FULL_VERSION_LIST: {
        for (const char* i = "ullVersionList"; *i; ++(*json), ++i) {
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
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
        if (**json != 'o') {
          *json = skip_to_next_char(*json, '"');
          return KEY_UNDEFINED;
        }

        ++(*json);
        if (**json == '\0') {
          return KEY_UNDEFINED;
        }

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
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
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
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
        }

        return BRANDS;
      } break;

      case READ_KEY_BITNESS: {
        for (const char* i = "tness"; *i; ++(*json), ++i) {
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
        }

        return BITNESS;
      } break;

      case READ_KEY_MOBILE: {
        for (const char* i = "ile"; *i; ++(*json), ++i) {
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
        }

        return MOBILE;
      } break;

      case READ_KEY_MODEL: {
        for (const char* i = "el"; *i; ++(*json), ++i) {
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
        }

        return MODEL;
      } break;

      case READ_KEY_PLATFORMVERSION: {
        for (const char* i = "ersion"; *i; ++(*json), ++i) {
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
        }

        return PLATFORMVERSION;
      } break;
    }
  }

  return KEY_UNDEFINED;
}

static enum Key read_sua_key(const char** json) {
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

  for (enum ReadKeyState state = READ_KEY_INIT; *(*json); ++(*json)) {
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
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
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
        if (**json != 'o') {
          *json = skip_to_next_char(*json, '"');
          return KEY_UNDEFINED;
        }

        ++(*json);
        if (**json == '\0') {
          return KEY_UNDEFINED;
        }

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
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
        }

        return PLATFORM;
      } break;

      case READ_KEY_BROWSERS: {
        for (const char* i = "owsers"; *i; ++(*json), ++i) {
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
        }

        return FULLVERSIONLIST;
      } break;

      case READ_KEY_BITNESS: {
        for (const char* i = "tness"; *i; ++(*json), ++i) {
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
        }

        return BITNESS;
      } break;

      case READ_KEY_MOBILE: {
        for (const char* i = "ile"; *i; ++(*json), ++i) {
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
        }

        return MOBILE;
      } break;

      case READ_KEY_MODEL: {
        for (const char* i = "el"; *i; ++(*json), ++i) {
          if (**json != *i) {
            *json = skip_to_next_char(*json, '"');
            return KEY_UNDEFINED;
          }
        }

        return MODEL;
      } break;
    }
  }

  return KEY_UNDEFINED;
}

static size_t read_version_sua(const char** json, char* buffer, size_t length) {
  enum version_state {
    version_read,
    version_skip,
    version_exit,
  } state = version_skip;

  for (size_t offset = 0;; ++(*json)) {
    if (**json == '\0') {
      return offset;
    }

    switch (state) {
      case version_read: {
        switch (**json) {
          case '\"': {
            state = version_skip;
          } break;

          case '\\': {
            if ((*json)[1] == '\\' || (*json)[1] == '"') {
              ++(*json);

              if (**json == '\0') {
                return offset;
              }

              buffer[offset++] = **json;

              if (offset >= length) {
                return offset;
              }
            }
          } break;

          default: {
            buffer[offset++] = **json;

            if (offset >= length) {
              return offset;
            }
          } break;
        }
      } break;

      case version_skip: {
        switch (**json) {
          case '"': {
            state = version_read;
          } break;

          case ',': {
            buffer[offset++] = '.';
            if (offset >= length) {
              return offset;
            }
          } break;

          case ']': {
            state = version_exit;
          } break;
        }
      } break;

      case version_exit: {
        return offset;
      } break;
    }
  }
}

static size_t read_string_value(const char** json, char* buffer, size_t length,
                                fiftyoneDegreesException* const exception) {
  *json = skip_to_next_char(*json, '"');
  if (**json == '\0') {
    exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
    return 0;
  }
  ++*json;

  for (size_t offset = safe_write_to_buffer(buffer, length, 0, '"', exception);;
       ++(*json)) {
    if (**json == '\0') {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return offset;
    }

    offset = safe_write_to_buffer(buffer, length, offset, **json, exception);

    switch (**json) {
      case '\"': {
        return offset;
      }

      case '\\': {
        if ((*json)[1] == '\\' || (*json)[1] == '"') {
          ++(*json);

          if (**json == '\0') {
            exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
            return offset;
          }

          offset =
              safe_write_to_buffer(buffer, length, offset, **json, exception);
        }
      } break;
    }
  }
}

static size_t read_brands_ghev_value(
    const char** json, char* buffer, size_t length,
    fiftyoneDegreesException* const exception) {
  *json = skip_to_next_char(*json, '[');
  if (**json == '\0') {
    exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
    return 0;
  }
  ++*json;

  for (size_t offset = 0;; ++*json) {
    if (**json == '\0') {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return offset;
    }

    *json = skip_to_next_char(*json, '{');
    if (**json == '\0') {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return offset;
    }

    *json = skip_to_next_char(*json + 1, '"');
    if (**json == '\0') {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return offset;
    }
    ++*json;

    for (const char* k = "brand\""; *k != '\0'; ++k, ++*json) {
      if (*k != **json) {
        exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
        return offset;
      }
    }

    *json = skip_to_next_char(*json, ':');
    if (**json == '\0') {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return offset;
    }
    ++*json;

    if (**json == '\0') {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return offset;
    }

    offset +=
        read_string_value(json, buffer + offset,
                          length > offset ? length - offset : 0, exception);

    offset = safe_write_to_buffer(buffer, length, offset, ';', exception);
    offset = safe_write_to_buffer(buffer, length, offset, 'v', exception);
    offset = safe_write_to_buffer(buffer, length, offset, '=', exception);

    *json = skip_to_next_char(*json, ',');
    if (**json == '\0') {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return offset;
    }

    *json = skip_to_next_char(*json + 1, '"');
    if (**json == '\0') {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return offset;
    }
    ++*json;

    for (const char* k = "version\""; *k != '\0'; ++k, ++*json) {
      if (*k != **json) {
        exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
        return offset;
      }
    }

    *json = skip_to_next_char(*json, ':');
    if (**json == '\0') {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return offset;
    }
    ++*json;

    if (**json == '\0') {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return offset;
    }

    offset +=
        read_string_value(json, buffer + offset,
                          length > offset ? length - offset : 0, exception);

    *json = skip_to_next_char(*json, '}');
    if (**json == '\0') {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return offset;
    }
    ++*json;
    if (**json == '\0') {
      exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
      return offset;
    }

    *json = skip_whitespaces(*json);

    switch (**json) {
      case '\0': {
        exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
        return offset;
      } break;

      case ']': {
        offset = safe_write_to_buffer(buffer, length, offset, '\n', exception);
        return offset;
      } break;

      case ',': {
        offset = safe_write_to_buffer(buffer, length, offset, ',', exception);
      } break;
    }
  }
}

static size_t read_brands_sua_value(const char** json, char* buffer,
                                    size_t length,
                                    fiftyoneDegreesException* const exception) {
  size_t offset = 0;

  *json = skip_to_next_char(*json, '[') + 1;

  for (; **json; ++*json) {
    *json = skip_to_next_char(*json, '{') + 1;
    *json = skip_to_next_char(*json, '"') + 1;

    for (const char* k = "brand\""; **json && *k; ++k, ++*json) {
      if (*k != **json) {
        return offset;
      }
    }

    *json = skip_to_next_char(*json, ':') + 1;

    if (**json == '\0') {
      return offset;
    }

    offset +=
        read_string_value(json, buffer + offset,
                          length > offset ? length - offset : 0, exception);

    if (length - offset < 4) {
      return offset;
    }

    buffer[offset++] = ';';
    buffer[offset++] = 'v';
    buffer[offset++] = '=';

    *json = skip_to_next_char(*json, ',') + 1;
    *json = skip_to_next_char(*json, '"') + 1;

    for (const char* k = "version\""; **json && *k; ++k, ++*json) {
      if (*k != **json) {
        return offset;
      }
    }

    *json = skip_to_next_char(*json, ':') + 1;

    if (**json == '\0') {
      return offset;
    }

    *json = skip_to_next_char(*json, '[') + 1;
    if (**json == '\0') {
      return offset;
    }

    buffer[offset++] = '"';
    if (offset >= length) {
      return offset;
    }

    offset += read_version_sua(json, buffer + offset,
                               length > offset ? length - offset : 0);

    if (offset >= length) {
      return offset;
    }

    buffer[offset++] = '"';

    if (offset >= length) {
      return offset;
    }

    *json = skip_to_next_char(*json, '}') + 1;

    *json = skip_whitespaces(*json);

    switch (**json) {
      case '\0':
      case ']': {
        if (offset < length) {
          buffer[offset++] = '\n';
        }

        return offset;
      } break;

      case ',': {
        buffer[offset++] = ',';

        if (offset >= length) {
          return offset;
        }
      } break;
    }
  }

  return offset;
}

static size_t read_platform_sua_value(
    const char** json, char* buffer, size_t length, char** platform,
    size_t* platform_size, char** version, size_t* version_size,
    fiftyoneDegreesException* const exception) {
  size_t offset = 0;

  *json = skip_to_next_char(*json, '{') + 1;
  *json = skip_to_next_char(*json, '"') + 1;

  for (const char* k = "brand\""; **json && *k; ++k, ++*json) {
    if (*k != **json) {
      return offset;
    }
  }

  *json = skip_to_next_char(*json, ':') + 1;

  if (**json == '\0') {
    return offset;
  }

  *platform = buffer;
  offset += read_string_value(json, buffer + offset,
                              length > offset ? length - offset : 0, exception);

  if (offset >= length) {
    return offset;
  }

  buffer[offset++] = '\n';
  *platform_size = offset;

  *version = buffer + offset;

  *json = skip_to_next_char(*json, ',') + 1;
  *json = skip_to_next_char(*json, '"') + 1;

  for (const char* k = "version\""; **json && *k; ++k, ++*json) {
    if (*k != **json) {
      return offset;
    }
  }

  *json = skip_to_next_char(*json, ':') + 1;

  if (**json == '\0') {
    return offset;
  }

  *json = skip_to_next_char(*json, '[') + 1;

  buffer[offset++] = '"';
  if (offset >= length) {
    return offset;
  }

  offset += read_version_sua(json, buffer + offset,
                             length > offset ? length - offset : 0);

  if (offset >= length) {
    return offset;
  }

  buffer[offset++] = '"';

  if (offset >= length) {
    return offset;
  }

  buffer[offset++] = '\n';
  *version_size = offset - *platform_size;

  return offset;
}

static int read_bool_ghev_value(const char** json) {
  *json = skip_whitespaces(*json);

  if (**json != ':') {
    return -1;
  }

  *json = skip_whitespaces(*json + 1);

  switch (**json) {
    case 't': {
      ++(*json);
      for (const char* i = "rue\0"; *i; ++(*json), ++i) {
        if (**json != *i) {
          return -1;
        }
      }

      return 1;
    }

    case 'f': {
      ++(*json);
      for (const char* i = "alse\0"; *i; ++(*json), ++i) {
        if (**json != *i) {
          return -1;
        }
      }

      return 0;
    }
  }

  return -1;
}

static int read_bool_sua_value(const char** json) {
  *json = skip_whitespaces(*json);

  if (**json != ':') {
    return -1;
  }

  *json = skip_whitespaces(*json + 1);

  switch (**json) {
    case '0': {
      ++(*json);

      return 0;
    }

    case '1': {
      ++(*json);

      return 1;
    }
  }

  return -1;
}

static inline size_t init_keys(char* const buffer, size_t length,
                               fiftyoneDegreesException* const exception) {
  for (size_t offset = 0; offset < sizeof(keys_src) - 1;
       offset = safe_write_to_buffer(buffer, length, offset, keys_src[offset],
                                     exception));
  return sizeof(keys_src) - 1;
}

// ------------------------------------------------------------------------------------------------

size_t fiftyoneDegreesTransformIterateGhevFromJson(
    const char* json, char* const buffer, size_t length,
    fiftyoneDegreesException* const exception,
    fiftyoneDegreesTransformCallback callback) {
  exception->status = FIFTYONE_DEGREES_STATUS_SUCCESS;

  size_t offset = init_keys(buffer, length, exception);

  fiftyoneDegreesKeyValuePair out_pairs[8] = {
      {NULL, sizeof("sec-ch-ua-arch") - 1, NULL, 0},
      {NULL, sizeof("sec-ch-ua") - 1, NULL, 0},
      {NULL, sizeof("sec-ch-ua-bitness") - 1, NULL, 0},
      {NULL, sizeof("sec-ch-ua-full-version-list") - 1, NULL, 0},
      {NULL, sizeof("sec-ch-ua-mobile") - 1, NULL, 0},
      {NULL, sizeof("sec-ch-ua-model") - 1, NULL, 0},
      {NULL, sizeof("sec-ch-ua-platform") - 1, NULL, 0},
      {NULL, sizeof("sec-ch-ua-platform-version") - 1, NULL, 0},
  };

  for (int i = 0, j = 0; i < 8; ++i) {
    out_pairs[i].key = buffer + j;
    j += out_pairs[i].keyLength;
  }

  size_t iterations = 0;

  enum State { INIT, READ_KEY, SKIP_VALUE };

  for (enum State state = INIT; '\0' != *json; ++json) {
    switch (state) {
      case INIT: {
        json = skip_whitespaces(json);

        if (*json != '{') {
          exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
          return 0;
        }

        json = skip_to_next_char(json, '"');
        if (*json == '\0') {
          return 0;
        }

        state = READ_KEY;
      } break;

      case READ_KEY: {
        enum Key key = read_ghev_key(&json);

        if (*json == '\0') {
          exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
          return iterations;
        }

        json = skip_whitespaces(json + 1);

        if (*json != ':') {
          exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
          return iterations;
        }

        json = skip_whitespaces(json);
        if (*json == '\0') {
          exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
          return iterations;
        }

        switch (key) {
          case KEY_UNDEFINED: {
            state = SKIP_VALUE;
          } break;

          case FULLVERSIONLIST:
          case BRANDS: {
            size_t vlen = read_brands_ghev_value(
                &json, buffer + offset, length > offset ? length - offset : 0,
                exception);

            if (exception->status == FIFTYONE_DEGREES_STATUS_CORRUPT_DATA) {
              return iterations;
            }

            if (exception->status !=
                FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY) {
              out_pairs[key].value = buffer + offset;
            }
            offset += vlen;

            out_pairs[key].valueLength = vlen;

            callback(out_pairs[key]);
            ++iterations;

            json = skip_to_next_char(json + 1, '"');
            if (*json == '\0') {
              return iterations;
            }

            state = READ_KEY;
          } break;

          case MOBILE: {
            int b = read_bool_ghev_value(&json);

            if (b == -1) {
              exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
              return iterations;
            }

            size_t tmp = offset;

            offset =
                safe_write_to_buffer(buffer, length, offset, '?', exception);
            offset = safe_write_to_buffer(buffer, length, offset, b ? '1' : '0',
                                          exception);
            offset =
                safe_write_to_buffer(buffer, length, offset, '\n', exception);

            if (exception->status !=
                FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY) {
              out_pairs[key].value = buffer + tmp;
            }
            out_pairs[key].valueLength = 3;

            callback(out_pairs[key]);
            ++iterations;

            json = skip_to_next_char(json, '"');
            if (*json == '\0') {
              return iterations;
            }

            state = READ_KEY;
          } break;

          case ARCHITECTURE:
          case BITNESS:
          case MODEL:
          case PLATFORM:
          case PLATFORMVERSION: {
            size_t vlen = read_string_value(
                &json, buffer + offset, length > offset ? length - offset : 0,
                exception);

            if (exception->status == FIFTYONE_DEGREES_STATUS_CORRUPT_DATA) {
              return iterations;
            }

            if (exception->status !=
                FIFTYONE_DEGREES_STATUS_INSUFFICIENT_MEMORY) {
              out_pairs[key].value = buffer + offset;
            }

            offset += vlen;
            offset =
                safe_write_to_buffer(buffer, length, offset, '\n', exception);
            out_pairs[key].valueLength = vlen + 1;

            callback(out_pairs[key]);
            ++iterations;

            json = skip_to_next_char(json + 1, '"');
            if (*json == '\0') {
              return iterations;
            }

            state = READ_KEY;
          } break;
        }
      } break;

      case SKIP_VALUE: {
        if (!skip_value(&json)) {
          exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
          return iterations;
        }

        state = READ_KEY;
      } break;
    }
  }

  return iterations;
}

size_t fiftyoneDegreesTransformIterateGhevFromBase64(
    const char* base64, char* buffer, size_t length,
    fiftyoneDegreesException* const exception,
    fiftyoneDegreesTransformCallback callback) {
  size_t offset = base64_decode(base64, buffer, length, exception);

  if (exception->status == FIFTYONE_DEGREES_STATUS_CORRUPT_DATA) {
    return 0;
  }

  return fiftyoneDegreesTransformIterateGhevFromJson(
      buffer, buffer + offset, length > offset ? length - offset : 0, exception,
      callback);
}

size_t fiftyoneDegreesTransformIterateSua(
    const char* json, char* const buffer, size_t length,
    fiftyoneDegreesException* const exception,
    fiftyoneDegreesTransformCallback callback) {
  enum States {
    INIT,        //
    READ_KEY,    //
    SKIP_VALUE,  //
  };

  static struct OutKey {
    char* key;
    size_t length;
  } out_keys[] = {
      {NULL, sizeof("sec-ch-ua-arch") - 1},
      {NULL, sizeof("sec-ch-ua") - 1},
      {NULL, sizeof("sec-ch-ua-bitness") - 1},
      {NULL, sizeof("sec-ch-ua-full-version-list") - 1},
      {NULL, sizeof("sec-ch-ua-mobile") - 1},
      {NULL, sizeof("sec-ch-ua-model") - 1},
      {NULL, sizeof("sec-ch-ua-platform") - 1},
      {NULL, sizeof("sec-ch-ua-platform-version") - 1},
  };

  size_t iterations = 0;
  size_t offset = sizeof(keys_src) - 1;

  if (offset >= length) {
    return iterations;
  }

  for (enum States state = INIT; *json; ++json) {
    switch (state) {
      case INIT: {
        json = skip_whitespaces(json);

        if (*json != '{') {
          return iterations;
        }

        for (size_t i = 0; i < offset; ++i) {
          if (length <= offset) {
            return iterations;
          }

          buffer[i] = keys_src[i];
        }

        for (int i = 0, j = 0; i < 8; ++i) {
          out_keys[i].key = buffer + j;
          j += out_keys[i].length;
        }

        json = skip_to_next_char(json, '"');
        if (*json == '\0') {
          return iterations;
        }
        state = READ_KEY;
      } break;

      case READ_KEY: {
        enum Key key = read_sua_key(&json);
        json = skip_whitespaces(json + 1);

        if (*json != ':') {
          return iterations;
        }

        json = skip_whitespaces(json);
        if (*json == '\0') {
          return iterations;
        }

        switch (key) {
          case KEY_UNDEFINED: {
            state = SKIP_VALUE;
          } break;

          case FULLVERSIONLIST: {
            size_t vlen = read_brands_sua_value(
                &json, buffer + offset, length > offset ? length - offset : 0,
                exception);

            if (vlen == 0) {
              state = SKIP_VALUE;
              break;
            }

            if (*json == '\0') {
              return iterations;
            }

            fiftyoneDegreesKeyValuePair key_value_pair = {
                .key = out_keys[key].key,           //
                .keyLength = out_keys[key].length,  //
                .value = buffer + offset,           //
                .valueLength = vlen,                //
            };

            callback(key_value_pair);
            ++iterations;

            offset += vlen;
            if (offset >= length) {
              return iterations;
            }

            json = skip_to_next_char(json + 1, '"');

            if (*json == '\0') {
              return iterations;
            }

            state = READ_KEY;
          } break;

          case MOBILE: {
            int b = read_bool_sua_value(&json);

            if (b == -1) {
              state = SKIP_VALUE;
              break;
            }

            offset =
                safe_write_to_buffer(buffer, length, offset, '?', exception);
            offset = safe_write_to_buffer(buffer, length, offset, b ? '1' : '0',
                                          exception);
            offset =
                safe_write_to_buffer(buffer, length, offset, '\n', exception);

            fiftyoneDegreesKeyValuePair key_value_pair = {
                .key = out_keys[key].key,           //
                .keyLength = out_keys[key].length,  //
                .value = buffer + offset,           //
                .valueLength = 3                    //
            };

            callback(key_value_pair);
            ++iterations;

            offset += 3;

            if (offset >= length) {
              return iterations;
            }

            json = skip_to_next_char(json, '"');

            if (*json == '\0') {
              return iterations;
            }

            state = READ_KEY;
          } break;

          case BRANDS: {
            // not implemented in sua. reading PLATFORM instead
          } break;

          case PLATFORMVERSION:
          case PLATFORM: {
            fiftyoneDegreesKeyValuePair platform = {
                .key = out_keys[PLATFORM].key,           //
                .keyLength = out_keys[PLATFORM].length,  //
                .value = NULL,                           //
                .valueLength = 0,                        //
            };

            fiftyoneDegreesKeyValuePair version = {
                .key = out_keys[PLATFORMVERSION].key,           //
                .keyLength = out_keys[PLATFORMVERSION].length,  //
                .value = NULL,                                  //
                .valueLength = 0,                               //
            };

            size_t vlen = read_platform_sua_value(
                &json, buffer + offset, length > offset ? length - offset : 0,
                &platform.value, &platform.valueLength, &version.value,
                &version.valueLength, exception);

            if (*json == '\0') {
              return iterations;
            }

            if (vlen == 0) {
              state = SKIP_VALUE;
              break;
            }

            callback(platform);
            callback(version);

            iterations += 2;

            offset += vlen;
            if (offset >= length) {
              return iterations;
            }

            json = skip_to_next_char(json + 1, '"');

            if (*json == '\0') {
              return iterations;
            }

            state = READ_KEY;
          } break;

          case ARCHITECTURE:
          case BITNESS:
          case MODEL: {
            size_t vlen = read_string_value(
                &json, buffer + offset, length > offset ? length - offset : 0,
                exception);

            if (offset + vlen < length) {
              buffer[offset + vlen] = '\n';
              ++vlen;
            }

            if (*json == '\0') {
              return iterations;
            }

            fiftyoneDegreesKeyValuePair key_value_pair = {
                .key = out_keys[key].key,           //
                .keyLength = out_keys[key].length,  //
                .value = buffer + offset,           //
                .valueLength = vlen,                //
            };

            callback(key_value_pair);
            ++iterations;

            offset += vlen;
            if (offset >= length) {
              return iterations;
            }

            json = skip_to_next_char(json + 1, '"');

            if (*json == '\0') {
              return iterations;
            }

            state = READ_KEY;
          } break;
        }
      } break;

      case SKIP_VALUE: {
        if (!skip_value(&json)) {
          return iterations;
        }

        state = READ_KEY;
      } break;
    }
  }

  return ++iterations;
}

size_t fiftyoneDegreesTransformGhevFromJson(
    const char* json, char* buffer, size_t length,
    fiftyoneDegreesException* const exception,
    fiftyoneDegreesKeyValuePairArray* const headers) {
  if (headers->capacity < 8) {
    exception->status = FIFTYONE_DEGREES_STATUS_INSUFFICIENT_CAPACITY;
    return 0;
  }

  exception->status = FIFTYONE_DEGREES_STATUS_SUCCESS;

  size_t offset = init_keys(buffer, length, exception);

  headers->count = 0;

  headers->items[0].keyLength = sizeof("sec-ch-ua-arch") - 1;
  headers->items[1].keyLength = sizeof("sec-ch-ua") - 1;
  headers->items[2].keyLength = sizeof("sec-ch-ua-bitness") - 1;
  headers->items[3].keyLength = sizeof("sec-ch-ua-full-version-list") - 1;
  headers->items[4].keyLength = sizeof("sec-ch-ua-mobile") - 1;
  headers->items[5].keyLength = sizeof("sec-ch-ua-model") - 1;
  headers->items[6].keyLength = sizeof("sec-ch-ua-platform") - 1;
  headers->items[7].keyLength = sizeof("sec-ch-ua-platform-version") - 1;

  for (int i = 0, j = 0; i < 8; ++i) {
    headers->items[i].key = buffer + j;
    j += headers->items[i].keyLength;
  }

  size_t iterations = 0;

  enum State { INIT, READ_KEY, SKIP_VALUE };

  for (enum State state = INIT; '\0' != *json; ++json) {
    switch (state) {
      case INIT: {
        json = skip_whitespaces(json);

        if (*json != '{') {
            exception->status = FIFTYONE_DEGREES_STATUS_CORRUPT_DATA;
            return 0;
        }

        json = skip_to_next_char(json, '"');
        if (*json == '\0') {
            return 0;
        }

        state = READ_KEY;
      } break;

      case READ_KEY: {
        enum Key key = read_ghev_key(&json);
        json = skip_whitespaces(json + 1);

        if (*json != ':') {
          return headers->count;
        }

        json = skip_whitespaces(json);
        if (*json == '\0') {
          return headers->count;
        }

        switch (key) {
          case KEY_UNDEFINED: {
            state = SKIP_VALUE;
          } break;

          case FULLVERSIONLIST:
          case BRANDS: {
            size_t vlen = read_brands_ghev_value(
                &json, buffer + offset, length > offset ? length - offset : 0,
                exception);

            if (*json == '\0') {
              return headers->count;
            }

            if (vlen == 0) {
              state = SKIP_VALUE;
              break;
            }

            headers->items[headers->count].value = buffer + offset;
            headers->items[headers->count].valueLength = vlen;

            ++headers->count;

            if (headers->count >= headers->capacity) {
              return headers->count;
            }

            offset += vlen;
            if (offset >= length) {
              return headers->count;
            }

            json = skip_to_next_char(json + 1, '"');

            if (*json == '\0') {
              return headers->count;
            }

            state = READ_KEY;
          } break;

          case MOBILE: {
            int b = read_bool_ghev_value(&json);

            if (b == -1) {
              state = SKIP_VALUE;
              break;
            }

            if (length - offset < 3) {
              return headers->count;
            }

            buffer[offset] = '?';
            buffer[offset + 1] = b ? '1' : '0';
            buffer[offset + 2] = '\n';

            headers->items[headers->count].value = buffer + offset;
            headers->items[headers->count].valueLength = 3;

            ++headers->count;

            if (headers->count >= headers->capacity) {
              return headers->count;
            }

            offset += 3;

            if (offset >= length) {
              return headers->count;
            }

            json = skip_to_next_char(json, '"');

            if (*json == '\0') {
              return headers->count;
            }

            state = READ_KEY;
          } break;

          case ARCHITECTURE:
          case BITNESS:
          case MODEL:
          case PLATFORM:
          case PLATFORMVERSION: {
            size_t vlen = read_string_value(
                &json, buffer + offset, length > offset ? length - offset : 0,
                exception);

            if (offset + vlen < length) {
              buffer[offset + vlen] = '\n';
              ++vlen;
            }

            if (*json == '\0') {
              return headers->count;
            }

            headers->items[headers->count].value = buffer + offset;
            headers->items[headers->count].valueLength = vlen;

            ++headers->count;

            if (headers->count >= headers->capacity) {
              return headers->count;
            }

            offset += vlen;
            if (offset >= length) {
              return headers->count;
            }

            json = skip_to_next_char(json + 1, '"');

            if (*json == '\0') {
              return headers->count;
            }

            state = READ_KEY;
          } break;
        }
      } break;

      case SKIP_VALUE: {
        if (!skip_value(&json)) {
          return headers->count;
        }

        state = READ_KEY;
      } break;
    }
  }

  return ++headers->count;
}

size_t fiftyoneDegreesTransformGhevFromBase64(
    const char* base64, char* buffer, size_t length,
    fiftyoneDegreesException* const exception,
    fiftyoneDegreesKeyValuePairArray* const headers) {
  size_t offset = base64_decode(base64, buffer, length, exception);
  if (exception->status != FIFTYONE_DEGREES_STATUS_SUCCESS) {
    return 0;
  }

  return fiftyoneDegreesTransformGhevFromJson(
      buffer, buffer + offset, length > offset ? length - offset : 0, exception,
      headers);
}

size_t fiftyoneDegreesTransformSua(
    const char* json, char* buffer, size_t length,
    fiftyoneDegreesException* const exception,
    fiftyoneDegreesKeyValuePairArray* const headers) {
  enum States {
    INIT,        //
    READ_KEY,    //
    SKIP_VALUE,  //
  };

  static struct OutKey {
    char* key;
    size_t length;
  } out_keys[] = {
      {NULL, sizeof("sec-ch-ua-arch") - 1},
      {NULL, sizeof("sec-ch-ua") - 1},
      {NULL, sizeof("sec-ch-ua-bitness") - 1},
      {NULL, sizeof("sec-ch-ua-full-version-list") - 1},
      {NULL, sizeof("sec-ch-ua-mobile") - 1},
      {NULL, sizeof("sec-ch-ua-model") - 1},
      {NULL, sizeof("sec-ch-ua-platform") - 1},
      {NULL, sizeof("sec-ch-ua-platform-version") - 1},
  };

  size_t offset = sizeof(keys_src) - 1;

  if (offset >= length) {
    return headers->count;
  }

  for (enum States state = INIT; *json; ++json) {
    switch (state) {
      case INIT: {
        json = skip_whitespaces(json);

        if (*json != '{') {
          return headers->count;
        }

        for (size_t i = 0; i < offset; ++i) {
          if (length <= offset) {
            return headers->count;
          }

          buffer[i] = keys_src[i];
        }

        for (int i = 0, j = 0; i < 8; ++i) {
          out_keys[i].key = buffer + j;
          j += out_keys[i].length;
        }

        json = skip_to_next_char(json, '"');
        if (*json == '\0') {
          return headers->count;
        }
        state = READ_KEY;
      } break;

      case READ_KEY: {
        enum Key key = read_sua_key(&json);
        json = skip_whitespaces(json + 1);

        if (*json != ':') {
          return headers->count;
        }

        json = skip_whitespaces(json);
        if (*json == '\0') {
          return headers->count;
        }

        switch (key) {
          case KEY_UNDEFINED: {
            state = SKIP_VALUE;
          } break;

          case FULLVERSIONLIST: {
            size_t vlen = read_brands_sua_value(
                &json, buffer + offset, length > offset ? length - offset : 0,
                exception);

            if (*json == '\0') {
              return headers->count;
            }

            if (vlen == 0) {
              state = SKIP_VALUE;
              break;
            }

            headers->items[headers->count].key = out_keys[key].key;
            headers->items[headers->count].keyLength = out_keys[key].length;
            headers->items[headers->count].value = buffer + offset;
            headers->items[headers->count].valueLength = vlen;

            ++headers->count;

            if (headers->count >= headers->capacity) {
              return headers->count;
            }

            offset += vlen;
            if (offset >= length) {
              return headers->count;
            }

            json = skip_to_next_char(json + 1, '"');

            if (*json == '\0') {
              return headers->count;
            }

            state = READ_KEY;
          } break;

          case MOBILE: {
            int b = read_bool_sua_value(&json);

            if (b == -1) {
              state = SKIP_VALUE;
              break;
            }

            if (length - offset < 3) {
              return headers->count;
            }

            buffer[offset] = '?';
            buffer[offset + 1] = b ? '1' : '0';
            buffer[offset + 2] = '\n';

            headers->items[headers->count].key = out_keys[key].key;
            headers->items[headers->count].keyLength = out_keys[key].length;
            headers->items[headers->count].value = buffer + offset;
            headers->items[headers->count].valueLength = 3;

            ++headers->count;

            if (headers->count >= headers->capacity) {
              return headers->count;
            }

            offset += 3;

            if (offset >= length) {
              return headers->count;
            }

            json = skip_to_next_char(json, '"');

            if (*json == '\0') {
              return headers->count;
            }

            state = READ_KEY;
          } break;

          case BRANDS: {
            // not implemented in sua. reading PLATFORM instead
          } break;

          case PLATFORMVERSION:
          case PLATFORM: {
            headers->items[headers->count].key = out_keys[PLATFORM].key;
            headers->items[headers->count].keyLength =
                out_keys[PLATFORM].length;
            headers->items[headers->count].value = NULL;
            headers->items[headers->count].valueLength = 0;

            ++headers->count;
            if (headers->count >= headers->capacity) {
              return headers->count;
            }

            headers->items[headers->count].key = out_keys[PLATFORMVERSION].key;
            headers->items[headers->count].keyLength =
                out_keys[PLATFORMVERSION].length;
            headers->items[headers->count].value = NULL;
            headers->items[headers->count].valueLength = 0;

            ++headers->count;
            if (headers->count >= headers->capacity) {
              return headers->count;
            }

            size_t vlen = read_platform_sua_value(
                &json, buffer + offset, length > offset ? length - offset : 0,
                &headers->items[headers->count - 2].value,
                &headers->items[headers->count - 2].valueLength,
                &headers->items[headers->count - 1].value,
                &headers->items[headers->count - 1].valueLength, exception);

            if (*json == '\0') {
              return headers->count;
            }

            if (vlen == 0) {
              state = SKIP_VALUE;
              break;
            }

            offset += vlen;
            if (offset >= length) {
              return headers->count;
            }

            json = skip_to_next_char(json + 1, '"');

            if (*json == '\0') {
              return headers->count;
            }

            state = READ_KEY;
          } break;

          case ARCHITECTURE:
          case BITNESS:
          case MODEL: {
            size_t vlen = read_string_value(
                &json, buffer + offset, length > offset ? length - offset : 0,
                exception);

            if (offset + vlen < length) {
              buffer[offset + vlen] = '\n';
              ++vlen;
            }

            if (*json == '\0') {
              return headers->count;
            }

            headers->items[headers->count].key = out_keys[key].key;
            headers->items[headers->count].keyLength = out_keys[key].length;
            headers->items[headers->count].value = buffer + offset;
            headers->items[headers->count].valueLength = vlen;

            ++headers->count;
            if (headers->count >= headers->capacity) {
              return headers->count;
            }

            offset += vlen;
            if (offset >= length) {
              return headers->count;
            }

            json = skip_to_next_char(json + 1, '"');

            if (*json == '\0') {
              return headers->count;
            }

            state = READ_KEY;
          } break;
        }
      } break;

      case SKIP_VALUE: {
        if (!skip_value(&json)) {
          return headers->count;
        }

        state = READ_KEY;
      } break;
    }
  }

  return ++headers->count;
}
