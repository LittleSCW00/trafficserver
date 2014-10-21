/** @file

  A brief file description

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

#include "HPACK.h"
#include "HPACKHuffman.h"

// 5.1.  Maximum Table Size
// The size of an entry is the sum of its name's length in octets (as
// defined in Section 6.2), its value's length in octets (see
// Section 6.2), plus 32.
const static unsigned ADDITIONAL_OCTETS = 32;

const static uint32_t HEADER_FIELD_LIMIT_LENGTH = 4096;

// Constants for regression test
const static int BUFSIZE_FOR_REGRESSION_TEST = 128;
const static int MAX_TEST_FIELD_NUM = 8;

typedef enum {
  TS_HPACK_STATIC_TABLE_0 = 0,
  TS_HPACK_STATIC_TABLE_AUTHORITY,
  TS_HPACK_STATIC_TABLE_METHOD_GET,
  TS_HPACK_STATIC_TABLE_METHOD_POST,
  TS_HPACK_STATIC_TABLE_PATH_ROOT,
  TS_HPACK_STATIC_TABLE_PATH_INDEX,
  TS_HPACK_STATIC_TABLE_SCHEME_HTTP,
  TS_HPACK_STATIC_TABLE_SCHEME_HTTPS,
  TS_HPACK_STATIC_TABLE_STATUS_200,
  TS_HPACK_STATIC_TABLE_STATUS_204,
  TS_HPACK_STATIC_TABLE_STATUS_206,
  TS_HPACK_STATIC_TABLE_STATUS_304,
  TS_HPACK_STATIC_TABLE_STATUS_400,
  TS_HPACK_STATIC_TABLE_STATUS_404,
  TS_HPACK_STATIC_TABLE_STATUS_500,
  TS_HPACK_STATIC_TABLE_ACCEPT_CHARSET,
  TS_HPACK_STATIC_TABLE_ACCEPT_ENCODING,
  TS_HPACK_STATIC_TABLE_ACCEPT_LANGUAGE,
  TS_HPACK_STATIC_TABLE_ACCEPT_RANGES,
  TS_HPACK_STATIC_TABLE_ACCEPT,
  TS_HPACK_STATIC_TABLE_ACCESS_CONTROL_ALLOW_ORIGIN,
  TS_HPACK_STATIC_TABLE_AGE,
  TS_HPACK_STATIC_TABLE_ALLOW,
  TS_HPACK_STATIC_TABLE_AUTHORIZATION,
  TS_HPACK_STATIC_TABLE_CACHE_CONTROL,
  TS_HPACK_STATIC_TABLE_CONTENT_DISPOSITION,
  TS_HPACK_STATIC_TABLE_CONTENT_ENCODING,
  TS_HPACK_STATIC_TABLE_CONTENT_LANGUAGE,
  TS_HPACK_STATIC_TABLE_CONTENT_LENGTH,
  TS_HPACK_STATIC_TABLE_CONTENT_LOCATION,
  TS_HPACK_STATIC_TABLE_CONTENT_RANGE,
  TS_HPACK_STATIC_TABLE_CONTENT_TYPE,
  TS_HPACK_STATIC_TABLE_COOKIE,
  TS_HPACK_STATIC_TABLE_DATE,
  TS_HPACK_STATIC_TABLE_ETAG,
  TS_HPACK_STATIC_TABLE_EXPECT,
  TS_HPACK_STATIC_TABLE_EXPIRES,
  TS_HPACK_STATIC_TABLE_FROM,
  TS_HPACK_STATIC_TABLE_HOST,
  TS_HPACK_STATIC_TABLE_IF_MATCH,
  TS_HPACK_STATIC_TABLE_IF_MODIFIED_SINCE,
  TS_HPACK_STATIC_TABLE_IF_NONE_MATCH,
  TS_HPACK_STATIC_TABLE_IF_RANGE,
  TS_HPACK_STATIC_TABLE_IF_UNMODIFIED_SINCE,
  TS_HPACK_STATIC_TABLE_LAST_MODIFIED,
  TS_HPACK_STATIC_TABLE_LINK,
  TS_HPACK_STATIC_TABLE_LOCATION,
  TS_HPACK_STATIC_TABLE_MAX_FORWARDS,
  TS_HPACK_STATIC_TABLE_PROXY_AUTHENTICATE,
  TS_HPACK_STATIC_TABLE_PROXY_AUTHORIZATION,
  TS_HPACK_STATIC_TABLE_RANGE,
  TS_HPACK_STATIC_TABLE_REFERER,
  TS_HPACK_STATIC_TABLE_REFRESH,
  TS_HPACK_STATIC_TABLE_RETRY_AFTER,
  TS_HPACK_STATIC_TABLE_SERVER,
  TS_HPACK_STATIC_TABLE_SET_COOKIE,
  TS_HPACK_STATIC_TABLE_STRICT_TRANSPORT_SECURITY,
  TS_HPACK_STATIC_TABLE_TRANSFER_ENCODING,
  TS_HPACK_STATIC_TABLE_USER_AGENT,
  TS_HPACK_STATIC_TABLE_VARY,
  TS_HPACK_STATIC_TABLE_VIA,
  TS_HPACK_STATIC_TABLE_WWW_AUTHENTICATE,
  TS_HPACK_STATIC_TABLE_ENTRY_NUM
} TS_HPACK_STATIC_TABLE_ENTRY;

const static struct {
  const char* name;
  const char* value;
} STATIC_TABLE[] = {
  {"", ""},
  {":authority", ""},
  {":method", "GET"},
  {":method", "POST"},
  {":path", "/"},
  {":path", "/index.html"},
  {":scheme", "http"},
  {":scheme", "https"},
  {":status", "200"},
  {":status", "204"},
  {":status", "206"},
  {":status", "304"},
  {":status", "400"},
  {":status", "404"},
  {":status", "500"},
  {"accept-charset", ""},
  {"accept-encoding", "gzip, deflate"},
  {"accept-language", ""},
  {"accept-ranges", ""},
  {"accept", ""},
  {"access-control-allow-origin", ""},
  {"age", ""},
  {"allow", ""},
  {"authorization", ""},
  {"cache-control", ""},
  {"content-disposition", ""},
  {"content-encoding", ""},
  {"content-language", ""},
  {"content-length", ""},
  {"content-location", ""},
  {"content-range", ""},
  {"content-type", ""},
  {"cookie", ""},
  {"date", ""},
  {"etag", ""},
  {"expect", ""},
  {"expires", ""},
  {"from", ""},
  {"host", ""},
  {"if-match", ""},
  {"if-modified-since", ""},
  {"if-none-match", ""},
  {"if-range", ""},
  {"if-unmodified-since", ""},
  {"last-modified", ""},
  {"link", ""},
  {"location", ""},
  {"max-forwards", ""},
  {"proxy-authenticate", ""},
  {"proxy-authorization", ""},
  {"range", ""},
  {"referer", ""},
  {"refresh", ""},
  {"retry-after", ""},
  {"server", ""},
  {"set-cookie", ""},
  {"strict-transport-security", ""},
  {"transfer-encoding", ""},
  {"user-agent", ""},
  {"vary", ""},
  {"via", ""},
  {"www-authenticate", ""}
};

int
Http2HeaderTable::get_header_from_indexing_tables(uint32_t index, MIMEFieldWrapper& field) const
{
  // Index Address Space starts at 1, so index == 0 is invalid.
  if (!index) return -1;

  if (index < TS_HPACK_STATIC_TABLE_ENTRY_NUM) {
    field.name_set(STATIC_TABLE[index].name, strlen(STATIC_TABLE[index].name));
    field.value_set(STATIC_TABLE[index].value, strlen(STATIC_TABLE[index].value));
  } else if (index < TS_HPACK_STATIC_TABLE_ENTRY_NUM + get_current_entry_num()) {
    const MIMEField* m_field = get_header(index - TS_HPACK_STATIC_TABLE_ENTRY_NUM + 1);

    int name_len, value_len;
    const char* name = m_field->name_get(&name_len);
    const char* value = m_field->value_get(&value_len);

    field.name_set(name, name_len);
    field.value_set(value, value_len);
  } else {
    // 3.3.3.  Index Address Space
    // Indices strictly greater than the sum of the lengths of both tables
    // MUST be treated as a decoding error.
    return -1;
  }

  return 0;
}

// 5.2.  Entry Eviction when Header Table Size Changes
// Whenever the maximum size for the header table is reduced, entries
// are evicted from the end of the header table until the size of the
// header table is less than or equal to the maximum size.
void
Http2HeaderTable::set_header_table_size(uint32_t new_size)
{
  uint32_t old_size = _settings_header_table_size;
  while (old_size > new_size) {
    int last_name_len, last_value_len;
    MIMEField* last_field = _headers.last();

    last_field->name_get(&last_name_len);
    last_field->value_get(&last_value_len);
    old_size -= ADDITIONAL_OCTETS + last_name_len + last_value_len;

    _headers.remove_index(_headers.length()-1);
    _mhdr->field_delete(last_field, false);
  }

  _settings_header_table_size = new_size;
}

void
Http2HeaderTable::add_header_field(const MIMEField * field)
{
  int name_len, value_len;
  const char * name = field->name_get(&name_len);
  const char * value = field->value_get(&value_len);
  uint32_t header_size = ADDITIONAL_OCTETS + name_len + value_len;

  if (header_size > _settings_header_table_size) {
    // 5.3. It is not an error to attempt to add an entry that is larger than the maximum size; an
    // attempt to add an entry larger than the entire table causes the table to be emptied of all existing entries.
    _headers.clear();
    _mhdr->fields_clear();
  } else {
    _current_size += header_size;
    while (_current_size > _settings_header_table_size) {
      int last_name_len, last_value_len;
      MIMEField* last_field = _headers.last();

      last_field->name_get(&last_name_len);
      last_field->value_get(&last_value_len);
      _current_size -= ADDITIONAL_OCTETS + last_name_len + last_value_len;

      _headers.remove_index(_headers.length()-1);
      _mhdr->field_delete(last_field, false);
    }

    MIMEField* new_field = _mhdr->field_create(name, name_len);
    new_field->value_set(_mhdr->m_heap, _mhdr->m_mime, value, value_len);
    // XXX Because entire Vec instance is copied, Its too expensive!
    _headers.insert(0, new_field);
  }
}

/*
 * Pseudo code
 *
 * if I < 2^N - 1, encode I on N bits
 * else
 *   encode (2^N - 1) on N bits
 *   I = I - (2^N - 1)
 *   while I >= 128
 *     encode (I % 128 + 128) on 8 bits
 *     I = I / 128
 *   encode I on 8 bits
 */
