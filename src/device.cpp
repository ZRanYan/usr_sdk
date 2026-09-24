#include <iostream>
#include "../include/debug.h"
#include "../include/device.h"
#include "../include/device_soft_cap.h"
#include "../include/dev_sensor.h"
#include "../include/dev_dlp.h"
#include "../include/dev_io.h"
#include "../include/dev_xstatus.h"

typedef enum
{
    MODULE_SENSOR = 0,
    MODULE_DLP = 1,
    MODULE_IO = 2,
    MODULE_XSTATUS = 3,
} DEV_MODULE;

namespace
{
    bool is_cap_enable(DEV_SOFT_CAPABILITY *cap, DEV_MODULE module)
    {
        switch (module)
        {
        case MODULE_SENSOR:
            return cap->isEnableSensor;

        case MODULE_DLP:
            return cap->isEnableDlp;

        case MODULE_IO:
            return cap->isEnableIo;

        case MODULE_XSTATUS:
            return cap->isEnableXst;

        default:
            return false;
        }
    }
}

#define CHECK_CAP_ENABLE(module)                          \
    do                                                    \
    {                                                     \
        if (!is_cap_enable(g_cap, module))                \
        {                                                 \
            DEBUG_LOG(APP, ERROR,                         \
                      "not support module:%d\n", module); \
            return RTN_NOT_SUPPORT;                       \
        }                                                 \
    } while (0)

CAM_DEV::CAM_DEV()
{
    init_debug("./log.txt");
    logPrintSettingInfo();
}

CAM_DEV::~CAM_DEV()
{
    // if (true == g_cap->isEnableSensor)
    {
        if (nullptr != this->sensor[0])
        {
            delete this->sensor[0];
            this->sensor[0] = nullptr;
        }
        if (nullptr != this->sensor[1])
        {
            delete this->sensor[1];
            this->sensor[1] = nullptr;
        }
    }
    // if (true == g_cap->isEnableIo)
    {
        if (nullptr != this->io)
        {
            delete this->io;
            this->io = nullptr;
        }
    }
    // if (true == g_cap->isEnableDlp)
    {
        if (nullptr != this->dlp)
        {
            delete this->dlp;
            this->dlp = nullptr;
        }
    }
    // if (true == g_cap->isEnableXst)
    {
        if (nullptr != this->xst)
        {
            delete this->xst;
            this->xst = nullptr;
        }
    }
}

DEV_RTN CAM_DEV::dev_get_version(DEV_VERSION_INFO_STRUCT &ver)
{
    ver.commitId = GIT_COMMIT;
    ver.gitBranch = GIT_BRANCH;
    ver.date = PROGRAM_DATE;
    ver.version = PROGRAM_VER;
    ver.orin_sn = "none";
    ver.ssd_sn = "none";
    CHECK_CAP_ENABLE(MODULE_XSTATUS);
    this->xst->dev_xstatus_get_orin_sn(ver.orin_sn);
    this->xst->dev_xstatus_get_ssd_sn(ver.ssd_sn);
    return RTN_OKAY;
}

void CAM_DEV::dev_set_debug_log(DEV_LOG_SET_PARAM logSet)
{
    logPrintDebugSet(logSet.level, logSet.module, logSet.log_storage);
}

DEV_RTN CAM_DEV::dev_init(DEV_SENSOR_ATTRIBUTE sensor, DEV_CAP_TYPE type)
{
    if (ORIN_DEV == type)
    {
        g_cap = &orin_dev;
    }
    else if (NXP_DEV == type)
    {
        g_cap = &nxp_dev;
    }
    else if (XILINX_DEV == type)
    {
        g_cap = &xilinx_dev;
    }
    else if (ORIN_EXT == type)
    {
        g_cap = &orin_ext_dev;
    }
    DEBUG_LOG(APP, INFO, "support type: %d \r\n", type);
    if (true == g_cap->isEnableSensor)
    {
        DEBUG_LOG(APP, INFO, "support Sensor cap init!\r\n");
        if (SENSOR_MAX_NUM >= sensor.sensorNum)
        {
            this->sensor_num = sensor.sensorNum;
        }
        else
        {
            DEBUG_LOG(APP, ERROR, "sensor.sensorNum:%d \r\n", sensor.sensorNum);
            return RTN_INVALID_ARG;
        }
        DEBUG_LOG(APP, INFO, "sensor_num:%d\r\n", this->sensor_num);
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (sensor.index & (1 << i))
            {
                this->sensor[i] = new DEV_SENSOR;
                this->sensor[i]->dev_sensor_init(i, sensor);
                sleep(1);
            }
        }
    }
    if (true == g_cap->isEnableIo)
    {
        this->io = new DEV_IO;
        this->io->dev_io_init();
    }
    if (true == g_cap->isEnableDlp)
    {
        DEBUG_LOG(APP, INFO, "support dlp cap init!\r\n");
        this->dlp = new DEV_DLP;
        this->dlp->dev_dlp_init(this->dlp_id);
    }
    if (true == g_cap->isEnableXst)
    {
        DEBUG_LOG(APP, INFO, "support xstatus init!\r\n");
        this->xst = new DEV_XSTATUS;
        this->xst->dev_xstatus_init();
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_sensor_set_cap_timeout(DEV_CAM_INDEX id, int timeout)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_set_cap_timeout_value(timeout);
        }
    }
    return RTN_OKAY;
}

