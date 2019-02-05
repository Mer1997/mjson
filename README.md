## mjson parse

================= =====  
+Author: mer  
+Date: 2019.1  
================= =====  

---
- 参考教程 [miloyip's json-tutori](https://github.com/miloyip/json-tutorial)
---

2019.1.27

+ 创建了解析器的基础版本，和简单的测试单元:
 - 添加对`null`值的解析
 - 添加对`true`值的解析
 - 添加对`false`值的解析
 - 添加相关测试

---

2019.1.28

+ 重构了部分代码，增加了解析器的功能:
 - 重构`json_parse_null`, `json_parse_true`, `json_parse_false`为 `json_parse_literal`
 - 增加了新的错误码:`JSON_PARSE_NUMBER_TOO_BIG`
 - 为`json_value`添加成员`double n`
 - 添加对`number`值的解析
 - 添加对json数字的测试用例

---

2019.1.30

+ 重构代码,完善功能,增加了对字符串的解析:
 - 为`json_value`添加了新的成员:`char* s`和`size_t len`
 - 重构了代码的逻辑,添加了对`json_value`的内存管理
 - 为`null`,`boolean`,`number`值添加`json_set_null`,`json_set_boolean`和`json_setnumber`访问函数
 - 添加了对`string`值的解析
 - 新增了错误码:`JSON_PARSE_MISS_QUOTATION_MARK`,`JSON_PARSE_INVALID_STRING_ESCAPE`,`JSON_PARSE_INVALID_STRING_CHAR`
 - 为`string`值添加了对应的测试用例
 - 修改了先前的部分测试逻辑

---

2019.2.2

+ 增加字符串对Unicode编码的支持:
 - 新增错误码`JSON_PARSE_INVALID_UNICODE_HEX`和`JSON_PARSE_INVALID_UNICODE_SURROGATE`
 - 完善了对UTF-8编码的解析
 - 添加了相关测试用例

---

2019.2.4

+ 增加了对`array`值的解析:
 - 新增错误码`JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET`
 - 增加了对数组值的解析
 - 修改了部分测试用例
 - 增加了数组部分的测试用例

