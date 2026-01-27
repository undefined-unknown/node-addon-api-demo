#ifndef YIMA_COMMON_H
#define YIMA_COMMON_H

#if defined(_WIN32)
    #ifdef YIMA_EXPORTS
        #define YIMA_API __declspec(dllexport)
    #else
        #define YIMA_API 
    #endif
#else
    #define YIMA_API __attribute__((visibility("default")))
#endif

#endif // YIMA_COMMON_H
