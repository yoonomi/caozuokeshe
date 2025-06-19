#!/bin/bash

# 文件系统交互式Shell演示脚本
# 这个脚本演示了新的命令行界面的所有功能

echo "========================================="
echo "文件系统交互式Shell演示"
echo "========================================="

# 创建命令输入文件
cat > commands_demo.txt << 'EOF'
help
init
users
login root root123
whoami
adduser alice alice123 1001 1001
adduser bob bob456 1002 1002
users
create test1.txt
open test1.txt
write 0 Hello World! This is a test file.
tell 0
size 0
read 0 50
seek 0 6 0
read 0 5
login alice alice123
whoami
create alice_private.txt
open alice_private.txt
write 1 This is Alice's private file.
close 1
login bob bob456
whoami
open test1.txt
read 2 20
write 2 Bob tries to modify this.
login root root123
chmod 0 600
login bob bob456
read 2 10
login alice alice123
open alice_private.txt
read 3 30
close 3
login root root123
ls
status
exit
EOF

echo "正在运行演示命令..."
echo "演示将展示以下功能："
echo "- 系统初始化和用户管理"
echo "- 文件创建、打开、读写操作"
echo "- 权限控制和用户切换"
echo "- 文件指针操作（seek/tell）"
echo "- 权限检查和访问控制"
echo ""
echo "开始演示..."
echo ""

# 运行演示
./filesystem < commands_demo.txt

echo ""
echo "========================================="
echo "演示完成！"
echo "========================================="
echo ""
echo "你可以手动运行 './filesystem' 来交互式使用文件系统"
echo ""
echo "主要命令："
echo "  init              - 初始化文件系统"
echo "  login <user> <pw> - 用户登录"
echo "  create <file>     - 创建文件"
echo "  open <file>       - 打开文件"
echo "  write <fd> <text> - 写入文件"
echo "  read <fd> <bytes> - 读取文件"
echo "  help              - 查看所有命令"
echo "  exit              - 退出程序"

# 清理临时文件
rm -f commands_demo.txt 