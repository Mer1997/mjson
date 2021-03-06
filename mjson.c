#include "mjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */
#include <stdio.h>

#ifndef JSON_PARSE_STACK_INIT_SIZE
#define JSON_PARSE_STACK_INIT_SIZE 256
#endif

#ifndef JSON_PARSE_STRINGIFY_INIT_SIZE
#define JSON_PARSE_STRINGIFY_INIT_SIZE 256
#endif

#define EXPECT(c, ch)	do{ assert(*c->json == (ch)); c->json++;}while(0)
#define ISDIGIT(ch)	((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)	do{ *(char*)json_context_push(c, sizeof(char)) = (ch);}while(0)
#define PUTS(c, s, len) memcpy(json_context_push(c, len), s, len)

typedef struct{
    const char *json;/*保存json串*/
    char *stack;/*栈底指针*/
    size_t size, top;/*栈大小与栈顶位置*/
}json_context;

/*入栈*/
static void* json_context_push(json_context *c, size_t size){
    void *ret;
    assert(size > 0);
    if(c->top + size >= c->size){
	if(c->size == 0)
	    c->size = JSON_PARSE_STACK_INIT_SIZE;
	while(c->top + size >= c->size)
	    c->size += c->size >>1;/*扩展栈容量c->size *1.5*/
	c->stack = (char*)realloc(c->stack,c->size);
    }

    /*返回新入栈数据的起始指针*/
    ret = c->stack + c->top;
    c->top +=size;
    return ret;
}

