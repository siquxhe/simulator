#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <ctype.h>
#include <string.h>

#include "103.h"
#include "tape.h"
#include "image_format.h"

FILE * output_file;
FILE * input_file;

num_t * machine_mem;
num_t reg_addr;

num_t reg_a;
num_t reg_b;
num_t reg_c;
num_t reg_d;

num_t reg_pc;
num_t reg_op;

int switch_output_numeral;
int switch_input_numeral;
int flag_stop_after_output;
int halt_after_output;

void set_pc(int val) {
    reg_pc = val & 0xfff;
}

void set_output_numeral(int val) {
    switch_output_numeral = val;
}

void set_input_numeral(int val) {
    switch_input_numeral = val;
}

void set_stop_after_output(int val) {
    flag_stop_after_output = val;
}
void init_mem() {
    machine_mem = malloc(sizeof(num_t) * 04000);
    /* 装载前清零：BRAM/堆上电不定，未覆盖地址不得留垃圾值参与运算（4f §5.2-5）。 */
    memset(machine_mem, 0, sizeof(num_t) * 04000);
}

void print_mem() {
    FILE * mem_file = fopen("mem.txt", "w");
    for (int i = 0; i < 2048; i++) {
        fprintf(mem_file, "%04o ", i);
        fprintf(mem_file, "%c ", (i & 0x40000000)? '-' : '+');
        fprintf(mem_file, "%010o\n", machine_mem[i] & 0x3fffffff);
    }
}

void run_machine() {
    num_t c1, c2;
    num_t op1, op2;
    while (1) {
        if (halt_after_output) {
            break;
        }
        // 1
        reg_addr = reg_pc;
        reg_pc = reg_pc + 1;
        
        // 2
        reg_c = machine_mem[reg_addr];
        reg_op = (reg_c >> 24) & 0x3f;
        op1 = reg_op >> 3;
        op2 = reg_op & 0x7;
        c1 = (reg_c >> 12) & 0xfff;
        c2 = (reg_c) & 0xfff;
        
        if (op2 == OP_ADD || op2 == OP_SUB || op2 == OP_DIV || op2 == OP_MUL || op2 == OP_AND) {
            reg_addr = c1;
            reg_c = machine_mem[reg_addr];
            reg_a = reg_c;

            reg_addr = c2;
            if (op1 == 0 || op1 == 1 || op1 == 4 || op1 == 5) {
                reg_c = machine_mem[reg_addr];
                reg_b = reg_c;
            }

            int sign_a, sign_b;
            if (op1 == 5 || op1 == 7) {
                sign_a = 1;
                sign_b = 1;
            } else {
                sign_a = (reg_a & 0x40000000) ? -1 : 1;
                sign_b = (reg_b & 0x40000000) ? -1 : 1;
            }

            if (op2 == OP_ADD) {
                reg_c = do_add(sign_a, reg_a & 0x3fffffff, sign_b, reg_b & 0x3fffffff);
            } else if (op2 == OP_SUB) {
                reg_c = do_sub(sign_a, reg_a & 0x3fffffff, sign_b, reg_b & 0x3fffffff);
            } else if (op2 == OP_DIV) {
                reg_c = do_div(sign_a, reg_a & 0x3fffffff, sign_b, reg_b & 0x3fffffff);
            } else if (op2 == OP_MUL) {
                reg_c = do_mul(sign_a, reg_a & 0x3fffffff, sign_b, reg_b & 0x3fffffff);
            } else if (op2 == OP_AND) {
                reg_c = do_and(sign_a, reg_a & 0x3fffffff, sign_b, reg_b & 0x3fffffff);
            }

            reg_b = reg_c;

            if (op1 == 0 || op1 == 2 || op1 == 4 || op1 == 6) {
                machine_mem[reg_addr] = reg_c;
            }
            if (op1 == 4 || op1 == 6) {
                do_output_reg_c();
            }

        } else if (reg_op == 007 || reg_op == 027) {
            // TODO: not support input
            reg_addr = c2;
            do_input(1);

        } else if (reg_op == 005 || reg_op == 015) {
            reg_addr = c1;
            reg_c = machine_mem[reg_addr];
            reg_addr = c2;
            machine_mem[reg_addr] = reg_c;
            reg_b = reg_c;

        } else if (reg_op == 045 || reg_op == 055) {
            reg_addr = c1;
            reg_c = machine_mem[reg_addr];
            reg_addr = c2;
            machine_mem[reg_addr] = reg_c;
            do_output_reg_c();

        } else if (reg_op == 024) {
            reg_pc = c1;
            reg_addr = c2;
            reg_c = reg_b;
            machine_mem[reg_addr] = reg_c;

        } else if (reg_op == 064) {
            reg_pc = c1;
            reg_addr = c2;
            reg_c = reg_b;
            machine_mem[reg_addr] = reg_c;
            do_output_reg_c();

        } else if (reg_op == 074) {
            reg_pc = c2;
            reg_b = reg_b & 0x3fffffff;

        } else if (reg_op == 034) {
            if (reg_b & 0x40000000) {
                reg_pc = c1;
            } else {
                reg_pc = c2;
            }

        } else if (reg_op == 004 || reg_op == 014 || reg_op == 044) {
            reg_pc = c1;
            reg_a = reg_c;
            reg_addr = c2;
            reg_c = machine_mem[reg_addr];
            reg_b = reg_c;
            break;

        } else if (reg_op == 054) {
            reg_pc = c1;
            reg_a = reg_c;
            reg_a = reg_a & 0x3fffffff;
            reg_addr = c2;
            reg_c = machine_mem[reg_addr];
            reg_b = reg_c;
            reg_b = reg_b & 0x3fffffff;
            break;

        } else if (reg_op == 017) {
            reg_addr = c1;
            reg_c = machine_mem[reg_addr];
            reg_addr = c2;
            reg_a = reg_c;
            reg_c = machine_mem[reg_addr];
            reg_b = reg_c;
            break;

        }  else if (reg_op == 037) {
            reg_addr = c1;
            reg_c = machine_mem[reg_addr];
            reg_addr = c2;
            reg_a = reg_c;
            reg_c = reg_b;
            break;

        } else if (reg_op == 057) {
            reg_addr = c1;
            reg_c = machine_mem[reg_addr];
            reg_addr = c2;
            reg_a = reg_c;
            reg_c = machine_mem[reg_addr];
            reg_b = reg_c;
            reg_a = reg_a & 0x3fffffff;
            reg_b = reg_b & 0x3fffffff;
            break;

        } else if (reg_op == 077) {
            reg_addr = c1;
            reg_c = machine_mem[reg_addr];
            reg_addr = c2;
            reg_a = reg_c;
            reg_c = reg_b;
            reg_a = reg_a & 0x3fffffff;
            reg_b = reg_b & 0x3fffffff;
            break;

        } else {
            fprintf(stderr, "unknow reg_op code\n");
            exit(EXIT_FAILURE);
        }
        print_status();
    }

    print_status();
}

