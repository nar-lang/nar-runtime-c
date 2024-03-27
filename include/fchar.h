#ifndef PROJECT_NAR_BASE_FCHAR_H
#define PROJECT_NAR_BASE_FCHAR_H

#include <stdint.h>
#include <stddef.h>

#define MAX_U8_SIZE 4

typedef uint32_t fchar_t;

static size_t fctou8(char *dst, fchar_t src) {
    if (src < 0x80) {
        *dst = (char) src;
        return 1;
    } else if (src < 0x800) {
        *dst++ = (char) (192 + src / 64);
        *dst = (char) (128 + src % 64);
        return 2;
    } else if (src - 0xd800u < 0x800) {
        return 0;
    } else if (src < 0x10000) {
        *dst++ = (char) (224 + src / 4096);
        *dst++ = (char) (128 + src / 64 % 64);
        *dst = (char) (128 + src % 64);
        return 3;
    } else if (src < 0x110000) {
        *dst++ = (char) (240 + src / 262144);
        *dst++ = (char) (128 + src / 4096 % 64);
        *dst++ = (char) (128 + src / 64 % 64);
        *dst = (char) (128 + src % 64);
        return 4;
    }
    return 0;
}

static size_t u8tofc(fchar_t *dst, const char *src, size_t src_len) {
    if (src_len > 0 && (src[0] & 0x80) == 0) {
        *dst = (fchar_t) src[0];
        return 1;
    } else if (src_len > 1 && (src[0] & 0xe0) == 0xc0) {
        *dst = (src[0] & 0x1f) << 6 | (src[1] & 0x3f);
        return 2;
    } else if (src_len > 2 && (src[0] & 0xf0) == 0xe0) {
        *dst = (src[0] & 0x0f) << 12 | (src[1] & 0x3f) << 6 | (src[2] & 0x3f);
        return 3;
    } else if (src_len > 3 && (src[0] & 0xf8) == 0xf0) {
        *dst = (src[0] & 0x07) << 18 | (src[1] & 0x3f) << 12 | (src[2] & 0x3f) << 6 |
               (src[3] & 0x3f);
        return 4;
    }
    return 0;
}

static size_t fcstou8s(char *dst, const fchar_t *src, size_t dst_len) {
    size_t len = 0;
    for (size_t i = 0; src[i] && len < dst_len; ++i) {
        len += fctou8(dst + len, src[i]);
    }
    dst[len] = '\0';
    return len;
}

static size_t fcsntou8s(char *dst, const fchar_t *src, size_t dst_len, size_t src_len) {
    size_t len = 0;
    for (size_t i = 0; src[i] && len < dst_len && i < src_len; ++i) {
        len += fctou8(dst + len, src[i]);
    }
    dst[len] = '\0';
    return len;
}

static size_t u8sntofcs(fchar_t *dst, const char *src, size_t dst_len, size_t src_len) {
    size_t src_offset = 0;
    size_t i;
    for (i = 0; src[src_offset] && i < dst_len; ++i) {
        src_offset += u8tofc(dst + i, src + src_offset, src_len - src_offset);
    }
    dst[i] = 0;
    return i;
}

static size_t u8stofcs(fchar_t *dst, const char *src, size_t dst_len) {
    const char *s;
    for (s = src; *s; ++s);
    return u8sntofcs(dst, src, (s - src), dst_len);;
}

static size_t fcslen(const fchar_t *str) {
    const fchar_t *s;
    for (s = str; *s; ++s);
    return (s - str);
}

#endif //PROJECT_NAR_BASE_FCHAR_H
