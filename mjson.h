#ifndef __MJSON_H__
#define __MJSON_H__

typedef enum { JSON_NULL, JSON_FALSE, JSON_TRUE, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT } json_type;

typedef struct {
    double n;
    json_type type;
}json_value;

enum {
    JSON_PARSE_OK = 0,/*正确码*/
    JSON_PARSE_EXPECT_VALUE,/*期望值*/
    JSON_PARSE_INVALID_VALUE,/*非法值异常*/
    JSON_PARSE_ROOT_NOT_SINGULAR,/*JSON词义树*/
    JSON_PARSE_NUMBER_TOO_BIG,/*数字过大*/
};

int json_parse(json_value *v, const char* json);/*解析json*/

json_type json_get_type(const json_value *v);/*获取json值的类型*/

double json_get_number(const json_value *v);

#endif /*__MJSON_H__*/
    
