#ifndef __MJSON_H__
#define __MJSON_H__

#include <stddef.h>/*size_t*/

typedef enum { JSON_NULL, JSON_FALSE, JSON_TRUE, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT } json_type;

typedef struct json_value json_value;
typedef struct json_member json_member;

struct json_value{
    union{
	struct{ json_member *m; size_t size, capacity;}o;/*object*/
	struct{ json_value *e; size_t size, capacity;}a;/*array*/
	struct{ char*s; size_t len; }s; /*string*/
	double n;/*number*/
    }u;
    json_type type;
};

/*key : value*/
struct json_member{
    char* k; size_t klen;
    json_value v;
};

enum {
    JSON_PARSE_OK = 0,/*正确码*/
    JSON_PARSE_EXPECT_VALUE,/**/
    JSON_PARSE_INVALID_VALUE,/*非法值*/
    JSON_PARSE_ROOT_NOT_SINGULAR,/*结构非法*/
    JSON_PARSE_NUMBER_TOO_BIG,/*数字过大*/
    JSON_PARSE_MISS_QUOTATION_MARK,/*没有找到双引号结尾*/
    JSON_PARSE_INVALID_STRING_ESCAPE,/*非法转义字符*/
    JSON_PARSE_INVALID_STRING_CHAR,/**/
    JSON_PARSE_INVALID_UNICODE_SURROGATE,/*代理项非法*/
    JSON_PARSE_INVALID_UNICODE_HEX,/*Unicode字符非法*/
    JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    JSON_PARSE_MISS_KEY,
    JSON_PARSE_MISS_COLON,
    JSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

#define json_init(v) do{ (v)->type = JSON_NULL; }while(0)

int json_parse(json_value *v, const char* json);/*解析json*/
char* json_stringify(const json_value *v, size_t *length);/*生成json*/

void json_copy(json_value *dst, const json_value *src);
void json_move(json_value *dst, json_value *src);
void json_swap(json_value *lhs, json_value *rhs);

void json_free(json_value *v);/**/

json_type json_get_type(const json_value *v);/*获取json值的类型*/
int json_is_equal(const json_value *lhs, const json_value *rhs);

#define json_set_null(v) json_free(v)

int json_get_boolean(const json_value *v);
void json_set_boolean(json_value *v, int b);

double json_get_number(const json_value *v);
void json_set_number(json_value *v, double n);

const char* json_get_string(const json_value *v);
size_t json_get_string_length(const json_value *v);
void json_set_string(json_value *v, const char *s, size_t len);

void json_set_array(json_value *v, size_t capacity);

size_t json_get_array_size(const json_value *v);
size_t json_get_array_capacity(const json_value *v);

void json_reserve_array(json_value *v, size_t capacity);
void json_shrink_array(json_value *v);
void json_clear_array(json_value *v);

json_value* json_get_array_element(const json_value *v, size_t index);
json_value* json_pushback_array_element(json_value *v);
void json_popback_array_element(json_value *v);
json_value* json_insert_array_element(json_value *v, size_t index);
void json_erase_array_element(json_value *v, size_t index, size_t count);

void json_set_object(json_value *v, size_t capacity);

size_t json_get_object_size(const json_value *v);

size_t json_get_object_capacity(const json_value *v);

void json_reserve_object(json_value *v, size_t capacity);
void json_shrink_object(json_value *v);
void json_clear_object(json_value *v);

const char* json_get_object_key(const json_value *v, size_t index);

size_t json_get_object_key_length(const json_value*v, size_t index);
json_value* json_get_object_value(const json_value *v, size_t index);

#define JSON_KEY_NOT_EXIST ((size_t)-1)

size_t json_find_object_index(const json_value *v, const char *key, size_t klen);
json_value* json_find_object_value(json_value *v, const char *key, size_t len);
json_value* json_set_object_value(json_value *v, const char *key, size_t klen);
void json_remove_object_value(json_value *v, size_t index);
   

#endif /*__MJSON_H__*/
    
