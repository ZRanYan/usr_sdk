#include <iostream>
#include "../include/debug.h"
#include "../include/device.h"
#include "../include/device_soft_cap.h"
#include "../include/dev_sensor.h"
#include "../include/dev_dlp.h"
#include "../include/dev_io.h"
#include "../include/dev_xstatus.h"

CAM_DEV::CAM_DEV() {
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

void CAM_DEV::dev_get_version(DEV_VERSION_INFO_STRUCT &ver)
{
    ver.commitId = GIT_COMMIT;
    ver.gitBranch = GIT_BRANCH;
    ver.date = PROGRAM_DATE;
    ver.version = PROGRAM_VER;
    if (g_cap->isEnableXst)
    {
        this->xst->dev_xstatus_get_orin_sn(ver.orin_sn);
        this->xst->dev_xstatus_get_ssd_sn(ver.ssd_sn);
    }
    else
    {
        ver.orin_sn = "none";
        ver.ssd_sn = "none";
    }
    return;
}

void CAM_DEV::dev_set_debug_log(DEV_LOG_SET_PARAM logSet)
{
    logPrintDebugSet(logSet.level, logSet.module, logSet.log_storage);
}

DEV_RTN CAM_DEV::dev_init(DEV_SENSOR_ATTRIBUTE sensor, DEV_CAP_TYPE type)
{
    if(ORIN_DEV == type)
    {
        g_cap = &orin_dev;
    }
    else if(NXP_DEV == type)
    {
        g_cap = &nxp_dev;
    }
    else if(XILINX_DEV == type)
    {
        g_cap = &xilinx_dev;
    }
    else if(ORIN_EXT == type)
    {
        g_cap = &orin_ext_dev;
    }
    DEBUG_LOG(APP, INFO, "support type: %d \r\n", type);
    if(true == g_cap->isEnableSensor)
    {
        DEBUG_LOG(APP, INFO, "support Sensor cap init!\r\n");
        if(SENSOR_MAX_NUM >= sensor.sensorNum)
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
                this->sensor[i]->dev_sensor_init(i,sensor.type, sensor.sensorNum, \
                        sensor.bufferNum, sensor.oot, sensor.usr_data);
                sleep(1);
            }
        }
    }
    if(true == g_cap->isEnableIo)
    {
        this->io = new DEV_IO;
        this->io->dev_io_init();
    }
    if(true == g_cap->isEnableDlp)
    {
        DEBUG_LOG(APP, INFO, "support dlp cap init!\r\n");
        this->dlp = new DEV_DLP;
        this->dlp->dev_dlp_init(this->dlp_id);
    }
    if(true == g_cap->isEnableXst)
    {
        DEBUG_LOG(APP, INFO, "support xstatus init!\r\n");
        this->xst = new DEV_XSTATUS;
        this->xst->dev_xstatus_init();
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_sensor_set_cap_timeout(DEV_CAM_INDEX id, int timeout)
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                this->sensor[i]->dev_sensor_set_cap_timeout_value(timeout);
            }
        }
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}

//to do
DEV_RTN CAM_DEV::dev_register_image_callback(DEV_CAM_INDEX id, void (*image_callback)(DEV_IMG_DEF frame, void* usr_data))
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                this->sensor[i]->dev_sensor_register_image_callback(image_callback);
            }
        }
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_unregister_image_callback(DEV_CAM_INDEX id)
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                this->sensor[i]->dev_sensor_register_image_callback(nullptr);
            }
        }
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_register_error_callback(DEV_CAM_INDEX id,void (*error_callback)(DEV_ERR_DEF error))
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                this->sensor[i]->dev_sensor_register_error_callback(error_callback);
            }
        }
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_unregister_error_callback(DEV_CAM_INDEX id)
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                this->sensor[i]->dev_sensor_register_error_callback(nullptr);
            }
        }
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_register_event_callback(void (*event_callback)(DEV_FLAG_TYPE event))
{
    if(true == g_cap->isEnableIo)
    {
        this->io->dev_io_register_event_callback(event_callback);
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_unregister_event_callback()
{
    if(true == g_cap->isEnableIo)
    {
        this->io->dev_io_register_event_callback(nullptr);
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_sensor_get_version(std::string& ver)
{
    if(true == g_cap->isEnableSensor)
    {
        this->sensor[0]->dev_sensor_get_ver(ver);
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_sesnor_set_expo_period(DEV_CAM_INDEX id, SENSOR_EXPO_PERIOD_PARAM& param)
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                this->sensor[i]->dev_sensor_set_expo_period_value(param);
            }
        }
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_sensor_set_gain(DEV_CAM_INDEX id, uint16_t gain)
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                this->sensor[i]->dev_sensor_set_gain_value(gain);
            }
        }
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_sensor_set_hflip(DEV_CAM_INDEX id, bool isEnable)
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                this->sensor[i]->dev_sensor_set_hflip_value(isEnable);
            }
        }
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_sensor_set_vflip(DEV_CAM_INDEX id, bool isEnable)
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                this->sensor[i]->dev_sensor_set_vflip_value(isEnable);
            }
        }
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_sensor_set_black(DEV_CAM_INDEX id, uint16_t value)
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                this->sensor[i]->dev_sensor_set_black_value(value);
            }
        }
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}
DEV_RTN CAM_DEV::dev_sensor_get_frame_rate(DEV_CAM_INDEX id, float &rate)
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                this->sensor[i]->dev_sensor_get_max_frame_value(rate);
            }
        }
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_sensor_set_roi(DEV_CAM_INDEX id, const DEV_ROI &roi)
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                this->sensor[i]->dev_sesnor_set_roi_value(roi);
            }
        }
    }
    else
    {
        return RTN_NOT_SUPPORT;
    }
    return RTN_OKAY;
}

