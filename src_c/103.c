#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <ctype.h>

#include "103.h"
#include "tape.h"

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