int64_t
encode_integer(uint8_t *buf_start, const uint8_t *buf_end, uint32_t value, uint8_t n)
{
  if (buf_start >= buf_end) return -1;

  uint8_t *p = buf_start;

  if (value < (static_cast<uint32_t>(1 << n) - 1)) {
    *(p++) |= value;
  } else {
    *(p++) |= (1 << n) - 1;
    value -= (1 << n) - 1;
    while (value >= 128) {
      if (p >= buf_end) {
        return -1;
      }
      *(p++) = (value & 0x7F) | 0x80;
      value = value >> 7;
    }
    if (p+1 >= buf_end) {
      return -1;
    }
    *(p++) = value;
  }
  return p - buf_start;
}

int64_t
encode_string(uint8_t *buf_start, const uint8_t *buf_end, const char* value, size_t value_len)
{
  uint8_t *p = buf_start;

  // Length
  const int64_t len = encode_integer(p, buf_end, value_len, 7);
  if (len == -1) return -1;
  p += len;
  if (buf_end < p || static_cast<size_t>(buf_end - p) < value_len) return -1;

  // Value String
  memcpy(p, value, value_len);
  p += value_len;
  return p - buf_start;
}

int64_t
encode_indexed_header_field(uint8_t *buf_start, const uint8_t *buf_end, uint32_t index)
{
  if (buf_start >= buf_end) return -1;

  uint8_t *p = buf_start;

  // Index
  const int64_t len = encode_integer(p, buf_end, index, 7);
  if (len == -1) return -1;

  // Representation type
  if (p+1 >= buf_end) {
    return -1;
  }
  *p |= '\x80';
  p += len;

  return p - buf_start;
}

