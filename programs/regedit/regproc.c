/*
 * Registry processing routines. Routines, common for registry
 * processing frontends.
 *
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2008 Alexander N. Sørnes <alex@thehandofagony.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <winnt.h>
#include <winreg.h>
#include <assert.h>
#include <wine/unicode.h>
#include <wine/debug.h>
#include "regproc.h"

#define REG_VAL_BUF_SIZE        4096

/* maximal number of characters in hexadecimal data line,
 * including the indentation, but not including the '\' character
 */
#define REG_FILE_HEX_LINE_LEN   (2 + 25 * 3)

extern const WCHAR* reg_class_namesW[];

static HKEY reg_class_keys[] = {
            HKEY_LOCAL_MACHINE, HKEY_USERS, HKEY_CLASSES_ROOT,
            HKEY_CURRENT_CONFIG, HKEY_CURRENT_USER, HKEY_DYN_DATA
        };

#define ARRAY_SIZE(A) (sizeof(A)/sizeof(*A))

/******************************************************************************
 * Allocates memory and converts input from multibyte to wide chars
 * Returned string must be freed by the caller
 */
static WCHAR* GetWideString(const char* strA)
{
    if(strA)
    {
        WCHAR* strW;
        int len = MultiByteToWideChar(CP_ACP, 0, strA, -1, NULL, 0);

        strW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        CHECK_ENOUGH_MEMORY(strW);
        MultiByteToWideChar(CP_ACP, 0, strA, -1, strW, len);
        return strW;
    }
    return NULL;
}

/******************************************************************************
 * Allocates memory and converts input from multibyte to wide chars
 * Returned string must be freed by the caller
 */
static WCHAR* GetWideStringN(const char* strA, int chars, DWORD *len)
{
    if(strA)
    {
        WCHAR* strW;
        *len = MultiByteToWideChar(CP_ACP, 0, strA, chars, NULL, 0);

        strW = HeapAlloc(GetProcessHeap(), 0, *len * sizeof(WCHAR));
        CHECK_ENOUGH_MEMORY(strW);
        MultiByteToWideChar(CP_ACP, 0, strA, chars, strW, *len);
        return strW;
    }
    *len = 0;
    return NULL;
}

/******************************************************************************
 * Allocates memory and converts input from wide chars to multibyte
 * Returned string must be freed by the caller
 */
char* GetMultiByteString(const WCHAR* strW)
{
    if(strW)
    {
        char* strA;
        int len = WideCharToMultiByte(CP_ACP, 0, strW, -1, NULL, 0, NULL, NULL);

        strA = HeapAlloc(GetProcessHeap(), 0, len);
        CHECK_ENOUGH_MEMORY(strA);
        WideCharToMultiByte(CP_ACP, 0, strW, -1, strA, len, NULL, NULL);
        return strA;
    }
    return NULL;
}

/******************************************************************************
 * Allocates memory and converts input from wide chars to multibyte
 * Returned string must be freed by the caller
 */
static char* GetMultiByteStringN(const WCHAR* strW, int chars, DWORD* len)
{
    if(strW)
    {
        char* strA;
        *len = WideCharToMultiByte(CP_ACP, 0, strW, chars, NULL, 0, NULL, NULL);

        strA = HeapAlloc(GetProcessHeap(), 0, *len);
        CHECK_ENOUGH_MEMORY(strA);
        WideCharToMultiByte(CP_ACP, 0, strW, chars, strA, *len, NULL, NULL);
        return strA;
    }
    *len = 0;
    return NULL;
}

static WCHAR *(*get_line)(FILE *);

/* parser definitions */
enum parser_state
{
    HEADER,              /* parsing the registry file version header */
    PARSE_WIN31_LINE,    /* parsing a Windows 3.1 registry line */
    LINE_START,          /* at the beginning of a registry line */
    KEY_NAME,            /* parsing a key name */
    DELETE_KEY,          /* deleting a registry key */
    DEFAULT_VALUE_NAME,  /* parsing a default value name */
    QUOTED_VALUE_NAME,   /* parsing a double-quoted value name */
    DATA_START,          /* preparing for data parsing operations */
    DELETE_VALUE,        /* deleting a registry value */
    DATA_TYPE,           /* parsing the registry data type */
    STRING_DATA,         /* parsing REG_SZ data */
    DWORD_DATA,          /* parsing DWORD data */
    HEX_DATA,            /* parsing REG_BINARY, REG_NONE, REG_EXPAND_SZ or REG_MULTI_SZ data */
    EOL_BACKSLASH,       /* preparing to parse multiple lines of hex data */
    HEX_MULTILINE,       /* parsing multiple lines of hex data */
    UNKNOWN_DATA,        /* parsing an unhandled or invalid data type */
    SET_VALUE,           /* adding a value to the registry */
    NB_PARSER_STATES
};

struct parser
{
    FILE              *file;           /* pointer to a registry file */
    WCHAR              two_wchars[2];  /* first two characters from the encoding check */
    BOOL               is_unicode;     /* parsing Unicode or ASCII data */
    short int          reg_version;    /* registry file version */
    HKEY               hkey;           /* current registry key */
    WCHAR             *key_name;       /* current key name */
    WCHAR             *value_name;     /* value name */
    DWORD              parse_type;     /* generic data type for parsing */
    DWORD              data_type;      /* data type */
    void              *data;           /* value data */
    DWORD              data_size;      /* size of the data (in bytes) */
    BOOL               backslash;      /* TRUE if the current line contains a backslash */
    enum parser_state  state;          /* current parser state */
};

typedef WCHAR *(*parser_state_func)(struct parser *parser, WCHAR *pos);

/* parser state machine functions */
static WCHAR *header_state(struct parser *parser, WCHAR *pos);
static WCHAR *parse_win31_line_state(struct parser *parser, WCHAR *pos);
static WCHAR *line_start_state(struct parser *parser, WCHAR *pos);
static WCHAR *key_name_state(struct parser *parser, WCHAR *pos);
static WCHAR *delete_key_state(struct parser *parser, WCHAR *pos);
static WCHAR *default_value_name_state(struct parser *parser, WCHAR *pos);
static WCHAR *quoted_value_name_state(struct parser *parser, WCHAR *pos);
static WCHAR *data_start_state(struct parser *parser, WCHAR *pos);
static WCHAR *delete_value_state(struct parser *parser, WCHAR *pos);
static WCHAR *data_type_state(struct parser *parser, WCHAR *pos);
static WCHAR *string_data_state(struct parser *parser, WCHAR *pos);
static WCHAR *dword_data_state(struct parser *parser, WCHAR *pos);
static WCHAR *hex_data_state(struct parser *parser, WCHAR *pos);
static WCHAR *eol_backslash_state(struct parser *parser, WCHAR *pos);
static WCHAR *hex_multiline_state(struct parser *parser, WCHAR *pos);
static WCHAR *unknown_data_state(struct parser *parser, WCHAR *pos);
static WCHAR *set_value_state(struct parser *parser, WCHAR *pos);

