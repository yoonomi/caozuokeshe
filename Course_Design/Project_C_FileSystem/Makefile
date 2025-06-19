# 文件系统项目 Makefile
# 编译模拟文件系统

# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O0
LDFLAGS = 

# 目标文件
TARGET = filesystem
OBJS = main.o file_ops.o fs_ops.o user_manager.o disk_simulator.o

# 头文件依赖
HEADERS = fs.h

# 默认目标
all: $(TARGET)

# 编译可执行文件
$(TARGET): $(OBJS)
	@echo "正在链接 $(TARGET)..."
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "编译完成: $(TARGET)"

# 编译目标文件
%.o: %.c $(HEADERS)
	@echo "正在编译 $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# 清理编译文件
clean:
	@echo "清理编译文件..."
	rm -f $(OBJS) $(TARGET)
	@echo "清理完成"

# 深度清理（包括备份文件等）
distclean: clean
	rm -f *~ *.bak *.swp
	rm -f core
	@echo "深度清理完成"

# 安装（简单的复制到系统目录）
install: $(TARGET)
	@echo "安装 $(TARGET) 到 /usr/local/bin/..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod 755 /usr/local/bin/$(TARGET)
	@echo "安装完成"

# 卸载
uninstall:
	@echo "卸载 $(TARGET)..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "卸载完成"

# 运行程序
run: $(TARGET)
	@echo "运行文件系统模拟器..."
	./$(TARGET)

# 用户保护测试
user_protection_test: user_protection_test.o user_manager.o file_ops.o fs_ops.o disk_simulator.o
	@echo "编译用户保护测试程序..."
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "运行用户保护测试..."
	./user_protection_test

# 调试运行
debug: $(TARGET)
	@echo "使用gdb调试运行..."
	gdb ./$(TARGET)

# 内存检查
valgrind: $(TARGET)
	@echo "使用valgrind检查内存..."
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# 代码格式化
format:
	@echo "格式化代码..."
	indent -linux *.c *.h
	@echo "格式化完成"

# 生成文档
doc:
	@echo "生成代码文档..."
	doxygen Doxyfile 2>/dev/null || echo "请先安装doxygen并配置Doxyfile"

# 代码统计
stats:
	@echo "代码统计信息:"
	@echo "================"
	@wc -l *.c *.h
	@echo "================"
	@echo "总文件数: $(shell ls *.c *.h 2>/dev/null | wc -l)"

# 创建发布包
package: clean
	@echo "创建发布包..."
	@mkdir -p ../filesystem-release
	@cp -r . ../filesystem-release/filesystem-src
	@cd .. && tar -czf filesystem-$(shell date +%Y%m%d).tar.gz filesystem-release/
	@rm -rf ../filesystem-release
	@echo "发布包已创建: ../filesystem-$(shell date +%Y%m%d).tar.gz"

# 测试编译（检查语法错误）
test-compile:
	@echo "测试编译..."
	$(CC) $(CFLAGS) -fsyntax-only *.c
	@echo "语法检查通过"

# 生成依赖关系
depend:
	@echo "生成依赖关系..."
	$(CC) -MM *.c > .depend
	@echo "依赖关系已生成到 .depend 文件"

# 包含依赖关系文件
-include .depend

# 显示帮助信息
help:
	@echo "可用的Make目标:"
	@echo "=================="
	@echo "all         - 编译程序 (默认)"
	@echo "clean       - 清理编译文件"
	@echo "distclean   - 深度清理"
	@echo "install     - 安装到系统"
	@echo "uninstall   - 从系统卸载"
	@echo "run         - 编译并运行"
	@echo "debug       - 使用gdb调试"
	@echo "valgrind    - 内存检查"
	@echo "format      - 代码格式化"
	@echo "doc         - 生成文档"
	@echo "stats       - 代码统计"
	@echo "package     - 创建发布包"
	@echo "test-compile- 测试编译"
	@echo "depend      - 生成依赖"
	@echo "help        - 显示此帮助"
	@echo "=================="

# 声明伪目标
.PHONY: all clean distclean install uninstall run debug valgrind format doc stats package test-compile depend help 