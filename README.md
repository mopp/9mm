# 9mm
Hobby C Compiler

based on [低レイヤを知りたい人のためのCコンパイラ作成入門](https://www.sigbus.info/compilerbook/#%E9%9B%BB%E5%8D%93%E3%83%AC%E3%83%99%E3%83%AB%E3%81%AE%E8%A8%80%E8%AA%9E%E3%81%AE%E4%BD%9C%E6%88%90).


# Production rule
```txt
program    = global
global     = (decl_var ";")* |
             function* |
             struct* |
             enum* |
             "typedef" struct ident ident ";"
             "expern" type ";"
struct     = "struct" ident ";"
             "struct" ident "{" decl_var; "}" ";"
enum       = "enum" "{" (ident (= num)? ",")+ ident (= num)? "}" ";"
function   = type ident "(" (decl_var ("," decl_var)*)* ")" block
             type ident "(" (decl_var ("," decl_var)*)* ")";
block      = "{" stmt* "}"
stmt       = "if" "(" expr ")" stmt ("else" stmt)? |
             "while" "(" expr ")" stmt |
             "for" "(" expr? ";" expr? ")" stmt |
             block |
             "return" expr? ";" |
             expr ";" |
             "break" ";"
expr       = ("(" type ")")? assign
assign     = and (("=" | "+=" | "-=" | "*=" | "/=") expr)?
and        = equality (("&&" | "||") and)?
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = "sizeof" "(" unary | "struct" ident )" |
             ("+" | "-" | "&" | "*" | "!")? term
term       = num |
             ident "(" (expr ("," expr)*)* ")" |
             ("++" | "--")* ref_var |
             decl_var |
             "(" expr ")
decl_var   = type ident ("[" num "]")
ref_var    = ident ( ("[" expr "]")? (("." | "->") ident)? )* ("++" | "--")*
type       = "static"? "struct"? ident ("const"+ "*")*
ident      = chars (chars | num)+
chars      = [a-zA-Z_]
num        = [0-9]+
```