static const parser_state_func parser_funcs[NB_PARSER_STATES] =
{
    header_state,              /* HEADER */
    parse_win31_line_state,    /* PARSE_WIN31_LINE */
    line_start_state,          /* LINE_START */
    key_name_state,            /* KEY_NAME */
    delete_key_state,          /* DELETE_KEY */
    default_value_name_state,  /* DEFAULT_VALUE_NAME */
    quoted_value_name_state,   /* QUOTED_VALUE_NAME */
    data_start_state,          /* DATA_START */
    delete_value_state,        /* DELETE_VALUE */
    data_type_state,           /* DATA_TYPE */
    string_data_state,         /* STRING_DATA */
    dword_data_state,          /* DWORD_DATA */
    hex_data_state,            /* HEX_DATA */
    eol_backslash_state,       /* EOL_BACKSLASH */
    hex_multiline_state,       /* HEX_MULTILINE */
    unknown_data_state,        /* UNKNOWN_DATA */
    set_value_state,           /* SET_VALUE */
};

/* set the new parser state and return the previous one */
static inline enum parser_state set_state(struct parser *parser, enum parser_state state)
{
    enum parser_state ret = parser->state;
    parser->state = state;
    return ret;
}

static void *resize_buffer(void *buf, size_t count)
{
    void *new_buf;

    if (buf)
        new_buf = HeapReAlloc(GetProcessHeap(), 0, buf, count);
    else
        new_buf = HeapAlloc(GetProcessHeap(), 0, count);

    CHECK_ENOUGH_MEMORY(new_buf);
    return new_buf;
}

/******************************************************************************
 * Converts a hex representation of a DWORD into a DWORD.
 */
static BOOL convert_hex_to_dword(WCHAR *str, DWORD *dw)
{
    WCHAR *p, *end;
    int count = 0;

    while (*str == ' ' || *str == '\t') str++;
    if (!*str) goto error;

    p = str;
    while (isxdigitW(*p))
    {
        count++;
        p++;
    }
    if (count > 8) goto error;

    end = p;
    while (*p == ' ' || *p == '\t') p++;
    if (*p && *p != ';') goto error;

    *end = 0;
    *dw = strtoulW(str, &end, 16);
    return TRUE;

error:
    return FALSE;
}

/******************************************************************************
 * Converts comma-separated hex data into a binary string and modifies
 * the input parameter to skip the concatenating backslash, if found.
 *
 * Returns TRUE or FALSE to indicate whether parsing was successful.
 */
static BOOL convert_hex_csv_to_hex(struct parser *parser, WCHAR **str)
{
    size_t size;
    BYTE *d;
    WCHAR *s;

    parser->backslash = FALSE;

    /* The worst case is 1 digit + 1 comma per byte */
    size = ((lstrlenW(*str) + 1) / 2) + parser->data_size;
    parser->data = resize_buffer(parser->data, size);

    s = *str;
    d = (BYTE *)parser->data + parser->data_size;

    while (*s)
    {
        WCHAR *end;
        unsigned long wc;

        wc = strtoulW(s, &end, 16);
        if (wc > 0xff) return FALSE;

        if (s == end && wc == 0)
        {
            while (*end == ' ' || *end == '\t') end++;
            if (*end == '\\')
            {
                parser->backslash = TRUE;
                *str = end + 1;
                return TRUE;
            }
            else if (*end == ';')
                return TRUE;
            return FALSE;
        }

        *d++ = wc;
        parser->data_size++;

        if (*end && *end != ',')
        {
            while (*end == ' ' || *end == '\t') end++;
            if (*end && *end != ';') return FALSE;
            return TRUE;
        }

        if (*end) end++;
        s = end;
    }

    return TRUE;
}

/******************************************************************************
 * Parses the data type of the registry value being imported and modifies
 * the input parameter to skip the string representation of the data type.
 *
 * Returns TRUE or FALSE to indicate whether a data type was found.
 */
