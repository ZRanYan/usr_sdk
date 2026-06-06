#ifndef _DEV_IO_H_
#define _DEV_IO_H_

// #include <cstring>
// #include <cstdint>
#include <sys/ioctl.h>
#include <string>
#include <fstream>
#include <thread>
#include <signal.h>     // 
#include <pthread.h>    // 
#include <fcntl.h>      // 
#include <unistd.h>     // 
#include "debug.h"
#include "dev_common.h"

#define IO_DEV_NAME "/dev/iomanager"


typedef struct
{
    uint8_t  outState;//输出状态
    uint8_t  defaultState; //默认状态，既维持完上面的时间配置的状态
    uint8_t  index; //输出io下标，IO_OUT_INDEX 枚举成员
    uint8_t  res[1];
    uint32_t   holdTime;//保持时间,单位微秒，配置0表示一直保持
}IO_PLUSE_SET;
typedef struct
{
    uint8_t index; //代表LED_FLAG成员
    uint8_t times; //闪烁次数
    uint16_t period; //单位毫秒,配置0 表示一直保持
}IO_FLASH_SET;

typedef struct 
{
    uint8_t num; //亮灯数量,跟下面的参数保持一致,最多四个参数
    uint8_t pwmValue;//pwm的值
    uint8_t res[2]; //预留填充
    uint8_t states[4]; //配置管脚状态,低四位代表管脚坐标，高四位代表输出状态，每个灯的输出时间和下面保持一致
    uint32_t exposureTime[4]; //sensor的曝光时间，单位(微秒)/1-r,2-g,3-b,4-w
    uint32_t periodTime[4]; //周期时间，单位(微秒)
}IO_RGB_SET; //配置一次参数，代表进行num次循环

typedef struct
{
    uint8_t num; //代表input下标，范围：0，1
    uint8_t trigType; //0-代表上升沿，1-代表下降沿
    uint16_t debounce; //防抖时间，单位(毫秒)
}SET_IO_TRIGGER_TYPE;

typedef struct
{
    uint8_t sync_num; // 输出方波数量
    uint8_t res[3];
    uint32_t sync_expo;   // 输出高电平时间，单位微秒
    uint32_t sync_period; // 输出周期时间，单位微秒
} INTERNAL_CONFIG;

typedef struct
{
    uint8_t camNum;     //代表要出图的数量
    uint8_t res[3];
    uint32_t ext_sync_expo;  // 仅输出一次外部脉冲信号的持续时间，单位微秒
    uint32_t ext_xtrig_expo; // 触发相机曝光时间，单位微秒
} EXTERNAL_CONFIG;

typedef union
{
    INTERNAL_CONFIG interConfig; // 内部控制
    EXTERNAL_CONFIG exterConfig; // 外部控制
} MODE_PARAM;

typedef struct
{
    uint8_t mode; // 模式参数,0-完全控制模式，1-外部配合控制模式
    uint8_t res[3];
    MODE_PARAM modeParam;
} IO_2D_SUPPORT_LIGHT_PARAM;

/**
 * @brief 配置1个dlp触发dlp+rgbw或者是不联动参数配置
 * 
 */
typedef struct
{
    uint8_t mode; //0-代表仅dlp闪烁,不联动rgb灯，后面的参数不生效，1-代表dlp+rgb联动方式，
    uint8_t syncDlpNum; //dlp触发的xtrig的总次数
    uint8_t syncRgbNum; //需要在光机触发的xtrig后的第几次执行rgb闪烁
    uint8_t pwmValue;//pwm的值
    uint32_t exposureTime[4]; //led的曝光时间，单位(微秒)/1-r,2-g,3-b,4-w,需要应用程序传参
}IO_RGB_SYNC_SIGNAL;

