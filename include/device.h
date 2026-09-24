/**
 * @file device.h
 * @author yangzhaoran (yangzhaoran@bopixel.com)
 * @brief 
 * @version 0.5.5
 * @date 2026-06-04
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef DEVICE_H
#define DEVICE_H
#include <memory> 
#include <string>
#include <vector>
#include "dev_common.h"

#define DLP_NUM 4
#define SENSOR_MAX_NUM 4

class DEV_SENSOR;
class DEV_DLP;
class DEV_IO;
class DEV_XSTATUS;

class CAM_DEV
{
public:
    CAM_DEV();
    ~CAM_DEV();
    DEV_RTN dev_get_version(DEV_VERSION_INFO_STRUCT &ver);
    void dev_set_debug_log(DEV_LOG_SET_PARAM logSet);
public:
    DEV_RTN dev_init(DEV_SENSOR_ATTRIBUTE sensor, DEV_CAP_TYPE type);
    DEV_RTN dev_sensor_set_cap_timeout(DEV_CAM_INDEX id, int timeout);
    DEV_RTN dev_register_image_callback(DEV_CAM_INDEX id, void (*image_callback)(DEV_IMG_DEF,void*));
    DEV_RTN dev_unregister_image_callback(DEV_CAM_INDEX id);
    DEV_RTN dev_register_event_callback(void (*event_callback)(DEV_FLAG_TYPE event));
    DEV_RTN dev_unregister_event_callback();

    DEV_RTN dev_register_error_callback(DEV_CAM_INDEX id, void (*error_callback)(DEV_ERR_DEF error));
    DEV_RTN dev_unregister_error_callback(DEV_CAM_INDEX id);

public: //配置sensor的地方
    DEV_RTN dev_sensor_get_version(std::string& ver);
    DEV_RTN dev_sesnor_set_expo_period(DEV_CAM_INDEX id, SENSOR_EXPO_PERIOD_PARAM& param);
    DEV_RTN dev_sensor_set_gain(DEV_CAM_INDEX id, uint16_t gain);
    DEV_RTN dev_sensor_set_hflip(DEV_CAM_INDEX id, bool isEnable);
    DEV_RTN dev_sensor_set_vflip(DEV_CAM_INDEX id, bool isEnable);
    DEV_RTN dev_sensor_set_black(DEV_CAM_INDEX id, uint16_t value);
    DEV_RTN dev_sensor_get_frame_rate(DEV_CAM_INDEX id, float& rate);
    DEV_RTN dev_sensor_set_roi(DEV_CAM_INDEX id, const DEV_ROI &roi);
    DEV_RTN dev_sensor_get_temp(DEV_CAM_INDEX id, float& temp);
    DEV_RTN dev_sensor_reg_set(DEV_CAM_INDEX id, DEV_SENSOR_REG_PARAM &reg);
    /**
     * @brief 针对自由出流的模式下，开关流的操作函数，一般在外触发不需要配置
     * 
     * @param id 
     * @param enable true:开流模式，false:关流模式
     * @return DEV_RTN 
     */
    DEV_RTN dev_sensor_set_stream_status(DEV_CAM_INDEX id, bool enable);

    /**
     * @brief 配置sensor出测试图
     * 
     * @param id 
     * @param mode 0-正常出图，其它为测试图的模式值
     * @return DEV_RTN 
     */
    DEV_RTN dev_sensor_set_test_pic(DEV_CAM_INDEX id, uint8_t mode);
    // DEV_RTN dev_sensor_set_power_on_off(DEV_CAM_INDEX id, bool onOff);