static BOOL parse_data_type(struct parser *parser, WCHAR **line)
{
    struct data_type { const WCHAR *tag; int len; int type; int parse_type; };

    static const WCHAR quote[] = {'"'};
    static const WCHAR hex[] = {'h','e','x',':'};
    static const WCHAR dword[] = {'d','w','o','r','d',':'};
    static const WCHAR hexp[] = {'h','e','x','('};

    static const struct data_type data_types[] = {
    /*    tag    len  type         parse type    */
        { quote,  1,  REG_SZ,      REG_SZ },
        { hex,    4,  REG_BINARY,  REG_BINARY },
        { dword,  6,  REG_DWORD,   REG_DWORD },
        { hexp,   4,  -1,          REG_BINARY }, /* REG_NONE, REG_EXPAND_SZ, REG_MULTI_SZ */
        { NULL,   0,  0,           0 }
    };

    const struct data_type *ptr;

    for (ptr = data_types; ptr->tag; ptr++)
    {
        if (strncmpW(ptr->tag, *line, ptr->len))
            continue;

        parser->parse_type = ptr->parse_type;
        parser->data_type = ptr->parse_type;
        *line += ptr->len;

        if (ptr->type == -1)
        {
            WCHAR *end;
            DWORD val;

            if (!**line || tolowerW((*line)[1]) == 'x')
                return FALSE;

            /* "hex(xx):" is special */
            val = wcstoul(*line, &end, 16);
            if (*end != ')' || *(end + 1) != ':' || (val == ~0u && errno == ERANGE))
                return FALSE;

            parser->data_type = val;
            *line = end + 2;
        }
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************
 * Replaces escape sequences with their character equivalents and
 * null-terminates the string on the first non-escaped double quote.
 *
 * Assigns a pointer to the remaining unparsed data in the line.
 * Returns TRUE or FALSE to indicate whether a closing double quote was found.
 */
static BOOL REGPROC_unescape_string(WCHAR *str, WCHAR **unparsed)
{
    int str_idx = 0;            /* current character under analysis */
    int val_idx = 0;            /* the last character of the unescaped string */
    int len = lstrlenW(str);
    BOOL ret;

    for (str_idx = 0; str_idx < len; str_idx++, val_idx++) {
        if (str[str_idx] == '\\') {
            str_idx++;
            switch (str[str_idx]) {
            case 'n':
                str[val_idx] = '\n';
                break;
            case 'r':
                str[val_idx] = '\r';
                break;
            case '0':
                str[val_idx] = '\0';
                break;
            case '\\':
            case '"':
                str[val_idx] = str[str_idx];
                break;
            default:
                output_message(STRING_ESCAPE_SEQUENCE, str[str_idx]);
                str[val_idx] = str[str_idx];
                break;
            }
        } else if (str[str_idx] == '"') {
            break;
        } else {
            str[val_idx] = str[str_idx];
        }
    }

    ret = (str[str_idx] == '"');
    *unparsed = str + str_idx + 1;
    str[val_idx] = '\0';
    return ret;
}

static HKEY parse_key_name(WCHAR *key_name, WCHAR **key_path)
{
    unsigned int i;

    if (!key_name) return 0;

    *key_path = strchrW(key_name, '\\');
    if (*key_path) (*key_path)++;

    for (i = 0; i < ARRAY_SIZE(reg_class_keys); i++)
    {
        int len = lstrlenW(reg_class_namesW[i]);
        if (!strncmpiW(key_name, reg_class_namesW[i], len) &&
           (key_name[len] == 0 || key_name[len] == '\\'))
        {
            return reg_class_keys[i];
        }
    }

    return 0;
}

static void close_key(struct parser *parser)
{
    if (parser->hkey)
    {
        HeapFree(GetProcessHeap(), 0, parser->key_name);
        parser->key_name = NULL;

        RegCloseKey(parser->hkey);
        parser->hkey = NULL;
    }
}

/******************************************************************************
 * Opens the registry key given by the input path.
 * This key must be closed by calling close_key().
 */
static LONG open_key(struct parser *parser, WCHAR *path)
{
    HKEY key_class;
    WCHAR *key_path;
    LONG res;

    close_key(parser);

    /* Get the registry class */
    if (!path || !(key_class = parse_key_name(path, &key_path)))
        return ERROR_INVALID_PARAMETER;

    res = RegCreateKeyExW(key_class, key_path, 0, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_ALL_ACCESS, NULL, &parser->hkey, NULL);

    if (res == ERROR_SUCCESS)
    {
        parser->key_name = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(path) + 1) * sizeof(WCHAR));
        CHECK_ENOUGH_MEMORY(parser->key_name);
        lstrcpyW(parser->key_name, path);
    }
    else
        parser->hkey = NULL;

    return res;
}

static void free_parser_data(struct parser *parser)
{
    if (parser->parse_type == REG_DWORD || parser->parse_type == REG_BINARY)
        HeapFree(GetProcessHeap(), 0, parser->data);

    parser->data = NULL;
    parser->data_size = 0;
}

static void prepare_hex_string_data(struct parser *parser)
{
    if (parser->data_type == REG_EXPAND_SZ || parser->data_type == REG_MULTI_SZ)
    {
        BYTE *data = parser->data;

        if (data[parser->data_size - 1] != 0)
        {
            data[parser->data_size] = 0;
            parser->data_size++;
        }

        if (!parser->is_unicode)
        {
            parser->data = GetWideStringN(parser->data, parser->data_size, &parser->data_size);
            parser->data_size *= sizeof(WCHAR);
            HeapFree(GetProcessHeap(), 0, data);
        }
    }
}

enum reg_versions {
    REG_VERSION_31,
    REG_VERSION_40,
    REG_VERSION_50,
    REG_VERSION_FUZZY,
    REG_VERSION_INVALID
};

static enum reg_versions parse_file_header(const WCHAR *s)
{
    static const WCHAR header_31[] = {'R','E','G','E','D','I','T',0};
    static const WCHAR header_40[] = {'R','E','G','E','D','I','T','4',0};
    static const WCHAR header_50[] = {'W','i','n','d','o','w','s',' ',
                                      'R','e','g','i','s','t','r','y',' ','E','d','i','t','o','r',' ',
                                      'V','e','r','s','i','o','n',' ','5','.','0','0',0};

    while (*s == ' ' || *s == '\t') s++;

    if (!strcmpW(s, header_31))
        return REG_VERSION_31;

    if (!strcmpW(s, header_40))
        return REG_VERSION_40;

    if (!strcmpW(s, header_50))
        return REG_VERSION_50;

    /* The Windows version accepts registry file headers beginning with "REGEDIT" and ending
     * with other characters, as long as "REGEDIT" appears at the start of the line. For example,
     * "REGEDIT 4", "REGEDIT9" and "REGEDIT4FOO" are all treated as valid file headers.
     * In all such cases, however, the contents of the registry file are not imported.
     */
    if (!strncmpW(s, header_31, 7)) /* "REGEDIT" without NUL */
        return REG_VERSION_FUZZY;

    return REG_VERSION_INVALID;
}

/* handler for parser HEADER state */
static WCHAR *header_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line, *header;

    if (!(line = get_line(parser->file)))
        return NULL;

    if (!parser->is_unicode)
    {
        header = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(line) + 3) * sizeof(WCHAR));
        CHECK_ENOUGH_MEMORY(header);
        header[0] = parser->two_wchars[0];
        header[1] = parser->two_wchars[1];
        lstrcpyW(header + 2, line);
        parser->reg_version = parse_file_header(header);
        HeapFree(GetProcessHeap(), 0, header);
    }
    else parser->reg_version = parse_file_header(line);

    switch (parser->reg_version)
    {
    case REG_VERSION_31:
        set_state(parser, PARSE_WIN31_LINE);
        break;
    case REG_VERSION_40:
    case REG_VERSION_50:
        set_state(parser, LINE_START);
        break;
    default:
        get_line(NULL); /* Reset static variables */
        return NULL;
    }

    return line;
}

