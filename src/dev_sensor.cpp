
#include "../include/dev_sensor.h"

DEV_SENSOR::DEV_SENSOR()
{
    flag_dqthread_running.store(false, std::memory_order_release);
}

DEV_SENSOR::~DEV_SENSOR()
{
    m_fd_release();
}

DEV_RTN DEV_SENSOR::dev_sensor_init(uint8_t index, DEV_SENSOR_ATTRIBUTE att)
{
    DEV_ROI m_roi = {
        .bitMode = SENSOR_8BIT,
        .binningMode = SENSOR_ROI,
        .x = 0,
        .y = 0,
        .w = SENSOR_IMX566_ALL_PIXEL_WIDTH,
        .h = SENSOR_IMX566_ALL_PIXEL_HIGHT,
        .alignWidth = 0
    };
    this->mBufferNum = (0 == att.bufferNum)?SENSOR_BUFFER_COUNT:att.bufferNum;
    if(1 > att.sensorNum || 4 < att.sensorNum)
    {
        DEBUG_LOG(SENSOR, ERROR, "invalid sensorNum param:%d\n", att.sensorNum);
        return RTN_INVALID_ARG;
    }
    this->index = index;
    this->usr_data = att.usr_data;
    m_roi.x = att.x;
    m_roi.y = att.y;
    m_roi.bitMode = this->mSensorAttr[(uint8_t)att.type].bit;
    m_roi.w = ROI_LIMIT(att.w, (this->mSensorAttr[(uint8_t)att.type].pixel_width));
    m_roi.h = ROI_LIMIT(att.w, (this->mSensorAttr[(uint8_t)att.type].pixel_hight));
    memcpy(&this->mRoi, &m_roi, sizeof(m_roi));
    this->m_sensorType = att.type;
    this->totalNum = att.sensorNum;
    if(0 != m_fd_init(att.v4l_sub_slot))
    {
        return RTN_FAIL;
    }
    this->buf_type = m_get_sensor_mp_lane();
    this->time_out = (0 == att.oot)?10000:att.oot;
    dev_sensor_set_cap_timeout_value(this->time_out);
    //自定义测是写入测试图
    this->mTestMode = att.testPicMode;
    if (0 != this->mTestMode)
    {
        dev_sensor_set_test_pic_value(this->mTestMode);
    }
    //默认配置下roi的参数
    dev_sesnor_set_roi_value(this->mRoi);
    return RTN_OKAY;
}

