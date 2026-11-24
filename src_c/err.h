/*
 * err.h 兼容层（mingw-w64 / w64devkit 未提供 BSD err.h）
 * 仅实现本项目用到的 err() / warn() 语义：打印格式串 + strerror(errno)。
 * 通过 -I<compat 目录> 引入，不改动原始源码。
 */
#ifndef COMPAT_ERR_H
#define COMPAT_ERR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

static inline void err(int eval, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(errno));
    exit(eval);
}

static inline void warn(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(errno));
}

#endif /* COMPAT_ERR_H */
