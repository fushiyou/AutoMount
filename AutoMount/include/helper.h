#ifndef _HELPER_H_
#define _HELPER_H_

#ifdef UNICODE
typedef wchar_t YCHAR;
#define _YTEXT(x) __T(x)
#define Ystrcpy(desbuf, dessize, srcbuf) wcscpy_s(desbuf, dessize, srcbuf)
#define Ystrlen(str) wcslen(str)
#define Ysprintf(buf,len,format,...)  swprintf_s(buf,len,format,##__VA_ARGS__)
#else
typedef char YCHAR;
#define _YTEXT(x) (x)
#define Ystrcpy(desbuf, srcbuf) strcpy(desbuf, srcbuf)
#define Ystrlen(str) strlen(str)
#define Ysprintf(buf,len,format,...)  sprintf_s(buf,len,format,##__VA_ARGS__);
#endif // !UNICODE
#endif