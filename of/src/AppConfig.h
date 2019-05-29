
#if !defined(TARGET_RASPBERRY_PI)
#define EMULATE_ON_OSX 1
#else
#define EMULATE_ON_OSX 0
#endif

#if EMULATE_ON_OSX
    #define USE_ARB 1
#else
    #define USE_ARB 0
#endif


#define CAN_MAP_VIDEO 1

