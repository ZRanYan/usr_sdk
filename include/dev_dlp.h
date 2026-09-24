#ifndef _DEV_DLP_H_
#define _DEV_DLP_H_

#include <cstdint>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <unistd.h>
#include "debug.h"
#include "dev_common.h"

#define DLP_MAX_DEVICES 4   // dlp设备最大数量
#define DLP_NAME_SIZE 64    // 每个dlp设备名的最大长度
#define MAX_CYRRENT 1023 //dlp配置最大电流
#define DLP_VER_MAX_LEN 128

#define DLP_DEV "/dev/dlp_"

typedef struct
{
    uint8_t sendLength;
    uint16_t recvLength;
    uint8_t res[1];
    uint8_t sendData[8];
    uint8_t recvData[1024];
}DLP_READ; //先写后读

//Write data
typedef struct
{
    uint16_t length;
    uint8_t mode; //预留dlp4052配置模式0-正常，1-编程模式
    uint8_t data[1025];
}DLP_CONTROL; //仅读或者写

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

typedef struct {
    uint32_t flash_size;      // Total flash memory size (bytes)
    uint32_t sector_count;    // Number of sectors
    uint32_t sector_addr[256]; // Starting address of all sectors
    unsigned char Type;            // A, B or C algothrim (0, 1, 2)
} FlashResult;


#define DLP_GET_VERSION _IOW('j', 0, DLP_VERSION)     // 获取参数

#define DLP_IOCTL_SET_DATA _IOW('j', 3, DLP_CONTROL)//设置参数
#define DLP_COMMAND_CONTROL _IOW('j', 4, DLP_READ)//获取参数
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

#define CHECK_INDEX_RETURN(index)              \
    do {                                                 \
        if ((index) >= (DLP_MAX_DEVICES)) {                          \
            DEBUG_LOG(APP, ERROR,                       \
                "invalid index: %u (>= DLP_MAX_DEVICES:%u)\n",           \
                (unsigned int)(index), (DLP_MAX_DEVICES)); \
            return (RTN_INVALID_ARG);                                \
        }                                                \
    } while (0)


class DEV_DLP
{
public:
    DEV_DLP();
    ~DEV_DLP();

public:
    DEV_RTN dev_dlp_init(int (&)[DLP_MAX_DEVICES]);
    void    dev_dlp_get_ver(std::string&);
    DEV_RTN dev_dlp_get_type_value(DEV_DLP_TYPE&);
    DEV_RTN dev_dlp_get_temp_value(uint8_t,float&);
    DEV_RTN dev_dlp_set_current_value(uint8_t, DEV_DLP_CURRENT_VALUE);
    DEV_RTN dev_dlp_set_current_all_value(uint16_t);
    DEV_RTN dev_dlp_get_pwm_value(uint8_t, DEV_DLP_CURRENT_VALUE&);
    DEV_RTN dev_dlp_set_group_info_value(uint8_t index, uint8_t write_ctl, const DEV_DLP_PATTERN_GROUP_INFO& sel_pack);
    DEV_RTN dev_dlp_get_group_info_value(uint8_t, DEV_DLP_PATTERN_GROUP_INFO&);
    DEV_RTN dev_dlp_trigonce_value(uint8_t);
    DEV_RTN dev_dlp_trigonce_new_set(uint8_t);
    DEV_RTN dev_dlp_get_min_expo_value(uint8_t, DEV_DLP_EXP&);
    DEV_RTN dev_dlp_set_delay_invert_value(uint8_t, bool, uint32_t);
    DEV_RTN dev_dlp_update_falsh_value(uint8_t, const char*, const char*);

    DEV_RTN dev_dlp_set_long_short_flip_value(uint8_t, bool, bool);
    DEV_RTN dev_dlp_set_trig_type_value(uint8_t index, DEV_DLP_TRIG_TYPE type);
    DEV_RTN dev_dlp_trig_in_config_value(uint8_t index, bool enable, bool polarity);
    DEV_RTN dev_dlp_set_source_mode_value(uint8_t index, DEV_DLP_TRIG_MODE mode);
    DEV_RTN dev_dlp_get_source_mode_value(uint8_t index, DEV_DLP_TRIG_MODE &mode);
    DEV_RTN dev_dlp_get_validate_status_value(uint8_t index, uint8_t &status);
    DEV_RTN dev_dlp_set_on_off_value(uint8_t index, bool onOff);
    DEV_RTN dev_dlp_led_config_value(uint8_t index, bool readOrwrite, DEV_DLP_LED_SET &ledSet);
    DEV_RTN dev_dlp_exp_period_config_value(uint8_t index, bool readOrwrite, DEV_DLP_EXPOSURE_PERIOD_SET &set);
    DEV_RTN dev_dlp_get_pattern_trigger_mode_value(uint8_t index, uint8_t &mode);
    DEV_RTN dev_dlp_set_reboot_value(uint8_t index, uint32_t &ver);
    DEV_RTN dev_dlp_config_pwm_polarity_value(uint8_t index, bool test, uint8_t &invert);
    DEV_RTN dev_dlp_set_lut_value(uint8_t index, const DEV_DLP_PATTERN_ORDER_SET &set);
    DEV_RTN dev_dlp_set_load_timing_value(uint8_t index, uint8_t image_index,uint8_t num);
    DEV_RTN dev_dlp_get_load_timing_value(uint8_t index, uint32_t &load_time);
    DEV_RTN dev_dlp_get_status_value(uint8_t index, uint8_t &hwStatus, uint8_t &sysStatus, uint8_t &mainStatus);
private:
    DEV_RTN dev_dlp_update_falsh_dlp4710(uint8_t, const char*);
    DEV_RTN dev_dlp_update_falsh_dlp4052(uint8_t, const char*, const char*);
    
    // 子函数封装：写操作（自带互斥锁临界区保护）
    int m_dlp_write_data(uint8_t index, std::initializer_list<uint8_t> list, uint8_t mode = 0);
    int m_dlp_write_data(uint8_t index, const uint8_t* data, uint16_t len, uint8_t mode = 0);
    int m_dlp_write_data(uint8_t index, const DLP_CONTROL& val);

    // 子函数封装：读操作（自带互斥锁临界区保护）
    int m_dlp_read_data(uint8_t index, std::initializer_list<uint8_t> send_list, uint8_t* recv_buf, uint16_t recv_len);
    int m_dlp_read_data(uint8_t index, const uint8_t* send_buf, uint8_t send_len, uint8_t* recv_buf, uint16_t recv_len);
    int m_dlp_read_data(uint8_t index, DLP_READ& read_val);

    DEV_DLP_TYPE m_dlp_read_id(void);
    int m_GetSectorNum(FlashResult *flash_result, unsigned int Addr);
    int m_calc_one_range(uint8_t index, uint32_t addr, uint32_t size, uint32_t *outChk);

private:
    std::mutex m_dlpMutex;           // 互斥锁，保护 m_dlpReadValue 和 m_dlpWriteValue 临界资源
    std::string m_ver="null";
    DEV_DLP_TYPE dlpType = DLP_4710;   //根据底层的驱动自动获取dlp的类型
    DLP_SET dlpSet={};
    DLP_READ m_dlpReadValue={}; //DLP_SET
    DLP_CONTROL m_dlpWriteValue={}; //DLP_CONTROL
    PATTERN_PARAM pattern_param={};
    FlashResult m_flashresult;

    // //定义曝光时间、曝光前时间、曝光后时间
    // uint16_t d_pre_exp;
    // uint16_t d_exp;
    // uint16_t d_post_exp;
};


#endif