public:
    DEV_RTN dev_dlp_find(int (&arr)[DLP_NUM]);
    DEV_RTN dev_dlp_get_version(std::string& ver); //支持dlp4052
    /**
     * @brief 获取dlp的类型
     * 
     * @param type 
     * @return DEV_RTN 
     */
    DEV_RTN dev_dlp_get_type(DEV_DLP_TYPE& type);
    DEV_RTN dev_dlp_get_temp(uint8_t index, float& temp);
    DEV_RTN dev_dlp_set_current(uint8_t index, DEV_DLP_CURRENT_VALUE current); 
    DEV_RTN dev_dlp_set_current_all(uint16_t current);//仅支持dlp30
    /**
     * @brief 获取dlp的电流值
     * 
     * @param index - 选中那个dlp的id
     * @param pwm - 获取dlp的电流值
     * @return DEV_RTN 
     */
    DEV_RTN dev_dlp_get_current(uint8_t index, DEV_DLP_CURRENT_VALUE& pwm);
    DEV_RTN dev_dlp_config_pwm_polarity(uint8_t index, bool readOrwrite, uint8_t &invert);
    DEV_RTN dev_dlp_get_min_expo(uint8_t index, DEV_DLP_EXP &param);
    DEV_RTN dev_dlp_get_group_info(uint8_t index, DEV_DLP_PATTERN_GROUP_INFO &param);
    DEV_RTN dev_dlp_set_groups_info(uint8_t index, std::vector<DEV_DLP_PATTERN_GROUP_INFO> groups);
    DEV_RTN dev_dlp_trigonce(uint8_t index);
    /**
     * @brief 配置dlp翻转和延时时间
     * 
     * @param index 
     * @param isInvert 
     * @param delayUs ：仅对dlp4710有效，dlp4502这个参数忽略
     * @return DEV_RTN 
     */
    DEV_RTN dev_dlp_set_delay_invert(uint8_t index, bool isInvert, uint32_t delayUs);//复用支持dlp4502
    DEV_RTN dev_dlp_updata_falsh(uint8_t index, const char* flash, const char* config);
    DEV_RTN dev_dlp_set_long_short_flip(uint8_t index, bool long_flip, bool short_flip);
    DEV_RTN dev_dlp_set_trig_type(uint8_t index, DEV_DLP_TRIG_TYPE type);
    DEV_RTN dev_dlp_trig_in_config(uint8_t index, bool enable, bool polarity);
    DEV_RTN dev_dlp_set_source_mode(uint8_t index, DEV_DLP_TRIG_MODE mode); //仅支持dlp4052
    DEV_RTN dev_dlp_get_source_mode(uint8_t index, DEV_DLP_TRIG_MODE &mode); //仅支持dlp4052
    /**
     * @brief DLPC350 序列校验流程
     * 
     * @param index 
     * @param status 
     * @return DEV_RTN 
     */
    DEV_RTN dev_dlp_get_validate_status(uint8_t index, uint8_t &status); //仅支持dlp4052
    /**
     * @brief 配置dlp开关
     * 
     * @param index 
     * @param enable 
     * @return DEV_RTN 
     */
    DEV_RTN dev_dlp_set_on_off(uint8_t index, bool enable); //仅支持dlp4052
    /**
     * @brief 
     * 
     * @param index 代表配置那个dlp的下坐标
     * @param readOrwrite 0-获取参数 1-配置参数
     * @param ledSet 传参结构体
     * @return DEV_RTN 
     */
    DEV_RTN dev_dlp_led_config(uint8_t index, bool readOrwrite, DEV_DLP_LED_SET &ledSet);//仅支持dlp4052
    DEV_RTN dev_dlp_exp_period_config(uint8_t index, bool readOrwrite, DEV_DLP_EXPOSURE_PERIOD_SET &set);//仅支持dlp4052
    DEV_RTN dev_dlp_get_pattern_trigger_mode(uint8_t index, uint8_t &mode);//仅支持dlp4052
    /**
     * @brief 软件复位命令
     * 
     * @param index 
     * @param ver ：复位后，会执行读取版本号信息
     * @return DEV_RTN 
     */
    DEV_RTN dev_dlp_set_reboot(uint8_t index, uint32_t &ver);//仅支持dlp4052
    DEV_RTN dev_dlp_set_lut(uint8_t index, const DEV_DLP_PATTERN_ORDER_SET &set);

    DEV_RTN dev_dlp_set_load_timing(uint8_t index, uint8_t image_index,uint8_t num);
    DEV_RTN dev_dlp_get_load_timing(uint8_t index, uint32_t &load_time);
    DEV_RTN dev_dlp_get_status(uint8_t index, uint8_t &hwStatus, uint8_t &sysStatus, uint8_t &mainStatus);
