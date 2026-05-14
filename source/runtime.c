/* Minimal libc shims for a freestanding GBA ROM build. */

void *memcpy(void *dst, const void *src, unsigned int n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

void *memset(void *dst, int c, unsigned int n)
{
    unsigned char *d = (unsigned char *)dst;
    unsigned char v = (unsigned char)c;
    while (n--) {
        *d++ = v;
    }
    return dst;
}

unsigned int strlen(const char *s)
{
    unsigned int n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}
