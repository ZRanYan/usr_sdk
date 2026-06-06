#ifndef _DEV_DLP_H_
#define _DEV_DLP_H_

#include <cstdint>
#include <sys/ioctl.h>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include "debug.h"
#include "dev_common.h"

#define DLP_MAX_DEVICES 4   // dlp设备最大数量
#define DLP_NAME_SIZE 64    // 每个dlp设备名的最大长度
#define MAX_CYRRENT 1023 //dlp配置最大电流
#define DLP_VER_MAX_LEN 128

#define DLP_DEV "/dev/dlp3010_"

typedef struct
{
    uint8_t sendLength;
    uint16_t recvLength;
    uint8_t res[1];
    uint8_t sendData[8];
    uint8_t recvData[256];
}DLP_READ; //先写后读

//Write data
typedef struct
{
    uint16_t length;
    uint8_t data[1025];
    uint8_t res[1]; //预留保持对齐
}DLP_WRITE; //仅读或者写

typedef struct 
{
    uint8_t adrr; //
    uint8_t flag; //读写标志,读是1,写是0
    unsigned short int addrReg;
    uint8_t data;//数据
}DLP_TEST_CONTROL; //用作测试sensor的自定义协议接口

typedef struct {
    uint32_t len;                 /* 实际字符串长度 */
    char  ver[DLP_VER_MAX_LEN];
}DLP_VERSION;

typedef struct
{
    uint8_t fdNum; //在线操作句柄数量
    uint8_t fdMap; //位标志操作句柄是否有效
    int fd[DLP_MAX_DEVICES]; //操作i2c句柄
    char device_list[DLP_MAX_DEVICES][DLP_NAME_SIZE];
    int id[DLP_MAX_DEVICES];//dlp_id
}DLP_SET; //dlp配置信息

#define DLP_GET_VERSION _IOW('j', 0, DLP_VERSION)     // 获取参数
#define DLP_IOCTL_GET_DATA _IOW('j', 2, DLP_WRITE)//no use
#define DLP_IOCTL_SET_DATA _IOW('j', 3, DLP_WRITE)//设置参数
#define DLP_COMMAND_CONTROL _IOW('j', 4, DLP_WRITE)//获取参数
#define DLP_TRIG_ONCE_ASSIGN  _IO('j', 5) //不需要传参

#define READ_RGB_LED_PWM                     0x55
#define WRITE_RGB_LED_PWM                    0x54
#define WRITE_TRIGGER_IN_CONFIGURATION       0x90
#define WRITE_TRIGGER_OUT_CONFIGURATION      0x92
#define READ_PATTERN_CONFIGURATION           0x97
#define WRITE_PATTERN_CONFIGURATION          0x96
#define WRITE_PATTERN_ORDER_TABLE_ENTRY      0x98
#define READ_PATTERN_ORDER_TABLE_ENTRY       0x99
#define READ_MIN_EXPO                        0x9D
#define WRITE_INTERNAL_PATTERN_CONTROL       0x9E
//test
#define PTHREAD_INTERNAL_PATTERN_STATUS      0x9F

class DEV_DLP
{
public:
    DEV_DLP();
    ~DEV_DLP();

public:
    DEV_RTN dev_dlp_init(int (&)[DLP_MAX_DEVICES]);
    void    dev_dlp_get_ver(std::string&);
    DEV_RTN dev_dlp_get_temp_value(uint8_t,float&);
    DEV_RTN dev_dlp_set_current_value(uint8_t, uint16_t);
    DEV_RTN dev_dlp_set_current_all_value(uint16_t);
    DEV_RTN dev_dlp_get_pwm_value(uint8_t, uint16_t&);
    DEV_RTN dev_dlp_set_group_info_value(uint8_t index, uint8_t write_ctl, const DEV_DLP_PATTERN_GROUP_INFO& sel_pack);
    DEV_RTN dev_dlp_get_group_info_value(uint8_t, DEV_DLP_PATTERN_GROUP_INFO&);
    DEV_RTN dev_dlp_trigonce_value(uint8_t);
    DEV_RTN dev_dlp_trigonce_new_set(uint8_t);
    DEV_RTN dev_dlp_get_min_expo_value(uint8_t, DEV_DLP_EXP&);
    DEV_RTN dev_dlp_set_delay_invert_value(uint8_t, bool, uint32_t);
    DEV_RTN dev_dlp_update_falsh_value(uint8_t, const char*);
    DEV_RTN dev_dlp_set_long_short_flip_value(uint8_t, bool, bool);
private:
    int m_dlp_write_data(uint8_t, DLP_WRITE&,std::initializer_list<uint8_t>);

private:
    std::string m_ver="null";
    DLP_SET dlpSet={};
    DLP_READ m_dlpReadValue={}; //DLP_SET
    DLP_WRITE m_dlpWriteValue={}; //DLP_CONTROL
    PATTERN_PARAM pattern_param={};
    // //定义曝光时间、曝光前时间、曝光后时间
    // uint16_t d_pre_exp;
    // uint16_t d_exp;
    // uint16_t d_post_exp;
};


#endif