public: //配置io的地方
    DEV_RTN dev_io_get_version(std::string& ver);
    /**
     * @brief 使能usb连接dlp光机和dlp电源的开关
     * 
     * @param isOpen ：使能usb是否连接上dlp光机
     * @param index ：使能dlp的电源，由于在四光机上，usb只能连上单个光机，所以要把其余的光机给断电处理
     * @return DEV_RTN 
     */
    DEV_RTN dev_io_set_dlp_enable_usb(uint8_t isOpen, DEV_IO_DLP_INDEX index);
    DEV_RTN dev_io_set_status_led(DEV_LED_INDEX index, uint16_t durationMs, uint8_t times);
    DEV_RTN dev_io_trig_out(DEV_IO_OUT_INDEX index, uint32_t durationUs, uint8_t status);
    /**
     * @brief 配置rgbw单独闪烁的配置参数，和dev_io_set_get_dlp_rgb_sync_states 参数功能互斥
     * 
     * @param mParam 
     * @return DEV_RTN 
     */
    DEV_RTN dev_io_set_rgb_status(const DEV_IO_RGB_PARAM& mParam);
    /**
     * @brief 单独触发一次rgbw出图，一次出图四张，相关的曝光参数结构体为DEV_IO_RGB_PARAM
     * 
     * @param mParam ：控制闪烁那些图
     * @return DEV_RTN 
     */
    DEV_RTN dev_io_rgb_signal_trig(const DEV_IO_RGB_TRIG_SET& mParam);
//---------------------------------------外部触发参数配置--------------------------------------
    /**
     * @brief 配置管脚的输入防抖参数
     * 
     * @param input_pin - 需要配置的那个输入管脚
     * @param edgeType - 0-代表上升沿，1-代表下降沿
     * @param debounceMs - 在防抖时间内不再响应管脚的信号输入，单位毫秒
     * @return DEV_RTN 
     */
    DEV_RTN dev_io_set_in(SOURCE_IO input_pin, uint8_t edgeType, uint16_t debounceMs);
    /**
     * @brief 控制sensor出图，并拉起外部光源进行sensor补光
     * 
     * @param picNum - 要曝光出图的数量
     * @param camExpo - 配置sensor的曝光时间(单位微秒)，并且在sensor开始曝光的时候拉高外部(EXT_TRGI_OUT1)SYNC管脚，
     *                                  sensor结束曝光的时候，拉低外部(EXT_TRGI_OUT1)SYNC管脚
     * @param expoPeriod - 一次出图的周期时间，包含sensor曝光和非曝光的时间总和，单位微秒
     * @return DEV_RTN - RTN_OKAY-代表正常，其余代表报错
     */
    DEV_RTN dev_io_ctrl_ext2d_intern_ctl(uint8_t picNum, uint32_t camExpo, uint32_t expoPeriod);
    /**
     * @brief 使用外部的光源信号输入管脚(IO_INPUT_1)2D配合控制sensor出图
     * 
     * @param picNum - 要曝光出图的数量，达到数量限制后，关闭管脚(IO_INPUT_1)2D的输入响应
     * @param camExpo - 等待管脚(IO_INPUT_1)2D输入响应后开始进行snesor曝光的时间参数配置,单位微秒
     * @param outTime - 立即输出一次(EXT_TRGI_OUT1)SYNC管脚的脉冲信号的持续时间，单位微秒
     * @return DEV_RTN - RTN_OKAY-代表正常，其余代表报错
     */
    DEV_RTN dev_io_ctrl_ext2d_exterl_ctl(uint8_t picNum, uint32_t camExpo, uint32_t outTime);
//----------------------------------------------------------------------------------------------------------
    /**
     * @brief 配置RGBW灯联动DLP的时间参数
     * 
     * @param syncParam 
     * @return DEV_RTN 
     */
    DEV_RTN dev_io_set_get_dlp_rgb_sync_states(const DEV_IO_DLP_RGB_SYNC_PARAM& syncParam);
    /**
     * @brief 清空驱动底层计数器的值，在投完一轮dlp图后，进行下一轮投图之前执行
     * 
     * @return DEV_RTN 
     */
    DEV_RTN dev_io_clear_trig_count(void);

public: //读取soc的系统状态的地方
    DEV_RTN dev_xstatus_get_temp(DEV_XSTATUS_TYPE type, float& value);
    DEV_RTN dev_xstatus_get_cam_status(DEV_XSTATUS_VALUE& mParam);

private:
    int dlp_id[DLP_NUM]={0};

private:
    uint8_t sensor_num = 1;
    DEV_SENSOR *sensor[SENSOR_MAX_NUM] = {nullptr};
    DEV_DLP *dlp = nullptr;
    DEV_IO *io = nullptr;
    DEV_XSTATUS *xst = nullptr;
};
#endif // DEVICE_H