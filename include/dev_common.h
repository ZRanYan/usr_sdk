
#ifndef _DEV_COMMON_H_
#define _DEV_COMMON_H_
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#define DRIVER_VER_MAX_LEN 128

#ifndef IN
    #define IN
#endif

#ifndef OUT
    #define OUT
#endif

typedef enum {
    RTN_OKAY                =   0,
    RTN_FAIL                =   1,
    RTN_NOT_ALLOWED         =   2,
    RTN_INVALID_ARG         =   3,
    RTN_BUSY_NOW            =   4,
    RTN_NOT_SUPPORT         =   5,
    RTN_UNKNOWN_ERR         =   6,
}DEV_RTN;

typedef enum{
    CAM_INDEX_ONE = 1<<0,
    CAM_INDEX_TWO = 1<<1,
    CAM_INDEX_THREE = 1<<2,
    CAM_INDEX_FOUR = 1<<3,
    CAM_INDEX_ALL = 0xff,
}DEV_CAM_INDEX; // CAM_INDEX_ONE|CAM_TWO //CAM_INDEX_ALL

typedef enum{
    SENSOR_IMX566 = 0,
    SENSOR_IMX565 =1,
    SENSOR_SC535 = 2,
    SENSOR_GMAX3405 = 3,
    SENSOR_OG02C1B = 4,
    SENSOR_OV5640 = 5,
    SENSOR_OG05B2B = 6,
    SENSOR_GMAX3412 = 7
}DEV_SENSOR_TYPE;

/**
 * @brief DLP光机的类型
 * 
 */
typedef enum{
    DLP_4710 = 0,
    DLP_4052 = 1,
}DEV_DLP_TYPE;
/**
 * @brief 配置DLP的电流参数
 * 
 */
typedef struct
{
    uint16_t blue_cur; //dlp4710使用2字节，dlp4052只有低位字节有效
    uint8_t  green_cur; //dlp4710光机参数无效
    uint8_t  red_cur; //dlp4710光机参数无效
}DEV_DLP_CURRENT_VALUE;

/**
 * @brief 针对SC535HGS配置连续抓拍参数需要设置曝光时间和帧周期时间
 * 索尼的sensor不需要配置这个
 * 
 */
typedef struct
{
	uint8_t type; //1-是配置参数，0-是获取系统的参数
	uint32_t expo; //配置曝光时间，单位微秒，必须是6的倍数
	uint32_t period; //配置出图的周期时间，单位微秒，必须是6的倍数
    float    maxFps; //根据曝光时间计算最大的帧率
    uint32_t minPeriod; //计算最小的帧周期时间
}SENSOR_EXPO_PERIOD_PARAM;
/**
 * @brief 初始化配置载板的sensor数量和类型
 * 
 */
typedef struct
{
    DEV_CAM_INDEX index;// 软件想要配置第几个sensor
    DEV_SENSOR_TYPE type;//sensor的类型
    uint16_t x; 
    uint16_t y;
    uint16_t w; //如不需要配置ROI参数，这个地方赋值为0
    uint16_t h; //如果不需要配置ROI参数，这个地方赋值为0
    uint8_t sensorNum; //1-代表单目，2-代表双目,代表硬件上实际插入的sensor数量
    uint8_t v4l_sub_slot; //非gmsl相机,单目配置参数为1，双目配置参数2,点胶机gmsl相机上配置4
    uint8_t bufferNum; //出图的mmap的buffer数量
    int oot;        //出图超时时间，单位：毫秒
    uint8_t testPicMode; //0-代表不配置测试图模式
    void *usr_data;     //传递自定义参数
}DEV_SENSOR_ATTRIBUTE;

typedef enum{
    SENSOR_8BIT = 0,
    SENSOR_10BIT = 1,
    SENSOR_12BIT = 2,
    SENSOR_16BIT = 3
}DEV_CAM_BIT;

typedef enum{
    SENSOR_ROI = 0,
    SENSOR_SUBSAMPLING = 1,
    SENSOR_BINNING = 2,
}DEV_CAM_MODE;

typedef struct{
    uint32_t len;                 /* 实际字符串长度 */
    char  ver[DRIVER_VER_MAX_LEN];
}DRIVER_VERSION;

/**
 * @brief 直接配置和读取sensor的寄存器的值
 * 
 */
typedef struct{
    uint8_t opt; //0-代表读，1-代表写
    uint8_t value;
    uint16_t addr;
}DEV_SENSOR_REG_PARAM; 

