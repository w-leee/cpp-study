# C++ 现代工程化与并发编程实战 (cpp-study)

基于现代 C++ (C++11/14/17) 的实战仓库。采用**多级 CMake 架构**

---

## 1. 新增版本

需要扩展时（如引入 OpenCV、TensorRT），新建独立版本目录，不改动旧版本：

```bash
# 1. 复制源码
cd ~/workspace/study
cp -r steps/basic/v2_threading steps/basic/v3_opencv
```

1. 修改 `v3_opencv/CMakeLists.txt`：所有 `run_v2` 改名为 `run_v3`；新库在此加 `find_package` 和 `target_link_libraries`。
2. 在最外层 `study/CMakeLists.txt` 追加：
   ```cmake
   add_subdirectory(steps/basic/v3_opencv)
   ```
3. 在 `.vscode/launch.json` 中新增对应版本的调试配置（复制现有一段，改三处：`name`、`program`、`cwd`）：
   ```json
   {
       "name": "🧪 调试 v3_opencv",
       "type": "cppdbg",
       "request": "launch",
       "program": "${workspaceFolder}/build/steps/basic/v3_opencv/run_v3",
       "args": [],
       "stopAtEntry": false,
       "cwd": "${workspaceFolder}/build/steps/basic/v3_opencv",
       "environment": [],
       "externalConsole": false,
       "MIMode": "gdb",
       "setupCommands": [
           {
               "description": "Enable pretty-printing for gdb",
               "text": "-enable-pretty-printing",
               "ignoreFailures": true
           }
       ],
       "console": "integratedTerminal"
   }
   ```
   注意 `program` 和 `cwd` 必须指向新版本在 `build` 下的实际路径，否则 F5 会报「文件不存在」。

---

## 2. 编译流程

> 只在 `build` 根目录执行编译，不要在子目录里操作。

```bash
cd ~/workspace/study/build
rm -rf *        # 清空缓存，防止旧文件污染
cmake ..        # 生成 Makefile
make            # 全量编译
```


---

## 3. 运行测试

`tasks.db` 使用相对路径，程序在哪个目录运行，数据就生成在哪。**进入各自子目录运行**，实现数据隔离。

```bash

cd ~/workspace/study/build/steps/basic/v1_base
./run_v1

cd ~/workspace/study/build/steps/basic/v2_threading
./run_v2
```

---

## 4. VS Code 断点调试

`.vscode/launch.json` 已配置双版本调试。在代码左侧点断点，「运行和调试」面板中选择：

- 🐞 调试单线程版 (v1_base)
- 🚀 调试多线程版 (v2_threading)

按 `F5` 即可（注意 `program` 和 `cwd` 路径指向）。

---

## 5. 内存检测 (Valgrind)

```bash
sudo apt update && sudo apt install valgrind -y

cd ~/workspace/study/build/steps/basic/v2_threading
valgrind --leak-check=full ./run_v2
```
---

## 6. Git 版本控制

`.gitignore` 已屏蔽 `build/` 目录。

```bash
cd ~/workspace/study
git add .
git commit -m "feat: xxxxx"  
git push
```