int64_t
encode_literal_header_field(uint8_t *buf_start, const uint8_t *buf_end, const MIMEFieldWrapper& header, uint32_t index, HEADER_INDEXING_TYPE type)
{
  uint8_t *p = buf_start;
  int64_t len;
  uint8_t prefix = 0, flag = 0;

  switch (type) {
  case INC_INDEXING:
    prefix = 6;
    flag = 0x40;
    break;
  case WITHOUT_INDEXING:
    prefix = 4;
    flag = 0x00;
    break;
  case NEVER_INDEXED:
    prefix = 4;
    flag = 0x10;
    break;
  default:
    return -1;
  }

  // Index
  len = encode_integer(p, buf_end, index, prefix);
  if (len == -1) return -1;

  // Representation type
  if (p+1 >= buf_end) {
    return -1;
  }
  *p |= flag;
  p += len;

  // Value String
  int value_len;
  const char* value = header.value_get(&value_len);
  len = encode_string(p, buf_end, value, value_len);
  if (len == -1) return -1;
  p += len;

  return p - buf_start;
}

int64_t
encode_literal_header_field(uint8_t *buf_start, const uint8_t *buf_end, const MIMEFieldWrapper& header, HEADER_INDEXING_TYPE type)
{
  uint8_t *p = buf_start;
  int64_t len;
  uint8_t flag = 0;

  ink_assert(type >= INC_INDEXING && type <= NEVER_INDEXED);
  switch (type) {
  case INC_INDEXING:
    flag = 0x40;
    break;
  case WITHOUT_INDEXING:
    flag = 0x00;
    break;
  case NEVER_INDEXED:
    flag = 0x10;
    break;
  default:
    return -1;
  }
  if (p+1 >= buf_end) {
    return -1;
  }
  *(p++) = flag;

  // Name String
  int name_len;
  const char* name = header.name_get(&name_len);
  len = encode_string(p, buf_end, name, name_len);
  if (len == -1) return -1;
  p += len;

  // Value String
  int value_len;
  const char* value = header.value_get(&value_len);
  len = encode_string(p, buf_end, value, value_len);
  if (len == -1) {
    return -1;
  }

  p += len;

  return p - buf_start;
}