/* handler for parser PARSE_WIN31_LINE state */
static WCHAR *parse_win31_line_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line, *value;
    static WCHAR hkcr[] = {'H','K','E','Y','_','C','L','A','S','S','E','S','_','R','O','O','T'};
    unsigned int key_end = 0;

    if (!(line = get_line(parser->file)))
        return NULL;

    if (strncmpW(line, hkcr, ARRAY_SIZE(hkcr)))
        return line;

    /* get key name */
    while (line[key_end] && !isspaceW(line[key_end])) key_end++;

    value = line + key_end;
    while (*value == ' ' || *value == '\t') value++;

    if (*value == '=') value++;
    if (*value == ' ') value++; /* at most one space is skipped */

    line[key_end] = 0;

    if (open_key(parser, line) != ERROR_SUCCESS)
    {
        output_message(STRING_OPEN_KEY_FAILED, line);
        return line;
    }

    parser->value_name = NULL;
    parser->data_type = REG_SZ;
    parser->data = value;
    parser->data_size = (lstrlenW(value) + 1) * sizeof(WCHAR);

    set_state(parser, SET_VALUE);
    return value;
}

/* handler for parser LINE_START state */
static WCHAR *line_start_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line, *p;

    if (!(line = get_line(parser->file)))
        return NULL;

    for (p = line; *p; p++)
    {
        switch (*p)
        {
        case '[':
            set_state(parser, KEY_NAME);
            return p + 1;
        case '@':
            set_state(parser, DEFAULT_VALUE_NAME);
            return p;
        case '"':
            set_state(parser, QUOTED_VALUE_NAME);
            return p + 1;
        case ' ':
        case '\t':
            break;
        default:
            return p;
        }
    }

    return p;
}

/* handler for parser KEY_NAME state */
static WCHAR *key_name_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *p = pos, *key_end;

    if (*p == ' ' || *p == '\t' || !(key_end = strrchrW(p, ']')))
        goto done;

    *key_end = 0;

    if (*p == '-')
    {
        set_state(parser, DELETE_KEY);
        return p + 1;
    }
    else if (open_key(parser, p) != ERROR_SUCCESS)
        output_message(STRING_OPEN_KEY_FAILED, p);

done:
    set_state(parser, LINE_START);
    return p;
}

/* handler for parser DELETE_KEY state */
static WCHAR *delete_key_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *p = pos;

    if (*p == 'H' || *p == 'h')
        delete_registry_key(p);

    set_state(parser, LINE_START);
    return p;
}

/* handler for parser DEFAULT_VALUE_NAME state */
static WCHAR *default_value_name_state(struct parser *parser, WCHAR *pos)
{
    parser->value_name = NULL;

    set_state(parser, DATA_START);
    return pos + 1;
}

/* handler for parser QUOTED_VALUE_NAME state */
static WCHAR *quoted_value_name_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *val_name = pos, *p;

    if (parser->value_name)
    {
        HeapFree(GetProcessHeap(), 0, parser->value_name);
        parser->value_name = NULL;
    }

    if (!REGPROC_unescape_string(val_name, &p))
        goto invalid;

    /* copy the value name in case we need to parse multiple lines and the buffer is overwritten */
    parser->value_name = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(val_name) + 1) * sizeof(WCHAR));
    CHECK_ENOUGH_MEMORY(parser->value_name);
    lstrcpyW(parser->value_name, val_name);

    set_state(parser, DATA_START);
    return p;

invalid:
    set_state(parser, LINE_START);
    return p;
}

/* handler for parser DATA_START state */
static WCHAR *data_start_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *p = pos;
    unsigned int len;

    while (*p == ' ' || *p == '\t') p++;
    if (*p != '=') goto done;
    p++;
    while (*p == ' ' || *p == '\t') p++;

    /* trim trailing whitespace */
    len = strlenW(p);
    while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t')) len--;
    p[len] = 0;

    if (*p == '-')
        set_state(parser, DELETE_VALUE);
    else
        set_state(parser, DATA_TYPE);
    return p;

done:
    set_state(parser, LINE_START);
    return p;
}

/* handler for parser DELETE_VALUE state */
static WCHAR *delete_value_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *p = pos + 1;

    while (*p == ' ' || *p == '\t') p++;
    if (*p && *p != ';') goto done;

    RegDeleteValueW(parser->hkey, parser->value_name);

done:
    set_state(parser, LINE_START);
    return p;
}

/* handler for parser DATA_TYPE state */
static WCHAR *data_type_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line = pos;

    if (!parse_data_type(parser, &line))
    {
        set_state(parser, LINE_START);
        return line;
    }

    switch (parser->parse_type)
    {
    case REG_SZ:
        set_state(parser, STRING_DATA);
        break;
    case REG_DWORD:
        set_state(parser, DWORD_DATA);
        break;
    case REG_BINARY: /* all hex data types, including undefined */
        set_state(parser, HEX_DATA);
        break;
    default:
        set_state(parser, UNKNOWN_DATA);
    }

    return line;
}

/* handler for parser STRING_DATA state */
static WCHAR *string_data_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line;

    parser->data = pos;

    if (!REGPROC_unescape_string(parser->data, &line))
        goto invalid;

    while (*line == ' ' || *line == '\t') line++;
    if (*line && *line != ';') goto invalid;

    parser->data_size = (lstrlenW(parser->data) + 1) * sizeof(WCHAR);

    set_state(parser, SET_VALUE);
    return line;

invalid:
    free_parser_data(parser);
    set_state(parser, LINE_START);
    return line;
}

/* handler for parser DWORD_DATA state */
static WCHAR *dword_data_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line = pos;

    parser->data = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD));
    CHECK_ENOUGH_MEMORY(parser->data);

    if (!convert_hex_to_dword(line, parser->data))
        goto invalid;

    parser->data_size = sizeof(DWORD);

    set_state(parser, SET_VALUE);
    return line;

invalid:
    free_parser_data(parser);
    set_state(parser, LINE_START);
    return line;
}

/* handler for parser HEX_DATA state */
static WCHAR *hex_data_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line = pos;

    if (!convert_hex_csv_to_hex(parser, &line))
        goto invalid;

    if (parser->backslash)
    {
        set_state(parser, EOL_BACKSLASH);
        return line;
    }

    prepare_hex_string_data(parser);

    set_state(parser, SET_VALUE);
    return line;

invalid:
    free_parser_data(parser);
    set_state(parser, LINE_START);
    return line;
}

/* handler for parser EOL_BACKSLASH state */
static WCHAR *eol_backslash_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *p = pos;

    while (*p == ' ' || *p == '\t') p++;
    if (*p && *p != ';') goto invalid;

    set_state(parser, HEX_MULTILINE);
    return pos;

invalid:
    free_parser_data(parser);
    set_state(parser, LINE_START);
    return p;
}