/*出栈*/
static void* json_context_pop(json_context *c, size_t size){
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

/*跳过空格*/
static void json_parse_whitespace(json_context *c){
    const char *p = c->json;
    while( *p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
	++p;
    c->json = p;
}

/*解析true, false, null*/
static int json_parse_literal(json_context *c, json_value *v, const char *literal, json_type type){
    size_t i;
    EXPECT(c, literal[0]);
    for(i = 0; literal[i+1]; ++i)
	if(c->json[i] != literal[i+1])
	    return JSON_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return JSON_PARSE_OK;
}

/*解析数字*/
static int json_parse_number(json_context *c, json_value *v){
    const char *p = c->json;
    if(*p == '-') p++;
    if(*p == '0') p++;
    else{
	if(!ISDIGIT1TO9(*p)) return JSON_PARSE_INVALID_VALUE;
	for(p++; ISDIGIT(*p); p++);
    }
    if(*p == '.'){
	p++;
	if(!ISDIGIT(*p)) return JSON_PARSE_INVALID_VALUE;
	for(p++; ISDIGIT(*p); p++);
    }
    if(*p == 'e' || *p == 'E'){
	p++;
	if(*p == '+' || *p == '-') p++;
	if(!ISDIGIT(*p)) return JSON_PARSE_INVALID_VALUE;
	for(p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->u.n = strtod(c->json, NULL);
    if(errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
	return JSON_PARSE_NUMBER_TOO_BIG;
    v->type = JSON_NUMBER;
    c->json = p;
    return JSON_PARSE_OK;
}

/*对Unicode码点进行转换*/
static const char* json_parse_hex4(const char *p, unsigned *u){
    int i;
    *u = 0;
    for( i = 0; i != 4; ++i){
	char ch = *p++;
	*u <<= 4;
	if(ch >= '0' && ch <= '9') *u |= ch - '0';
	else if( ch >= 'A' && ch <= 'F') *u |= ch - ('A' - 10);
	else if( ch >= 'a' && ch <= 'f') *u |= ch - ('a' - 10);
	else return NULL;
    }
    return p;
}

/*写入unicode*/
static void json_encode_utf8(json_context *c, unsigned u){
    if(u <= 0x7F)
	PUTC(c, u & 0xFF);
    else if( u <= 0x7FF){
	PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
	PUTC(c, 0x80 | ( u	 & 0x3F));
    }
    else if( u <= 0xFFFF){
	PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
	PUTC(c, 0x80 | ((u >>  6) & 0x3F));
	PUTC(c, 0x80 | ( u	  & 0x3F));
    }
    else{
	assert(u <= 0x10FFFF);
	PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
	PUTC(c, 0x80 | ((u >> 12) & 0x3F));
	PUTC(c, 0x80 | ((u >>  6) & 0x3F));
	PUTC(c, 0x80 | ( u	  & 0x3F));
    }
}

#define STRING_ERROR(ret) do{ c->top = head; return ret; } while(0)


/*escape = {'/', 'b', 'f', 'n', 'r', 't'};*/
/*解析字符串*/
static int json_parse_string_raw(json_context *c, char *str[], size_t *len){
    size_t head = c->top;
    unsigned u, u2;
    const char *p;
    EXPECT(c, '\"');
    p = c->json;
    for(;;){
	char ch = *p++;
	switch(ch){
	    case '\"':
		*len = c->top - head;
		/*It's wrong method because str was not allocated space in json_parse_string()*/
		/*memcpy(*str, (char *)json_context_pop(c, *len), *len);*/
		
		/*This method will cause memory leaks*/
		/*	
	       	*str = (char*)malloc(sizeof(char) * (*len));
		memcpy(*str, (char*)json_context_pop(c, *len), *len);
		*/


		/*correct method*/
		*str = (char *)json_context_pop(c, *len);
		
		c->json = p;
		return JSON_PARSE_OK;
	    case '\0':
		STRING_ERROR(JSON_PARSE_MISS_QUOTATION_MARK);
	    case '\\':
                switch (*p++) {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/':  PUTC(c, '/' ); break;
                    case 'b':  PUTC(c, '\b'); break;
                    case 'f':  PUTC(c, '\f'); break;
                    case 'n':  PUTC(c, '\n'); break;
                    case 'r':  PUTC(c, '\r'); break;
                    case 't':  PUTC(c, '\t'); break;
		    case 'u':
			if( !(p = json_parse_hex4(p, &u)))
			    STRING_ERROR(JSON_PARSE_INVALID_UNICODE_HEX);
			if(u >= 0xD800 && u <= 0xDBFF){/*代理对*/
			    if(*p++ != '\\')
				STRING_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE);
			    if(*p++ != 'u')
				STRING_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE);
			    if(!(p = json_parse_hex4(p, &u2)))
				STRING_ERROR(JSON_PARSE_INVALID_UNICODE_HEX);
			    if(u2 < 0xDC00 || u2 > 0xDFFF)
				STRING_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE);
			    u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
			}
			json_encode_utf8(c, u);
			break;
                    default:
			STRING_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE);
		}
		break;
	     default :
		if((unsigned char)ch < 0x20)
		    STRING_ERROR(JSON_PARSE_INVALID_STRING_CHAR);
		PUTC(c, ch);
	}
    }
}

static int json_parse_string(json_context *c, json_value *v){
    int ret;
    char *s;
    size_t len;
    if((ret = json_parse_string_raw(c, &s, &len)) == JSON_PARSE_OK)
	json_set_string(v, s, len);
    return ret;
}
static int json_parse_value(json_context *c, json_value *v);

static int json_parse_array(json_context *c, json_value *v){
    size_t i, size = 0;
    int ret;
    EXPECT(c, '[');
    json_parse_whitespace(c);
    if(*c->json == ']'){
	c->json++;
	v->type = JSON_ARRAY;
	v->u.a.size = 0;
	v->u.a.e = NULL;
	return JSON_PARSE_OK;
    }
    while(1){
	json_value e;
	json_init(&e);
	json_parse_whitespace(c);
	if((ret = json_parse_value(c, &e)) != JSON_PARSE_OK)
	    break;
	memcpy( json_context_push(c, sizeof(json_value)), &e, sizeof(json_value));
	size++;
	json_parse_whitespace(c);
	if(*c->json == ','){
	    c->json++;
	    json_parse_whitespace(c);
	}
	else if(*c->json == ']'){
	    c->json++;
	    v->type = JSON_ARRAY;
	    v->u.a.size = size;
	    size *= sizeof(json_value);
	    memcpy(v->u.a.e = (json_value*)malloc(size), json_context_pop(c, size), size);
	    return JSON_PARSE_OK;
	}
	else{
	    ret = JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
	    break;
	}
    }
    /*pop stack and free value*/
    for(i = 0; i < size; ++i)
	json_free((json_value*)json_context_pop(c, sizeof(json_value)));
    return ret;
}

static int json_parse_object(json_context *c, json_value *v){
    size_t size,i;
    json_member m;
    int ret;
    EXPECT(c, '{');
    json_parse_whitespace(c);
    if(*c->json == '}'){
	c->json++;
	v->type = JSON_OBJECT;
	v->u.o.m = 0;
	v->u.o.size = 0;
	return JSON_PARSE_OK;
    }
    m.k = NULL;
    size = 0;
    for(;;){
	json_init(&m.v);
	char *str;
	json_parse_whitespace(c);
	/* parse key to m.k, m.klen*/
	if( *c->json != '"'){
	    ret = JSON_PARSE_MISS_KEY;
	    break;
	}
	if((ret = json_parse_string_raw(c, &str, &m.klen)) != JSON_PARSE_OK)
	    break;
	
	m.k = (char*)malloc(m.klen+1);
	memcpy(m.k, str, m.klen);
	m.k[m.klen] = '\0';

	json_parse_whitespace(c);
	/* parse ws colon ws*/
	if(*c->json != ':'){
	    ret = JSON_PARSE_MISS_COLON;
	    break;
	}
	c->json++;
	json_parse_whitespace(c);

	/*parse value*/
	if((ret = json_parse_value(c, &m.v)) != JSON_PARSE_OK)
	    break;
	memcpy(json_context_push(c, sizeof(json_member)), &m, sizeof(json_member));
	size++;
	m.k = NULL; /* ownership is transferred to member on stack*/
	
	json_parse_whitespace(c);
	/*TODO: parse ws [comma | right-curly-brack] ws */
	

	if(*c->json == ','){
	    c->json++;
	    
	}
	else if(*c->json == '}'){
	    size_t s = sizeof(json_member) *size;
	    c->json++;
	    v->type = JSON_OBJECT;
	    v->u.o.size = size;
	    memcpy(v->u.o.m = (json_member*)malloc(s), json_context_pop(c, s), s);
	    return JSON_PARSE_OK;
	} 


	else{
	    ret = JSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
	    break;
	}

    }
    /* pop and free members on the stack */
    free(m.k);
    for(i = 0; i != size; ++i){
	json_member*m = (json_member*)json_context_pop(c, sizeof(json_member));
	free(m->k);
	json_free(&m->v);
    }
    v->type = JSON_NULL;
    return ret;
}

/*解析json值,返回解析状态[JSON_PARSE_OK或其他错误码]*/
static int json_parse_value(json_context *c, json_value *v){
    switch(*c->json){
	case 'n' : return json_parse_literal(c, v, "null", JSON_NULL);
	case 'f' : return json_parse_literal(c, v, "false", JSON_FALSE);
	case 't' : return json_parse_literal(c, v, "true", JSON_TRUE);
	case '"' : return json_parse_string(c, v);
	case '[' : return json_parse_array(c, v);
	case '{' : return json_parse_object(c, v);
	case '\0': return JSON_PARSE_EXPECT_VALUE;
	default  : return json_parse_number(c, v);
    }
}

/*处理json串并调用json_parse_value解析json值*/
int json_parse(json_value *v, const char *json){
    json_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    json_init(v);
    json_parse_whitespace(&c);
    if ((ret = json_parse_value(&c, v)) == JSON_PARSE_OK) {
        json_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = JSON_NULL;
            ret = JSON_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);
    free(c.stack);
    return ret;
}

#if 0
#else

static void json_stringify_string(json_context *c, const char *s, size_t len){

    static const char hex_digits[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    size_t i, size;
    char *head, *p;
    assert(s != NULL);
    p = head = (char*)json_context_push(c, size = len * 6 + 2);
    *p++ = '"';
    for(i = 0; i != len; ++i){
	unsigned char ch = (unsigned char)s[i];
	switch(ch){
	    case '\"': *p++ = '\\'; *p++ = '\"'; break;
	    case '\\': *p++ = '\\'; *p++ = '\\'; break;
	    case '\b': *p++ = '\\'; *p++ = 'b';	 break;
	    case '\f': *p++ = '\\'; *p++ = 'f';  break;
	    case '\n': *p++ = '\\'; *p++ = 'n';  break;
	    case '\r': *p++ = '\\'; *p++ = 'r';  break;
	    case '\t': *p++ = '\\'; *p++ = 't';  break;
	    default:
		if(ch < 0x20){
		    *p++ = '\\'; *p++ = 'u'; *p++ = '0'; *p++ = '0';
		    *p++ = hex_digits[ch>>4];
		    *p++ = hex_digits[ch&15];
		}\
		else
		    *p++ = s[i];
	}
    }
    *p++ = '"';
    c->top -= size - (p - head);

}

#endif

static void json_stringify_value(json_context *c, const json_value *v){
    size_t i;
    switch(v->type){
	case JSON_NULL:	    PUTS(c, "null", 4); break;
	case JSON_FALSE:    PUTS(c, "false",5); break;
	case JSON_TRUE:	    PUTS(c, "true", 4); break;
	case JSON_NUMBER:   c->top -= 32 - sprintf((char*)json_context_push(c,32), "%.17g", v->u.n); break;
	case JSON_STRING: json_stringify_string(c, v->u.s.s, v->u.s.len); break;
	case JSON_ARRAY:
	    PUTC(c, '[');
	    for(i = 0; i != v->u.a.size; ++i){
		if( i > 0) PUTC(c, ',');
		json_stringify_value(c, &v->u.a.e[i]);
	    }
	    PUTC(c, ']');
	    break;
	case JSON_OBJECT:
	    PUTC(c, '{');
	    for(i = 0; i != v->u.o.size; ++i){
		if(i > 0) PUTC(c, ',');
		json_stringify_string(c, v->u.o.m[i].k, v->u.o.m[i].klen);
		PUTC(c, ':');
		json_stringify_value(c, &v->u.o.m[i].v);
	    }
	    PUTC(c, '}');
	    break;
	default: assert(0 && "invalid type");
    }
}

char* json_stringify(const json_value *v, size_t *length){
    json_context c;
    assert(v!= NULL);
    c.stack = (char *)malloc(c.size = JSON_PARSE_STRINGIFY_INIT_SIZE);
    c.top = 0;
    json_stringify_value(&c, v);
    if(length) *length = c.top;
    PUTC(&c, '\0');
    return c.stack;
}

void json_free(json_value *v){
    size_t i;
    assert(v != NULL);
    switch(v->type){
    
    case JSON_STRING:
	free(v->u.s.s);
	break;
    case JSON_ARRAY:
	for(i = 0; i < v->u.a.size; i++)
	    json_free(&v->u.a.e[i]);
	free(v->u.a.e);
	break;
    default: break;
    }
    v->type = JSON_NULL;
}

json_type json_get_type(const json_value *v){
    assert( v != NULL);
    return v->type;
}

int json_get_boolean(const json_value *v){
    assert(v != NULL && (v->type == JSON_TRUE || v->type == JSON_FALSE));
    return v->type == JSON_TRUE;
}

void json_set_boolean(json_value *v, int b){
    assert(v != NULL);
    json_free(v);
    v->type = b ? JSON_TRUE : JSON_FALSE;
}

double json_get_number(const json_value *v){
    assert(v != NULL && v->type == JSON_NUMBER);
    return v->u.n;
}

void json_set_number(json_value *v, double n){
    assert(v != NULL);
    json_free(v);
    v->u.n = n;
    v->type = JSON_NUMBER;
}

const char* json_get_string(const json_value *v){
    assert(v != NULL && v->type == JSON_STRING);
    return v->u.s.s;
}

size_t json_get_string_length(const json_value *v){
    assert(v != NULL && v->type == JSON_STRING);
    return v->u.s.len;
}

void json_set_string(json_value *v, const char *s, size_t len){
    assert( v != NULL && (s != NULL || len == 0));
    json_free(v);
    v->u.s.s = (char *)malloc(len+1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';/*以NULL结尾的C风格字符串*/
    v->u.s.len = len;
    v->type = JSON_STRING;
}

void json_set_array(json_value *v, size_t capacity){
    assert(v != NULL);
    json_free(v);
    v->type = JSON_ARRAY;
    v->u.a.size = 0;
    v->u.a.capacity = capacity;
    v->u.a.e = capacity > 0 ? (json_value *)malloc(capacity * sizeof(json_value)) : NULL;
}

size_t json_get_array_size(const json_value *v){
    assert(v != NULL && v->type == JSON_ARRAY);
    return v->u.a.size;
}

size_t json_get_array_capacity(const json_value *v){
    assert(v != NULL && v->type == JSON_ARRAY);
    return v->u.a.capacity;
}

void json_reserve_array(json_value *v, size_t capacity){
    assert(v != NULL && v->type == JSON_ARRAY);
    if(v->u.a.capacity < capacity){
	v->u.a.capacity = capacity;
	v->u.a.e = (json_value*)realloc(v->u.a.e, capacity * sizeof(json_value));
    }
}

void json_shrink_array(json_value *v){
    assert(v != NULL && v->type == JSON_ARRAY);
    if(v->u.a.capacity > v->u.a.size){
        v->u.a.capacity = v->u.a.size;
        v->u.a.e = (json_value*)realloc(v->u.a.e, v->u.a.capacity * sizeof(json_value));
    }

}

void json_clear_array(json_value *v){
    assert(v != NULL && v->type == JSON_ARRAY);
    json_erase_array_element(v, 0, v->u.a.size);
}

json_value* json_get_array_element(const json_value *v, size_t index){
    assert (v != NULL && v->type == JSON_ARRAY);
    assert (index < v->u.a.size);
    return &v->u.a.e[index];
}

json_value* json_pushback_array_element(json_value *v){
    assert(v != NULL && v->type == JSON_ARRAY);
    if( v->u.a.size == v->u.a.capacity)
	json_reserve_array(v, v->u.a.capacity == 0 ? 2 : v->u.a.capacity * 1.5);
    json_init(&v->u.a.e[v->u.a.size]);
    return &v->u.a.e[v->u.a.size++];
}

void json_popback_array_element(json_value *v){
    assert(v != NULL && v->type  == JSON_ARRAY && v->u.a.size > 0);
    json_free(&v->u.a.e[--v->u.a.size]);
}

json_value* json_insert_array_element(json_value *v, size_t index){
    assert(v != NULL && v->type == JSON_ARRAY && index <= v->u.a.size);
    /*TODO*/
    return NULL;
}

void json_erase_array_element(json_value *v, size_t index, size_t count){
    assert(v != NULL && v->type == JSON_ARRAY && index + count  <= v->u.a.size);
    /*TODO*/
}

void json_set_object(json_value *v ,size_t capacity){
    
    assert(v != NULL);
    json_free(v);
    v->type = JSON_OBJECT;
    v->u.o.size = 0;
    v->u.o.capacity = capacity;
    v->u.o.m = capacity > 0 ? (json_member*)malloc(capacity * sizeof(json_member)) : NULL;
}

size_t json_get_object_size(const json_value *v){
    assert(v != NULL && v->type == JSON_OBJECT);
    return v->u.o.size;
}

size_t json_get_object_capacity(const json_value* v){
    assert(v != NULL && v->type == JSON_OBJECT);
    /*TODO*/
    return 0;
}

void json_reserve_object(json_value *v, size_t capacity){
    assert(v != NULL && v->type == JSON_OBJECT);
    /*TODO*/
}

void json_shrink_object(json_value *v){
    assert (v != NULL && v->type == JSON_OBJECT);
    /*TODO*/
}

void json_clear_object(json_value *v){
    assert(v != NULL && v->type == JSON_OBJECT);
    /*TODO*/

}

const char* json_get_object_key(const json_value *v, size_t index){
    assert(v != NULL && v->type == JSON_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].k;
}

size_t json_get_object_key_length(const json_value *v, size_t index){
    assert(v != NULL && v->type == JSON_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].klen;
}

json_value* json_get_object_value(const json_value *v, size_t index){
    assert(v != NULL && v->type == JSON_OBJECT);
    assert(index < v->u.o.size);
    return &v->u.o.m[index].v;
}

size_t json_find_object_index(const json_value *v, const char *key, size_t klen){
    size_t i;
    assert(v != NULL && v->type == JSON_OBJECT && key != NULL);
    for(i = 0; i != v->u.o.size; ++i){
        if(v->u.o.m[i].klen == klen && memcmp(v->u.o.m[i].k, key, klen) == 0)
            return i;
    }
    return JSON_KEY_NOT_EXIST;
}

json_value* json_find_object_value(json_value *v, const char *key, size_t klen){
    size_t index = json_find_object_index(v, key, klen);
    return index != JSON_KEY_NOT_EXIST ? &v->u.o.m[index].v : NULL;
}

json_value* json_set_object_value(json_value *v, const char* key, size_t klen){
    assert(v != NULL && v->type == JSON_OBJECT && key != NULL);
    /*TODO*/
    return NULL;
}

void json_remove_object_value(json_value *v, size_t index){
    assert(v != NULL && v->type == JSON_OBJECT && index < v->u.o.size);
    /*TODO*/
    
}

int json_is_equal(const json_value *lhs, const json_value *rhs){
    size_t i;
    assert(lhs != NULL && rhs != NULL);
    if(lhs->type != rhs->type)
        return 0;
    switch(lhs->type){
        case JSON_STRING:   return lhs->u.s.len == rhs->u.s.len && memcpy(lhs->u.s.s, rhs->u.s.s, lhs->u.s.len) == 0;
        case JSON_NUMBER:   return lhs->u.n == rhs->u.n;
        case JSON_ARRAY:
            if(lhs->u.a.size != rhs->u.a.size)
                return 0;
            for(i = 0; i != lhs->u.a.size; ++i)
                if(!json_is_equal(&lhs->u.a.e[i], &rhs->u.a.e[i]))
                    return 0;
            return 1;
        case JSON_OBJECT:
            /**/
        default:
            return 1;
    }
}

void json_copy(json_value *dst, const json_value *src){
    size_t i,len;
    assert(src != NULL && dst != NULL & src != dst);
    switch(src->type){
        case JSON_STRING:
            json_set_string(dst, src->u.s.s, src->u.s.len);
            break;
        case JSON_ARRAY:
            /*TODO*/
	    len = json_get_array_size(src);
	    /*
	    for(i = 0; i != len; ++i){
		json_copy(dst+i,(src+i)->u.a.e);
	    }*/
            break;
        case JSON_OBJECT:
            /*TODO*/
            break;
        default:
            json_free(dst);
            memcpy(dst, src, sizeof(json_value));
            break;
    }
}

void json_move(json_value *dst, json_value *src){
    assert(dst != NULL && src != NULL && src != dst);
    json_free(dst);
    memcpy(dst, src, sizeof(json_value));
    json_init(src);
}

void json_swap(json_value *lhs, json_value *rhs){
    assert(lhs != NULL && rhs != NULL);
    if( lhs != rhs){
	json_value temp; 
	memcpy(&temp, lhs, sizeof(json_value));
	memcpy(lhs,   rhs, sizeof(json_value));
	memcpy(rhs, &temp, sizeof(json_value));

    }
}