DEV_RTN CAM_DEV::dev_sensor_get_temp(DEV_CAM_INDEX id, float& temp)
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                return this->sensor[i]->dev_sesnor_get_temp_value(temp);
            }
        }
    }
    return RTN_NOT_SUPPORT;
}
DEV_RTN CAM_DEV::dev_sensor_reg_set(DEV_CAM_INDEX id, DEV_SENSOR_REG_PARAM &reg)
{
    if(true == g_cap->isEnableSensor)
    {
        for (uint16_t i = 0; i < this->sensor_num; i++)
        {
            if (id & (1 << i))
            {
                return this->sensor[i]->dev_sensor_reg_set_value(reg);
            }
        }
    }
    return RTN_NOT_SUPPORT;
}
DEV_RTN CAM_DEV::dev_dlp_find(int (&arr)[DLP_NUM])
{
    if(true == g_cap->isEnableDlp)
    {
        std::copy(this->dlp_id, this->dlp_id + DLP_NUM, arr);
        return RTN_OKAY;
    }
    return RTN_NOT_SUPPORT;
}
DEV_RTN CAM_DEV::dev_dlp_get_version(std::string& ver)
{
    if(true == g_cap->isEnableDlp)
    {
        this->dlp->dev_dlp_get_ver(ver);
        return RTN_OKAY;
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_dlp_get_temp(uint8_t index,float& temp)
{
    if(true == g_cap->isEnableDlp)
    {
        return this->dlp->dev_dlp_get_temp_value(index, temp);
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_dlp_set_current(uint8_t index, uint16_t current)
{
    if(true == g_cap->isEnableDlp)
    {
        return this->dlp->dev_dlp_set_current_value(index, current);
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_dlp_set_current_all(uint16_t current)
{
    if(true == g_cap->isEnableDlp)
    {
        return this->dlp->dev_dlp_set_current_all_value(current);
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_dlp_get_current(uint8_t index, uint16_t& pwm)
{
    if(true == g_cap->isEnableDlp)
    {
        return this->dlp->dev_dlp_get_pwm_value(index, pwm);
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_dlp_set_groups_info(uint8_t index, std::vector<DEV_DLP_PATTERN_GROUP_INFO> groups)
{
    int size = groups.size();
    uint8_t write_ctl = 0;
    DEV_RTN rtn;
    if (true == g_cap->isEnableDlp)
    {
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
    }
    return rtn;
}

DEV_RTN CAM_DEV::dev_dlp_get_group_info(uint8_t index, DEV_DLP_PATTERN_GROUP_INFO &param)
{
    DEV_RTN rtn;
    if(true == g_cap->isEnableDlp)
    {
        rtn = this->dlp->dev_dlp_set_group_info_value(index, 2, param); //
        rtn = this->dlp->dev_dlp_get_group_info_value(index, param);
        return rtn;
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_dlp_trigonce(uint8_t index)
{
    if(true == g_cap->isEnableDlp)
    {
        return this->dlp->dev_dlp_trigonce_new_set(index);
    }
    return RTN_NOT_SUPPORT;
}
DEV_RTN CAM_DEV::dev_dlp_get_min_expo(uint8_t index, DEV_DLP_EXP &param)
{
    if(true == g_cap->isEnableDlp)
    {
        return this->dlp->dev_dlp_get_min_expo_value(index, param);
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_dlp_set_delay_invert(uint8_t index, bool isInvert, uint32_t delayUs)
{
    if(true == g_cap->isEnableDlp)
    {
        return this->dlp->dev_dlp_set_delay_invert_value(index, isInvert, delayUs);
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_dlp_updata_falsh(uint8_t index, const char* path)
{
    if(true == g_cap->isEnableDlp)
    {
        return this->dlp->dev_dlp_update_falsh_value(index, path);
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_dlp_set_long_short_flip(uint8_t index, bool long_flip, bool short_flip)
{
    if(true == g_cap->isEnableDlp)
    {
        return this->dlp->dev_dlp_set_long_short_flip_value(index, long_flip, short_flip);
    }
    return RTN_NOT_SUPPORT;
}
DEV_RTN CAM_DEV::dev_io_get_version(std::string& ver)
{
    if(true == g_cap->isEnableIo)
    {
        return this->io->dev_io_get_ver(ver);;
    }
    return RTN_NOT_SUPPORT;
}
DEV_RTN CAM_DEV::dev_io_set_dlp_enable_usb(uint8_t isOpen, DEV_IO_DLP_INDEX index)
{
    if(true == g_cap->isEnableIo)
    {
        return this->io->dev_io_set_dlp_enable_usb_value(isOpen, index);
    }
    return RTN_NOT_SUPPORT;
}
DEV_RTN CAM_DEV::dev_io_set_status_led(DEV_LED_INDEX index, uint16_t durationMs, uint8_t times)
{
    if(true == g_cap->isEnableIo)
    {
        return this->io->dev_io_set_status_led_value(index, durationMs, times);
    }
    return RTN_NOT_SUPPORT;
}
DEV_RTN CAM_DEV::dev_io_trig_out(DEV_IO_OUT_INDEX index, uint32_t durationUs, uint8_t status)
{
    if(true == g_cap->isEnableIo)//需要在这判断是单目还是双目的相机
    {
        if(IO_SET_RES_TRIG_SENSOR != index)
        {
            return this->io->dev_io_trig_out_value(index, durationUs, status);
        }
        else
        {
            return RTN_INVALID_ARG;
        }
    }
    return RTN_NOT_SUPPORT;
}

// DEV_RTN CAM_DEV::dev_io_dlp_in(uint8_t index, uint32_t durationMs, uint8_t status)
// {
//     DEV_IO_OUT_INDEX mIndex = (DEV_IO_OUT_INDEX)(index + IO_SET_OUT_DLP1);
//     uint32_t durationUs = durationMs * 1000;
//     if(true == g_cap->isEnableIo)
//     {
//         return this->io->dev_io_trig_out_value(mIndex, durationUs, status);
//     }
//     return RTN_NOT_SUPPORT;
// }

DEV_RTN CAM_DEV::dev_io_set_rgb_status(const DEV_IO_RGB_PARAM &mParam)
{
    if(true == g_cap->isEnableIo)
    {
        return this->io->dev_io_set_rgb_status_value(mParam);
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_io_rgb_signal_trig(const DEV_IO_RGB_TRIG_SET& mParam)
{
    if(true == g_cap->isEnableIo)
    {
        return this->io->dev_io_rgb_signal_trig_value(mParam);
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_io_clear_trig_count()
{
    if(true == g_cap->isEnableIo)
    {
        return this->io->dev_io_clear_trig_count_value();
    }
    return RTN_NOT_SUPPORT;
}
DEV_RTN CAM_DEV::dev_io_set_in(SOURCE_IO index, uint8_t type, uint16_t debounceMs)
{
    if(true == g_cap->isEnableIo)
    {
        return this->io->dev_io_set_in_value(index, type, debounceMs);
    }
    return RTN_NOT_SUPPORT;
}
DEV_RTN CAM_DEV::dev_io_ctrl_ext2d_intern_ctl(uint8_t picNum, uint32_t camExpo, uint32_t expoPeriod)
{
    if(true == g_cap->isEnableIo)
    {
        return this->io->dev_io_ctrl_ext2d_intern_ctl_value(picNum, camExpo, expoPeriod);
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_io_ctrl_ext2d_exterl_ctl(uint8_t picNum, uint32_t camExpo, uint32_t outTime)
{
    if(true == g_cap->isEnableIo)
    {
        return this->io->dev_io_ctrl_ext2d_exterl_ctl_value(picNum, camExpo, outTime);
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_io_set_get_dlp_rgb_sync_states(const DEV_IO_DLP_RGB_SYNC_PARAM& syncParam)
{
    if(true == g_cap->isEnableIo)
    {
        return this->io->dev_io_set_get_dlp_rgb_sync_states_value(syncParam);
    }
    return RTN_NOT_SUPPORT;
}

DEV_RTN CAM_DEV::dev_xstatus_get_temp(DEV_XSTATUS_TYPE type, float& value)
{
    if (!g_cap->isEnableXst)
        return RTN_NOT_SUPPORT;

    return (this->xst->dev_xstatus_get_temp_value(type, value) == 0)
           ? RTN_OKAY
           : RTN_FAIL;
}
DEV_RTN CAM_DEV::dev_xstatus_get_cam_status(DEV_XSTATUS_VALUE &mParam)
{
    if (!g_cap->isEnableXst)
        return RTN_NOT_SUPPORT;
    
    int ret = this->xst->dev_xstatus_get_temp_value(CPU_TEMP_TYPE, mParam.cpuTemp);
    ret |= this->xst->dev_xstatus_get_temp_value(GPU_TEMP_TYPE, mParam.gpuTemp);
    ret |= this->xst->dev_xstatus_get_temp_value(CPU_MEMY_TYPE, mParam.cpuMem);
    ret |= this->xst->dev_xstatus_get_temp_value(GPU_MEMY_TYPE, mParam.gpuMem);
    ret |= this->xst->dev_xstatus_get_temp_value(TMP117_TEMP_TYPE, mParam.exterTemp);
    return ((0 == ret) ? RTN_OKAY : RTN_FAIL);
}