/* handler for parser HEX_MULTILINE state */
static WCHAR *hex_multiline_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line;

    if (!(line = get_line(parser->file)))
    {
        prepare_hex_string_data(parser);
        set_state(parser, SET_VALUE);
        return pos;
    }

    while (*line == ' ' || *line == '\t') line++;
    if (!*line || *line == ';') return line;

    if (!isxdigitW(*line)) goto invalid;

    set_state(parser, HEX_DATA);
    return line;

invalid:
    free_parser_data(parser);
    set_state(parser, LINE_START);
    return line;
}

/* handler for parser UNKNOWN_DATA state */
static WCHAR *unknown_data_state(struct parser *parser, WCHAR *pos)
{
    output_message(STRING_UNKNOWN_DATA_FORMAT, parser->data_type);

    set_state(parser, LINE_START);
    return pos;
}

/* handler for parser SET_VALUE state */
static WCHAR *set_value_state(struct parser *parser, WCHAR *pos)
{
    RegSetValueExW(parser->hkey, parser->value_name, 0, parser->data_type,
                   parser->data, parser->data_size);

    free_parser_data(parser);

    if (parser->reg_version == REG_VERSION_31)
        set_state(parser, PARSE_WIN31_LINE);
    else
        set_state(parser, LINE_START);

    return pos;
}

static WCHAR *get_lineA(FILE *fp)
{
    static WCHAR *lineW;
    static size_t size;
    static char *buf, *next;
    char *line;

    HeapFree(GetProcessHeap(), 0, lineW);

    if (!fp) goto cleanup;

    if (!size)
    {
        size = REG_VAL_BUF_SIZE;
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        CHECK_ENOUGH_MEMORY(buf);
        *buf = 0;
        next = buf;
    }
    line = next;

    while (next)
    {
        char *p = strpbrk(line, "\r\n");
        if (!p)
        {
            size_t len, count;
            len = strlen(next);
            memmove(buf, next, len + 1);
            if (size - len < 3)
            {
                char *new_buf = HeapReAlloc(GetProcessHeap(), 0, buf, size * 2);
                CHECK_ENOUGH_MEMORY(new_buf);
                buf = new_buf;
                size *= 2;
            }
            if (!(count = fread(buf + len, 1, size - len - 1, fp)))
            {
                next = NULL;
                lineW = GetWideString(buf);
                return lineW;
            }
            buf[len + count] = 0;
            next = buf;
            line = buf;
            continue;
        }
        next = p + 1;
        if (*p == '\r' && *(p + 1) == '\n') next++;
        *p = 0;
        lineW = GetWideString(line);
        return lineW;
    }

cleanup:
    lineW = NULL;
    if (size) HeapFree(GetProcessHeap(), 0, buf);
    size = 0;
    return NULL;
}

static WCHAR *get_lineW(FILE *fp)
{
    static size_t size;
    static WCHAR *buf, *next;
    WCHAR *line;

    if (!fp) goto cleanup;

    if (!size)
    {
        size = REG_VAL_BUF_SIZE;
        buf = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
        CHECK_ENOUGH_MEMORY(buf);
        *buf = 0;
        next = buf;
    }
    line = next;

    while (next)
    {
        static const WCHAR line_endings[] = {'\r','\n',0};
        WCHAR *p = strpbrkW(line, line_endings);
        if (!p)
        {
            size_t len, count;
            len = strlenW(next);
            memmove(buf, next, (len + 1) * sizeof(WCHAR));
            if (size - len < 3)
            {
                WCHAR *new_buf = HeapReAlloc(GetProcessHeap(), 0, buf, (size * 2) * sizeof(WCHAR));
                CHECK_ENOUGH_MEMORY(new_buf);
                buf = new_buf;
                size *= 2;
            }
            if (!(count = fread(buf + len, sizeof(WCHAR), size - len - 1, fp)))
            {
                next = NULL;
                return buf;
            }
            buf[len + count] = 0;
            next = buf;
            line = buf;
            continue;
        }
        next = p + 1;
        if (*p == '\r' && *(p + 1) == '\n') next++;
        *p = 0;
        return line;
    }

cleanup:
    if (size) HeapFree(GetProcessHeap(), 0, buf);
    size = 0;
    return NULL;
}

/******************************************************************************
 * Checks whether the buffer has enough room for the string or required size.
 * Resizes the buffer if necessary.
 *
 * Parameters:
 * buffer - pointer to a buffer for string
 * len - current length of the buffer in characters.
 * required_len - length of the string to place to the buffer in characters.
 *   The length does not include the terminating null character.
 */
static void REGPROC_resize_char_buffer(WCHAR **buffer, DWORD *len, DWORD required_len)
{
    required_len++;
    if (required_len > *len) {
        *len = required_len;
        if (!*buffer)
            *buffer = HeapAlloc(GetProcessHeap(), 0, *len * sizeof(**buffer));
        else
            *buffer = HeapReAlloc(GetProcessHeap(), 0, *buffer, *len * sizeof(**buffer));
        CHECK_ENOUGH_MEMORY(*buffer);
    }
}

/******************************************************************************
 * Prints string str to file
 */
static void REGPROC_export_string(WCHAR **line_buf, DWORD *line_buf_size, DWORD *line_len, WCHAR *str, DWORD str_len)
{
    DWORD i, pos;
    DWORD extra = 0;

    REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len + str_len + 10);

    /* escaping characters */
    pos = *line_len;
    for (i = 0; i < str_len; i++) {
        WCHAR c = str[i];
        switch (c) {
        case '\n':
            extra++;
            REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len + str_len + extra);
            (*line_buf)[pos++] = '\\';
            (*line_buf)[pos++] = 'n';
            break;

        case '\r':
            extra++;
            REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len + str_len + extra);
            (*line_buf)[pos++] = '\\';
            (*line_buf)[pos++] = 'r';
            break;

        case '\\':
        case '"':
            extra++;
            REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len + str_len + extra);
            (*line_buf)[pos++] = '\\';
            /* Fall through */

        default:
            (*line_buf)[pos++] = c;
            break;
        }
    }
    (*line_buf)[pos] = '\0';
    *line_len = pos;
}