#define IO_GET_VERSION  _IOW('k', 0, DRIVER_VERSION) //获取驱动版本
#define IO_TEST _IOW('k', 1, IO_PLUSE_SET)    // 测试使用，暂时不用
#define IO_GET_TRIGGER_PIN _IOW('k', 2, int)  // 获取中断管脚，返回值在IO_INPUT_FLAG 枚举成员
#define IO_SET_OUT _IOW('k', 3, IO_PLUSE_SET) // 配置IO输出电平，配置高，配置低
#define IO_SET_DLP_STATUS _IOW('k', 4, uint8_t)                           // 配置dlp总电源开关,只要低四位，1,2,4,8代表依次使能不同的dlp光机，0x0f代表全部使能
#define IO_GET_PIN_STATUS _IOW('k', 5, uint8_t)                           // 获取电路板id的号，暂时不用
#define IO_SET_FLASH_LED  _IOW('k', 6, IO_FLASH_SET)                  // 
#define IO_SET_RGB_PARAM  _IOW('k', 7, IO_RGB_SET)                    // 配置rgb灯闪烁周期和曝光时间
#define IO_SET_IO_TRIGGER_PARAM _IOW('k', 8, SET_IO_TRIGGER_TYPE)    //
#define IO_2D_SET_MODE_PARAM _IOW('k', 9, IO_2D_SUPPORT_LIGHT_PARAM) //
#define IO_SET_USB_ENABLE_DLP_STATUS _IOW('k', 10, uint8_t)               // 高电平使能usb和dlp连接，
#define IO_SET_RGB_TRIG_CAM _IOW('k', 11, uint32_t)                       //配置触发sensor的xtrig1管脚
#define IO_SET_CLEAR_TRIG_COUNT   _IOW('k', 12, uint8_t)                           //配置触发唤醒操作，不用传参
#define IO_SET_GET_RGB_SYNC_SIGNAL      _IOW('k', 13, IO_RGB_SYNC_SIGNAL)   //配置dlp和rgb联动参数
/**
 * @brief 生效 IO_SET_RGB_PARAM 里面的rgb灯的触发参数，触发的时候会判断IO_RGB_SYNC_SIGNAL的mode参数是否为0，否则配置参数不响应
 * 
 */
#define IO_SET_SINGLE_RGB_TRIG          _IOW('k', 14, DEV_IO_RGB_TRIG_SET)   //生效 IO_SET_RGB_PARAM 里面的rgb灯的触发参数，
class DEV_IO
{
public:
    DEV_IO();
    ~DEV_IO();

public:
    DEV_RTN dev_io_init();
    DEV_RTN dev_io_get_ver(std::string&);
    DEV_RTN dev_io_register_event_callback(void (*io_callback)(DEV_FLAG_TYPE));
    DEV_RTN dev_io_set_dlp_enable_usb_value(uint8_t,DEV_IO_DLP_INDEX);
    DEV_RTN dev_io_set_status_led_value(DEV_LED_INDEX, uint16_t, uint8_t);
    DEV_RTN dev_io_trig_out_value(uint8_t, uint32_t, uint8_t);
    DEV_RTN dev_io_set_rgb_status_value(const DEV_IO_RGB_PARAM&); 
    DEV_RTN dev_io_rgb_signal_trig_value(const DEV_IO_RGB_TRIG_SET &);
    DEV_RTN dev_io_set_get_dlp_rgb_sync_states_value(const DEV_IO_DLP_RGB_SYNC_PARAM&);
    DEV_RTN dev_io_clear_trig_count_value();
    DEV_RTN dev_io_set_in_value(uint8_t, uint8_t, uint16_t);
    DEV_RTN dev_io_ctrl_ext2d_intern_ctl_value(uint8_t, uint32_t, uint32_t);
    DEV_RTN dev_io_ctrl_ext2d_exterl_ctl_value(uint8_t, uint32_t , uint32_t);
public:
    void (*event_external_notifier)(DEV_FLAG_TYPE event) = nullptr;
    int triggerPin = 0;
    int m_io_fd = -1;


// private:
    
};


#endif