DEV_RTN DEV_SENSOR::dev_sensor_set_cap_timeout_value(int timeout)
{
    struct v4l2_control control;
    int ret = 0;
    memset(&control, 0, sizeof(control));
    control.id = TEGRA_CAMERA_CID_VI_CAPTURE_TIMEOUT;
    control.value = timeout; //10秒时间
    ret = ioctl(video_fd, VIDIOC_S_CTRL, &control);
    if (0 != ret)
    {
        DEBUG_LOG(SENSOR, ERROR,"VIDIOC_S_CTRL set error \r\n");
        return RTN_FAIL;
    }
    else
    {
        DEBUG_LOG(SENSOR, INFO,"VIDIOC_S_CTRL set value:%d success \r\n", control.value);
    }
    return RTN_OKAY;
}
void DEV_SENSOR::dev_sensor_get_ver(std::string& ver)
{
    DRIVER_VERSION sensorVer;
    ioctl(this->v4l2_fd, V4L2_CID_GET_VERSION, &sensorVer);
    DEBUG_LOG(SENSOR, INFO, "len:%d ver:%s\r\n",sensorVer.len, sensorVer.ver);
    uint32_t real_len = std::min(sensorVer.len,static_cast<uint32_t>(DRIVER_VER_MAX_LEN));
    ver.assign(sensorVer.ver, real_len);
    return;
}
DEV_RTN DEV_SENSOR::dev_sensor_set_expo_period_value(SENSOR_EXPO_PERIOD_PARAM& param)
{
    int ret = 0;
    SENSOR_SOC_EXPO_SET_PARAM mParam;
    mParam.type = param.type;
    mParam.expo = param.expo;
    mParam.period = param.period;
    mParam.minPeriod = 0;
    ret = ioctl(this->v4l2_fd, CAM_SET_EXPO_PERIOD_PARAM, &mParam);
    if (0 == ret)
    {
        DEBUG_LOG(SENSOR, INFO, "set sensor expo_period:%ld  %ldok!\r\n", param.period, param.expo);
        param.maxFps = 1000000.0f/mParam.minPeriod;
        param.minPeriod = mParam.minPeriod;
    }
    else
    {
        DEBUG_LOG(SENSOR, ERROR, "set failed ret: %d\r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}
DEV_RTN DEV_SENSOR::dev_sensor_set_gain_value(uint16_t gain)
{
    int ret = 0;
    int64_t val = 0;
    if (SENSOR_IMX566_MAX_GAIN < gain)
    {
        DEBUG_LOG(SENSOR, WARN, "valid param:%d\r\n", gain);
        return RTN_INVALID_ARG;
    }
    val = gain;
    ret = ioctl(this->v4l2_fd, CAM_SET_GAIN, &val);
    if (0 == ret)
    {
        this->gain = gain;
        DEBUG_LOG(SENSOR, INFO, "set sensor gain:%d ok!\r\n", gain);
    }
    else
    {
        DEBUG_LOG(SENSOR, ERROR, "set failed ret: %d\r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}
DEV_RTN DEV_SENSOR::dev_sensor_set_hflip_value(bool value)
{
    int ret = 0;
    int64_t val = 0;
    val = value;
    ret = ioctl(this->v4l2_fd, CAM_SET_HFLIP, &val);
    if (0 == ret)
    {
        this->m_hflip = value;
        DEBUG_LOG(SENSOR, INFO, "set sensor value:%d ok!\r\n", value);
    }
    else
    {
        DEBUG_LOG(SENSOR, ERROR, "set failed ret: %d\r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}
DEV_RTN DEV_SENSOR::dev_sensor_set_vflip_value(bool value)
{
    int ret = 0;
    int64_t val = 0;
    val = value;
    ret = ioctl(this->v4l2_fd, CAM_SET_VFLIP, &val);
    if (0 == ret)
    {
        this->m_vflip = value;
        DEBUG_LOG(SENSOR, INFO, "set sensor value:%d ok!\r\n", value);
    }
    else
    {
        DEBUG_LOG(SENSOR, ERROR, "set failed ret: %d\r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}

DEV_RTN DEV_SENSOR::dev_sensor_set_black_value(uint16_t black)
{
    int ret = 0;
    int64_t val = 0;
    if (SENSOR_IMX566_MAX_BLACK < black)
    {
        DEBUG_LOG(SENSOR, WARN, "valid param:%d\r\n", black);
        return RTN_INVALID_ARG;
    }
    val = black;
    ret = ioctl(this->v4l2_fd, CAM_SET_BLACK_LEVEL, &val);
    if (0 == ret)
    {
        this->black = black;
        DEBUG_LOG(SENSOR, INFO, "set sensor value:%d ok!\r\n", black);
    }
    else
    {
        DEBUG_LOG(SENSOR, ERROR, "set failed ret: %d\r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}

DEV_RTN DEV_SENSOR::dev_sensor_get_max_frame_value(float& max_frame)
{
    SENSOR_ATTRIBUTE mValue;
    int ret = 0;
    memset(&mValue, 0, sizeof(mValue));
    ret = ioctl(this->v4l2_fd, CAM_GET_MIN_EXPOSE, &mValue);
    if (0 == ret)
    {
        max_frame = mValue.max_frame/10.0f;
        DEBUG_LOG(SENSOR, INFO, "exposureUnit:%d  max_frame:%d min_trigger_fall:%d min_trigger_rise:%d\r\n", \
                mValue.exposureUnit, mValue.max_frame, mValue.min_trigger_fall, mValue.min_trigger_rise);
    }
    else
    {
        DEBUG_LOG(SENSOR, ERROR, "get failed ret: %d\r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}
DEV_RTN DEV_SENSOR::dev_sesnor_get_temp_value(float &temp)
{
    int value = 0,ret = 0;
    ret = ioctl(this->v4l2_fd, V4L2_CID_GET_TEMPERATURE_VAL, &value);
    if (0 == ret)
    {
        temp = value/100.0f;//精确到两位小数点
        DEBUG_LOG(SENSOR, INFO, "temperature:%d \r\n", value);
    }
    else
    {
        DEBUG_LOG(SENSOR, ERROR, "get failed ret: %d\r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}

DEV_RTN DEV_SENSOR::dev_sensor_reg_set_value(DEV_SENSOR_REG_PARAM &reg)
{
    int ret = ioctl(this->v4l2_fd, V4L2_CID_REGIST_IOCTL, &reg);
    if (0 == ret)
    {
        DEBUG_LOG(SENSOR, INFO, "opt:%d addr:0x%x value:%d 0x%x\r\n", reg.opt, reg.addr, reg.value, reg.value);
    }
    else
    {
        DEBUG_LOG(SENSOR, ERROR, "get failed ret: %d\r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}

void DEV_SENSOR::m_sensor_aligned_set(IN DEV_ROI in, OUT SENSOR_FORMAT_ROI_PARAM& out, SENSOR_SOC_WIDTH& alignWidth)
{
    const int MIN_H = SENSOR_IMX566_MIN_H, MIN_W = SENSOR_IMX566_MIN_W;
    int MAX_H = this->mSensorAttr[(uint8_t)this->m_sensorType].pixel_hight, MAX_W = this->mSensorAttr[(uint8_t)this->m_sensorType].pixel_width;
    int h,w;
    memset(&out, 0, sizeof(out));
    DEBUG_LOG(SENSOR, INFO, "input x:%d y:%d w:%d h:%d bitMode:%d binningMode:%d \r\n", \
                    in.x, in.y, in.w, in.h, in.bitMode, in.binningMode);
    out.binningMode = in.binningMode;
    out.bitMode = in.bitMode;
    if(SENSOR_ROI != in.binningMode)
    {
        MAX_H /= 2;
        MAX_W /= 2;
    }
    out.start_x = (in.x > 0 ? in.x : 0) & ~0x7;
    out.start_y = (in.y > 0 ? in.y : 0) & ~0x7;

    h = (in.h < MIN_H ?MIN_H:in.h);
    h = ((h > MAX_H) ? MAX_H: h);
    h &= ~0x7;

    w = (in.w < MIN_W ? MIN_W : in.w);
    w = ((w >= MAX_W) ? MAX_W: w);
    w &= ~0x7;

    out.height = h;
    out.width = w;

    if(((out.start_x + out.width) > MAX_W) || ((out.start_y + out.height) > MAX_H))
    {
        out.start_x = MAX_W - out.width;
        out.start_x &= ~0x7;
        out.start_y = MAX_H - out.height;
        out.start_y &= ~0x7;
        DEBUG_LOG(SENSOR, ERROR, "ROI x/y+w/h exceed max limit, auto adjust to x=%d, y=%d\n", out.start_x, out.start_y);
    }
    alignWidth.roi_width = w;
    alignWidth.sensor_width = ((0 == in.bitMode)?MAX_W:MAX_W*2);
    alignWidth.soc_width = ALIGN_UP(((0 == in.bitMode)?alignWidth.roi_width:alignWidth.roi_width*2), WIDTH_ALIGN);
    DEBUG_LOG(SENSOR, DEBUG, "roi_width:%d %d %d\r\n", alignWidth.roi_width, alignWidth.sensor_width, alignWidth.soc_width);
    DEBUG_LOG(SENSOR, DEBUG, "roi_hight:%d \r\n", out.height);
    return;
}
DEV_RTN DEV_SENSOR::m_sensor_q_buffer(struct v4l2_buffer* v4l2_buf, \
        const enum v4l2_buf_type buf_type, const enum v4l2_memory memory_type)
{
    int ret = 0;
    v4l2_buf->type = buf_type;
    v4l2_buf->memory = memory_type;
    ret = ioctl(video_fd, VIDIOC_QBUF, v4l2_buf);
    if (ret)
    {
        DEBUG_LOG(SENSOR, ERROR, "VIDIOC_QBUF error ret:%d \n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}
DEV_RTN DEV_SENSOR::m_sensor_buffers_mmap(bool isRequest)
{
    int ret;
    struct v4l2_requestbuffers reqbuf;
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.type = this->buf_type;
    reqbuf.memory = this->memory_type;
    if (true == isRequest)
    {
        if (m_flag_mmaped)
        {
            DEBUG_LOG(SENSOR, WARN, "Buffers already mmaped, skip request.\n");
            return RTN_OKAY;
        }
        reqbuf.count = mBufferNum;
        ret = ioctl(this->video_fd, VIDIOC_REQBUFS, &reqbuf);
        if (ret)
        {
            DEBUG_LOG(SENSOR, ERROR, "VIDIOC_REQBUFS error %d: %s \r\n", errno, strerror(errno));
            return RTN_FAIL;
        }
        else
        {
            DEBUG_LOG(SENSOR, INFO, "VIDIOC_REQBUFS:req %d get %d buffers.\r\n", mBufferNum, reqbuf.count);
        }
        this->buffers = new BUFFERS_CAPTURE_DEF[reqbuf.count];
        if (NULL == this->buffers)
        {
            DEBUG_LOG(SENSOR, ERROR, "malloc buffers error\n");
            return RTN_FAIL;
        }
        memset(this->buffers, 0, reqbuf.count * sizeof(BUFFERS_CAPTURE_DEF));
        for (uint8_t i = 0; i < reqbuf.count; i++)
        {
            struct v4l2_buffer buf;
            struct v4l2_plane planes[MAX_PLANES];
            memset(&buf, 0, sizeof(buf));
            memset(planes, 0, sizeof(planes));
            buf.type = this->buf_type;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            if(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == this->buf_type)
            {
                buf.m.planes = planes;
                buf.length = MAX_PLANES;
            }
            ret = ioctl(this->video_fd, VIDIOC_QUERYBUF, &buf);
            if (ret)
            {
                DEBUG_LOG(SENSOR, ERROR, "VIDIOC_QUERYBUF error %d: %s\n", errno, strerror(errno));
                return RTN_FAIL;
            }
            buffers[i].idx = i;
            if(V4L2_BUF_TYPE_VIDEO_CAPTURE == this->buf_type)
            {
                buffers[i].length[0] = buf.length;
                buffers[i].mem[0] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, this->video_fd, buf.m.offset);
                if (MAP_FAILED == buffers[i].mem[0])
                {
                    DEBUG_LOG(SENSOR, ERROR, "mmap buffer %d error %d: %s\n", i, errno, strerror(errno));
                    return RTN_FAIL;
                }
                DEBUG_LOG(SENSOR, DEBUG, "mmap buffer %d: size=%u addr=%p offset=%u\n", i, buf.length, buffers[i].mem[0], buf.m.offset);
            }
            else if(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == this->buf_type)
            {
                for(uint8_t p = 0; p < buf.length; p++)
                {
                    this->buffers[i].length[p] = buf.m.planes[p].length;
                    this->buffers[i].mem[p] = mmap(NULL,
                                        buf.m.planes[p].length,
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED,
                                        this->video_fd,
                                        buf.m.planes[p].m.mem_offset);
                    if(MAP_FAILED == this->buffers[i].mem[p])
                    {
                        DEBUG_LOG(SENSOR, ERROR, "mmap buffers[%d].length[%d]:%d failed!\r\n", i, p, this->buffers[i].length[p]);
                        return RTN_FAIL;
                    }
                    else
                    {
                        DEBUG_LOG(SENSOR, INFO, "mmap buffers[%d].length[%d]:%d ok!\r\n", i, p, this->buffers[i].length[p]);
                    }
                }
            }
            buffers[i].count = reqbuf.count;
            ret = ioctl(this->video_fd, VIDIOC_QBUF, &buf);
            if (ret)
            {
                DEBUG_LOG(SENSOR, INFO,"VIDIOC_QBUF:%d failed, ret:%d \r\n", i, ret);
                return RTN_FAIL;
            }
            else
            {
                DEBUG_LOG(SENSOR, INFO,"VIDIOC_QBUF:%d ok\r\n", i);
            }
        }
        this->sensor_buffer_length = this->buffers[0].length[0];
        m_flag_mmaped = true;
        current_frame.sensor_index = index;
        current_frame.width = this->m_RoiParam.width;
        current_frame.height = this->m_RoiParam.height;
        current_frame.all_bytes = this->sensor_buffer_length; //帧数据大小
        if(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == this->buf_type)
        {
            current_frame.emb_bytes = SENSOR_IMX566_EMB_BUFF_SIZE;
        }
        else
        {
            current_frame.emb_bytes = 0;
        }
        current_frame.stride = this->m_WidthInfo.soc_width; //实际
        current_frame.bit_mode = this->m_RoiParam.bitMode;
        current_frame.data = nullptr;
        current_frame.embData = nullptr;
        return RTN_OKAY;
    }
    else
    {
        reqbuf.count = 0;
        ret = ioctl(this->video_fd, VIDIOC_REQBUFS, &reqbuf);
        if (ret)
        {
            DEBUG_LOG(SENSOR, ERROR, "index:%d VIDIOC_REQBUFS error %d: %s\n", index, errno, strerror(errno));
            return RTN_FAIL;
        }
        else
        {
            DEBUG_LOG(SENSOR, DEBUG ,"index:%d VIDIOC_REQBUFS:req %d get %d buffers.\n", index, 0, reqbuf.count);
        }
        if (m_flag_mmaped)
        {
            for (uint8_t i = 0; i < mBufferNum; i++)
            {
                if(V4L2_BUF_TYPE_VIDEO_CAPTURE == this->buf_type)
                {
                    if(this->buffers[i].mem[0] && (this->buffers[i].length[0] > 0))
                    {
                        ret = munmap(buffers[i].mem[0], this->buffers[i].length[0]);
                        if(0!= ret)
                        {
                            DEBUG_LOG(SENSOR, ERROR,"release mmap buffer failed ret:%d \r\n", ret);
                            goto ERR_EXIT;
                        }
                        this->buffers[i].mem[0] = nullptr;
                        this->buffers[i].length[0] = 0;
                    }
                }
                else if(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == this->buf_type)
                {
                    for(uint8_t j = 0; j < this->buffers[i].count; j++)
                    {
                        ret = munmap(this->buffers[i].mem[j], buffers[i].length[j]);
                        if(0!= ret)
                        {
                            DEBUG_LOG(SENSOR, ERROR,"release mmap buffer[%d].mem[%d] failed ret:%d \r\n", i,j,ret);
                            goto ERR_EXIT;
                        }
                        this->buffers[i].mem[j] = nullptr;
                        buffers[i].length[j] = 0;
                    }
                }
            }
            if(nullptr != this->buffers)
            {
                delete[] this->buffers;
                this->buffers = nullptr;
                m_flag_mmaped = false;
            }
        }
        else
        {
            DEBUG_LOG(SENSOR, WARN,"Buffers not mmaped or haved release, skip release.\n");
        }
        return RTN_OKAY;
    }
    return RTN_OKAY;
ERR_EXIT:
    if(nullptr != this->buffers)
    {
        delete this->buffers;
        this->buffers = nullptr;
        m_flag_mmaped = false;
    }
    return RTN_FAIL;
}
DEV_RTN DEV_SENSOR::m_sensor_stream_operate(bool isOpen)
{
    int ret = 0;
    enum v4l2_buf_type type = this->buf_type;
    DEBUG_LOG(SENSOR, INFO, "isOpen:%d \r\n", isOpen);
    if (true == isOpen)
    {
        if (true == flag_streamOn)
        {
            DEBUG_LOG(SENSOR, WARN, "Stream already ON, skip STREAMON.\n");
            return RTN_OKAY;
        }
        ret = ioctl(this->video_fd, VIDIOC_STREAMON, &type);
        if (ret)
        {
            DEBUG_LOG(SENSOR, ERROR, "VIDIOC_STREAMON failed ret:%d errno:%d %s\n", ret, errno, strerror(errno));
            return RTN_FAIL;
        }
        flag_streamOn = true;
    }
    else
    {
        if (false == flag_streamOn)
        {
            DEBUG_LOG(SENSOR, WARN, "Stream already OFF, skip STREAMOFF.\n");
            return RTN_OKAY;
        }
        DEBUG_LOG(SENSOR, INFO, "VIDIOC_STREAMOFF type:0x%x\n", type);
        ret = ioctl(this->video_fd, VIDIOC_STREAMOFF, &type);
        if (ret)
        {
            DEBUG_LOG(SENSOR, ERROR, "VIDIOC_STREAMOFF failed ret:%d errno:%d %s\n", ret, errno, strerror(errno));
            return RTN_FAIL;
        }
        flag_streamOn = false;
    }
    return RTN_OKAY;
}
bool DEV_SENSOR::m_is_all_zero(const void *data, size_t size)
{
    // const uint64_t *p64 = (const uint64_t *)data;
    // size_t n64 = size / sizeof(uint64_t);
    // for (size_t i = 0; i < n64; ++i)
    // {
    //     if (p64[i] != 0)
    //         return 0;
    // }

    // const unsigned char *p8 = (const unsigned char *)(p64 + n64);
    // size_t rem = size % sizeof(uint64_t);
    // for (size_t i = 0; i < rem; ++i)
    // {
    //     if (p8[i] != 0)
    //         return 0;
    // }
    // return 1;
     // 对齐检查
    uintptr_t addr = reinterpret_cast<uintptr_t>(data);
    size_t n64 = 0;

    if (addr % alignof(uint64_t) == 0) {
        // 地址对齐，可以按 uint64_t 扫描
        const uint64_t *p64 = reinterpret_cast<const uint64_t *>(data);
        n64 = size / sizeof(uint64_t);
        for (size_t i = 0; i < n64; ++i) {
            if (p64[i] != 0)
                return false;
        }
    }

    // 处理剩余字节或未对齐部分
    const unsigned char *p8 = reinterpret_cast<const unsigned char *>(data) + n64 * sizeof(uint64_t);
    size_t rem = size % sizeof(uint64_t);
    for (size_t i = 0; i < rem; ++i) {
        if (p8[i] != 0)
            return false;
    }

    return true;
}

int DEV_SENSOR::m_fd_init(uint8_t solt)
{
    char dev_name[32];
    snprintf(dev_name, sizeof(dev_name),
             SENSOR_DEV, this->index);
    this->video_fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
    // 按平台 media-ctl 拓扑，sensor subdev 与 video 节点存在固定偏移（常见为 +2）
    // 例如: video0/video1 对应 v4l-subdev2/v4l-subdev3。
    snprintf(dev_name, sizeof(dev_name),
             SENSOR_SUB_DEV, this->index + solt);
    this->v4l2_fd = open(dev_name, O_RDWR, 0);
    if (this->video_fd < 0 || this->v4l2_fd < 0)
    {
        DEBUG_LOG(SENSOR, ERROR, "open %s error\n", dev_name);
        return -1;
    }
    return 0;
}

int DEV_SENSOR::m_fd_release()
{
    dev_sensor_stream_set(false);
    if (-1 != this->video_fd)
    {
        close(this->video_fd);
        this->video_fd = -1;
    }
    if (-1 != v4l2_fd)
    {
        close(v4l2_fd);
        v4l2_fd = -1;
    }
    sync();
    return 0;
}


DEV_RTN DEV_SENSOR::dev_sensor_set_test_pic_value(uint8_t testMode)
{
    // struct v4l2_control ctrl;
    int64_t val = 0;
    int ret = 0;
    if(3 < testMode)
    {
        DEBUG_LOG(SENSOR, ERROR, "testMode value invalid:%d\n", testMode);
        return RTN_FAIL;
    }
    DEBUG_LOG(SENSOR, INFO, "set testMode:%d\n", testMode);
    // memset(&ctrl, 0, sizeof(ctrl));
    val = testMode;
    // ctrl.id = V4L2_CID_CUSTOM_TEST;
    // ctrl.value = testMode; //
    // ret = ioctl(this->video_fd, VIDIOC_S_CTRL, &ctrl); //修改sensor的配置
    ret = ioctl(this->v4l2_fd, CAM_SET_CUSTOM_TEST, &val);
    if(0 != ret)
    {
        DEBUG_LOG(SENSOR, ERROR, "set testMode %d error ret %d \r\n", testMode, ret);
        return RTN_FAIL;
    }   
    return RTN_OKAY;
}

DEV_RTN DEV_SENSOR::dev_sensor_set_power_on_off_value(bool onOff)
{
#if 0
    // int64_t val = (onOff?1:0);
    // int ret = 0;
    DEBUG_LOG(SENSOR, INFO, "set CAM_SET_SENSOR_POWER_STATUS:%d\n", val);
    // ret = ioctl(this->v4l2_fd, CAM_SET_SENSOR_POWER_STATUS, &val);
    // if(0 != ret)
    // {
    //     DEBUG_LOG(SENSOR, ERROR, "set CAM_SET_SENSOR_POWER_STATUS %d error ret %d \r\n", val, ret);
    //     return RTN_FAIL;
    // }
#endif
    return RTN_OKAY;
}


void DEV_SENSOR::m_saveBufferToFile(const char* buffer, int size, const std::string& filePath)
{
     std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) return;
    file.write(buffer, size);
    file.close();
}
bool DEV_SENSOR::m_sensor_callback(const uint8_t buff_index, uint32_t sequence)
{
    if (this->buffers[buff_index].mem[0] == nullptr)
    {
        DEBUG_LOG(SENSOR, ERROR, "index:%d buffers[%u].mem[0] is NULL\n", index, buff_index);
        return false;
    }
    current_frame.data = (unsigned char *)this->buffers[buff_index].mem[0];
    current_frame.embData = (unsigned char *)this->buffers[buff_index].mem[1];
    current_frame.sequence = sequence;
    g_pic_index++;
#if USER_SENSOR_TEST
    std::snprintf(buffer, sizeof(buffer),
                  "./pic/index_%d_frame_index_%d_w_%d_h_%d_s_%d_length_%d.raw",
                  index, g_pic_index,
                  current_frame.width,
                  current_frame.height,
                  current_frame.stride,
                  current_frame.all_bytes);
    std::string current_frame_string(buffer);
    std::cout<<current_frame_string<<std::endl;
    this->dev_sensor_set_test_pic_value(g_pic_index%3 + 1);
    this->m_saveBufferToFile((char *)current_frame.data, \
           current_frame.all_bytes, current_frame_string);
#endif
    if (nullptr != image_notifiers) {
        image_notifiers(current_frame, usr_data);
    }
    return true;
}

void DEV_SENSOR::m_sensor_threads(uint8_t index)
{
    // 直接使用this里面内容
    int ret = 0;
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[FMT_NUM_PLANES];
    fd_set fds;
    int dq_count = 0;
    DEBUG_LOG(SENSOR, INFO, "index:%d threads start! \r\n", index);
    while (flag_dqthread_running.load(std::memory_order_relaxed)) //仅原子性，无顺序保证
    {
        FD_ZERO(&fds);
        FD_SET(this->video_fd, &fds);
        ret = select(this->video_fd + 1, &fds, NULL, NULL, NULL); // 修改为无线等待模式
        if (ret < 0) {
            if (errno == EINTR) continue;
            // DEBUG_LOG(SENSOR, ERROR, "select error %d\n", errno);
            usleep(5000);
            continue;
        }
        dq_count = 0;
        while (flag_dqthread_running.load(std::memory_order_relaxed)) {
            memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
            v4l2_buf.type = buf_type;
            v4l2_buf.memory = memory_type;
            if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == buf_type) {
                v4l2_buf.m.planes = planes;
                v4l2_buf.length = MAX_PLANES;
            }
            ret = ioctl(this->video_fd, VIDIOC_DQBUF, &v4l2_buf);
            if (ret < 0) {
                if (errno == EAGAIN || errno == EINVAL) 
                    break;                    // 没有更多 buffer
            }
            if(0x2001 == v4l2_buf.flags)
            {
                // static auto old = std::chrono::steady_clock::now();
                // auto  start = std::chrono::steady_clock::now();
                // int64_t   dur = std::chrono::duration_cast<std::chrono::microseconds>(start - old).count();
                // printf("pld:%lld us \r\n", (long long)dur);
                // old = start;
                this->m_sensor_callback(v4l2_buf.index, v4l2_buf.sequence);
                // auto  end = std::chrono::steady_clock::now();
                // int64_t   dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                // printf("dq_count:%d sequence:%u flags:%d dur:%ld us \r\n", dq_count, v4l2_buf.sequence, v4l2_buf.flags, dur);
            }
            dq_count++;
            if (flag_dqthread_running.load(std::memory_order_relaxed)) {
                ret = ioctl(this->video_fd, VIDIOC_QBUF, &v4l2_buf);
                // if( 0 != ret)
                    // printf("711:ret = %d \r\n", ret);
            }
        }
    }
}

DEV_RTN DEV_SENSOR::dev_sensor_stream_set(bool isOpen)
{
    DEV_RTN ret = RTN_OKAY;
    if (true == isOpen)
    {
        if (true == flag_dqthread_running.load(std::memory_order_relaxed))
        {
            DEBUG_LOG(SENSOR, WARN, "DQ thread already running, skip start.\n");
            return RTN_OKAY;
        }
        ret = m_sensor_buffers_mmap(true);
        if (RTN_OKAY != ret)
        {
            DEBUG_LOG(SENSOR, ERROR,"set index:%d error! \r\n", index);
            goto ERR_EXIT;
        }
        ret = m_sensor_stream_operate(true);
        if (RTN_OKAY != ret)
        {
            DEBUG_LOG(SENSOR, ERROR,"set index:%d error! \r\n", index);
            goto ERR_EXIT;
        }
        if (false == flag_dqthread_running.load(std::memory_order_relaxed))
        {
            flag_dqthread_running.store(true, std::memory_order_release);
            // this->m_threads = std::thread([this]() {this->m_sensor_threads(index);});
            this->m_threads = std::thread([this](){
                                cpu_set_t cpuset;
                                CPU_ZERO(&cpuset);
                                CPU_SET(7, &cpuset);
                                pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
                                this->m_sensor_threads(index);});
        }
    }
    else
    {
        if(false == flag_dqthread_running.load(std::memory_order_relaxed))
        {
            DEBUG_LOG(SENSOR, WARN, "DQ thread not running, skip stop.\n");
            return RTN_OKAY;
        }
        
        flag_dqthread_running.store(false, std::memory_order_release);
        
        if (this->m_threads.joinable()) {
            this->m_threads.join();            // 等待线程退出
        }
        ret = m_sensor_stream_operate(false);
        if (RTN_OKAY != ret)
            DEBUG_LOG(SENSOR, ERROR,"set index:%d error! \r\n", index);

        ret = m_sensor_buffers_mmap(false);
        if (RTN_OKAY != ret)
        {
            DEBUG_LOG(SENSOR, ERROR,"set index:%d error! \r\n", index);
        }
    }
    return ret;
ERR_EXIT:
    m_sensor_stream_operate(false);
    m_sensor_buffers_mmap(false);
    return ret;
}

DEV_RTN DEV_SENSOR::m_sensor_set_stride(uint32_t value)//配置nx的对齐宽度数据
{
    struct v4l2_ext_control ctrl = {0};
    struct v4l2_ext_controls ctrls = {0};
    if(value == m_stride_value)
    {
        return RTN_OKAY;
    }
    m_stride_value = value;
    ctrl.id = TEGRA_CAMERA_CID_VI_PREFERRED_STRIDE;
    ctrl.value = m_stride_value;
    ctrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ctrls.count = 1;
    ctrls.controls = &ctrl;
    if (ioctl(this->video_fd, VIDIOC_S_EXT_CTRLS, &ctrls) < 0)
    {
        DEBUG_LOG(SENSOR, ERROR, "index:%d set errno:%d %s\n", index, errno, strerror(errno));
        return RTN_FAIL;
    }
    else
    {
        DEBUG_LOG(SENSOR, INFO,"set index:%d stride:%d \r\n", index, value);
    }
    return RTN_OKAY;
}

v4l2_buf_type DEV_SENSOR::m_get_sensor_mp_lane()
{
    struct v4l2_capability cap;
    ioctl(this->video_fd, VIDIOC_QUERYCAP, &cap);
    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
    {
        return V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }
    return V4L2_BUF_TYPE_VIDEO_CAPTURE;
}

DEV_RTN DEV_SENSOR::m_sensor_roi_set(const SENSOR_FORMAT_ROI_PARAM &roi,SENSOR_SOC_WIDTH alignWidth)
{
    int ret;
    struct v4l2_format mFormat;
    struct v4l2_pix_format_mplane *pix_mp;
    this->dev_sensor_stream_set(false);
    memset(&mFormat, 0, sizeof(mFormat));
    mFormat.type = this->buf_type;
    m_sensor_set_stride(alignWidth.soc_width);
    ret = ioctl(this->video_fd, VIDIOC_G_FMT, &mFormat);
    if (ret < 0)
    {
        DEBUG_LOG(SENSOR, ERROR, "VIDIOC_G_FMT failed ret:%d errno:%d %s\n", ret, errno, strerror(errno));
        return RTN_FAIL;
    }
    else
    {
        DEBUG_LOG(SENSOR, INFO, "Before change VIDIOC_G_FMT ok ret:%d\n", ret);
    }
    ret = ioctl(this->v4l2_fd, CAM_SET_ROI_FORMAT, &roi);
    if (ret < 0)
    {
        DEBUG_LOG(SENSOR, ERROR, "CAM_SET_ROI_FORMAT failed ret:%d errno:%d %s\n", ret, errno, strerror(errno));
        return RTN_FAIL;
    }
    // m_sensor_set_stride(alignWidth.soc_width);
    if(V4L2_BUF_TYPE_VIDEO_CAPTURE == this->buf_type)
    {
        mFormat.fmt.pix.width = alignWidth.roi_width;
        DEBUG_LOG(SENSOR, WARN, "roi_width:%d sensor_width:%d soc_width:%d\r\n", \
                            alignWidth.roi_width, alignWidth.sensor_width, alignWidth.soc_width);
        mFormat.fmt.pix.height = roi.height;
        mFormat.fmt.pix.pixelformat = bitModePixfmt[roi.bitMode];
        mFormat.fmt.pix.bytesperline = alignWidth.soc_width;
        DEBUG_LOG(SENSOR, INFO, "bit:%d width:%d height:%d soc_width:%d\r\n", roi.bitMode, mFormat.fmt.pix.width, mFormat.fmt.pix.height, alignWidth.soc_width);
    }
    else
    {
        pix_mp = &mFormat.fmt.pix_mp;
        pix_mp->width = alignWidth.roi_width;
        pix_mp->height = roi.height;
        pix_mp->pixelformat = bitModePixfmt[roi.bitMode];
        pix_mp->field = V4L2_FIELD_NONE;
        // for (int i = 0; i < mFormat.fmt.pix_mp.num_planes; i++)
        // {
        //     printf("plane[%d]: sizeimage=%d bytesperline=%d\n",
        //            i,
        //            mFormat.fmt.pix_mp.plane_fmt[i].sizeimage,
        //            mFormat.fmt.pix_mp.plane_fmt[i].bytesperline);
        // }
        pix_mp->plane_fmt[1].sizeimage = SENSOR_IMX566_EMB_BUFF_SIZE;
    }
    ret = ioctl(this->video_fd, VIDIOC_S_FMT, &mFormat);
    if (ret < 0)
    {
        DEBUG_LOG(SENSOR, ERROR, "index:%d VIDIOC_S_FMT failed ret:%d errno:%d %s\n", index, ret, errno, strerror(errno));
    }
    else
    {
        DEBUG_LOG(SENSOR, INFO, "VIDIOC_S_FMT ok ret:%d\n", ret);
    }
    this->dev_sensor_stream_set(true);
    return RTN_OKAY;
}
DEV_RTN DEV_SENSOR::dev_sesnor_set_roi_value(const DEV_ROI &roi) // DEV_ROI会对传参进行对齐判断
{
    // memcpy(&this->mRoi, &roi, sizeof(DEV_ROI));
    m_sensor_aligned_set(roi, this->m_RoiParam, this->m_WidthInfo);
    if (RTN_OKAY != m_sensor_roi_set(this->m_RoiParam, this->m_WidthInfo))
        return RTN_FAIL;
    return RTN_OKAY;
}

DEV_RTN DEV_SENSOR::dev_sensor_register_image_callback(ImageNotifier func)
{
    this->image_notifiers = func;
    return RTN_OKAY;
}
DEV_RTN DEV_SENSOR::dev_sensor_register_error_callback(SensorErrorNotifier func)
{
    this->error_notifiers = func;
    return RTN_OKAY;
}