/*
 * 6.1.  Integer representation
 *
 * Pseudo code
 *
 * decode I from the next N bits
 *    if I < 2^N - 1, return I
 *    else
 *        M = 0
 *        repeat
 *            B = next octet
 *            I = I + (B & 127) * 2^M
 *            M = M + 7
 *        while B & 128 == 128
 *        return I
 *
 */
inline int64_t
decode_integer(uint32_t& dst, const uint8_t *buf_start, const uint8_t *buf_end, uint8_t n)
{
  const uint8_t *p = buf_start;

  dst = (*p & ((1 << n) - 1));
  if (dst == static_cast<uint32_t>(1 << n) - 1) {
    int m = 0;
    do {
      if (++p >= buf_end) return -1;

      uint32_t added_value = *p & 0x7f;
      if ((UINT32_MAX >> m) < added_value) {
        // Excessively large integer encodings - in value or octet
        // length - MUST be treated as a decoding error.
        return -1;
      }
      dst += added_value << m;
      m += 7;
    } while (*p & 0x80);
  }

  return p - buf_start + 1;
}

// 6.2 return content from String Data (Length octets)
// with huffman decoding if it is encoded
int64_t
decode_string(char **c_str, uint32_t& c_str_length, const uint8_t *buf_start, const uint8_t *buf_end)
{
  const uint8_t *p = buf_start;
  bool isHuffman = *p & 0x80;
  uint32_t encoded_string_len = 0;
  int64_t len = 0;

  len = decode_integer(encoded_string_len, p, buf_end, 7);
  if (len == -1) return -1;
  p += len;

  if (encoded_string_len > HEADER_FIELD_LIMIT_LENGTH || buf_start + encoded_string_len >= buf_end) {
    return -1;
  }

  if (isHuffman) {
    // Allocate temporary area twice the size of before decoded data
    *c_str = static_cast<char*>(ats_malloc(encoded_string_len * 2));

    len = huffman_decode(*c_str, p, encoded_string_len);
    if (len == -1) return -1;
    c_str_length = len;
  } else {
    *c_str = static_cast<char*>(ats_malloc(encoded_string_len));

    memcpy(*c_str, reinterpret_cast<const char*>(p), encoded_string_len);

    c_str_length = encoded_string_len;
  }

  return p + encoded_string_len - buf_start;
}

// 7.1. Indexed Header Field Representation
int64_t
decode_indexed_header_field(MIMEFieldWrapper& header, const uint8_t *buf_start, const uint8_t *buf_end, Http2HeaderTable& header_table)
{
  uint32_t index = 0;
  int64_t len = 0;
  len = decode_integer(index, buf_start, buf_end, 7);
  if (len == -1) return -1;

  if (header_table.get_header_from_indexing_tables(index, header) == -1) {
    return -1;
  }

  return len;
}

// 7.2.  Literal Header Field Representation
int64_t
decode_literal_header_field(MIMEFieldWrapper& header, const uint8_t *buf_start, const uint8_t *buf_end, Http2HeaderTable& header_table)
{
  const uint8_t *p = buf_start;
  bool isIncremental = false;
  uint32_t index = 0;
  int64_t len = 0;

  if (*p & 0x40) {
    // 7.2.1. index extraction based on Literal Header Field with Incremental Indexing
    len = decode_integer(index, p, buf_end, 6);
    if (len == -1) return -1;
    isIncremental = true;
  } else if (*p & 0x10) {
    // 7.2.3. index extraction Literal Header Field Never Indexed
    len = decode_integer(index, p, buf_end, 4);
    if (len == -1) return -1;
  } else {
    // 7.2.2. index extraction Literal Header Field without Indexing
    len = decode_integer(index, p, buf_end, 4);
    if (len == -1) return -1;
  }
  p += len;

  if (index) {
    header_table.get_header_from_indexing_tables(index, header);
  } else {
    char *c_name = NULL;
    uint32_t c_name_len = 0;
    len = decode_string(&c_name, c_name_len, p, buf_end);
    if (len == -1) return -1;
    p += len;
    header.name_set(c_name, c_name_len);
    ats_free(c_name);
  }
  char *c_value = NULL;
  uint32_t c_value_len = 0;
  len = decode_string(&c_value, c_value_len, p, buf_end);
  if (len == -1) return -1;
  p += len;
  header.value_set(c_value, c_value_len);
  ats_free(c_value);

  // Incremental Indexing adds header to header table as new entry
  if (isIncremental) {
    header_table.add_header_field(header.field_get());
  }

  return p - buf_start;
}

