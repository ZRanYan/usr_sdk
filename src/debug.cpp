
#include "../include/debug.h"

char buf[BUF_SIZE];

DEBUG_MQ_STRUCT debugMqData;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t g_debugMutex = PTHREAD_MUTEX_INITIALIZER;

static DEBUG_CTRL_STRUCT debugCtrlConfig =
{
        .level = DEBUG,
        .debugModuleFlagValue =
        {
            .debugModuleFlag = 0xFFFF,
        },
        .debugShowHeadValue =
        {
            .debugShowHeadFlag = LOGO_LEVEL_STRING|LOGO_MODULE_STRING|LOGO_FILE_STRING|LOGO_FUNCTION_STRING|LOGO_ROWS_STRING,
        },
        .debugPrintExhibitValue =
        {
            .debugPrintExhibitFlag = PRINT_TERMINAL /*| PRINT_FLASH*/,
        },
        .logFilePath = "./log_print.txt",
        .logFileSize = 10240000 * 2,
};
static time_t programStartTime;
static char programStartTimeBuff[20];
static const char *levelString[] = {"ERROR", "WARN", "INFO", "DEBUG", " "};
// static char *printModuleName[] = {"app", "fpga", "sensor", "net", "alg","other"};
static MODULE_NAME_STRUCT printModuleString[] = 
{
    {APP, "app"},
    {FPGA, "fpga"},
    {SENSOR, "sensor"},
    {NET, "net"},
    {ALG, "alg"},
    {PIC, "pic"},
    {OTHER, "other"}
};
static SHOW_HEAD_CONFIG_STRUCT headLog[] = {INIT_MEMBER(LOGO_LEVEL_STRING), INIT_MEMBER(LOGO_MODULE_STRING), INIT_MEMBER(LOGO_FILE_STRING),
                                            INIT_MEMBER(LOGO_FUNCTION_STRING), INIT_MEMBER(LOGO_YEAR_MONTH_DAY_STRING),
                                            INIT_MEMBER(LOGO_MINUTES_AND_SECONDS_STRING), INIT_MEMBER(LOGO_ROWS_STRING)};

static void getNowTime(char *buf, int bufLength, time_t *now)
{
    if(NULL == now)
    {
        return;
    }
    struct tm *info;
    time(now);
    info = localtime(now);
    //sinfo->tm_hour = (info->tm_hour)+8;
    if(NULL != buf && 0 < bufLength)
        strftime(buf, bufLength, "%Y-%m-%d %H:%M:%S", info);
    return;
}

void convertSecondsToTimeFormat(double seconds,char *buf, int length)
{
    int years, months, days, hours, minutes;
    double remaining_seconds;
    years = months = days = hours = minutes = 0;
    minutes = (int)(seconds / 60);
    remaining_seconds = fmod(seconds, 60);
    hours = minutes / 60;
    minutes %= 60;
    days = hours / 24;
    hours %= 24;
    months = days / 30; // 简单起见，假设每个月30天
    days %= 30;
    years = months / 12;
    months %= 12;
    snprintf(buf, length, "%d-%d-%d %d:%d:%0.2f", years, months, days, hours, minutes, remaining_seconds);
    return;
}

/*打印当前的参数配置情况*/
void logPrintSettingInfo()
{
    size_t i, j;
    char timeBuf[20];
    time_t currentTime;
    /* unused: struct tm *info; */
    getNowTime(NULL, 0, &currentTime);
    double sec = difftime(currentTime, programStartTime);
    convertSecondsToTimeFormat(sec, timeBuf, 20);
    printf("-------------------current debug setting---------------------\n");
    printf("git branch:%s\t commitId:%s\t date:%s\n",GIT_BRANCH, GIT_COMMIT, PROGRAM_DATE);
    printf("program start time:%s \n", programStartTimeBuff);
    printf("survival time:%0.2fs  %s\n", sec, timeBuf);
    printf("debug level:\n");
    printf("\t%s\n", levelString[debugCtrlConfig.level]);
    printf("debug module:\n\t");
    for (i = 0; i < sizeof(printModuleString) / sizeof(printModuleString[0]); i++)
    {
        if ((1 << i) == (debugCtrlConfig.debugModuleFlagValue.debugModuleFlag & (1 << i)))
        {
            printf("%s ", printModuleString[i].printString);
        }
    }
    printf("\ndebug show head:\n\t");
    for (i = 0, j = 0; i < sizeof(headLog) / sizeof(headLog[0]); i++)
    {
        if ((headLog[i].headLog) == (debugCtrlConfig.debugShowHeadValue.debugShowHeadFlag & (headLog[i].headLog)))
        {
            j++;
            printf("%s_string ", headLog[i].headLogString);
            if (j % 4 == 0)
            {
                printf("\n\t");
            }
        }
    }
    printf("\ndebug print exhibit:\n\t");
    if (PRINT_TERMINAL == (debugCtrlConfig.debugPrintExhibitValue.debugPrintExhibitFlag & PRINT_TERMINAL))
    {
        printf("PRINT_TERMINAL ");
    }
    if (PRINT_FLASH == (debugCtrlConfig.debugPrintExhibitValue.debugPrintExhibitFlag & PRINT_FLASH))
    {
        printf("logFile save:%s", debugCtrlConfig.logFilePath);
    }
    printf("\n----------------------------end----------------------------\n");
}