typedef struct
{
    DEV_CAM_BIT bitMode;     // 0:8bit,1:10bit,2:12bit
    DEV_CAM_MODE binningMode; // 0: no binning,1: sub, 2:2x2 binning
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint16_t alignWidth;
} DEV_ROI;
/**
 * @brief 出图回调结构体参数，包含图像的属性参数
 * 
 */
typedef struct{
    uint8_t     sensor_index; //0-代表第一个sensor出图，1-代表第二个sensor出图
    uint16_t    width;
    uint16_t    height;
    uint16_t    mer_fps;
    uint32_t    all_bytes; //raw的 buf size
    uint16_t    emb_bytes; //emb的开的buffer大小，实际不需要怎么大，imx566的sensor只有92字节有效数据
    uint16_t    stride; // bytes per scanline (bytesperline)
    uint16_t    bit_mode; // 0:8bit,1:10bit,2:12bit
    uint32_t    sequence; //新增kernel传出来图像张数
    unsigned char   *data; //图像的raw数据
    unsigned char   *embData; //emb数据
}DEV_IMG_DEF;

typedef struct{
    unsigned char number_of_patterns=0;
    unsigned int pre_exp=0;
    unsigned int exp=0;
    unsigned int post_exp=0;
}PATTERN_PARAM;

typedef enum
{
    ORIN_DEV = 0,
    NXP_DEV = 1,
    XILINX_DEV = 2,
    ORIN_EXT = 3,
}DEV_CAP_TYPE;

typedef enum{
    FLAG_TYPE_TRIG_2D       =   0,
    FLAG_TYPE_TRIG_3D       =   1,
    FLAG_TYPE_TRIG_TOUT     = 2,
    FLAG_TYPE_TRIG_FINSIHED =   3, //收图完整标志
}DEV_FLAG_TYPE;

typedef enum{
  ERR_NONE                =   0,
  ERR_FRAME_LOST          =   1,
  ERR_QBUFFER_ERROR       =   2,
  ERR_UNKNOWN_ERR         =   6,
}DEV_ERR_DEF;

typedef enum {
    LED_1 = 0, //根网口指示灯进行混用了
    LED_2,
    LED_3,
}DEV_LED_INDEX;

typedef enum
{
    IO_SET_OUT_SYNC = 0,
    IO_SET_OUT_AC = 1,
    IO_SET_OUT_CC = 2,
    IO_SET_RES_TRIG_SENSOR = 3, //双sensor的软件触发
}DEV_IO_OUT_INDEX;

typedef enum{
    LED_OFF,
    LED_ON,
    LED_FLASH
}DEV_LED_MODE;

typedef enum
{
    IO_INPUT_1 = 0,  // 2D
    IO_INPUT_2 = 1,  // 3D
    CAM_XTRIG_1 = 2, //第一个sensor的xtrig
    CAM_TOUT_0 = 3,  //第一个sensor的tout0
    CAM_TOUT_0_EXT = 4, //第二个sensor的xtrig
} SOURCE_IO;

typedef enum{
    DLP_OFF = 0,
    DLP_0 = 1<<0,
    DLP_1 = 1<<1,
    DLP_2 = 1<<2,
    DLP_3 = 1<<3,
    DLP_ALL = 0xff
}DEV_IO_DLP_INDEX; //用于配置dlp联通usb进行配置的参数

typedef struct{
    uint16_t pre_exp;
    uint16_t exp;
    uint16_t post_exp;
}DEV_DLP_EXP;

typedef struct DEV_DLP_SELECT_S{
    uint8_t group_id;
    uint8_t num_pattern;
    DEV_DLP_EXP exp_s;
}DEV_DLP_PATTERN_GROUP_INFO;
/**
 * @brief DLP45光机设置图卡顺序专用
 * @param pattern_num - 设置投影多少张图【例如设备内一共有47张图，只要投影17张图则赋值17】
 * @param img_num - 以24bit为区分的话，要设置的图卡涉及多少个24bit就赋值多少【例如47张图，但是其中第一张图是8bit则一共54bit，涉及3组】
 * @param bit_depth - 1-代表1bit，8-代表8bit
 * @param image_squ - 设置图卡顺序涉及的图组顺序【例如先设置第2组图卡，再设置第1组图卡，再设置第2组图卡，则依次填入1 0 1】
 * @param pattern_squ - 每组图卡内图卡的顺序【例如设置第一组内的顺序，则依次填入0 8,9,10......23，因为第一组内的第一张图是8bit】
 * @param pre_num_squ - 第多少张图片是一组【第一张图(8bit)占第一组24的0-7位，剩下的图片一张占一位，故第一组是17张图;第二组都是1bit图，故第二个值给17+24=41】
 * 
 */
