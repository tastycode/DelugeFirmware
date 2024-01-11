#define CACHE_LINE_SIZE 32
#define OFF_USER_CODE_START 0x20
#define OFF_USER_CODE_END 0x24

#define ENV_DEV 1
#ifndef D_PRINTLN

#define D_MERGE(x, y) x##y
#define D_CONCAT(x, y) D_MERGE(x,y)
#define D_AUTOVAR D_CONCAT(debug, __LINE__)
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define FILE_LINE __FILE__ ":" TOSTRING(__LINE__) "\t"
#define D_PRINT_RAW(...) do { \
    char D_AUTOVAR[1024]; \
    snprintf(D_AUTOVAR, sizeof(D_AUTOVAR), __VA_ARGS__); \
    printf("%s", D_AUTOVAR); \
} while (0)
#define D_PRINT(...) do { \
    char D_AUTOVAR[1024]; \
    snprintf(D_AUTOVAR, sizeof(D_AUTOVAR), __VA_ARGS__); \
    printf(FILE_LINE); \
    printf("%s", D_AUTOVAR); \
} while (0)
#define D_PRINTLN(...) do { \
    char D_AUTOVAR[1024]; \
    snprintf(D_AUTOVAR, sizeof(D_AUTOVAR), __VA_ARGS__); \
    printf(FILE_LINE); \
    printf("%s", D_AUTOVAR); \
    printf("\n"); \
} while (0)

#endif