// 7.3. Header Table Size Update
int64_t
update_header_table_size(const uint8_t *buf_start, const uint8_t *buf_end, Http2HeaderTable& header_table)
{
  if (buf_start == buf_end) return -1;

  int64_t len = 0;

  // Update header table size if its required.
  if ((*buf_start & 0xe0) == 0x20) {
    uint32_t size = 0;
    len = decode_integer(size, buf_start, buf_end, 5);
    if (len == -1) return -1;

    header_table.set_header_table_size(size);
  }

  return len;
}

#if TS_HAS_TESTS

#include "TestBox.h"

/***********************************************************************************
 *                                                                                 *
 *                   Test cases for regression test                                *
 *                                                                                 *
 * Some test cases are based on examples of specification.                         *
 * http://tools.ietf.org/html/draft-ietf-httpbis-header-compression-09#appendix-D  *
 *                                                                                 *
 ***********************************************************************************/

// D.1.  Integer Representation Examples
const static struct {
  uint32_t raw_integer;
  uint8_t* encoded_field;
  int encoded_field_len;
  int prefix;
} integer_test_case[] = {
  { 10, (uint8_t*)"\x0A", 1, 5 },
  { 1337, (uint8_t*)"\x1F\x9A\x0A", 3, 5 },
  { 42, (uint8_t*)"\x2A", 1, 8 }
};

// Example: custom-key: custom-header
const static struct {
  char* raw_string;
  uint32_t raw_string_len;
  uint8_t* encoded_field;
  int encoded_field_len;
} string_test_case[] = {
  { (char*)"custom-key", 10, (uint8_t*)"\xA" "custom-key", 11 },
  { (char*)"custom-key", 10, (uint8_t*)"\x88" "\x25\xa8\x49\xe9\x5b\xa9\x7d\x7f", 9 }
};

// D.2.4.  Indexed Header Field
const static struct {
  int index;
  char* raw_name;
  char* raw_value;
  uint8_t* encoded_field;
  int encoded_field_len;
} indexed_test_case[] = {
  { 2, (char*)":method", (char*)"GET", (uint8_t*)"\x82", 1 }
};

// D.2.  Header Field Representation Examples
const static struct {
  char* raw_name;
  char* raw_value;
  int index;
  HEADER_INDEXING_TYPE type;
  uint8_t* encoded_field;
  int encoded_field_len;
} literal_test_case[] = {
  { (char*)"custom-key", (char*)"custom-header", 0, INC_INDEXING, (uint8_t*)"\x40\x0a" "custom-key\x0d" "custom-header", 26 },
  { (char*)"custom-key", (char*)"custom-header", 0, WITHOUT_INDEXING, (uint8_t*)"\x00\x0a" "custom-key\x0d" "custom-header", 26 },
  { (char*)"custom-key", (char*)"custom-header", 0, NEVER_INDEXED, (uint8_t*)"\x10\x0a" "custom-key\x0d" "custom-header", 26 },
  { (char*)":path", (char*)"/sample/path", 4, INC_INDEXING, (uint8_t*)"\x44\x0c" "/sample/path", 14 },
  { (char*)":path", (char*)"/sample/path", 4, WITHOUT_INDEXING, (uint8_t*)"\x04\x0c" "/sample/path", 14 },
  { (char*)":path", (char*)"/sample/path", 4, NEVER_INDEXED, (uint8_t*)"\x14\x0c" "/sample/path", 14 },
  { (char*)"password", (char*)"secret", 0, INC_INDEXING, (uint8_t*)"\x40\x08" "password\x06" "secret", 17 },
  { (char*)"password", (char*)"secret", 0, WITHOUT_INDEXING, (uint8_t*)"\x00\x08" "password\x06" "secret", 17 },
  { (char*)"password", (char*)"secret", 0, NEVER_INDEXED, (uint8_t*)"\x10\x08" "password\x06" "secret", 17 }
};