// to do
DEV_RTN CAM_DEV::dev_register_image_callback(DEV_CAM_INDEX id, void (*image_callback)(DEV_IMG_DEF frame, void *usr_data))
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_register_image_callback(image_callback);
        }
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_unregister_image_callback(DEV_CAM_INDEX id)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_register_image_callback(nullptr);
        }
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_register_error_callback(DEV_CAM_INDEX id, void (*error_callback)(DEV_ERR_DEF error))
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_register_error_callback(error_callback);
        }
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_unregister_error_callback(DEV_CAM_INDEX id)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_register_error_callback(nullptr);
        }
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_register_event_callback(void (*event_callback)(DEV_FLAG_TYPE event))
{
    CHECK_CAP_ENABLE(MODULE_IO);
    this->io->dev_io_register_event_callback(event_callback);
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_unregister_event_callback()
{
    CHECK_CAP_ENABLE(MODULE_IO);
    this->io->dev_io_register_event_callback(nullptr);
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_sensor_get_version(std::string &ver)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    this->sensor[0]->dev_sensor_get_ver(ver);
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_sesnor_set_expo_period(DEV_CAM_INDEX id, SENSOR_EXPO_PERIOD_PARAM &param)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_set_expo_period_value(param);
        }
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_sensor_set_gain(DEV_CAM_INDEX id, uint16_t gain)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_set_gain_value(gain);
        }
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_sensor_set_hflip(DEV_CAM_INDEX id, bool isEnable)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_set_hflip_value(isEnable);
        }
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_sensor_set_vflip(DEV_CAM_INDEX id, bool isEnable)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_set_vflip_value(isEnable);
        }
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_sensor_set_black(DEV_CAM_INDEX id, uint16_t value)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_set_black_value(value);
        }
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_sensor_get_frame_rate(DEV_CAM_INDEX id, float &rate)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_get_max_frame_value(rate);
        }
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_sensor_set_roi(DEV_CAM_INDEX id, const DEV_ROI &roi)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sesnor_set_roi_value(roi);
        }
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_sensor_get_temp(DEV_CAM_INDEX id, float &temp)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sesnor_get_temp_value(temp);
        }
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_sensor_reg_set(DEV_CAM_INDEX id, DEV_SENSOR_REG_PARAM &reg)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_reg_set_value(reg);
        }
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_sensor_set_stream_status(DEV_CAM_INDEX id, bool enable)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_stream_set(enable);
        }
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_sensor_set_test_pic(DEV_CAM_INDEX id, uint8_t mode)
{
    CHECK_CAP_ENABLE(MODULE_SENSOR);
    for (uint16_t i = 0; i < this->sensor_num; i++)
    {
        if (id & (1 << i))
        {
            this->sensor[i]->dev_sensor_set_test_pic_value(mode);
        }
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_dlp_find(int (&arr)[DLP_NUM])
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    std::copy(this->dlp_id, this->dlp_id + DLP_NUM, arr);
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_dlp_get_version(std::string &ver)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    this->dlp->dev_dlp_get_ver(ver);
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_dlp_get_type(DEV_DLP_TYPE &type)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    this->dlp->dev_dlp_get_type_value(type);
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_dlp_get_temp(uint8_t index, float &temp)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_get_temp_value(index, temp);
}

DEV_RTN CAM_DEV::dev_dlp_set_current(uint8_t index, DEV_DLP_CURRENT_VALUE current)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_set_current_value(index, current);
}

DEV_RTN CAM_DEV::dev_dlp_set_current_all(uint16_t current)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_set_current_all_value(current);
}

DEV_RTN CAM_DEV::dev_dlp_get_current(uint8_t index, DEV_DLP_CURRENT_VALUE &pwm)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_get_pwm_value(index, pwm);
}
DEV_RTN CAM_DEV::dev_dlp_config_pwm_polarity(uint8_t index, bool readOrwrite, uint8_t &invert)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_config_pwm_polarity_value(index, readOrwrite, invert);
}