void do_input(int input_once) {
    char ch;
    // 初始程序纸带装载（input_once==0）恒为八进制；运行期录入按输入进制开关。
    // 十进制为 BCD 打包：每位占 4 bit；八进制每位占 3 bit。
    int dec = (input_once && switch_input_numeral == DECIMAL);
    int shift = dec ? 4 : 3;
    int sign_neg = 0;
    while ((ch = fgetc(input_file)) != EOF) {
        if (ch == SPACE) {
            // nothing to do
        } else if (ch == POS) {
            if (dec) {
                reg_c = 0;
                sign_neg = 0;
            } else {
                reg_c <<= 3;
                reg_c |= 0x6;
            }
        } else if (ch == NEG) {
            if (dec) {
                reg_c = 0;
                sign_neg = 1;
            } else {
                reg_c <<= 3;
                reg_c |= 0x7;
            }
        } else if (ch >= NUM0 && ch <= NUM9) {
            reg_c <<= shift;
            reg_c |= (ch - NUM0);
        } else if (ch == SET_ADD) {
            reg_addr = reg_c & 0xfff;
        } else if (ch == WRITE) {
            if (dec) {
                // BCD 左对齐到 30 位数值域顶部（7 位 ×4 bit，低 2 bit 留空），与输出解包位序一致
                reg_c <<= 2;
                if (sign_neg) {
                    reg_c |= 0x40000000;
                }
            }
            machine_mem[reg_addr] = reg_c;
            if (input_once) break;
        } else if (ch == END_ARR) {
            break;
        }
    }
}

