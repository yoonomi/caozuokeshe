#!/bin/bash

# test_demo.sh - 系统调用测试演示脚本
# 作者：Yomi
# 功能：编译并演示自定义系统调用测试程序

echo "===========================================" 
echo "    自定义系统调用测试演示"
echo "    System Call Test Demo"
echo "==========================================="
echo

# 检查test_syscall.c是否存在
if [ ! -f "test_syscall.c" ]; then
    echo "❌ 错误: test_syscall.c 文件不存在"
    echo "请确保在正确的目录中运行此脚本"
    exit 1
fi

echo "📁 当前目录: $(pwd)"
echo "📄 源文件: test_syscall.c"
echo

# 编译程序
echo "🔨 正在编译程序..."
gcc -Wall -Wextra -std=c99 -g -o test_syscall test_syscall.c

# 检查编译是否成功
if [ $? -eq 0 ]; then
    echo "✅ 编译成功!"
    echo "📦 生成可执行文件: test_syscall"
else
    echo "❌ 编译失败!"
    echo "请检查源代码是否有错误"
    exit 1
fi

echo
echo "==========================================="
echo "    开始测试运行"
echo "==========================================="
echo

# 运行程序
echo "🚀 运行测试程序 - 单次调用:"
echo "-------------------------------------------"
./test_syscall

echo
echo "-------------------------------------------"
echo "🚀 运行测试程序 - 多次调用:"
echo "-------------------------------------------"
./test_syscall -m 3

echo
echo "==========================================="
echo "    重要说明"
echo "==========================================="
echo "📝 此程序在当前系统中运行将显示错误，这是正常的！"
echo "📝 因为我们尚未在内核中实际添加这个系统调用。"
echo
echo "🔍 预期的错误信息:"
echo "   ✗ 状态: 失败 (FAILED)"
echo "   ✗ 错误码: 38"
echo "   ✗ 错误描述: Function not implemented"
echo "   ✗ 可能原因: 系统调用未实现或内核版本不匹配"
echo
echo "🎯 要使程序正常工作，您需要:"
echo "   1. 修改内核源码添加系统调用"
echo "   2. 重新编译并安装新内核"
echo "   3. 重启使用新内核"
echo "   4. 再次运行此测试程序"
echo
echo "💡 提示: 查看 README.md 获取详细的内核修改指导"
echo "===========================================" 