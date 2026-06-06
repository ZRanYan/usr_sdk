#ifndef DEBUG_H
#define DEBUG_H

#include <stdarg.h> // 包含可变参数的头文件
#include <stdio.h>

#include <string.h>
#include <pthread.h>
#include <mqueue.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/time.h>
#include <math.h>
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

// #define DEBUG_LOG_OPEN

#define BUF_SIZE 1024
#define ARRAY_SIZE 216
#define FILE_NAME_LENGTH 20
#define FUNC_NAME_LENGTH 30
#define PRINT_MESS_LENGTH 512
#define LOG_FILE_PATH_LENGTH 50

#define INIT_MEMBER(a) \
{                  \
        a, #a          \
}

typedef enum
{
    ERROR,
    WARN,
    INFO,
    DEBUG,
    PRINT_LEVEL_END
} PRINT_LEVEL;
typedef enum
{
    APP = 1 << 0,
    FPGA = 1 << 1,
    SENSOR = 1 << 2,
    NET = 1 << 3,
    ALG = 1<<4,
    PIC = 1<<5,
    OTHER = 1 << 6
} MODULE_NAME;
typedef enum
{
    LOGO_NONE = 0,
    LOGO_LEVEL_STRING = 1 << 0,
    LOGO_MODULE_STRING = 1 << 1,
    LOGO_FILE_STRING = 1 << 2,
    LOGO_FUNCTION_STRING = 1 << 3,
    LOGO_YEAR_MONTH_DAY_STRING = 1 << 4,
    LOGO_MINUTES_AND_SECONDS_STRING = 1 << 5,
    LOGO_ROWS_STRING = 1 << 6,
} SHOW_HEAD_LOGO;
typedef struct
{
    SHOW_HEAD_LOGO headLog;
    char headLogString[35];
} SHOW_HEAD_CONFIG_STRUCT;
typedef enum
{
    PRINT_TERMINAL = 1 << 0,
    PRINT_FLASH = 2 << 0
} LOG_STORAGE_METHOD;
typedef struct
{
    PRINT_LEVEL level;
    union
    {
        unsigned short int debugModuleFlag; //
        struct
        {
            unsigned short int bit1_app : 1;
            unsigned short int bit2_fpga : 1;
            unsigned short int bit3_sensor : 1;
            unsigned short int bit4_net : 1;
            unsigned short int bit5_alg : 1;
            unsigned short int bit6_pic : 1;
            unsigned short int bit7_other : 1;
        } bitField;
    } debugModuleFlagValue;
    union
    {
        unsigned short int debugShowHeadFlag; // 整数成员
        struct
        {
            unsigned short int bit1_level : 1;
            unsigned short int bit2_module : 1;
            unsigned short int bit3_file : 1;
            unsigned short int bit4_function : 1;
            unsigned short int bit5_year_month_day : 1;
            unsigned short int bit6_minutes_seconds : 1;
            unsigned short int bit7_rows : 1;
        } bitField;
    } debugShowHeadValue;
    union
    {
        unsigned short int debugPrintExhibitFlag; //
        struct
        {
            unsigned short int bit1_terminal : 1;
            unsigned short int bit2_flash : 1;
        } bitField;
    } debugPrintExhibitValue;
    char logFilePath[LOG_FILE_PATH_LENGTH];
    unsigned int logFileSize; // 单位为Kb
} DEBUG_CTRL_STRUCT;
typedef struct
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int msec;
} DEBUG_TIME_INFO;

typedef struct
{
    pthread_mutex_t mutex;
    PRINT_LEVEL printLevel;
    MODULE_NAME printModuleName;
    DEBUG_TIME_INFO mTime;
    char fileName[FILE_NAME_LENGTH];
    char funcName[FUNC_NAME_LENGTH];
    unsigned int lineNum;
    char printMess[PRINT_MESS_LENGTH]; // 需要的打印信息
} DEBUG_MQ_MESSAGE_STRUCT;

typedef struct
{
    volatile unsigned short int wirteIndex; // only
    volatile unsigned short int readIndex;  // only
    pthread_mutex_t mutex_wirte;
    DEBUG_MQ_MESSAGE_STRUCT data[ARRAY_SIZE];
} DEBUG_MQ_STRUCT;

typedef struct
{
    MODULE_NAME moduleName;
    char printString[10];
}MODULE_NAME_STRUCT;


void init_debug(const char *path);
void debugTest();
void logPrint(PRINT_LEVEL printLevel, MODULE_NAME moduleLevel, const char *fileName, const char *funcName, const int lineNum, const char *fmt, ...);
void logPrintSettingInfo();
void logPrintDebugSet(unsigned char  level, unsigned short int module, unsigned short int isTxt);


#ifdef DEBUG_LOG_OPEN
#define DEBUG_LOG(level, module, ...) \
    logPrint(module, level, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG(...) \
    logPrint(INFO, OTHER, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define DEBUG_LOG(level, module, ...)
#define LOG(...)
#endif

#ifdef __cplusplus
}
#endif

#endif // DEBUG_H