void do_output_reg_c() {
    if (flag_stop_after_output) {
        halt_after_output = 1;
    }
    fprintf(output_file, "%c", (reg_c & 0x40000000) ? '-' : '+');
    if (switch_output_numeral == DECIMAL) {
        for (int i = 26; i >= 0; i -= 4) {
            fprintf(output_file, "%d", (reg_c >> i) & 0xF);
        }
    } else if (switch_output_numeral == OCTAL) {
        for (int i = 27; i >= 0; i -= 3) {
            fprintf(output_file, "%d", (reg_c >> i) & 0x7);
        }
    }
    fprintf(output_file, "\n");
}

num_t do_add(int sign_a, int num_a, int sign_b, int num_b) {
    int res = (sign_b * num_b) + (sign_a * num_a);
    num_t ans;
    if (res > 0x3fffffff || res < -0x3fffffff) {
        fprintf(stderr, "overflow\n");
        exit(EXIT_FAILURE);
    }
    if (res > 0) {
        ans = res & 0x3fffffff;
    } else if (res < 0) {
        ans = ((-res) & 0x3fffffff) | 0x40000000;
    } else {
        ans = (sign_b < 0) << 30;
    }
    return ans;
}

num_t do_sub(int sign_a, int num_a, int sign_b, int num_b) {
    int res = (sign_b * num_b) - (sign_a * num_a);
    num_t ans;
    if (res > 0x3fffffff || res < -0x3fffffff) {
        fprintf(stderr, "overflow\n");
        exit(EXIT_FAILURE);
    }
    if (res > 0) {
        ans = res & 0x3fffffff;
    } else if (res < 0) {
        ans = ((-res) & 0x3fffffff) | 0x40000000;
    } else {
        ans = (sign_b < 0) << 30;
    }
    return ans;
}

num_t do_div(int sign_a, long long num_a, int sign_b, long long num_b) {
    long long res = (((long long)num_b << 30) / num_a);
    num_t ans;
    if (res > 0x3fffffff) {
        fprintf(stderr, "overflow\n");
        exit(EXIT_FAILURE);
    }
    if (sign_a * sign_b > 0) {
        ans = res & 0x3fffffff;
    } else if (sign_a * sign_b < 0) {
        ans = (res & 0x3fffffff) | 0x40000000;
    } else {
        ans = (sign_b < 0) << 30;
    }
    return ans;
}

num_t do_mul(int sign_a, long long num_a, int sign_b, long long num_b) {
    long long res = (((long long)num_b * num_a) >> 30);
    num_t ans;
    if (res > 0x3fffffff) {
        fprintf(stderr, "overflow\n");
        exit(EXIT_FAILURE);
    }
    if (sign_a * sign_b > 0) {
        ans = res & 0x3fffffff;
    } else if (sign_a * sign_b < 0) {
        ans = (res & 0x3fffffff) | 0x40000000;
    } else {
        ans = (sign_b < 0) << 30;
    }
    return ans;
}

num_t do_and(int sign_a, int num_a, int sign_b, int num_b) {
    int res = num_b & num_a;
    num_t ans = res | ((sign_b < 0) << 30);
    return ans;
}

void print_status() {
    printf("pc: %04o\t", reg_pc);
    printf("addr: %04o\t", reg_addr);
    printf("op: %02o\t", reg_op);
    printf("c: %c %010o", (reg_c & 0x40000000) ? '-' : '+', reg_c & 0x3fffffff);
    printf("\n");
}

