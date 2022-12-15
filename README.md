# Reverse lang
## Theory
We have our common standard of abstract syntax tree (AST) that represents program on our programming languages.

_And it will be written here at some point in the future..._

This project contains the following parts:

* Frontend converts code to AST or AST to code
* *   Lexical analyzer splits code into tokens for syntax analyzer.
* *   Syntax analyzer converts tokens to AST.
* Middleend simplifies AST by collapsing constants and deleting neutral elements
* Backend assembles AST to my own assembler language that can be executed on my own [processor](https://github.com/foxido/cpu)
* Processor emulator of real processor that can execute programs on my assembler language (git submodule)

## Practice
1. Download repository & compile
```bash
git clone --depth=1 --recurse-submodules https://github.com/foxidokun/ReverseLang
cd ReverseLang
make all
```
3. Compile examples && have fun
```bash
./compile_and_run.sh ./examples/fin.edoc
```