static void REGPROC_export_binary(WCHAR **line_buf, DWORD *line_buf_size, DWORD *line_len, DWORD type, BYTE *value, DWORD value_size, BOOL unicode)
{
    DWORD hex_pos, data_pos;
    const WCHAR *hex_prefix;
    const WCHAR hex[] = {'h','e','x',':',0};
    WCHAR hex_buf[17];
    const WCHAR concat[] = {'\\','\r','\n',' ',' ',0};
    DWORD concat_prefix, concat_len;
    const WCHAR newline[] = {'\r','\n',0};
    CHAR* value_multibyte = NULL;

    if (type == REG_BINARY) {
        hex_prefix = hex;
    } else {
        const WCHAR hex_format[] = {'h','e','x','(','%','x',')',':',0};
        hex_prefix = hex_buf;
        sprintfW(hex_buf, hex_format, type);
        if ((type == REG_SZ || type == REG_EXPAND_SZ || type == REG_MULTI_SZ) && !unicode)
        {
            value_multibyte = GetMultiByteStringN((WCHAR*)value, value_size / sizeof(WCHAR), &value_size);
            value = (BYTE*)value_multibyte;
        }
    }

    concat_len = lstrlenW(concat);
    concat_prefix = 2;

    hex_pos = *line_len;
    *line_len += lstrlenW(hex_prefix);
    data_pos = *line_len;
    *line_len += value_size * 3;
    /* - The 2 spaces that concat places at the start of the
     *   line effectively reduce the space available for data.
     * - If the value name and hex prefix are very long
     *   ( > REG_FILE_HEX_LINE_LEN) or *line_len divides
     *   without a remainder then we may overestimate
     *   the needed number of lines by one. But that's ok.
     * - The trailing '\r' takes the place of a comma so
     *   we only need to add 1 for the trailing '\n'
     */
    *line_len += *line_len / (REG_FILE_HEX_LINE_LEN - concat_prefix) * concat_len + 1;
    REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len);
    lstrcpyW(*line_buf + hex_pos, hex_prefix);
    if (value_size)
    {
        const WCHAR format[] = {'%','0','2','x',0};
        DWORD i, column;

        column = data_pos; /* no line wrap yet */
        i = 0;
        while (1)
        {
            sprintfW(*line_buf + data_pos, format, (unsigned int)value[i]);
            data_pos += 2;
            if (++i == value_size)
                break;

            (*line_buf)[data_pos++] = ',';
            column += 3;

            /* wrap the line */
            if (column >= REG_FILE_HEX_LINE_LEN) {
                lstrcpyW(*line_buf + data_pos, concat);
                data_pos += concat_len;
                column = concat_prefix;
            }
        }
    }
    lstrcpyW(*line_buf + data_pos, newline);
    HeapFree(GetProcessHeap(), 0, value_multibyte);
}

/******************************************************************************
 * Writes the given line to a file, in multi-byte or wide characters
 */
static void REGPROC_write_line(FILE *file, const WCHAR* str, BOOL unicode)
{
    if(unicode)
    {
        fwrite(str, sizeof(WCHAR), lstrlenW(str), file);
    } else
    {
        char* strA = GetMultiByteString(str);
        fputs(strA, file);
        HeapFree(GetProcessHeap(), 0, strA);
    }
}

/******************************************************************************
 * Writes contents of the registry key to the specified file stream.
 *
 * Parameters:
 * file - writable file stream to export registry branch to.
 * key - registry branch to export.
 * reg_key_name_buf - name of the key with registry class.
 *      Is resized if necessary.
 * reg_key_name_size - length of the buffer for the registry class in characters.
 * val_name_buf - buffer for storing value name.
 *      Is resized if necessary.
 * val_name_size - length of the buffer for storing value names in characters.
 * val_buf - buffer for storing values while extracting.
 *      Is resized if necessary.
 * val_size - size of the buffer for storing values in bytes.
 */
static void export_hkey(FILE *file, DWORD value_type, BYTE **val_buf, DWORD *val_size,
                        WCHAR **line_buf, DWORD *line_buf_size, BOOL unicode)
{
    DWORD line_len = 0;
    DWORD val_size1 = *val_size;
    WCHAR *wstr = (WCHAR *)*val_buf;

                if (val_size1 < sizeof(WCHAR) || val_size1 % sizeof(WCHAR) ||
                    wstr[val_size1 / sizeof(WCHAR) - 1]) {
                    REGPROC_export_binary(line_buf, line_buf_size, &line_len, value_type, *val_buf, val_size1, unicode);
                } else {
                    const WCHAR start[] = {'"',0};
                    const WCHAR end[] = {'"','\r','\n',0};
                    DWORD len;

                    len = lstrlenW(start);
                    REGPROC_resize_char_buffer(line_buf, line_buf_size, line_len + len);
                    lstrcpyW(*line_buf + line_len, start);
                    line_len += len;

                    REGPROC_export_string(line_buf, line_buf_size, &line_len, wstr, lstrlenW(wstr));

                    REGPROC_resize_char_buffer(line_buf, line_buf_size, line_len + lstrlenW(end));
                    lstrcpyW(*line_buf + line_len, end);
                }
            REGPROC_write_line(file, *line_buf, unicode);
}

/******************************************************************************
 * Reads contents of the specified file into the registry.
 */
BOOL import_registry_file(FILE *reg_file)
{
    BYTE s[2];
    struct parser parser;
    WCHAR *pos;

    if (!reg_file || (fread(s, 2, 1, reg_file) != 1))
        return FALSE;

    parser.is_unicode = (s[0] == 0xff && s[1] == 0xfe);
    get_line = parser.is_unicode ? get_lineW : get_lineA;

    parser.file          = reg_file;
    parser.two_wchars[0] = s[0];
    parser.two_wchars[1] = s[1];
    parser.reg_version   = -1;
    parser.hkey          = NULL;
    parser.key_name      = NULL;
    parser.value_name    = NULL;
    parser.parse_type    = 0;
    parser.data_type     = 0;
    parser.data          = NULL;
    parser.data_size     = 0;
    parser.backslash     = FALSE;
    parser.state         = HEADER;

    pos = parser.two_wchars;

    /* parser main loop */
    while (pos)
        pos = (parser_funcs[parser.state])(&parser, pos);

    if (parser.reg_version == REG_VERSION_FUZZY || parser.reg_version == REG_VERSION_INVALID)
        return parser.reg_version == REG_VERSION_FUZZY;

    HeapFree(GetProcessHeap(), 0, parser.value_name);
    close_key(&parser);

    return TRUE;
}

/******************************************************************************
 * Removes the registry key with all subkeys. Parses full key name.
 *
 * Parameters:
 * reg_key_name - full name of registry branch to delete. Ignored if is NULL,
 *      empty, points to register key class, does not exist.
 */
