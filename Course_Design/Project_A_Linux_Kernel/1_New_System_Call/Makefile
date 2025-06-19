# Makefile for testing custom system call
# 编译自定义系统调用测试程序

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = test_syscall
SOURCE = test_syscall.c

# 默认目标
all: $(TARGET)

# 编译测试程序
$(TARGET): $(SOURCE)
	@echo "编译系统调用测试程序..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)
	@echo "编译完成: $(TARGET)"

# 运行测试
test: $(TARGET)
	@echo "运行系统调用测试..."
	./$(TARGET)

# 多次测试
test-multiple: $(TARGET)
	@echo "运行多次系统调用测试..."
	./$(TARGET) -m 5

# 清理
clean:
	@echo "清理编译文件..."
	rm -f $(TARGET)
	@echo "清理完成"

# 安装（复制到系统目录）
install: $(TARGET)
	@echo "安装测试程序到 /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod 755 /usr/local/bin/$(TARGET)
	@echo "安装完成"

# 卸载
uninstall:
	@echo "卸载测试程序..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "卸载完成"

# 显示帮助
help:
	@echo "可用的Make目标:"
	@echo "=================="
	@echo "all             - 编译程序 (默认)"
	@echo "test            - 编译并运行单次测试"
	@echo "test-multiple   - 编译并运行多次测试"
	@echo "clean           - 清理编译文件"
	@echo "install         - 安装到系统"
	@echo "uninstall       - 从系统卸载"
	@echo "help            - 显示此帮助"
	@echo "=================="

# 声明伪目标
.PHONY: all test test-multiple clean install uninstall help 