static unsigned int debugPrintHeadPack(pthread_mutex_t *mutex, char *buffer, int bufferSize, unsigned int *written, const char *format, ...)
{
    pthread_mutex_lock(mutex);
    va_list args;
    va_start(args, format);
    unsigned int remaining = bufferSize - *written;
    unsigned int length = vsnprintf(buffer + *written, remaining, format, args);
    va_end(args);
    if (length >= remaining)
    {
        printf("Output was truncated.\n");
    }
    *written += length;
    pthread_mutex_unlock(mutex);
    return length;
}
// static int firstCalculateBitPos(unsigned int num)
// {
//     int pos = 0;
//     while (((num) & (1 << pos)) == 0)
//         pos++;
//     return pos;
// }
static void logGetCurrentTimeInfo(DEBUG_TIME_INFO *times)
{
    struct timeval tv;
    time_t now;
    struct tm *info;
    time(&now);
    info = localtime(&now);
    gettimeofday(&tv, NULL); // 获取当前时间
    // 提取年月日时分秒信息
    times->year = info->tm_year+1900;
    times->month = info->tm_mon+1;
    times->day = info->tm_mday;
    times->hour = info->tm_hour;
    times->minute = info->tm_min;
    times->second = info->tm_sec;
    times->msec = tv.tv_usec;
    return;
}

static void debugPrintSavePosition(const char *debugBuf)
{
    /* unused: static char isFirstFlag = 0; // 是不是第一次写 */
    static FILE *file = NULL;
    static long currentFileIndex = 0;
    if (PRINT_TERMINAL == (debugCtrlConfig.debugPrintExhibitValue.debugPrintExhibitFlag & PRINT_TERMINAL))
    {
        puts(debugBuf);
    }
    if (PRINT_FLASH == (debugCtrlConfig.debugPrintExhibitValue.debugPrintExhibitFlag & PRINT_FLASH))
    {
        if (NULL == file)
        {
            file = fopen(debugCtrlConfig.logFilePath, "w");
            if (file == NULL)
            {
                perror("Failed to open file");
                return;
            }
            fseek(file, 0, SEEK_END);
            currentFileIndex = ftell(file);
        }

        if (currentFileIndex > debugCtrlConfig.logFileSize * 1024)
        {
            currentFileIndex = 0;
        }

        fseek(file, currentFileIndex, SEEK_SET);
        currentFileIndex += fwrite(debugBuf, sizeof(char), strlen(debugBuf), file);
        return;
    }
    if (NULL != file)
    {
        fclose(file);
        file = NULL;
    }
    return;
}

static void debug_save_data(DEBUG_MQ_MESSAGE_STRUCT *mqMessage)
{
    // static int i=0;
    pthread_mutex_lock(&debugMqData.mutex_wirte);
    pthread_mutex_lock(&debugMqData.data[debugMqData.wirteIndex].mutex);
    DEBUG_MQ_MESSAGE_STRUCT *mMessage = NULL;
    /*保存信息到共享数据池里*/
    mMessage = &debugMqData.data[debugMqData.wirteIndex];
    memcpy(mMessage, mqMessage, sizeof(DEBUG_MQ_MESSAGE_STRUCT));
    debugMqData.wirteIndex++;
    debugMqData.wirteIndex = debugMqData.wirteIndex & (ARRAY_SIZE - 1);
    pthread_mutex_unlock(&debugMqData.data[debugMqData.wirteIndex].mutex);
    pthread_mutex_unlock(&debugMqData.mutex_wirte);
    return;
}