void delete_registry_key(WCHAR *reg_key_name)
{
    WCHAR *key_name = NULL;
    HKEY key_class;

    if (!reg_key_name || !reg_key_name[0])
        return;

    if (!(key_class = parse_key_name(reg_key_name, &key_name)))
    {
        if (key_name) *(key_name - 1) = 0;
        error_exit(STRING_INVALID_SYSTEM_KEY, reg_key_name);
    }

    if (!*key_name)
        error_exit(STRING_DELETE_FAILED, reg_key_name);

    RegDeleteTreeW(key_class, key_name);
}

static WCHAR *REGPROC_escape_string(WCHAR *str, size_t str_len, size_t *line_len)
{
    size_t i, escape_count, pos;
    WCHAR *buf;

    for (i = 0, escape_count = 0; i < str_len; i++)
    {
        WCHAR c = str[i];
        if (c == '\r' || c == '\n' || c == '\\' || c == '"' || c == '\0')
            escape_count++;
    }

    buf = resize_buffer(NULL, (str_len + escape_count + 1) * sizeof(WCHAR));

    for (i = 0, pos = 0; i < str_len; i++, pos++)
    {
        WCHAR c = str[i];

        switch (c)
        {
        case '\r':
            buf[pos++] = '\\';
            buf[pos] = 'r';
            break;
        case '\n':
            buf[pos++] = '\\';
            buf[pos] = 'n';
            break;
        case '\\':
            buf[pos++] = '\\';
            buf[pos] = '\\';
            break;
        case '"':
            buf[pos++] = '\\';
            buf[pos] = '"';
            break;
        case '\0':
            buf[pos++] = '\\';
            buf[pos] = '0';
            break;
        default:
            buf[pos] = c;
        }
    }

    buf[pos] = 0;
    *line_len = pos;
    return buf;
}

static size_t export_value_name(FILE *fp, WCHAR *name, size_t len, BOOL unicode)
{
    static const WCHAR quoted_fmt[] = {'"','%','s','"','=',0};
    static const WCHAR default_name[] = {'@','=',0};
    size_t line_len;

    if (name && *name)
    {
        WCHAR *str = REGPROC_escape_string(name, len, &line_len);
        WCHAR *buf = resize_buffer(NULL, (line_len + 4) * sizeof(WCHAR));
        line_len = sprintfW(buf, quoted_fmt, str);
        REGPROC_write_line(fp, buf, unicode);
        HeapFree(GetProcessHeap(), 0, buf);
        HeapFree(GetProcessHeap(), 0, str);
    }
    else
    {
        line_len = lstrlenW(default_name);
        REGPROC_write_line(fp, default_name, unicode);
    }

    return line_len;
}

static void export_dword_data(WCHAR **buf, DWORD *data)
{
    static const WCHAR fmt[] = {'d','w','o','r','d',':','%','0','8','x',0};

    *buf = resize_buffer(NULL, 15 * sizeof(WCHAR));
    sprintfW(*buf, fmt, *data);
}

static size_t export_hex_data_type(FILE *fp, DWORD type, BOOL unicode)
{
    static const WCHAR hex[] = {'h','e','x',':',0};
    static const WCHAR hexp_fmt[] = {'h','e','x','(','%','x',')',':',0};
    size_t line_len;

    if (type == REG_BINARY)
    {
        line_len = lstrlenW(hex);
        REGPROC_write_line(fp, hex, unicode);
    }
    else
    {
        WCHAR *buf = resize_buffer(NULL, 15 * sizeof(WCHAR));
        line_len = sprintfW(buf, hexp_fmt, type);
        REGPROC_write_line(fp, buf, unicode);
        HeapFree(GetProcessHeap(), 0, buf);
    }

    return line_len;
}

#define MAX_HEX_CHARS 77

static void export_hex_data(FILE *fp, WCHAR **buf, DWORD type, DWORD line_len,
                            void *data, DWORD size, BOOL unicode)
{
    static const WCHAR fmt[] = {'%','0','2','x',0};
    static const WCHAR hex_concat[] = {'\\','\r','\n',' ',' ',0};
    size_t num_commas, i, pos;

    line_len += export_hex_data_type(fp, type, unicode);

    if (!unicode && (type == REG_EXPAND_SZ || type == REG_MULTI_SZ))
        data = GetMultiByteStringN(data, size / sizeof(WCHAR), &size);

    num_commas = size - 1;
    *buf = resize_buffer(NULL, (size * 3) * sizeof(WCHAR));

    for (i = 0, pos = 0; i < size; i++)
    {
        pos += sprintfW(*buf + pos, fmt, ((BYTE *)data)[i]);
        if (i == num_commas) break;
        (*buf)[pos++] = ',';
        (*buf)[pos] = 0;
        line_len += 3;

        if (line_len >= MAX_HEX_CHARS)
        {
            REGPROC_write_line(fp, *buf, unicode);
            REGPROC_write_line(fp, hex_concat, unicode);
            line_len = 2;
            pos = 0;
        }
    }
}

static void export_data(FILE *fp, DWORD type, size_t line_len, void *data, size_t size, BOOL unicode)
{
    WCHAR *buf = NULL;
    static const WCHAR newline[] = {'\r','\n',0};

    switch (type)
    {
    case REG_DWORD:
        export_dword_data(&buf, data);
        break;
    case REG_NONE:
    case REG_EXPAND_SZ:
    case REG_BINARY:
    case REG_MULTI_SZ:
    default:
        export_hex_data(fp, &buf, type, line_len, data, size, unicode);
        break;
    }

    REGPROC_write_line(fp, buf, unicode);
    HeapFree(GetProcessHeap(), 0, buf);
    REGPROC_write_line(fp, newline, unicode);
}

static WCHAR *build_subkey_path(WCHAR *path, DWORD path_len, WCHAR *subkey_name, DWORD subkey_len)
{
    WCHAR *subkey_path;
    static const WCHAR fmt[] = {'%','s','\\','%','s',0};

    subkey_path = resize_buffer(NULL, (path_len + subkey_len + 2) * sizeof(WCHAR));
    sprintfW(subkey_path, fmt, path, subkey_name);

    return subkey_path;
}

static void export_key_name(FILE *fp, WCHAR *name, BOOL unicode)
{
    static const WCHAR fmt[] = {'\r','\n','[','%','s',']','\r','\n',0};
    WCHAR *buf;

    buf = resize_buffer(NULL, (lstrlenW(name) + 7) * sizeof(WCHAR));
    sprintfW(buf, fmt, name);
    REGPROC_write_line(fp, buf, unicode);
    HeapFree(GetProcessHeap(), 0, buf);
}