DEV_RTN CAM_DEV::dev_dlp_set_groups_info(uint8_t index, std::vector<DEV_DLP_PATTERN_GROUP_INFO> groups)
{
    int size = groups.size();
    uint8_t write_ctl = 0;
    DEV_RTN rtn;
    CHECK_CAP_ENABLE(MODULE_DLP);
    for (int i = 0; i < size; i++)
    {
        if (i == 0)
            write_ctl = 1;
        else
        {
            write_ctl = 0;
        }
        rtn = this->dlp->dev_dlp_set_group_info_value(index, write_ctl, groups[i]);
    }
    return rtn;
}

DEV_RTN CAM_DEV::dev_dlp_get_group_info(uint8_t index, DEV_DLP_PATTERN_GROUP_INFO &param)
{
    DEV_RTN rtn;
    CHECK_CAP_ENABLE(MODULE_DLP);
    rtn = this->dlp->dev_dlp_set_group_info_value(index, 2, param); //
    rtn = this->dlp->dev_dlp_get_group_info_value(index, param);
    return rtn;
}

DEV_RTN CAM_DEV::dev_dlp_trigonce(uint8_t index)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_trigonce_new_set(index);
}
DEV_RTN CAM_DEV::dev_dlp_get_min_expo(uint8_t index, DEV_DLP_EXP &param)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_get_min_expo_value(index, param);
}

DEV_RTN CAM_DEV::dev_dlp_set_delay_invert(uint8_t index, bool isInvert, uint32_t delayUs)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_set_delay_invert_value(index, isInvert, delayUs);
}

DEV_RTN CAM_DEV::dev_dlp_updata_falsh(uint8_t index, const char *path, const char *config)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_update_falsh_value(index, path, config);
}

DEV_RTN CAM_DEV::dev_dlp_set_long_short_flip(uint8_t index, bool long_flip, bool short_flip)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_set_long_short_flip_value(index, long_flip, short_flip);
}

DEV_RTN CAM_DEV::dev_dlp_set_trig_type(uint8_t index, DEV_DLP_TRIG_TYPE type)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_set_trig_type_value(index, type);
}
DEV_RTN CAM_DEV::dev_dlp_trig_in_config(uint8_t index, bool enable, bool polarity)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_trig_in_config_value(index, enable, polarity);
}

DEV_RTN CAM_DEV::dev_dlp_set_source_mode(uint8_t index, DEV_DLP_TRIG_MODE mode)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_set_source_mode_value(index, mode);
}
DEV_RTN CAM_DEV::dev_dlp_get_source_mode(uint8_t index, DEV_DLP_TRIG_MODE &mode)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_get_source_mode_value(index, mode);
}

DEV_RTN CAM_DEV::dev_dlp_get_validate_status(uint8_t index, uint8_t &status)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_get_validate_status_value(index, status);
}
DEV_RTN CAM_DEV::dev_dlp_set_on_off(uint8_t index, bool enable)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_set_on_off_value(index, enable);
}
DEV_RTN CAM_DEV::dev_dlp_get_pattern_trigger_mode(uint8_t index, uint8_t &mode)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_get_pattern_trigger_mode_value(index, mode);
}
DEV_RTN CAM_DEV::dev_dlp_set_reboot(uint8_t index, uint32_t &ver)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_set_reboot_value(index, ver);
}
DEV_RTN CAM_DEV::dev_dlp_set_lut(uint8_t index, const DEV_DLP_PATTERN_ORDER_SET &set)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_set_lut_value(index, set);
}

DEV_RTN CAM_DEV::dev_dlp_set_load_timing(uint8_t index, uint8_t image_index, uint8_t num)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_set_load_timing_value(index, image_index, num);
}
DEV_RTN CAM_DEV::dev_dlp_get_load_timing(uint8_t index, uint32_t &load_time)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_get_load_timing_value(index, load_time);
}
DEV_RTN CAM_DEV::dev_dlp_get_status(uint8_t index, uint8_t &hwStatus, uint8_t &sysStatus, uint8_t &mainStatus)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_get_status_value(index, hwStatus, sysStatus, mainStatus);
}

DEV_RTN CAM_DEV::dev_dlp_led_config(uint8_t index, bool readOrwrite, DEV_DLP_LED_SET &ledSet)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_led_config_value(index, readOrwrite, ledSet);
}

DEV_RTN CAM_DEV::dev_dlp_exp_period_config(uint8_t index, bool readOrwrite, DEV_DLP_EXPOSURE_PERIOD_SET &set)
{
    CHECK_CAP_ENABLE(MODULE_DLP);
    return this->dlp->dev_dlp_exp_period_config_value(index, readOrwrite, set);
}