void debug_send_log_message(DEBUG_MQ_MESSAGE_STRUCT *mqMessage)
{
    /* unused: static int i = 0; */
    debug_save_data(mqMessage);
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return;
}

void logPrint(PRINT_LEVEL printLevel, MODULE_NAME moduleLevel, const char *fileName, const char *funcName, const int lineNum, const char *fmt, ...)
{
    /* unused: unsigned int length = 0; */
    DEBUG_MQ_MESSAGE_STRUCT mqMessageData;
    va_list valist;
    memset(&mqMessageData, 0, sizeof(mqMessageData));
    // memset(&currentTimeInfo, 0, sizeof(currentTimeInfo));
    // 判断打印优先级，是否进行显示
    if (printLevel > debugCtrlConfig.level)
    {
        return;
    }
    // 判断某些模块是否要显示
    if (moduleLevel != (debugCtrlConfig.debugModuleFlagValue.debugModuleFlag & moduleLevel))
    {
        return;
    }
    if (LOGO_YEAR_MONTH_DAY_STRING == (debugCtrlConfig.debugShowHeadValue.debugShowHeadFlag & LOGO_YEAR_MONTH_DAY_STRING) ||
        LOGO_MINUTES_AND_SECONDS_STRING == (debugCtrlConfig.debugShowHeadValue.debugShowHeadFlag & LOGO_MINUTES_AND_SECONDS_STRING))
    {
        logGetCurrentTimeInfo(&mqMessageData.mTime);
    }
    // 封装数据包
    mqMessageData.mutex = PTHREAD_MUTEX_INITIALIZER;
    mqMessageData.printLevel = printLevel;
    mqMessageData.printModuleName = moduleLevel;
    const char *tmpfilename = strrchr(fileName, '/');
    if(NULL != tmpfilename)
    {
        tmpfilename += 1;
        strncpy(mqMessageData.fileName, tmpfilename, strlen(tmpfilename));
    }
    else
    {
        strncpy(mqMessageData.fileName, fileName, sizeof(mqMessageData.fileName));
    }

    strncpy(mqMessageData.funcName, funcName, sizeof(mqMessageData.funcName));
    mqMessageData.lineNum = lineNum;
    va_start(valist, fmt);
    vsnprintf(mqMessageData.printMess, sizeof(mqMessageData.printMess), fmt, valist);
    va_end(valist);
    debug_send_log_message(&mqMessageData);
    return;
}

void logDataProcess(DEBUG_MQ_MESSAGE_STRUCT *data)
{
    unsigned int length = 0;
    // static char buf[BUF_SIZE];
    DEBUG_MQ_MESSAGE_STRUCT mTmpData;
    while (pthread_mutex_lock(&data->mutex) != 0)
    {
        usleep(100);
    }
    memcpy(&mTmpData, data, sizeof(DEBUG_MQ_MESSAGE_STRUCT));
    pthread_mutex_unlock(&data->mutex);
    memset(buf, 0, sizeof(buf));
    if (data->printMess == NULL || *data->printMess == '\0')
    {
        return;
    }
    // printf("%s:%s:%d %s\n", mTmpData.fileName, mTmpData.funcName, mTmpData.lineNum,mTmpData.printMess);
    // 判断显示的头部标识信息内容
    if (debugCtrlConfig.debugShowHeadValue.bitField.bit1_level)
    {
        debugPrintHeadPack(&g_debugMutex, buf, BUF_SIZE, &length, "%s ", levelString[mTmpData.printLevel]);
    }
    if (debugCtrlConfig.debugShowHeadValue.bitField.bit2_module)
    {
        for (size_t i = 0; i < (sizeof(printModuleString) / sizeof(printModuleString[0])); i++)
        {
            if(printModuleString[i].moduleName == (data->printModuleName&debugCtrlConfig.debugModuleFlagValue.debugModuleFlag))
            {
                debugPrintHeadPack(&g_debugMutex, buf, BUF_SIZE, &length, "[%s]", printModuleString[i].printString);
                break;
            }
        }
    }
    if (debugCtrlConfig.debugShowHeadValue.bitField.bit3_file)
    {
        debugPrintHeadPack(&g_debugMutex, buf, BUF_SIZE, &length, "[%s]", mTmpData.fileName);
    }
    if (debugCtrlConfig.debugShowHeadValue.bitField.bit4_function)
    {
        debugPrintHeadPack(&g_debugMutex, buf, BUF_SIZE, &length, "[%s]", mTmpData.funcName);
    }
    if (debugCtrlConfig.debugShowHeadValue.bitField.bit5_year_month_day)
    {
        debugPrintHeadPack(&g_debugMutex, buf, BUF_SIZE, &length, "[%d-%d-%d]", mTmpData.mTime.year, mTmpData.mTime.month, mTmpData.mTime.day);
    }
    if (debugCtrlConfig.debugShowHeadValue.bitField.bit6_minutes_seconds)
    {
        debugPrintHeadPack(&g_debugMutex, buf, BUF_SIZE, &length, "[%d:%d:%d.%d]", mTmpData.mTime.hour, mTmpData.mTime.minute, mTmpData.mTime.second, mTmpData.mTime.msec);
    }
    if (debugCtrlConfig.debugShowHeadValue.bitField.bit7_rows)
    {
        debugPrintHeadPack(&g_debugMutex, buf, BUF_SIZE, &length, "-%d:", mTmpData.lineNum);
    }

    if (5 > (BUF_SIZE - length))
    {
        printf("buf too small\n");
        return;
    }
    strncpy(buf + length, mTmpData.printMess, BUF_SIZE - length);
    debugPrintSavePosition(buf);
    return;
}

