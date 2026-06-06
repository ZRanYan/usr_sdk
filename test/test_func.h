#ifndef _TEST_FUNC_H_
#define _TEST_FUNC_H_

#include <memory>
#include <dlfcn.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <time.h>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <csignal>
#include <atomic>
#include <ctime>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <glob.h>
#include <cstdarg>
#include <opencv2/opencv.hpp>
#include "../include/device.h"

using namespace std;

typedef enum {
    BUFFER_EMPTY = 0,
    BUFFER_WRITING,
    BUFFER_FULL,
    BUFFER_READING
} BufferState;

typedef struct {
    uint8_t* data;           // 原始Raw数据缓冲区
    char fileName[256];         //保存的文件名字
    BufferState state;       // 缓冲区状态
    pthread_mutex_t slot_mutex;  // 每个槽位独立互斥锁（保护单个buffer）
} BufferSlot;

typedef struct {
    uint16_t stride;    // 每行字节数（width * bytes_per_pixel）
    uint16_t height;
    uint16_t bit_mode;  // 8, 10, 12, 16 等
    uint8_t  buffNum;   // 缓冲区数量
} FrameInfo;

typedef struct {
    FrameInfo info;
    BufferSlot* buffers;
    int writeIndex;          // 生产者写指针
    int readIndex;           // 消费者读指针
    int count;               // 当前已填充缓冲区数量
    
    pthread_mutex_t queue_mutex;
    pthread_cond_t  cond_full;   // 有空闲缓冲区
    pthread_cond_t  cond_empty;  // 有数据可消费

    // 消费者线程相关
    pthread_t consumerThread;
    bool consumerRunning;
} RingBuffer;

typedef enum
{
    ONE_SENSOR_ONE_MACHINE = 0, //单目单光机
    ONE_SENSOR_FOUR_MACHINE = 1, //单目四光机
    TWO_SENSOR_ONE_MACHINE = 2 //双目单光机
}EMBEDDED_DEVICE_TYPE;

typedef enum
{
    NORMAL_OPERATING_MODE = 0, //正常操作模式
    DLP_RGB_HYBRID_FLASH = 1, //混合闪
    DLP_FLASH_SEPARATE = 2, //DLP单独闪
    DLP_CALL_BACK_TEST = 3,
    DLP_SELF_TEST = 4 //半成品自动化测试
}USR_DEBUG_TEST_TYPE;

typedef enum
{
    CAM_ONE = 0, //单目，单光机
    CAM_ONE_F = 1, //单目，四光机
    CAM_TWO = 2, //双目，单光机
    CAM_INIT = 0xff
}BOARD_TYPE;

struct DEV_Param
{
    int SET_NUM;
    std::atomic<int> g_dlp_index[2];
    int SET_PIC_NUM;
    std::atomic<int> g_pic_index[2];
    std::mutex g_mtx1;
    std::condition_variable g_cv1;
    std::mutex g_mtx2;
    std::condition_variable g_cv2;
    bool g_is_dual;
    EMBEDDED_DEVICE_TYPE g_device_type;
    USR_DEBUG_TEST_TYPE g_is_debug_test_flag;
    RingBuffer* g_rb; //环形队列保存的数据
};

struct CmdContext
{
    int argc; //自测程序往里面传参的参数
    vector<string> argv;
    CAM_DEV *dev; //调用动态库的接口
    //私有数据
    int boardType; //代表那些相机
    //需要用到的全局参数
    struct DEV_Param m_pram;
    string m_version;
};

using CmdFunc = function<void(CmdContext &ctx)>; //声明类函数封装调用

struct CmdItem
{
    int cmd; //命令字
    const char *usage;//使用说明
    const char *desc; //命令描述
    int minArgc;
    int maxArgc;
    CmdFunc func;
};

void cmd_print_help(const vector<CmdItem> &cmdTable);
vector<CmdItem> build_cmd_table();
const CmdItem *find_cmd(const vector<CmdItem> &table, int cmd);
void execute_cmd(const vector<CmdItem> &table, CmdContext &ctx);

void cmd_dev_print_version_info(CAM_DEV *dev, bool isPrint);

void dev_print_version_info(CAM_DEV *dev, DEV_VERSION_INFO_STRUCT &ver, bool isPrint);
void print_system_status(bool is_dual,
                               EMBEDDED_DEVICE_TYPE device_type,
                               USR_DEBUG_TEST_TYPE debug_type,
                               std::ofstream *ofs = nullptr);
void dev_set_log_print(CAM_DEV *dev);
void dev_get_sensor_ver(CAM_DEV *dev, string &ver, bool isPrint);
void sensor_set_gain(CAM_DEV *dev, BOARD_TYPE type, uint16_t gain);
void sensor_set_hv_flip(CAM_DEV *dev, BOARD_TYPE type, bool isEnable);
void sensor_set_black(CAM_DEV *dev, BOARD_TYPE type, uint16_t value);
void sensor_get_temp_value(CAM_DEV *dev);
int m_get_count_wildcard(const char *str);
void dev_set_dlp_status(CAM_DEV *dev, bool isPrint);

RingBuffer* initRingBuffer(FrameInfo *mParam);
int producerPut(RingBuffer* rb, const uint8_t* rawData, size_t dataLen, char *fileName);
void destroyRingBuffer(RingBuffer* rb);
int startConsumerThread(RingBuffer* rb);
void printFile(const std::string& filename);
void print_net_info(const std::string &ifname, std::ofstream &ofs);
void dev_get_soc_status(CAM_DEV *dev, DEV_XSTATUS_VALUE &data, bool isPrint);
void dev_set_io_output_set(CAM_DEV *dev, int index, int value);
void dev_set_io_input_param_set(CAM_DEV *dev, int index);
void dev_set_io_ext_internal_mode(CAM_DEV *dev, int mode, int value, int value_2);
void dev_sensor_debug_test(CAM_DEV *dev, int opt, int addr, int val);
void dev_set_two_sensor_trig(CAM_DEV *dev);
void dev_set_io_rgb_status(CAM_DEV *dev, uint8_t set);
void dev_set_io_led_status(CAM_DEV *dev, int index);
void dev_set_io_enable_dlp_usb(CAM_DEV *dev, int index);
void dev_get_dlp_ver(CAM_DEV *dev, string &ver, bool isPrint);
void sensor_set_roi(CAM_DEV *dev, DEV_CAM_INDEX type);
void dev_get_io_ver(CAM_DEV *dev, string &ver, bool isPrint);
void sensor_image_self_test_save(DEV_IMG_DEF frame, void *usr_data);
void save_test_log_result(CAM_DEV *dev, struct DEV_Param *devParam);

void sensor_get_raw_data(DEV_IMG_DEF frame, void *usr_data);
void sensor_image_count(DEV_IMG_DEF frame, void *usr_data);
void dev_set_dlp_rgb_xtrig_status( CAM_DEV *dev, struct DEV_Param *devParam, uint32_t num);
DEV_RTN dev_test_dlp_updata_flash(CAM_DEV *dev, uint8_t index, bool isPrint);

void dev_judgment_node(struct DEV_Param &mParam, DEV_SENSOR_ATTRIBUTE &sensor);

#endif