DEV_RTN CAM_DEV::dev_io_get_version(std::string &ver)
{
    CHECK_CAP_ENABLE(MODULE_IO);
    return this->io->dev_io_get_ver(ver);
}
DEV_RTN CAM_DEV::dev_io_set_dlp_enable_usb(uint8_t isOpen, DEV_IO_DLP_INDEX index)
{
    CHECK_CAP_ENABLE(MODULE_IO);
    return this->io->dev_io_set_dlp_enable_usb_value(isOpen, index);
}
DEV_RTN CAM_DEV::dev_io_set_status_led(DEV_LED_INDEX index, uint16_t durationMs, uint8_t times)
{
    CHECK_CAP_ENABLE(MODULE_IO);
    return this->io->dev_io_set_status_led_value(index, durationMs, times);
}
DEV_RTN CAM_DEV::dev_io_trig_out(DEV_IO_OUT_INDEX index, uint32_t durationUs, uint8_t status)
{
    CHECK_CAP_ENABLE(MODULE_IO);
    if (IO_SET_RES_TRIG_SENSOR != index)
    {
        return this->io->dev_io_trig_out_value(index, durationUs, status);
    }
    return RTN_INVALID_ARG;
}

DEV_RTN CAM_DEV::dev_io_set_rgb_status(const DEV_IO_RGB_PARAM &mParam)
{
    CHECK_CAP_ENABLE(MODULE_IO);
    return this->io->dev_io_set_rgb_status_value(mParam);
}

DEV_RTN CAM_DEV::dev_io_rgb_signal_trig(const DEV_IO_RGB_TRIG_SET &mParam)
{
    CHECK_CAP_ENABLE(MODULE_IO);
    return this->io->dev_io_rgb_signal_trig_value(mParam);
}

DEV_RTN CAM_DEV::dev_io_clear_trig_count()
{
    CHECK_CAP_ENABLE(MODULE_IO);
    return this->io->dev_io_clear_trig_count_value();
}
DEV_RTN CAM_DEV::dev_io_set_in(SOURCE_IO index, uint8_t type, uint16_t debounceMs)
{
    CHECK_CAP_ENABLE(MODULE_IO);
    return this->io->dev_io_set_in_value(index, type, debounceMs);
}
DEV_RTN CAM_DEV::dev_io_ctrl_ext2d_intern_ctl(uint8_t picNum, uint32_t camExpo, uint32_t expoPeriod)
{
    CHECK_CAP_ENABLE(MODULE_IO);
    return this->io->dev_io_ctrl_ext2d_intern_ctl_value(picNum, camExpo, expoPeriod);
}

DEV_RTN CAM_DEV::dev_io_ctrl_ext2d_exterl_ctl(uint8_t picNum, uint32_t camExpo, uint32_t outTime)
{
    CHECK_CAP_ENABLE(MODULE_IO);
    return this->io->dev_io_ctrl_ext2d_exterl_ctl_value(picNum, camExpo, outTime);
}

DEV_RTN CAM_DEV::dev_io_set_get_dlp_rgb_sync_states(const DEV_IO_DLP_RGB_SYNC_PARAM &syncParam)
{
    CHECK_CAP_ENABLE(MODULE_IO);
    return this->io->dev_io_set_get_dlp_rgb_sync_states_value(syncParam);
}

DEV_RTN CAM_DEV::dev_xstatus_get_temp(DEV_XSTATUS_TYPE type, float &value)
{
    CHECK_CAP_ENABLE(MODULE_XSTATUS);
    return (this->xst->dev_xstatus_get_temp_value(type, value) == 0)
               ? RTN_OKAY
               : RTN_FAIL;
}
DEV_RTN CAM_DEV::dev_xstatus_get_cam_status(DEV_XSTATUS_VALUE &mParam)
{
    CHECK_CAP_ENABLE(MODULE_XSTATUS);
    int ret = this->xst->dev_xstatus_get_temp_value(CPU_TEMP_TYPE, mParam.cpuTemp);
    ret |= this->xst->dev_xstatus_get_temp_value(GPU_TEMP_TYPE, mParam.gpuTemp);
    ret |= this->xst->dev_xstatus_get_temp_value(CPU_MEMY_TYPE, mParam.cpuMem);
    ret |= this->xst->dev_xstatus_get_temp_value(GPU_MEMY_TYPE, mParam.gpuMem);
    ret |= this->xst->dev_xstatus_get_temp_value(TMP117_TEMP_TYPE, mParam.exterTemp);
    return ((0 == ret) ? RTN_OKAY : RTN_FAIL);
}