#define MAX_SUBKEY_LEN   257

static int export_registry_data(FILE *fp, HKEY key, WCHAR *path, BOOL unicode)
{
    WCHAR *line_buf;
    DWORD line_buf_size = KEY_MAX_LEN + REG_VAL_BUF_SIZE;
    LONG rc;
    DWORD max_value_len = 256, value_len;
    DWORD max_data_bytes = 2048, data_size;
    DWORD subkey_len;
    DWORD i, type, path_len;
    WCHAR *value_name, *subkey_name, *subkey_path;
    BYTE *data;
    HKEY subkey;

    line_buf = HeapAlloc(GetProcessHeap(), 0, line_buf_size * sizeof(WCHAR));
    CHECK_ENOUGH_MEMORY(line_buf);

    export_key_name(fp, path, unicode);

    value_name = resize_buffer(NULL, max_value_len * sizeof(WCHAR));
    data = resize_buffer(NULL, max_data_bytes);

    i = 0;
    for (;;)
    {
        value_len = max_value_len;
        data_size = max_data_bytes;
        rc = RegEnumValueW(key, i, value_name, &value_len, NULL, &type, data, &data_size);
        if (rc == ERROR_SUCCESS)
        {
            size_t line_len = export_value_name(fp, value_name, value_len, unicode);
            if (type != REG_SZ)
                export_data(fp, type, line_len, data, data_size, unicode);
            else
                export_hkey(fp, type, &data, &data_size, &line_buf, &line_buf_size, unicode);
            i++;
        }
        else if (rc == ERROR_MORE_DATA)
        {
            if (data_size > max_data_bytes)
            {
                max_data_bytes *= 2;
                data = resize_buffer(data, max_data_bytes);
            }
            else
            {
                max_value_len *= 2;
                value_name = resize_buffer(value_name, max_value_len * sizeof(WCHAR));
            }
        }
        else break;
    }

    HeapFree(GetProcessHeap(), 0, data);
    HeapFree(GetProcessHeap(), 0, value_name);
    HeapFree(GetProcessHeap(), 0, line_buf);

    subkey_name = resize_buffer(NULL, MAX_SUBKEY_LEN * sizeof(WCHAR));

    path_len = lstrlenW(path);

    i = 0;
    for (;;)
    {
        subkey_len = MAX_SUBKEY_LEN;
        rc = RegEnumKeyExW(key, i, subkey_name, &subkey_len, NULL, NULL, NULL, NULL);
        if (rc == ERROR_SUCCESS)
        {
            subkey_path = build_subkey_path(path, path_len, subkey_name, subkey_len);
            if (!RegOpenKeyExW(key, subkey_name, 0, KEY_READ, &subkey))
            {
                export_registry_data(fp, subkey, subkey_path, unicode);
                RegCloseKey(subkey);
            }
            HeapFree(GetProcessHeap(), 0, subkey_path);
            i++;
        }
        else break;
    }

    HeapFree(GetProcessHeap(), 0, subkey_name);
    return 0;
}

static FILE *REGPROC_open_export_file(WCHAR *file_name, BOOL unicode)
{
    FILE *file;
    static const WCHAR hyphen[] = {'-',0};

    if (!strcmpW(file_name, hyphen))
    {
        file = stdout;
        _setmode(_fileno(file), _O_BINARY);
    }
    else
    {
        static const WCHAR wb_mode[] = {'w','b',0};

        file = _wfopen(file_name, wb_mode);
        if (!file)
        {
            static const WCHAR regedit[] = {'r','e','g','e','d','i','t',0};
            _wperror(regedit);
            error_exit(STRING_CANNOT_OPEN_FILE, file_name);
        }
    }

    if (unicode)
    {
        static const BYTE bom[] = {0xff,0xfe};
        static const WCHAR header[] = {'W','i','n','d','o','w','s',' ',
                                       'R','e','g','i','s','t','r','y',' ','E','d','i','t','o','r',' ',
                                       'V','e','r','s','i','o','n',' ','5','.','0','0','\r','\n'};

        fwrite(bom, sizeof(BYTE), ARRAY_SIZE(bom), file);
        fwrite(header, sizeof(WCHAR), ARRAY_SIZE(header), file);
    }
    else
        fputs("REGEDIT4\r\n", file);

    return file;
}

static HKEY open_export_key(HKEY key_class, WCHAR *subkey, WCHAR *path)
{
    HKEY key;

    if (!RegOpenKeyExW(key_class, subkey, 0, KEY_READ, &key))
        return key;

    output_message(STRING_OPEN_KEY_FAILED, path);
    return NULL;
}

static BOOL export_key(WCHAR *file_name, WCHAR *path, BOOL unicode)
{
    HKEY key_class, key;
    WCHAR *subkey;
    FILE *fp;
    BOOL ret;

    if (!(key_class = parse_key_name(path, &subkey)))
    {
        if (subkey) *(subkey - 1) = 0;
        output_message(STRING_INVALID_SYSTEM_KEY, path);
        return FALSE;
    }

    if (!(key = open_export_key(key_class, subkey, path)))
        return FALSE;

    fp = REGPROC_open_export_file(file_name, unicode);
    ret = export_registry_data(fp, key, path, unicode);
    fclose(fp);

    RegCloseKey(key);
    return ret;
}

static BOOL export_all(WCHAR *file_name, WCHAR *path, BOOL unicode)
{
    FILE *fp;
    int i;
    HKEY classes[] = {HKEY_LOCAL_MACHINE, HKEY_USERS}, key;
    WCHAR *class_name;

    fp = REGPROC_open_export_file(file_name, unicode);

    for (i = 0; i < ARRAY_SIZE(classes); i++)
    {
        if (!(key = open_export_key(classes[i], NULL, path)))
        {
            fclose(fp);
            return FALSE;
        }

        class_name = resize_buffer(NULL, (lstrlenW(reg_class_namesW[i]) + 1) * sizeof(WCHAR));
        lstrcpyW(class_name, reg_class_namesW[i]);

        export_registry_data(fp, classes[i], class_name, unicode);

        HeapFree(GetProcessHeap(), 0, class_name);
        RegCloseKey(key);
    }

    fclose(fp);
    return TRUE;
}

BOOL export_registry_key(WCHAR *file_name, WCHAR *path, DWORD format)
{
    BOOL unicode = (format == REG_FORMAT_5);

    if (path && *path)
        return export_key(file_name, path, unicode);
    else
        return export_all(file_name, path, unicode);
}
