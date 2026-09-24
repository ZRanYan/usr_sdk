#ifndef _DEV_SENSOR_H_
#define _DEV_SENSOR_H_

#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <linux/v4l2-mediabus.h>
#include <linux/v4l2-subdev.h>
#include <string>
#include <thread>
#include <sys/mman.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <atomic>
#include <mutex>
#include "dev_common.h"
#include "debug.h"

#define USER_SENSOR_TEST 0
#if 1
#define SENSOR_DEV "/dev/video%u"
#define SENSOR_SUB_DEV "/dev/v4l-subdev%u"
#else
#define SENSOR_DEV "/dev/camera-%u"
#define SENSOR_SUB_DEV "/dev/cam-sudev%u"
#endif

#define SENSOR_IMX566_MAX_GAIN 480
#define SENSOR_IMX566_MAX_BLACK 4095

#define SENSOR_IMX566_ALL_PIXEL_HIGHT 2848
#define SENSOR_IMX566_ALL_PIXEL_WIDTH 2856

#define SENSOR_IMX565_ALL_PIXEL_HIGHT 3008
#define SENSOR_IMX565_ALL_PIXEL_WIDTH 4128

#define SENSOR_SC535_ALL_PIXEL_HIGHT 2048
#define SENSOR_SC535_ALL_PIXEL_WIDTH 2448

#define SENSOR_GMAX3405_ALL_PIXEL_HIGHT 2048
#define SENSOR_GMAX3405_ALL_PIXEL_WIDTH 2448

// #define SENSOR_OG02C1B_ALL_PIXEL_HIGHT 1080
#define SENSOR_OG02C1B_ALL_PIXEL_HIGHT 630
#define SENSOR_OG02C1B_ALL_PIXEL_WIDTH 1440

#define SENSOR_OV5640_ALL_PIXEL_HIGHT 1080
#define SENSOR_OV5640_ALL_PIXEL_WIDTH 1920

#define SENSOR_OG05B2B_ALL_PIXEL_HIGHT 1944
#define SENSOR_OG05B2B_ALL_PIXEL_WIDTH 2592

#define SENSOR_GMAX3412_ALL_PIXEL_HIGHT 3072
#define SENSOR_GMAX3412_ALL_PIXEL_WIDTH 4096

#define SENSOR_IMX566_EMB_BUFF_SIZE   8192

#define SENSOR_IMX566_MIN_H  32
#define SENSOR_IMX566_MIN_W  64
#define SENSOR_BUFFER_COUNT  8
#define MAX_PLANES 2
// #define VIDEO_MAX_PLANES     8

#define FMT_NUM_PLANES VIDEO_MAX_PLANES

#define ALIGN_UP(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))

#define WIDTH_ALIGN     64
#define BUFFER_ALIGN    (64 * 1024)  //64kB对齐

#define V4L2_CID_CUSTOM_TEST                (V4L2_CID_CONTRAST)
#define V4L2_CID_GET_VERSION                (V4L2_CID_USER_BASE + 0)
#define V4L2_CID_REGIST_IOCTL               (V4L2_CID_USER_BASE + 1)
#define V4L2_CID_GET_TEMPERATURE_VAL        (V4L2_CID_USER_BASE + 2)
#define V4L2_CID_SET_PATTERN_GENERATOR      (V4L2_CID_USER_BASE + 3)
#define CAM_SET_PWM_XTRIG                   (V4L2_CID_USER_BASE + 4)
#define CAM_GET_MIN_EXPOSE                  (V4L2_CID_USER_BASE + 5)
#define CAM_SET_ROI_FORMAT                  (V4L2_CID_USER_BASE + 6)
#define CAM_SET_GAIN                        (V4L2_CID_USER_BASE + 7)
#define CAM_SET_BLACK_LEVEL					(V4L2_CID_USER_BASE + 8)
#define CAM_SET_HFLIP						(V4L2_CID_USER_BASE + 9)
#define CAM_SET_VFLIP						(V4L2_CID_USER_BASE + 10)
//私有的sc535hgs的配置参数
#define CAM_SET_EXPO_PERIOD_PARAM			(V4L2_CID_USER_BASE + 11)
#define CAM_GET_SENSOR_TYPE					(V4L2_CID_USER_BASE + 12)

#define CAM_SET_CUSTOM_TEST                 (V4L2_CID_USER_BASE + 13)
#define CAM_SET_SENSOR_POWER_STATUS			(V4L2_CID_USER_BASE + 14)


#define TEGRA_CAMERA_CID_BASE           (V4L2_CTRL_CLASS_CAMERA | 0x2000)
#define TEGRA_CAMERA_CID_VI_PREFERRED_STRIDE (TEGRA_CAMERA_CID_BASE + 110)
#define TEGRA_CAMERA_CID_VI_CAPTURE_TIMEOUT (TEGRA_CAMERA_CID_BASE+111)

