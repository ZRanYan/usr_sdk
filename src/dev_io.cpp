#include "../include/dev_io.h"

static DEV_IO *m_io = nullptr;

DEV_IO::DEV_IO()
{
    m_io = this;
}

DEV_IO::~DEV_IO()
{
    if(-1 != this->m_io_fd)
    {
        close(this->m_io_fd);
        this->m_io_fd = -1;
    }
}

static void m_ioSignalHandlerTrig(int sig)
{
    m_io->triggerPin = ioctl(m_io->m_io_fd, IO_GET_TRIGGER_PIN, &m_io->triggerPin);
    if(nullptr != m_io->event_external_notifier)
    {
        m_io->event_external_notifier((DEV_FLAG_TYPE)m_io->triggerPin);
    }
}

DEV_RTN DEV_IO::dev_io_register_event_callback(void (*io_callback)(DEV_FLAG_TYPE))
{
    this->event_external_notifier = io_callback;
    return RTN_OKAY;
}

DEV_RTN DEV_IO::dev_io_init()
{
    this->m_io_fd = open(IO_DEV_NAME, O_RDWR);
    if (this->m_io_fd < 0)
    {
        DEBUG_LOG(APP, ERROR, "open %s error \r\n", IO_DEV_NAME);
        return RTN_FAIL;
    }
    else
    {
        DEBUG_LOG(APP, INFO, "open %s ok \r\n", IO_DEV_NAME);
    }
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGIO);
    pthread_sigmask(SIG_UNBLOCK, &mask, NULL); //
    signal(SIGIO, m_ioSignalHandlerTrig);
    fcntl(this->m_io_fd, F_SETOWN, getpid());
    int flag = fcntl(this->m_io_fd, F_GETFL);
    fcntl(this->m_io_fd, F_SETFL, flag | FASYNC);
    this->dev_io_set_status_led_value(LED_2, 0, 0);
    return RTN_OKAY;
}
DEV_RTN DEV_IO::dev_io_get_ver(std::string& ver)
{
    DRIVER_VERSION ioVer;
    ioctl(this->m_io_fd, IO_GET_VERSION, &ioVer);
    DEBUG_LOG(APP, INFO, "len:%d ver:%s\r\n",ioVer.len, ioVer.ver);
    uint32_t real_len = std::min(ioVer.len,static_cast<uint32_t>(DRIVER_VER_MAX_LEN));
    ver.assign(ioVer.ver, real_len);
    return RTN_OKAY;
}

DEV_RTN DEV_IO::dev_io_set_dlp_enable_usb_value(uint8_t enable, DEV_IO_DLP_INDEX index)
{
    int ret = 0;
    uint8_t value = (uint8_t)index;
    if( 1 < enable)
    {
        DEBUG_LOG(APP, ERROR, "input %d invalid arg!\r\n", enable);
        return RTN_INVALID_ARG;
    }
    ret = ioctl(this->m_io_fd, IO_SET_USB_ENABLE_DLP_STATUS, &enable);
    ret |= ioctl(this->m_io_fd, IO_SET_DLP_STATUS, &value);
    if(0 != ret)
    {
        DEBUG_LOG(APP, ERROR, "set:%d ret: %d error!\r\n", index, ret);
        return RTN_FAIL;
    }
    else
    {
        DEBUG_LOG(APP, INFO, "set:%d index:0x%x\r\n", enable, value);
    }
    return RTN_OKAY;
}
DEV_RTN DEV_IO::dev_io_set_status_led_value(DEV_LED_INDEX index, uint16_t durationMs, uint8_t times)
{
    int ret = 0;
    IO_FLASH_SET flashSet;
    flashSet.index = (uint8_t)index;
    flashSet.period = durationMs;
    flashSet.times = times;
    ret = ioctl(this->m_io_fd, IO_SET_FLASH_LED, &flashSet);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    else
    {
        DEBUG_LOG(APP, INFO, "index:%d %d %d \r\n",index, durationMs, times);
    }
    return RTN_OKAY;
}

DEV_RTN DEV_IO::dev_io_trig_out_value(uint8_t index, uint32_t trigUs, uint8_t status)
{
    IO_PLUSE_SET m_set;
    int ret = 0;
    m_set.defaultState = 0;
    m_set.holdTime = trigUs;
    m_set.outState = status;
    m_set.index = index;
    ret = ioctl(this->m_io_fd, IO_SET_OUT, &m_set);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    else
    {
        DEBUG_LOG(APP, INFO, "index:%d %d %d \r\n",index, trigUs, status);
    }
    return RTN_OKAY;
}


DEV_RTN DEV_IO::dev_io_clear_trig_count_value()
{
    int ret = ioctl(this->m_io_fd, IO_SET_CLEAR_TRIG_COUNT, NULL);
    if (0 != ret) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }else{
        DEBUG_LOG(APP, INFO, "set ok \r\n");
    }
    return RTN_OKAY;
}

