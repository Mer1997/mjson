#ifndef __MJSON_H__
#define __MJSON_H__

typedef enum { JSON_NULL, JSON_FALSE, JSON_TRUE, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT } json_type;

typedef struct {
    union{
	struct{ char*s; size_t len; }s; /*string*/
	double n;/*number*/
    }u;
    json_type type;
}json_value;

enum {
    JSON_PARSE_OK = 0,/*正确码*/
    JSON_PARSE_EXPECT_VALUE,/**/
    JSON_PARSE_INVALID_VALUE,/*非法值*/
    JSON_PARSE_ROOT_NOT_SINGULAR,/*结构非法*/
    JSON_PARSE_NUMBER_TOO_BIG,/*数字过大*/
    JSON_PARSE_MISS_QUOTATION_MARK,/*没有找到双引号结尾*/
    JSON_PARSE_INVALID_STRING_EXCAPE,/*非法转义字符*/
    JSON_PARSE_INVALID_STRING_CHAR/**/
};

#define json_init(v) do{ (v)->type = JSON_NULL; }while(0)

int json_parse(json_value *v, const char* json);/*解析json*/

void json_free(json_value *v);/**/

json_type json_get_type(const json_value *v);/*获取json值的类型*/

#define json_set_null(v) json_free(v)

int json_get_boolean(const json_value *v);
void json_set_boolean(json_value *v, int b);

double json_get_number(const json_value *v);
void json_set_number(json_value *v, double n);

const char* json_get_string(const json_value *v);
size_t json_get_string_length(const json_value *v);
void json_set_string(json_value *v, const char *s, size_t len);

#endif /*__MJSON_H__*/
    