#define ROI_LIMIT(v, max) (((v) <= 0 || (v) > (max)) ? (max) : (v))

typedef struct{
    __u16 exposureUnit; //曝光的1H时间单位倍数，既最小的曝光时间要求(xtrig持续电平的时间)
    __u16 min_trigger_fall; //相对上一次xtrig的结束点(既上升沿)的下一次xtrig的下降沿触发的禁止触发时间，单位(微秒)
    __u16 min_trigger_rise; //相对上一次xtrig的结束点(既上升沿)的下一次xtrig的上升降沿触发的禁止触发时间，单位(微秒)
	__u16 max_frame; //最高帧率
}SENSOR_ATTRIBUTE;

typedef struct
{
	__u8 isEnable; //0-disable,1-enable
	__u8 bitMode; //0-8bit,1-10bit,2-12bit
	__u8 binningMode; //0-no binning,1-hbinning,2-vbinning,3-hvbinning
	__u8 res;	//保持对齐
	__u16 start_x;
	__u16 start_y;
	__u16 width;
	__u16 height;
}SENSOR_FORMAT_ROI_PARAM;

typedef struct
{
    uint32_t roi_width;     //sensor配置roi的宽度像素数值，跟bit没有关系
    uint32_t sensor_width;  //sensor出图最大的像素宽度，跟bit有关系
    uint32_t soc_width;     //soc出图实际宽度，跟bit有关系，相对roi_width做向上的64对齐
}SENSOR_SOC_WIDTH;

typedef struct
{
    unsigned int idx;
    unsigned int padding[FMT_NUM_PLANES];
    void *mem[FMT_NUM_PLANES];
    uint32_t length[FMT_NUM_PLANES];
    unsigned int count;
} BUFFERS_CAPTURE_DEF;

typedef struct
{
    uint8_t type; //1-是配置参数，0-是获取系统的参数
	uint32_t expo; //配置曝光时间，单位微秒，必须是6的倍数
	uint32_t period; //配置出图的周期时间，单位微秒，必须是6的倍数
	uint32_t minPeriod; //根据曝光时间和出图的行数计算最小的帧周期时间参数，单位微秒
}SENSOR_SOC_EXPO_SET_PARAM;

/**
 * @brief sensor的属性参数，第一个参数为哨兵，作为实际生效的参数
 * 
 */
typedef struct 
{
    DEV_SENSOR_TYPE type;
    DEV_CAM_BIT bit;
    uint16_t  pixel_width;
    uint16_t  pixel_hight;
}SENSOR_PARAM_ATTR;

typedef void (*ImageNotifier)(DEV_IMG_DEF frame, void *usr_data);
typedef void (*SensorErrorNotifier)(DEV_ERR_DEF err);

