# C++编程作业集合

这个仓库包含了我的C++编程作业集合，每个文件夹代表一个独立的编程题目。

## 仓库结构

每个题目目录都包含以下内容：
- `README.md`: 题目描述
- `remark.md`: 代码评价和技术分析
- `src/`: 源代码文件夹

## 题目列表

1. [学生数据处理系统](./problem1/README.md) - 使用现代C++20/23实现的学生数据处理系统，应用了命令模式、工厂函数和策略模式设计思想

## 构建和运行

每个项目目录中的src文件夹包含完整的源代码。要编译和运行代码，请进入对应项目的src目录，然后执行：

```bash
clang++ -std=c++23 main.cpp -o main
./main
```

注意：由于使用了C++23特性，需要支持C++23的编译器（我是用的是clang20）。 