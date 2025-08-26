#ifndef __KLIBTEST_H__
#define __KLIBTEST_H__

#include <klib.h>
#include <klib-macros.h>

/**
 * @brief 辅助打印函数
 * 
 * @param name 测试名称
 * @param passed 测试是否通过
 */
void print_result(const char *name, bool passed);

#endif /* __KLIBTEST_H__ */