// D.3.  Request Examples without Huffman Coding - D.3.1.  First Request
const static struct {
  char* raw_name;
  char* raw_value;
} raw_field_test_case[][MAX_TEST_FIELD_NUM] = {
  {
    { (char*)":method",    (char*)"GET" },
    { (char*)":scheme",    (char*)"http" },
    { (char*)":path",      (char*)"/" },
    { (char*)":authority", (char*)"www.example.com" },
    { (char*)"", (char*)"" } // End of this test case
  }
};
const static struct {
  uint8_t* encoded_field;
  int encoded_field_len;
} encoded_field_test_case[] = {
  {
    (uint8_t*)"\x40" "\x7:method"    "\x3GET"
              "\x40" "\x7:scheme"    "\x4http"
              "\x40" "\x5:path"      "\x1/"
              "\x40" "\xa:authority" "\xfwww.example.com",
    64
  }
};

/***********************************************************************************
 *                                                                                 *
 *                                Regression test codes                            *
 *                                                                                 *
 ***********************************************************************************/

REGRESSION_TEST(HPACK_EncodeInteger)(RegressionTest * t, int, int *pstatus)
{
  TestBox box(t, pstatus);
  box = REGRESSION_TEST_PASSED;
  uint8_t buf[BUFSIZE_FOR_REGRESSION_TEST];

  for (unsigned int i=0; i<sizeof(integer_test_case)/sizeof(integer_test_case[0]); i++) {
    memset(buf, 0, BUFSIZE_FOR_REGRESSION_TEST);

    int len = encode_integer(buf, buf+BUFSIZE_FOR_REGRESSION_TEST, integer_test_case[i].raw_integer, integer_test_case[i].prefix);

    box.check(len == integer_test_case[i].encoded_field_len, "encoded length was %d, expecting %d",
        len, integer_test_case[i].encoded_field_len);
    box.check(memcmp(buf, integer_test_case[i].encoded_field, len) == 0, "encoded value was invalid");
  }
}

REGRESSION_TEST(HPACK_EncodeString)(RegressionTest * t, int, int *pstatus)
{
  TestBox box(t, pstatus);
  box = REGRESSION_TEST_PASSED;

  uint8_t buf[BUFSIZE_FOR_REGRESSION_TEST];
  int len;

  // FIXME Current encoder don't support huffman conding.
  for (unsigned int i=0; i<1; i++) {
    memset(buf, 0, BUFSIZE_FOR_REGRESSION_TEST);

    len = encode_string(buf, buf+BUFSIZE_FOR_REGRESSION_TEST, string_test_case[i].raw_string, string_test_case[i].raw_string_len);

    box.check(len == string_test_case[i].encoded_field_len, "encoded length was %d, expecting %d",
        len, integer_test_case[i].encoded_field_len);
    box.check(memcmp(buf, string_test_case[i].encoded_field, len) == 0, "encoded string was invalid");
  }
}

REGRESSION_TEST(HPACK_EncodeIndexedHeaderField)(RegressionTest * t, int, int *pstatus)
{
  TestBox box(t, pstatus);
  box = REGRESSION_TEST_PASSED;

  uint8_t buf[BUFSIZE_FOR_REGRESSION_TEST];

  for (unsigned int i=0; i<sizeof(indexed_test_case)/sizeof(indexed_test_case[0]); i++) {
    memset(buf, 0, BUFSIZE_FOR_REGRESSION_TEST);

    int len = encode_indexed_header_field(buf, buf+BUFSIZE_FOR_REGRESSION_TEST, indexed_test_case[i].index);

    box.check(len == indexed_test_case[i].encoded_field_len, "encoded length was %d, expecting %d",
        len, indexed_test_case[i].encoded_field_len);
    box.check(memcmp(buf, indexed_test_case[i].encoded_field, len) == 0, "encoded value was invalid");
  }
}

