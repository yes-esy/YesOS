```c
void kernel_vsprintf(char *buf, const char *fmt, va_list args)
{
    char c;
    char *str;
    int num;
    uint32_t unum;
    char out[128];
    int i, index;
    int min_width = 0;
    int precision = -1;
    int flags = 0;
    
    /* 定义标志位 */
    #define FLAG_LEFT_ALIGN   0x001  /* '-' 左对齐 */
    #define FLAG_PLUS         0x002  /* '+' 显示正负号 */
    #define FLAG_SPACE        0x004  /* ' ' 正数前加空格 */
    #define FLAG_SPECIAL      0x008  /* '#' 特殊格式 */
    #define FLAG_ZERO_PAD     0x010  /* '0' 零填充 */
    #define FLAG_UPPERCASE    0x020  /* 大写字母 */
    #define FLAG_LONG         0x040  /* 'l' 长整型 */
    #define FLAG_LONGLONG     0x080  /* 'll' 长长整型 */
    #define FLAG_UNSIGNED     0x100  /* 无符号类型 */

    while (*fmt) {
        /* 复制普通字符 */
        if (*fmt != '%') {
            *buf++ = *fmt++;
            continue;
        }

        /* 处理格式说明符 */
        fmt++;
        flags = 0;
        min_width = 0;
        precision = -1;

        /* 处理标志 */
        while (1) {
            if (*fmt == '-') {
                flags |= FLAG_LEFT_ALIGN;
            } else if (*fmt == '+') {
                flags |= FLAG_PLUS;
            } else if (*fmt == ' ') {
                flags |= FLAG_SPACE;
            } else if (*fmt == '#') {
                flags |= FLAG_SPECIAL;
            } else if (*fmt == '0') {
                flags |= FLAG_ZERO_PAD;
            } else {
                break;
            }
            fmt++;
        }

        /* 处理宽度 */
        if (*fmt == '*') {
            min_width = va_arg(args, int);
            fmt++;
        } else while (*fmt >= '0' && *fmt <= '9') {
            min_width = min_width * 10 + (*fmt - '0');
            fmt++;
        }

        /* 处理精度 */
        if (*fmt == '.') {
            fmt++;
            if (*fmt == '*') {
                precision = va_arg(args, int);
                fmt++;
            } else {
                precision = 0;
                while (*fmt >= '0' && *fmt <= '9') {
                    precision = precision * 10 + (*fmt - '0');
                    fmt++;
                }
            }
        }

        /* 处理长度修饰符 */
        if (*fmt == 'l') {
            flags |= FLAG_LONG;
            fmt++;
            if (*fmt == 'l') {
                flags |= FLAG_LONGLONG;
                fmt++;
            }
        }

        /* 处理转换说明符 */
        switch (*fmt) {
        case 'd':
        case 'i':
            if (flags & FLAG_LONGLONG) {
                num = (int)va_arg(args, long long);
            } else if (flags & FLAG_LONG) {
                num = (int)va_arg(args, long);
            } else {
                num = va_arg(args, int);
            }
            
            index = 0;
            if (num < 0) {
                num = -num;
                out[index++] = '-';
            } else if (flags & FLAG_PLUS) {
                out[index++] = '+';
            } else if (flags & FLAG_SPACE) {
                out[index++] = ' ';
            }
            
            i = 0;
            char temp[128];
            if (num == 0) {
                temp[i++] = '0';
            } else {
                while (num > 0) {
                    temp[i++] = (num % 10) + '0';
                    num /= 10;
                }
            }
            
            /* 处理精度 */
            int padding = (precision > i) ? precision - i : 0;
            while (padding-- > 0) {
                temp[i++] = '0';
            }
            
            /* 倒序复制 */
            while (i > 0) {
                out[index++] = temp[--i];
            }
            out[index] = '\0';
            
            /* 处理宽度和对齐 */
            int len = kernel_strlen(out);
            if (min_width > len) {
                if (flags & FLAG_LEFT_ALIGN) {
                    /* 左对齐 */
                    kernel_strcpy(buf, out);
                    buf += len;
                    for (i = 0; i < min_width - len; i++) {
                        *buf++ = ' ';
                    }
                } else {
                    /* 右对齐 */
                    char pad = (flags & FLAG_ZERO_PAD) ? '0' : ' ';
                    for (i = 0; i < min_width - len; i++) {
                        *buf++ = pad;
                    }
                    kernel_strcpy(buf, out);
                    buf += len;
                }
            } else {
                kernel_strcpy(buf, out);
                buf += len;
            }
            break;
            
        case 'u':
            flags |= FLAG_UNSIGNED;
            /* 处理无符号整数 */
            if (flags & FLAG_LONGLONG) {
                unum = (uint32_t)va_arg(args, unsigned long long);
            } else if (flags & FLAG_LONG) {
                unum = (uint32_t)va_arg(args, unsigned long);
            } else {
                unum = va_arg(args, unsigned int);
            }
            
            i = 0;
            char utemp[128];
            if (unum == 0) {
                utemp[i++] = '0';
            } else {
                while (unum > 0) {
                    utemp[i++] = (unum % 10) + '0';
                    unum /= 10;
                }
            }
            
            /* 处理精度 */
            padding = (precision > i) ? precision - i : 0;
            while (padding-- > 0) {
                utemp[i++] = '0';
            }
            
            index = 0;
            /* 倒序复制 */
            while (i > 0) {
                out[index++] = utemp[--i];
            }
            out[index] = '\0';
            
            /* 处理宽度和对齐 */
            len = kernel_strlen(out);
            if (min_width > len) {
                if (flags & FLAG_LEFT_ALIGN) {
                    /* 左对齐 */
                    kernel_strcpy(buf, out);
                    buf += len;
                    for (i = 0; i < min_width - len; i++) {
                        *buf++ = ' ';
                    }
                } else {
                    /* 右对齐 */
                    char pad = (flags & FLAG_ZERO_PAD) ? '0' : ' ';
                    for (i = 0; i < min_width - len; i++) {
                        *buf++ = pad;
                    }
                    kernel_strcpy(buf, out);
                    buf += len;
                }
            } else {
                kernel_strcpy(buf, out);
                buf += len;
            }
            break;
            
        case 'x':
        case 'X':
            if (*fmt == 'X') {
                flags |= FLAG_UPPERCASE;
            }
            
            /* 处理16进制整数 */
            if (flags & FLAG_LONGLONG) {
                unum = (uint32_t)va_arg(args, unsigned long long);
            } else if (flags & FLAG_LONG) {
                unum = (uint32_t)va_arg(args, unsigned long);
            } else {
                unum = va_arg(args, unsigned int);
            }
            
            index = 0;
            /* 特殊前缀 0x 或 0X */
            if ((flags & FLAG_SPECIAL) && unum != 0) {
                out[index++] = '0';
                out[index++] = (flags & FLAG_UPPERCASE) ? 'X' : 'x';
            }
            
            i = 0;
            char xtemp[128];
            if (unum == 0) {
                xtemp[i++] = '0';
            } else {
                while (unum > 0) {
                    c = unum % 16;
                    if (c < 10) {
                        xtemp[i++] = c + '0';
                    } else {
                        xtemp[i++] = c - 10 + ((flags & FLAG_UPPERCASE) ? 'A' : 'a');
                    }
                    unum /= 16;
                }
            }
            
            /* 处理精度 */
            padding = (precision > i) ? precision - i : 0;
            while (padding-- > 0) {
                xtemp[i++] = '0';
            }
            
            /* 倒序复制 */
            while (i > 0) {
                out[index++] = xtemp[--i];
            }
            out[index] = '\0';
            
            /* 处理宽度和对齐 */
            len = kernel_strlen(out);
            if (min_width > len) {
                if (flags & FLAG_LEFT_ALIGN) {
                    /* 左对齐 */
                    kernel_strcpy(buf, out);
                    buf += len;
                    for (i = 0; i < min_width - len; i++) {
                        *buf++ = ' ';
                    }
                } else {
                    /* 右对齐 */
                    char pad = (flags & FLAG_ZERO_PAD) ? '0' : ' ';
                    for (i = 0; i < min_width - len; i++) {
                        *buf++ = pad;
                    }
                    kernel_strcpy(buf, out);
                    buf += len;
                }
            } else {
                kernel_strcpy(buf, out);
                buf += len;
            }
            break;
            
        case 'o':
            /* 处理8进制整数 */
            if (flags & FLAG_LONGLONG) {
                unum = (uint32_t)va_arg(args, unsigned long long);
            } else if (flags & FLAG_LONG) {
                unum = (uint32_t)va_arg(args, unsigned long);
            } else {
                unum = va_arg(args, unsigned int);
            }
            
            index = 0;
            /* 特殊前缀 0 */
            if ((flags & FLAG_SPECIAL) && unum != 0) {
                out[index++] = '0';
            }
            
            i = 0;
            char otemp[128];
            if (unum == 0) {
                otemp[i++] = '0';
            } else {
                while (unum > 0) {
                    otemp[i++] = (unum % 8) + '0';
                    unum /= 8;
                }
            }
            
            /* 处理精度 */
            padding = (precision > i) ? precision - i : 0;
            while (padding-- > 0) {
                otemp[i++] = '0';
            }
            
            /* 倒序复制 */
            while (i > 0) {
                out[index++] = otemp[--i];
            }
            out[index] = '\0';
            
            /* 处理宽度和对齐 */
            len = kernel_strlen(out);
            if (min_width > len) {
                if (flags & FLAG_LEFT_ALIGN) {
                    /* 左对齐 */
                    kernel_strcpy(buf, out);
                    buf += len;
                    for (i = 0; i < min_width - len; i++) {
                        *buf++ = ' ';
                    }
                } else {
                    /* 右对齐 */
                    char pad = (flags & FLAG_ZERO_PAD) ? '0' : ' ';
                    for (i = 0; i < min_width - len; i++) {
                        *buf++ = pad;
                    }
                    kernel_strcpy(buf, out);
                    buf += len;
                }
            } else {
                kernel_strcpy(buf, out);
                buf += len;
            }
            break;
            
        case 'c':
            c = (char)va_arg(args, int);
            if (min_width > 1) {
                if (flags & FLAG_LEFT_ALIGN) {
                    *buf++ = c;
                    for (i = 0; i < min_width - 1; i++) {
                        *buf++ = ' ';
                    }
                } else {
                    for (i = 0; i < min_width - 1; i++) {
                        *buf++ = ' ';
                    }
                    *buf++ = c;
                }
            } else {
                *buf++ = c;
            }
            break;
            
        case 's':
            str = va_arg(args, char *);
            if (!str) {
                str = "(null)";
            }
            
            len = kernel_strlen(str);
            /* 处理精度 */
            if (precision >= 0 && precision < len) {
                len = precision;
            }
            
            if (min_width > len) {
                if (flags & FLAG_LEFT_ALIGN) {
                    /* 左对齐 */
                    for (i = 0; i < len; i++) {
                        *buf++ = *str++;
                    }
                    for (i = 0; i < min_width - len; i++) {
                        *buf++ = ' ';
                    }
                } else {
                    /* 右对齐 */
                    for (i = 0; i < min_width - len; i++) {
                        *buf++ = ' ';
                    }
                    for (i = 0; i < len; i++) {
                        *buf++ = *str++;
                    }
                }
            } else {
                for (i = 0; i < len; i++) {
                    *buf++ = *str++;
                }
            }
            break;
            
        case 'p':
            /* 处理指针类型，以16进制形式输出 */
            flags |= FLAG_SPECIAL;
            unum = (uint32_t)va_arg(args, void *);
            
            index = 0;
            out[index++] = '0';
            out[index++] = 'x';
            
            i = 0;
            char ptemp[16];
            if (unum == 0) {
                ptemp[i++] = '0';
            } else {
                while (unum > 0) {
                    c = unum % 16;
                    if (c < 10) {
                        ptemp[i++] = c + '0';
                    } else {
                        ptemp[i++] = c - 10 + 'a';
                    }
                    unum /= 16;
                }
            }
            
            /* 处理精度 */
            padding = (precision > i) ? precision - i : 0;
            while (padding-- > 0) {
                ptemp[i++] = '0';
            }
            
            /* 倒序复制 */
            while (i > 0) {
                out[index++] = ptemp[--i];
            }
            out[index] = '\0';
            
            /* 处理宽度和对齐 */
            len = kernel_strlen(out);
            if (min_width > len) {
                if (flags & FLAG_LEFT_ALIGN) {
                    /* 左对齐 */
                    kernel_strcpy(buf, out);
                    buf += len;
                    for (i = 0; i < min_width - len; i++) {
                        *buf++ = ' ';
                    }
                } else {
                    /* 右对齐 */
                    char pad = (flags & FLAG_ZERO_PAD) ? '0' : ' ';
                    for (i = 0; i < min_width - len; i++) {
                        *buf++ = pad;
                    }
                    kernel_strcpy(buf, out);
                    buf += len;
                }
            } else {
                kernel_strcpy(buf, out);
                buf += len;
            }
            break;
            
        case '%':
            *buf++ = '%';
            break;
            
        default:
            *buf++ = '%';
            *buf++ = *fmt;
            break;
        }
        
        fmt++;
    }
    
    *buf = '\0';
}
```