void *receiver_thread(void *arg)
{
    /* unused: int j = 0; */
    unsigned int tmpWirteIndex = 0;
    int difference = 0;
    /* unused: struct timespec wait_time; */
    while (1)
    {
        pthread_cond_wait(&cond, &mutex);
        while (pthread_mutex_lock(&debugMqData.mutex_wirte) != 0)
        {
            usleep(100);
        }
        tmpWirteIndex = debugMqData.wirteIndex;
        pthread_mutex_unlock(&debugMqData.mutex_wirte);

        difference = tmpWirteIndex - debugMqData.readIndex;

        if (0 == difference)
        {
            continue;
        }
        else if (difference < 0)
        {
            for (unsigned int i = debugMqData.readIndex; i < ARRAY_SIZE; ++i)
            {
                logDataProcess(&debugMqData.data[i]);
            }
            for (unsigned int i = 0; i < tmpWirteIndex; ++i)
            {
                logDataProcess(&debugMqData.data[i]);
            }
        }
        else
        {
            for (unsigned int i = debugMqData.readIndex; i < tmpWirteIndex; i++)
            {
                logDataProcess(&debugMqData.data[i]);
            }
        }
        debugMqData.readIndex = tmpWirteIndex; // Update the coordinates
    }
}

void init_debug(const char *path)
{
    static pthread_t receiver_tid;
    getNowTime(programStartTimeBuff, sizeof(programStartTimeBuff), &programStartTime);
    strcpy(debugCtrlConfig.logFilePath, path);
    pthread_create(&receiver_tid, NULL, receiver_thread, NULL);
    debugMqData.readIndex = 0;
    debugMqData.wirteIndex = 0;
    debugMqData.mutex_wirte = PTHREAD_MUTEX_INITIALIZER;
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        debugMqData.data[i].mutex = PTHREAD_MUTEX_INITIALIZER;
    }
    DEBUG_LOG(APP,INFO,"---------------start-------------\n");
}

void *thread_function(void *thread_id)
{
    for (int i = 0; i < 1000; i++)
    {
        DEBUG_LOG(APP, ERROR, "test:%d------i:%d\n", thread_id, i);
    }
    pthread_exit(NULL);
}

void debugTest()
{
#define NUM_THREADS 400
    pthread_t threads[NUM_THREADS];
    int rc;
    int t;

    for (t = 0; t < NUM_THREADS; t++)
    {
        printf("Creating thread %d\n", t + 1);
        rc = pthread_create(&threads[t], NULL, thread_function, (void *)(intptr_t)t);
        if (rc)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for (t = 0; t < NUM_THREADS; t++)
    {
        pthread_join(threads[t], NULL);
        printf("Thread %d completed\n", t);
    }
}

void logPrintDebugSet(unsigned char level, unsigned short int module, unsigned short int isTxt)
{
    debugCtrlConfig.level = (PRINT_LEVEL)level;
    debugCtrlConfig.debugModuleFlagValue.debugModuleFlag = module;
    debugCtrlConfig.debugPrintExhibitValue.debugPrintExhibitFlag = isTxt;
    return;
}