REGRESSION_TEST(HPACK_EncodeLiteralHeaderField)(RegressionTest * t, int, int *pstatus)
{
  TestBox box(t, pstatus);
  box = REGRESSION_TEST_PASSED;

  uint8_t buf[BUFSIZE_FOR_REGRESSION_TEST];
  int len;

  for (unsigned int i=0; i<sizeof(literal_test_case)/sizeof(literal_test_case[0]); i++) {
    memset(buf, 0, BUFSIZE_FOR_REGRESSION_TEST);

    HTTPHdr* headers = new HTTPHdr();
    headers->create(HTTP_TYPE_RESPONSE);
    MIMEField *field = mime_field_create(headers->m_heap, headers->m_http->m_fields_impl);
    MIMEFieldWrapper header(field, headers->m_heap, headers->m_http->m_fields_impl);

    header.value_set(literal_test_case[i].raw_value, strlen(literal_test_case[i].raw_value));
    if (literal_test_case[i].index > 0) {
      len = encode_literal_header_field(buf, buf+BUFSIZE_FOR_REGRESSION_TEST, header, literal_test_case[i].index, literal_test_case[i].type);
    } else {
      header.name_set(literal_test_case[i].raw_name, strlen(literal_test_case[i].raw_name));
      len = encode_literal_header_field(buf, buf+BUFSIZE_FOR_REGRESSION_TEST, header, literal_test_case[i].type);
    }

    box.check(len == literal_test_case[i].encoded_field_len, "encoded length was %d, expecting %d", len, literal_test_case[i].encoded_field_len);
    box.check(memcmp(buf, literal_test_case[i].encoded_field, len) == 0, "encoded value was invalid");
  }

}

REGRESSION_TEST(HPACK_Encode)(RegressionTest * t, int, int *pstatus)
{
  TestBox box(t, pstatus);
  box = REGRESSION_TEST_PASSED;

  uint8_t buf[BUFSIZE_FOR_REGRESSION_TEST];
  Http2HeaderTable header_table;

  // FIXME Current encoder don't support indexing.
  for (unsigned int i=0; i<sizeof(encoded_field_test_case)/sizeof(encoded_field_test_case[0]); i++) {
    HTTPHdr* headers = new HTTPHdr();
    headers->create(HTTP_TYPE_REQUEST);

    for (unsigned int j=0; j<sizeof(raw_field_test_case[i])/sizeof(raw_field_test_case[i][0]); j++) {
      const char* expected_name  = raw_field_test_case[i][j].raw_name;
      const char* expected_value = raw_field_test_case[i][j].raw_value;
      if (strlen(expected_name) == 0) break;

      MIMEField* field = mime_field_create(headers->m_heap, headers->m_http->m_fields_impl);
      mime_field_name_value_set(headers->m_heap, headers->m_http->m_fields_impl, field, -1,
          expected_name,  strlen(expected_name), expected_value, strlen(expected_value),
          true, strlen(expected_name) + strlen(expected_value), 1);
      mime_hdr_field_attach(headers->m_http->m_fields_impl, field, 1, NULL);
    }

    memset(buf, 0, BUFSIZE_FOR_REGRESSION_TEST);
    int len = convert_from_1_1_to_2_header(headers, buf, BUFSIZE_FOR_REGRESSION_TEST, header_table);

    box.check(len == encoded_field_test_case[i].encoded_field_len, "encoded length was %d, expecting %d",
        len, encoded_field_test_case[i].encoded_field_len);
    box.check(memcmp(buf, encoded_field_test_case[i].encoded_field, len) == 0, "encoded value was invalid");
  }
}

REGRESSION_TEST(HPACK_DecodeInteger)(RegressionTest * t, int, int *pstatus)
{
  TestBox box(t, pstatus);
  box = REGRESSION_TEST_PASSED;

  uint32_t actual;

  for (unsigned int i=0; i<sizeof(integer_test_case)/sizeof(integer_test_case[0]); i++) {
    int len = decode_integer(actual, integer_test_case[i].encoded_field,
        integer_test_case[i].encoded_field + integer_test_case[i].encoded_field_len,
        integer_test_case[i].prefix);

    box.check(len == integer_test_case[i].encoded_field_len, "decoded length was %d, expecting %d",
        len, integer_test_case[i].encoded_field_len);
    box.check(actual == integer_test_case[i].raw_integer, "decoded value was %d, expected %d",
        actual, integer_test_case[i].raw_integer);
  }
}

REGRESSION_TEST(HPACK_DecodeString)(RegressionTest * t, int, int *pstatus)
{
  TestBox box(t, pstatus);
  box = REGRESSION_TEST_PASSED;

  char* actual;
  uint32_t actual_len;

  hpack_huffman_init();

  for (unsigned int i=0; i<sizeof(string_test_case)/sizeof(string_test_case[0]); i++) {
    int len = decode_string(&actual, actual_len, string_test_case[i].encoded_field,
        string_test_case[i].encoded_field + string_test_case[i].encoded_field_len);

    box.check(len == string_test_case[i].encoded_field_len, "decoded length was %d, expecting %d",
        len, string_test_case[i].encoded_field_len);
    box.check(actual_len == string_test_case[i].raw_string_len, "length of decoded string was %d, expecting %d",
        actual_len, string_test_case[i].raw_string_len);
    box.check(memcmp(actual, string_test_case[i].raw_string, actual_len) == 0, "decoded string was invalid");

    ats_free(actual);
  }
}

