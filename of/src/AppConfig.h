
#define EMULATE_ON_OSX !defined(TARGET_RASPBERRY_PI)

#if EMULATE_ON_OSX
    #define USE_ARB 1
#else
    #define USE_ARB 0
#endif