DEV_RTN DEV_IO::dev_io_set_in_value(uint8_t index, uint8_t type, uint16_t debounceMs)
{
    SET_IO_TRIGGER_TYPE trig_in;
    int ret = 0;
    trig_in.num=index;
    trig_in.trigType=type;
    trig_in.debounce = debounceMs;
    ret = ioctl(this->m_io_fd, IO_SET_IO_TRIGGER_PARAM, &trig_in);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }else{
        DEBUG_LOG(APP, INFO, "set %d %d %d ok \r\n", index, type, debounceMs);
    }
    return RTN_OKAY;
}
DEV_RTN DEV_IO::dev_io_ctrl_ext2d_intern_ctl_value(uint8_t syncNum, uint32_t syncExpo, uint32_t syncPeriod)
{
    int ret = 0;
    IO_2D_SUPPORT_LIGHT_PARAM io_2D_light;
    io_2D_light.mode= 0;
    io_2D_light.modeParam.interConfig.sync_num = syncNum;
    io_2D_light.modeParam.interConfig.sync_expo= syncExpo;
    io_2D_light.modeParam.interConfig.sync_period=syncPeriod;

    ret = ioctl(this->m_io_fd, IO_2D_SET_MODE_PARAM, &io_2D_light);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    else
    {
        DEBUG_LOG(APP, INFO, "set %d %d %d ok \r\n", syncNum, syncExpo, syncPeriod);
    }
    return RTN_OKAY;
}

DEV_RTN DEV_IO::dev_io_ctrl_ext2d_exterl_ctl_value(uint8_t picNum, uint32_t camExpo, uint32_t outTime)
{
    int ret = 0;
    IO_2D_SUPPORT_LIGHT_PARAM io_2D_light;
    io_2D_light.mode= 1;
    io_2D_light.modeParam.exterConfig.camNum=picNum;
    io_2D_light.modeParam.exterConfig.ext_sync_expo=outTime;
    io_2D_light.modeParam.exterConfig.ext_xtrig_expo=camExpo;
    ret = ioctl(this->m_io_fd, IO_2D_SET_MODE_PARAM, &io_2D_light);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    else
    {
        DEBUG_LOG(APP, INFO, "set %d %d %d ok \r\n", picNum, camExpo, outTime);
    }
    return RTN_OKAY;
}
DEV_RTN DEV_IO::dev_io_set_rgb_status_value(const DEV_IO_RGB_PARAM &param)
{
    int ret = 0;
    IO_RGB_SET io_rgb_set;
    io_rgb_set.num=4;//light_num:4
    if(1<=param.rgbPwm && 99>=param.rgbPwm)
    {
        io_rgb_set.pwmValue=param.rgbPwm;//pwm_value,
    }
    io_rgb_set.states[0]=0x1F;//r
    io_rgb_set.states[1]=0x2F;//g
    io_rgb_set.states[2]=0x4F;//b
    io_rgb_set.states[3]=0x8F;//w
    io_rgb_set.exposureTime[0]=param.r_expo;//expo
    io_rgb_set.exposureTime[1]=param.g_expo;
    io_rgb_set.exposureTime[2]=param.b_expo;
    io_rgb_set.exposureTime[3]=param.w_expo;
    io_rgb_set.periodTime[0]=param.period;//period
    io_rgb_set.periodTime[1]=param.period;
    io_rgb_set.periodTime[2]=param.period;
    io_rgb_set.periodTime[3]=param.period;

    ret = ioctl(this->m_io_fd, IO_SET_RGB_PARAM, &io_rgb_set);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }else{
        DEBUG_LOG(APP, INFO, "set ok \r\n");
    }
    return RTN_OKAY;
}
DEV_RTN DEV_IO::dev_io_rgb_signal_trig_value(const DEV_IO_RGB_TRIG_SET &mRgbTrigSet)
{
    int ret = ioctl(this->m_io_fd, IO_SET_SINGLE_RGB_TRIG, &mRgbTrigSet);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }else{
        DEBUG_LOG(APP, INFO, "set ok \r\n");
    }
    return RTN_OKAY;
}

DEV_RTN DEV_IO::dev_io_set_get_dlp_rgb_sync_states_value(const DEV_IO_DLP_RGB_SYNC_PARAM& syncValue)
{
    int ret = 0;
    IO_RGB_SYNC_SIGNAL mParam;
    mParam.mode = syncValue.mode;
    mParam.syncDlpNum = syncValue.syncDlpNum;
    mParam.syncRgbNum = syncValue.syncRgbNum;
    mParam.exposureTime[0] = syncValue.rgbExpoTime[0];
    mParam.exposureTime[1] = syncValue.rgbExpoTime[1];
    mParam.exposureTime[2] = syncValue.rgbExpoTime[2];
    mParam.exposureTime[3] = syncValue.rgbExpoTime[3];

    ret = ioctl(this->m_io_fd, IO_SET_GET_RGB_SYNC_SIGNAL, &mParam);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    DEBUG_LOG(APP, INFO, "set %d %d %d ok \r\n", mParam.mode, mParam.syncDlpNum, mParam.syncRgbNum);
    return RTN_OKAY;
}