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
