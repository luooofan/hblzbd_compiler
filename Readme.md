# hblzbd_compiler

## NOTE

该分支(opt_genarm_regalloc)实现了对Sysy2021语言的词法分析、语法分析、语义分析、中间代码生成、目标代码生成全过程：
- 使用flex完成词法分析
- 自行设计了抽象语法树结点，使用bison完成语法分析，构造出一棵抽象语法树
- 自行设计了带label语句的四元式中间表示和符号表数据结构
- 遍历抽象语法树，进行语义分析和中间代码生成，生成了四元式列表和符号表
- 从四元式列表和符号表构造出中间表示的控制流图
- 基于Arm指令集参考卡设计了Arm指令数据结构
- 从中间表示的控制流图先生成带虚拟寄存器的目标代码
- 通过图着色寄存器分配生成最终可执行的目标代码文件
- 优化部分只完成了将函数参数和局部变量尽可能地放到寄存器中，使用指令代替除和取余的函数调用这两个优化

最终能跑通2020和2021年全部功能、性能测例

但由于四元式和符号表数据结构在设计时未考虑充分，同时想使用SSA形式IR做中间代码优化进而转为目标代码，不打算再维护从四元式到目标代码的转化，故该分支停止开发

## Build

```shell
make -j
```

## Run

**Usage: ./compiler [-S] [-l log_file] [-o output_file] [-O level] input_file**

```shell
./compiler -S -o testcase.s testcase.sy
```

## Test

**Usage: test_script.py [-h] [-r] [-v] [-L LINKED_LIBRARY_PATH] test_path**

```shell
python3 ./utils/test_script.py "./official_test/functional_test/*.sy" -r
python3 ./utils/test_script.py "./official_test/performance_test/*.sy" -r -v 
```