int load_image(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "load_image: 打不开 %s\n", path); return 1; }

    if (fseek(f, 0, SEEK_END) != 0) { fprintf(stderr, "load_image: 定位镜像失败\n"); fclose(f); return 1; }
    long sz = ftell(f);
    if (sz < 0) { fprintf(stderr, "load_image: 读取镜像大小失败\n"); fclose(f); return 1; }
    rewind(f);

    if (sz < (long)(4 * 3)) { fprintf(stderr, "load_image: 镜像过短（不足目录头+校验尾）\n"); fclose(f); return 1; }
    if (sz % 4 != 0) { fprintf(stderr, "load_image: 镜像长度非 4 字节倍数\n"); fclose(f); return 1; }

    uint32_t nwords = (uint32_t)(sz / 4);
    uint32_t *w = malloc((size_t)sz);
    if (!w) { fprintf(stderr, "load_image: 内存分配失败\n"); fclose(f); return 1; }

    uint8_t buf[4];
    for (uint32_t i = 0; i < nwords; i++) {
        if (fread(buf, 1, 4, f) != 4) {
            fprintf(stderr, "load_image: 读取截断\n"); free(w); fclose(f); return 1;
        }
        w[i] = img_get_le32(buf);
    }
    fclose(f);

    /* 魔数 */
    if (w[0] != IMG_MAGIC) {
        fprintf(stderr, "load_image: 魔数错（非合法镜像）\n"); free(w); return 1;
    }

    uint32_t n         = img_hdr1_n(w[1]);
    uint32_t entry     = img_hdr1_entry(w[1]);
    uint32_t ver_algo  = img_hdr1_ver_algo(w[1]);
    uint32_t ver       = ver_algo >> 3;
    uint32_t algo      = ver_algo & 0x7u;

    if (ver != IMG_VERSION) { fprintf(stderr, "load_image: 镜像版本不支持\n"); free(w); return 1; }
    if (algo != IMG_ALGO_SUM) { fprintf(stderr, "load_image: 校验算法不支持（CRC-16 预留）\n"); free(w); return 1; }
    if (n == 0 || n > IMG_MAX_SEG) { fprintf(stderr, "load_image: 段数非法（%u）\n", n); free(w); return 1; }
    if (entry >= IMG_WORDS) { fprintf(stderr, "load_image: 入口 PC 越界\n"); free(w); return 1; }

    const uint32_t hdr_words = 2;
    uint32_t body_start_word = hdr_words + 2u * n;
    uint32_t body_words = 0;

    /* 第一遍：类型/字数/目的边界校验，累计镜像体字数。 */
    for (uint32_t i = 0; i < n; i++) {
        uint32_t a   = w[hdr_words + 2u * i];
        uint32_t b   = w[hdr_words + 2u * i + 1];
        uint32_t dst = img_seg_a_dst(a);
        uint32_t type = img_seg_a_type(a);
        uint32_t len = img_seg_b_len(b);

        if (type == SEG_RSV || type > SEG_TYPE_MAX) {
            fprintf(stderr, "load_image: 段 %u 类型保留/未知（%u）\n", i, type); free(w); return 1;
        }
        if (len == 0) { fprintf(stderr, "load_image: 段 %u 字数为 0\n", i); free(w); return 1; }
        if (dst + len > IMG_WORDS) { fprintf(stderr, "load_image: 段 %u 目的越界（%u+%u）\n", i, dst, len); free(w); return 1; }
        body_words += len;
    }

    /* 第二遍：body 内字索引范围校验。 */
    for (uint32_t i = 0; i < n; i++) {
        uint32_t a   = w[hdr_words + 2u * i];
        uint32_t b   = w[hdr_words + 2u * i + 1];
        uint32_t off = img_seg_a_off(a);
        uint32_t len = img_seg_b_len(b);
        if (off + len > body_words) {
            fprintf(stderr, "load_image: 段 %u 镜像体偏移越界（索引 %u, 字数 %u）\n", i, off, len);
            free(w); return 1;
        }
    }

    if (body_start_word + body_words + 1u != nwords) {
        fprintf(stderr, "load_image: 镜像体字数与段表不符\n"); free(w); return 1;
    }

    /* 复核整体校验和（覆盖目录头 + 段表 + 镜像体，不含校验尾）。 */
    uint32_t sum = 0;
    for (uint32_t i = 0; i < nwords - 1u; i++) {
        sum = (sum + img_sum_word(w[i])) & IMG_SUM_MASK;
    }
    if (sum != w[nwords - 1u]) {
        fprintf(stderr, "load_image: 校验和不匹配（算出 %08x，镜像 %08x）\n", sum, w[nwords - 1u]);
        free(w); return 1;
    }

    /* 装载：先清零主存，再按段写入。 */
    memset(machine_mem, 0, sizeof(num_t) * 04000);
    for (uint32_t i = 0; i < n; i++) {
        uint32_t a   = w[hdr_words + 2u * i];
        uint32_t b   = w[hdr_words + 2u * i + 1];
        uint32_t dst = img_seg_a_dst(a);
        uint32_t off = img_seg_a_off(a);
        uint32_t len = img_seg_b_len(b);
        for (uint32_t k = 0; k < len; k++) {
            machine_mem[dst + k] = w[body_start_word + off + k];
        }
    }

    free(w);
    set_pc((int)entry);
    return 0;
}