REGRESSION_TEST(HPACK_DecodeIndexedHeaderField)(RegressionTest * t, int, int *pstatus)
{
  TestBox box(t, pstatus);
  box = REGRESSION_TEST_PASSED;

  Http2HeaderTable header_table;

  for (unsigned int i=0; i<sizeof(indexed_test_case)/sizeof(indexed_test_case[0]); i++) {
    HTTPHdr* headers = new HTTPHdr();
    headers->create(HTTP_TYPE_REQUEST);
    MIMEField *field = mime_field_create(headers->m_heap, headers->m_http->m_fields_impl);
    MIMEFieldWrapper header(field, headers->m_heap, headers->m_http->m_fields_impl);

    int len = decode_indexed_header_field(header, indexed_test_case[i].encoded_field,
        indexed_test_case[i].encoded_field+indexed_test_case[i].encoded_field_len, header_table);

    box.check(len == indexed_test_case[i].encoded_field_len, "decoded length was %d, expecting %d",
        len, indexed_test_case[i].encoded_field_len);

    int name_len;
    const char* name = header.name_get(&name_len);
    box.check(memcmp(name, indexed_test_case[i].raw_name, name_len) == 0,
      "decoded header name was invalid");

    int actual_value_len;
    const char* actual_value = header.value_get(&actual_value_len);
    box.check(memcmp(actual_value, indexed_test_case[i].raw_value, actual_value_len) == 0,
      "decoded header value was invalid");
  }
}

REGRESSION_TEST(HPACK_DecodeLiteralHeaderField)(RegressionTest * t, int, int *pstatus)
{
  TestBox box(t, pstatus);
  box = REGRESSION_TEST_PASSED;

  Http2HeaderTable header_table;

  for (unsigned int i=0; i<sizeof(literal_test_case)/sizeof(literal_test_case[0]); i++) {
    HTTPHdr* headers = new HTTPHdr();
    headers->create(HTTP_TYPE_REQUEST);
    MIMEField *field = mime_field_create(headers->m_heap, headers->m_http->m_fields_impl);
    MIMEFieldWrapper header(field, headers->m_heap, headers->m_http->m_fields_impl);

    int len = decode_literal_header_field(header, literal_test_case[i].encoded_field,
        literal_test_case[i].encoded_field+literal_test_case[i].encoded_field_len, header_table);

    box.check(len == literal_test_case[i].encoded_field_len, "decoded length was %d, expecting %d",
        len, literal_test_case[i].encoded_field_len);

    int name_len;
    const char* name = header.name_get(&name_len);
    box.check(memcmp(name, literal_test_case[i].raw_name, name_len) == 0,
      "decoded header name was invalid");

    int actual_value_len;
    const char* actual_value = header.value_get(&actual_value_len);
    box.check(memcmp(actual_value, literal_test_case[i].raw_value, actual_value_len) == 0,
      "decoded header value was invalid");
  }
}

REGRESSION_TEST(HPACK_Decode)(RegressionTest * t, int, int *pstatus)
{
  TestBox box(t, pstatus);
  box = REGRESSION_TEST_PASSED;

  Http2HeaderTable header_table;

  for (unsigned int i=0; i<sizeof(encoded_field_test_case)/sizeof(encoded_field_test_case[0]); i++) {
    HTTPHdr* headers = new HTTPHdr();
    headers->create(HTTP_TYPE_REQUEST);

    headers->hpack_parse_req(encoded_field_test_case[i].encoded_field,
        encoded_field_test_case[i].encoded_field + encoded_field_test_case[i].encoded_field_len,
        true, header_table);

    for (unsigned int j=0; j<sizeof(raw_field_test_case[i])/sizeof(raw_field_test_case[i][0]); j++) {
      const char* expected_name  = raw_field_test_case[i][j].raw_name;
      const char* expected_value = raw_field_test_case[i][j].raw_value;
      if (strlen(expected_name) == 0) break;

      MIMEField* field = headers->field_find(expected_name, strlen(expected_name));
      box.check(field != NULL, "A MIMEField that has \"%s\" as name doesn't exist", expected_name);

      int actual_value_len;
      const char* actual_value = field->value_get(&actual_value_len);
      box.check(strncmp(expected_value, actual_value, actual_value_len) == 0, "A MIMEField that has \"%s\" as value doesn't exist", expected_value);
    }
  }
}

#endif /* TS_HAS_TESTS */