class DEV_SENSOR
{
public:
    DEV_SENSOR();
    ~DEV_SENSOR();
public:
    DEV_RTN dev_sensor_init(uint8_t, DEV_SENSOR_ATTRIBUTE);
    DEV_SENSOR_TYPE dev_sensor_get_type_value();
    DEV_RTN dev_sensor_set_cap_timeout_value(int);
    void    dev_sensor_get_ver(std::string&);
    DEV_RTN dev_sensor_set_expo_period_value(SENSOR_EXPO_PERIOD_PARAM&);
    DEV_RTN dev_sensor_set_gain_value(uint16_t);
    DEV_RTN dev_sensor_set_hflip_value(bool);
    DEV_RTN dev_sensor_set_vflip_value(bool);
    DEV_RTN dev_sensor_set_black_value(uint16_t);
    DEV_RTN dev_sensor_get_max_frame_value(float&);
    DEV_RTN dev_sesnor_set_roi_value(const DEV_ROI&); //DEV_ROI会对传参进行对齐判断
    DEV_RTN dev_sensor_register_image_callback(ImageNotifier);
    DEV_RTN dev_sensor_register_error_callback(SensorErrorNotifier);
    DEV_RTN dev_sesnor_get_temp_value(float&);
    DEV_RTN dev_sensor_reg_set_value(DEV_SENSOR_REG_PARAM&);
    DEV_RTN dev_sensor_set_test_pic_value(uint8_t);
    DEV_RTN dev_sensor_set_power_on_off_value(bool);
    DEV_RTN dev_sensor_stream_set(bool);
private:
    int m_fd_init(uint8_t);
    int m_fd_release();
    bool m_is_all_zero(const void *data, size_t size); //判断一段数据是不是都为0
    void m_sensor_aligned_set(IN DEV_ROI, OUT SENSOR_FORMAT_ROI_PARAM&, OUT SENSOR_SOC_WIDTH&); //计算需要配置驱动的参数
    DEV_RTN m_sensor_roi_set(const SENSOR_FORMAT_ROI_PARAM&,SENSOR_SOC_WIDTH);
    DEV_RTN m_sensor_stream_operate(bool);
    DEV_RTN m_sensor_buffers_mmap(bool);
    DEV_RTN m_sensor_q_buffer(struct v4l2_buffer*, const enum v4l2_buf_type, const enum v4l2_memory);
    void m_sensor_threads(uint8_t);//sensor取流线程
    bool m_sensor_callback(const uint8_t, uint32_t);//sensor取图回调函数
    void m_saveBufferToFile(const char* buffer, int size, const std::string& filePath);
    DEV_RTN m_sensor_set_stride(uint32_t);//配置nx的对齐宽度数据
    void mPrintFmtInfo(struct v4l2_format &fmt, const enum v4l2_buf_type type); //自定义打印sensor的分辨率参数
    v4l2_buf_type m_get_sensor_mp_lane();
private:
    enum v4l2_buf_type buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    const enum v4l2_memory memory_type = V4L2_MEMORY_MMAP; //共支持三种内存类型
    const __u32 bitModePixfmt[3] = {V4L2_PIX_FMT_SRGGB8, V4L2_PIX_FMT_SRGGB10, V4L2_PIX_FMT_SRGGB12};
    char buffer[128]={0};
private:
    DEV_IMG_DEF current_frame = {};
    std::atomic<bool> flag_dqthread_running;  //代表取流线程标志
    pthread_t m_sensor_dq_thread;   //取流线程
    std::thread m_threads;
    ImageNotifier image_notifiers={nullptr};
    SensorErrorNotifier error_notifiers={nullptr};
    void *usr_data; //用于透传传递给图像回调函数的指针数据，可以为空，类不做检查
private:
    int video_fd = -1;
    int v4l2_fd = -1;
    DEV_ROI mRoi;
    uint8_t index = 0; //代表当前类是第几个sensor
    uint8_t totalNum = 1; //代表系统总共有几个sensor
    int time_out = 10000; //底层出图超时时间，单位ms
    int mTestMode = 0; //默认测试模式，0-正常出图模式
    uint16_t gain = 0;
    uint8_t mBufferNum = SENSOR_BUFFER_COUNT;
    bool m_vflip = false;
    bool m_hflip = false;
    uint16_t black = 15; //黑电平的值
    DEV_SENSOR_TYPE m_sensorType = SENSOR_IMX566; //是imx566还是imx565，有宽高限制
    SENSOR_FORMAT_ROI_PARAM m_RoiParam;
    SENSOR_SOC_WIDTH m_WidthInfo;
    BUFFERS_CAPTURE_DEF *buffers = nullptr;
    uint32_t sensor_buffer_length = 0;
    bool m_flag_mmaped = false; // mmap映射标志位
    bool flag_streamOn = false; // 开流标志位
    uint32_t m_stride_value = 0;
    int g_pic_index = 0;
    SENSOR_PARAM_ATTR mSensorAttr[10] =
    {
       {SENSOR_IMX566, SENSOR_8BIT, SENSOR_IMX566_ALL_PIXEL_WIDTH, SENSOR_IMX566_ALL_PIXEL_HIGHT},
       {SENSOR_IMX565, SENSOR_8BIT, SENSOR_IMX565_ALL_PIXEL_WIDTH, SENSOR_IMX565_ALL_PIXEL_HIGHT},
       {SENSOR_SC535, SENSOR_10BIT, SENSOR_SC535_ALL_PIXEL_WIDTH, SENSOR_SC535_ALL_PIXEL_HIGHT},
       {SENSOR_GMAX3405, SENSOR_12BIT, SENSOR_GMAX3405_ALL_PIXEL_WIDTH, SENSOR_GMAX3405_ALL_PIXEL_HIGHT},
       {SENSOR_OG02C1B, SENSOR_8BIT, SENSOR_OG02C1B_ALL_PIXEL_WIDTH, SENSOR_OG02C1B_ALL_PIXEL_HIGHT},
       {SENSOR_OV5640, SENSOR_16BIT, SENSOR_OV5640_ALL_PIXEL_WIDTH, SENSOR_OV5640_ALL_PIXEL_HIGHT},
       {SENSOR_OG05B2B, SENSOR_8BIT, SENSOR_OG05B2B_ALL_PIXEL_WIDTH, SENSOR_OG05B2B_ALL_PIXEL_HIGHT},
       {SENSOR_GMAX3412, SENSOR_12BIT, SENSOR_GMAX3412_ALL_PIXEL_WIDTH, SENSOR_GMAX3412_ALL_PIXEL_HIGHT}
    };
};

#endif