typedef struct
{
    uint8_t pattern_num;  
    uint8_t img_num;  
    uint32_t expo_time;//us
    uint32_t period_time;//us
    std::vector<uint8_t> bit_depth;
    std::vector<uint8_t> image_squ;
    std::vector<uint8_t> pattern_squ;
    std::vector<uint8_t> pre_num_squ; 
}DEV_DLP_PATTERN_ORDER_SET;

/**
 * @brief DLP触发类型
 * 
 */
typedef enum
{
    TRIG_PAUSE = 0,
    TRIG_CONTINUOUS = 1,
}DEV_DLP_TRIG_TYPE;
/**
 * @brief 触发类型，仅支持dlp4052
 * 
 */
typedef enum
{
    TRIG_VIDEO = 0,
    TRIG_PATTERN = 1,
}DEV_DLP_TRIG_MODE;

typedef struct
{
    uint32_t exposure_time; //单位微秒
    uint32_t period_time;
}DEV_DLP_EXPOSURE_PERIOD_SET;

typedef struct{
    uint32_t r_expo; //时间单位微秒
    uint32_t g_expo;
    uint32_t b_expo;
    uint32_t w_expo;
    uint32_t period;
    uint8_t  rgbPwm; //rgbw灯的亮度参数，值越大，亮度越暗
}DEV_IO_RGB_PARAM;

typedef struct{
    bool rgbw_on[4];
}DEV_IO_RGB_TRIG_SET;//配置依次亮灯的配置
/**
 * @brief 灯的开关只能开一个
 * 
 */
typedef struct{
    bool ctlMethod; //Control_method(0:Manual, 1:Pattern)
    bool red_on;    //开关
    bool green_on;
    bool blue_on;
}DEV_DLP_LED_SET;

typedef struct{
    uint8_t mode;      //0-代表仅dlp闪烁,不联动rgb灯，后面的参数不生效，1-代表dlp+rgb联动方式
    uint8_t syncDlpNum;//dlp触发的xtrig的总次数
    uint8_t syncRgbNum;//需要在光机触发的xtrig后的第几次执行rgb闪烁
    uint8_t pwmValue; //rgbw灯的亮度参数，值越大，亮度越暗
    uint32_t rgbExpoTime[4]; //rgb的曝光时间，单位(微秒)/1-r,2-g,3-b,4-w
}DEV_IO_DLP_RGB_SYNC_PARAM;

typedef enum
{
    CPU_TEMP_TYPE = 0,
    GPU_TEMP_TYPE = 1,
    CPU_MEMY_TYPE = 2,
    GPU_MEMY_TYPE = 3,
    TMP117_TEMP_TYPE = 4
}DEV_XSTATUS_TYPE;

typedef struct{
    float cpuTemp;
    float gpuTemp;
    float cpuMem;
    float gpuMem;
    float exterTemp; //tmp117温度传感器采集数据
}DEV_XSTATUS_VALUE;

typedef struct
{
    std::string commitId;
    std::string gitBranch;
    std::string date;
    std::string version;
    std::string orin_sn; //orin开发板的sn的唯一编号
    std::string ssd_sn; //固态硬盘的sn唯一编号
}DEV_VERSION_INFO_STRUCT;

typedef enum
{
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,
    LOG_PRINT_LEVEL_END
}DEV_LOG_LEVEL;

typedef enum
{
    LOG_APP = 1 << 0,
    LOG_FPGA = 1 << 1,
    LOG_SENSOR = 1 << 2,
    LOG_NET = 1 << 3,
    LOG_ALG = 1<<4,
    LOG_PIC = 1<<5,
    LOG_OTHER = 1 << 6,
    LOG_ALL = 0xff
} DEV_LOG_MODULE;

typedef enum
{
    LOG_PRINT_TERMINAL = 1 << 0,
    LOG_PRINT_FLASH = 2 << 0
} DEV_LOG_STORAGE_METHOD;

typedef struct
{
    uint8_t level; //对应DEV_LOG_LEVEL
    uint16_t module;  //对应DEV_LOG_MODULE枚举值,支持多个参数以|方式配置
    uint16_t log_storage;//对应DEV_LOG_STORAGE_METHOD枚举值，支持多个参数以|方式配置
}DEV_LOG_SET_PARAM;

#endif
