# 9mm
Hobby C (subset) Compiler  
`9mm` can compile itself.


## How to build/test
```console
> make 9mm

> ./9mm
Usage:
  ./9mm [--test] [--str 'your program'] [FILEPATH]

  --test run test
  --str  input c codes as a string

# test "9mm"
> make test

# Build "9mms" which is selfhosted 9mm.
> make selfhost

# Test it.
> make test_selfhost
```

## Production rule
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


# Acknowledge and Reference
- [低レイヤを知りたい人のためのCコンパイラ作成入門](https://www.sigbus.info/compilerbook).


# License
The MIT License (MIT)  
See [LICENSE](./LICENSE)
