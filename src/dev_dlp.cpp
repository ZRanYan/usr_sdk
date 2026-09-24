
#include "../include/dev_dlp.h"

DEV_DLP::DEV_DLP()
{
    return;
}

DEV_DLP::~DEV_DLP()
{
    for (uint8_t i = 0; i < DLP_MAX_DEVICES; i++)
    {
        if(1 == dlpSet.id[i])
        {
            close(dlpSet.fd[i]);
            dlpSet.fd[i] = -1;
        }
    }
}

DEV_RTN DEV_DLP::dev_dlp_init(int (&arr)[DLP_MAX_DEVICES])
{
    bool isHaveVer = false;
    DRIVER_VERSION dlpVer;
    char file_name[DLP_NAME_SIZE]; // 当前设备名
    for (uint8_t i = 0; i < DLP_MAX_DEVICES; i++)
    {
        snprintf(file_name, sizeof(file_name),"%s%u", DLP_DEV, i);
        this->dlpSet.fd[i] = open(file_name, O_RDWR);
        if(dlpSet.fd[i] < 0)
        {
            dlpSet.id[i]=0;
            DEBUG_LOG(APP, WARN, "open %s failed!\r\n", file_name);
            continue;
        }
        else
        {
            arr[i] = dlpSet.id[i] = 1;
            if(false == isHaveVer)
            {
                isHaveVer = true;
                ioctl(this->dlpSet.fd[i], DLP_GET_VERSION, &dlpVer);
                m_ver.assign(dlpVer.ver, std::min(dlpVer.len,static_cast<uint32_t>(DRIVER_VER_MAX_LEN)));
            }
        }
        dlpSet.fdMap |= 1<<i;
        strncpy(dlpSet.device_list[i], file_name, DLP_NAME_SIZE - 1); //数组拷贝 // 存入一维数组
        dlpSet.fdNum++;
    }
    //判断dlp的类型是dlp4710还是dlp4052
    this->dlpType = m_dlp_read_id();
    DEBUG_LOG(APP, INFO, "total dlp num:%d \r\n", dlpSet.fdNum);
    return ((0 == dlpSet.fdNum)?RTN_FAIL:RTN_OKAY);
}

int DEV_DLP::m_dlp_write_data(uint8_t index, std::initializer_list<uint8_t> list, uint8_t mode)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return -1;
    }

    if (list.size() > sizeof(m_dlpWriteValue.data))
        return -1;

    std::lock_guard<std::mutex> lock(this->m_dlpMutex);
    memset(&m_dlpWriteValue, 0, sizeof(m_dlpWriteValue));
    m_dlpWriteValue.length = static_cast<uint16_t>(list.size());
    m_dlpWriteValue.mode = mode;
    memcpy(m_dlpWriteValue.data, list.begin(), m_dlpWriteValue.length);
    return ioctl(this->dlpSet.fd[index], DLP_IOCTL_SET_DATA, &m_dlpWriteValue);
}

int DEV_DLP::m_dlp_write_data(uint8_t index, const uint8_t* data, uint16_t len, uint8_t mode)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return -1;
    }

    if (!data || len > sizeof(m_dlpWriteValue.data))
        return -1;

    std::lock_guard<std::mutex> lock(this->m_dlpMutex);
    memset(&m_dlpWriteValue, 0, sizeof(m_dlpWriteValue));
    m_dlpWriteValue.length = len;
    m_dlpWriteValue.mode = mode;
    memcpy(m_dlpWriteValue.data, data, len);
    return ioctl(this->dlpSet.fd[index], DLP_IOCTL_SET_DATA, &m_dlpWriteValue);
}

int DEV_DLP::m_dlp_write_data(uint8_t index, const DLP_CONTROL& val)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return -1;
    }

    std::lock_guard<std::mutex> lock(this->m_dlpMutex);
    m_dlpWriteValue = val;
    return ioctl(this->dlpSet.fd[index], DLP_IOCTL_SET_DATA, &m_dlpWriteValue);
}

int DEV_DLP::m_dlp_read_data(uint8_t index, std::initializer_list<uint8_t> send_list, uint8_t* recv_buf, uint16_t recv_len)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return -1;
    }

    if (send_list.size() > sizeof(m_dlpReadValue.sendData))
        return -1;

    std::lock_guard<std::mutex> lock(this->m_dlpMutex);
    memset(&m_dlpReadValue, 0, sizeof(m_dlpReadValue));
    m_dlpReadValue.sendLength = static_cast<uint8_t>(send_list.size());
    m_dlpReadValue.recvLength = recv_len;
    memcpy(m_dlpReadValue.sendData, send_list.begin(), m_dlpReadValue.sendLength);

    int ret = ioctl(this->dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
    if (ret == 0 && recv_buf != nullptr && recv_len > 0)
    {
        uint16_t copy_len = std::min(recv_len, static_cast<uint16_t>(sizeof(m_dlpReadValue.recvData)));
        memcpy(recv_buf, m_dlpReadValue.recvData, copy_len);
    }
    return ret;
}

int DEV_DLP::m_dlp_read_data(uint8_t index, const uint8_t* send_buf, uint8_t send_len, uint8_t* recv_buf, uint16_t recv_len)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return -1;
    }

    if (send_buf != nullptr && send_len > sizeof(m_dlpReadValue.sendData))
        return -1;

    std::lock_guard<std::mutex> lock(this->m_dlpMutex);
    memset(&m_dlpReadValue, 0, sizeof(m_dlpReadValue));
    m_dlpReadValue.sendLength = send_len;
    m_dlpReadValue.recvLength = recv_len;
    if (send_buf != nullptr && send_len > 0)
    {
        memcpy(m_dlpReadValue.sendData, send_buf, send_len);
    }

    int ret = ioctl(this->dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
    if (ret == 0 && recv_buf != nullptr && recv_len > 0)
    {
        uint16_t copy_len = std::min(recv_len, static_cast<uint16_t>(sizeof(m_dlpReadValue.recvData)));
        memcpy(recv_buf, m_dlpReadValue.recvData, copy_len);
    }
    return ret;
}

int DEV_DLP::m_dlp_read_data(uint8_t index, DLP_READ& read_val)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return -1;
    }

    std::lock_guard<std::mutex> lock(this->m_dlpMutex);
    m_dlpReadValue = read_val;
    int ret = ioctl(this->dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
    read_val = m_dlpReadValue;
    return ret;
}

DEV_DLP_TYPE DEV_DLP::m_dlp_read_id(void)
{
    FILE *fp;
    char buf[16] = {0};
    int value = -1;

    fp = fopen("/sys/kernel/debug/nv_dlp/dlp_id", "r");
    if (fp == NULL) {
        perror("fopen");
        return DLP_4710;
    }
    if (fgets(buf, sizeof(buf), fp) != NULL) {
        value = atoi(buf);   // "0\n" -> 0, "1\n" -> 1
    }
    fclose(fp);
    if(0 == value)
    {
        return DLP_4710;
    }
    return DLP_4052;
}

void DEV_DLP::dev_dlp_get_ver(std::string& ver)
{
    ver = m_ver;
    return;
}
DEV_RTN DEV_DLP::dev_dlp_get_type_value(DEV_DLP_TYPE& type)
{
    type = this->dlpType;
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_get_temp_value(uint8_t index, float& temp)
{
    CHECK_INDEX_RETURN(index);
    int ret = 0;
    uint16_t raw = 0;
    uint8_t recvBuf[2] = {0};
    
    ret = m_dlp_read_data(index, {0xD6}, recvBuf, 2);
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "error ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    raw = (recvBuf[1] << 8) | recvBuf[0];
    temp = (raw & 0x07FF) / 10.0f;
    temp *= ((1 == ((raw >> 11) & 0x01))?-1:1);
    DEBUG_LOG(APP, INFO, "temp:%f \r\n", temp);
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_set_current_value(uint8_t index, DEV_DLP_CURRENT_VALUE current)
{
    CHECK_INDEX_RETURN(index);
    int ret = 0;
    if (DLP_4710 == this->dlpType)
    {
        uint8_t recvBuf[6] = {0};
        ret = m_dlp_read_data(index, {READ_RGB_LED_PWM}, recvBuf, 6);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
        recvBuf[4] = (current.blue_cur & 0xFF);
        recvBuf[5] = current.blue_cur >> 8;
        
        uint8_t writeBuf[7] = {0};
        writeBuf[0] = WRITE_RGB_LED_PWM;
        memcpy(&writeBuf[1], recvBuf, 6);

        ret = m_dlp_write_data(index, writeBuf, 7);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
    }
    else if (DLP_4052 == this->dlpType)
    {
        uint8_t writeBuf[4] = {
            0x4B,
            static_cast<uint8_t>(current.red_cur),
            static_cast<uint8_t>(current.green_cur),
            static_cast<uint8_t>(current.blue_cur & 0xFF)
        };
        ret = m_dlp_write_data(index, writeBuf, 4);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
        else
        {
            DEBUG_LOG(APP, INFO, "ret:%d \r\n", ret);
        }
    }
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_set_current_all_value(uint16_t current)
{
    DEV_RTN ret = RTN_OKAY;
    uint8_t i=0;
    DEV_DLP_CURRENT_VALUE cur;
    cur.blue_cur = current;
    cur.red_cur = 0;
    cur.green_cur = 0;
    for(i=0;i<DLP_MAX_DEVICES;i++)
    {
        if(1 == this->dlpSet.id[i])
        {
            ret = dev_dlp_set_current_value(i, cur);
            if(ret != RTN_OKAY)
            {
                DEBUG_LOG(APP, ERROR, "set failed!\r\n");
                break;
            }
        }
    }
    return ret;
}
DEV_RTN DEV_DLP::dev_dlp_config_pwm_polarity_value(uint8_t index, bool readOrwrite, uint8_t &invert)
{
    int ret  = 0;
    CHECK_INDEX_RETURN(index);
    if (DLP_4052 == this->dlpType)
    {
        if(readOrwrite)
        {
            uint8_t writeBuf[2] = {0x0B, static_cast<uint8_t>(invert & 0x01)};
            ret = m_dlp_write_data(index, writeBuf, 2);
            if (ret < 0)
            {
                DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
                return RTN_FAIL;
            }
            else
            {
                DEBUG_LOG(APP, INFO, "ret:%d \r\n", ret);
            }
        }
        else
        {
            uint8_t recvBuf[1] = {0};
            ret = m_dlp_read_data(index, {0x0B}, recvBuf, 1);
            if (ret < 0)
            {
                DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
                return RTN_FAIL;
            }
            else
            {
                DEBUG_LOG(APP, INFO, "ret:%d \r\n", ret);
            }
            invert = recvBuf[0] & 0x01;
        }
    }
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_get_pwm_value(uint8_t index, DEV_DLP_CURRENT_VALUE& pwm)
{
    int ret  = 0;
    CHECK_INDEX_RETURN(index);
    if (DLP_4710 == this->dlpType)
    {
        uint8_t recvBuf[6] = {0};
        ret = m_dlp_read_data(index, {READ_RGB_LED_PWM}, recvBuf, 6);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
        pwm.blue_cur = (recvBuf[5] << 8) | recvBuf[4];
    }
    else if (DLP_4052 == this->dlpType)
    {
        uint8_t recvBuf[4] = {0};
        ret = m_dlp_read_data(index, {0x4B}, recvBuf, 4);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
        pwm.red_cur = recvBuf[1];
        pwm.green_cur = recvBuf[2];
        pwm.blue_cur = recvBuf[3];
    }
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_set_group_info_value(uint8_t index, uint8_t write_ctl,const DEV_DLP_PATTERN_GROUP_INFO& sel_pack)
{
	CHECK_INDEX_RETURN(index);
    uint8_t writeBuf[25] = {0};
    writeBuf[0] = WRITE_PATTERN_ORDER_TABLE_ENTRY;
    writeBuf[1] = write_ctl;              // 1
    writeBuf[2] = (sel_pack.group_id & 0xFF); // 1
    writeBuf[3] = sel_pack.num_pattern;         // 4
    writeBuf[4] = 0x04;
    writeBuf[5] = 0x00;
    writeBuf[6] = 0x00;
    writeBuf[7] = 0x00;
    writeBuf[8] = 0x00;
    writeBuf[9] = 0x00;
    writeBuf[10] = 0x00;
    writeBuf[11] = 0x00;
    writeBuf[12] = 0x00;
    writeBuf[13] = sel_pack.exp_s.exp & 0xFF;
    writeBuf[14] = (sel_pack.exp_s.exp >> 8) & 0xFF;
    writeBuf[15] = (sel_pack.exp_s.exp >> 16) & 0xFF;
    writeBuf[16] = (sel_pack.exp_s.exp >> 24) & 0xFF;
    writeBuf[17] = sel_pack.exp_s.pre_exp & 0xFF;
    writeBuf[18] = (sel_pack.exp_s.pre_exp >> 8) & 0xFF;
    writeBuf[19] = (sel_pack.exp_s.pre_exp >> 16) & 0xFF;
    writeBuf[20] = (sel_pack.exp_s.pre_exp >> 24) & 0xFF;
    writeBuf[21] = sel_pack.exp_s.post_exp & 0xFF;
    writeBuf[22] = (sel_pack.exp_s.post_exp >> 8) & 0xFF;
    writeBuf[23] = (sel_pack.exp_s.post_exp >> 16) & 0xFF;
    writeBuf[24] = (sel_pack.exp_s.post_exp >> 24) & 0xFF;
    int ret = m_dlp_write_data(index, writeBuf, 25);
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_get_group_info_value(uint8_t index, DEV_DLP_PATTERN_GROUP_INFO& param)
{
    int ret = 0;
    uint8_t recvBuf[24] = {0};
    ret = m_dlp_read_data(index, {READ_PATTERN_ORDER_TABLE_ENTRY, param.group_id}, recvBuf, 24);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    param.num_pattern=recvBuf[1];
    param.exp_s.exp=(recvBuf[14]<<24) | (recvBuf[13]<<16) |(recvBuf[12]<<8) | (recvBuf[11]);
    param.exp_s.pre_exp=(recvBuf[18]<<24) | (recvBuf[17]<<16) |(recvBuf[16]<<8) | (recvBuf[15]);
    param.exp_s.post_exp=(recvBuf[22]<<24) | (recvBuf[21]<<16) |(recvBuf[20]<<8) | (recvBuf[19]);
    if(0 == param.exp_s.exp)
    {
        return RTN_FAIL;
    }
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_trigonce_value(uint8_t index)
{
    CHECK_INDEX_RETURN(index);
    int ret = 0;
    ret = this->m_dlp_write_data(index, {WRITE_INTERNAL_PATTERN_CONTROL, 0x00, 0x00});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_trigonce_new_set(uint8_t index)
{
    CHECK_INDEX_RETURN(index);
    int ret = 0;
    if (DLP_4710 == this->dlpType)
    {
        ret = ioctl(this->dlpSet.fd[index], DLP_TRIG_ONCE_ASSIGN, NULL);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
    }
    else if(DLP_4052 == this->dlpType)
    {
        ret = m_dlp_write_data(index, {0x65, 0x02});
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
    }
    
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_get_min_expo_value(uint8_t index, DEV_DLP_EXP& param)
{
    int ret;
    uint8_t sendBuf[7] = {
        READ_MIN_EXPO, 0x01, 0x02,
        static_cast<uint8_t>(param.exp & 0xFF),
        static_cast<uint8_t>((param.exp >> 8) & 0xFF),
        static_cast<uint8_t>((param.exp >> 16) & 0xFF),
        static_cast<uint8_t>((param.exp >> 24) & 0xFF)
    };
    uint8_t recvBuf[13] = {0};
    ret = m_dlp_read_data(index, sendBuf, 7, recvBuf, 13);
    if(ret<0){
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    param.exp=(recvBuf[4]<<24) | (recvBuf[3]<<16) |(recvBuf[2]<<8) | (recvBuf[1]);
    param.pre_exp=(recvBuf[8]<<24) | (recvBuf[7]<<16) |(recvBuf[6]<<8) | (recvBuf[5]);
    param.post_exp=(recvBuf[12]<<24) | (recvBuf[11]<<16) |(recvBuf[10]<<8) | (recvBuf[9]);
    if(0 == param.exp)
    {
        DEBUG_LOG(APP, ERROR, "param.exp : 0 \r\n");
        return RTN_FAIL;
    }
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_set_delay_invert_value(uint8_t index, bool isInvert, uint32_t delayUs)
{
    int ret = 0;
	CHECK_INDEX_RETURN(index);
    if(DLP_4710 == this->dlpType)
    {
        uint8_t writeBuf[6] = {
            0x92,
            static_cast<uint8_t>(isInvert ? 0x07 : 0x03),
            static_cast<uint8_t>(delayUs & 0xFF),
            static_cast<uint8_t>((delayUs >> 8) & 0xFF),
            static_cast<uint8_t>((delayUs >> 16) & 0xFF),
            static_cast<uint8_t>((delayUs >> 24) & 0xFF)
        };
        ret = m_dlp_write_data(index, writeBuf, 6);
        if (ret < 0) {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
    }
    else if(DLP_4052 == this->dlpType)
    {
        uint8_t invertVal = (isInvert == 1) ? 0x02 : 0x00;
        ret = m_dlp_write_data(index, {0x6A, invertVal, 0xBB, 0xBB});
        if (ret < 0) {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
    }
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_set_long_short_flip_value(uint8_t index, bool long_flip, bool short_flip)
{
    CHECK_INDEX_RETURN(index);
    int ret = 0;
    uint8_t result  = 0;
    result |= (static_cast<int>(short_flip) << 2);  // 放到第三位（bit 2）
    result |= (static_cast<int>(long_flip) << 1); // 放到第二位（bit 1）
    ret = m_dlp_write_data(index, {0x14, result});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_set_trig_type_value(uint8_t index, DEV_DLP_TRIG_TYPE type)
{
    int ret = 0;
    CHECK_INDEX_RETURN(index);
    switch(type)
    {
        case TRIG_PAUSE:
            ret = m_dlp_write_data(index, {WRITE_INTERNAL_PATTERN_CONTROL, 0x01, 0x00});
            break;
        case TRIG_CONTINUOUS:
            ret = m_dlp_write_data(index, {WRITE_INTERNAL_PATTERN_CONTROL, 0x00, 0xff});
            break;
        default:
            return RTN_FAIL;
            break;
    }
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    else
        return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_trig_in_config_value(uint8_t index, bool enable, bool polarity)
{
    int ret = 0;
    uint8_t result = 0;
    CHECK_INDEX_RETURN(index);
    result |= (true == enable ? 0x01 : 0x00);   //
    result |= (true == polarity ? 0x02 : 0x00); //
    ret = m_dlp_write_data(index, {WRITE_TRIGGER_IN_CONFIGURATION, result});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_set_source_mode_value(uint8_t index, DEV_DLP_TRIG_MODE mode)
{
    int ret = 0;
    uint8_t status;
    CHECK_INDEX_RETURN(index);
    if (DLP_4052 == this->dlpType)
    {
        uint8_t modeVal = (TRIG_VIDEO == mode) ? 0x00 : 0x01;
        ret = m_dlp_write_data(index, {0x69, modeVal});
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
        this->dev_dlp_get_validate_status_value(index, status);
        usleep(100000);
    }
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_get_source_mode_value(uint8_t index, DEV_DLP_TRIG_MODE &mode)
{
    int ret = 0;
    CHECK_INDEX_RETURN(index);
    if (DLP_4052 == this->dlpType)
    {
        uint8_t recvBuf[2] = {0};
        ret = m_dlp_read_data(index, {0x69}, recvBuf, 2);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
        mode = ((0 == recvBuf[1]) ? TRIG_VIDEO : TRIG_PATTERN);
    }
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_get_validate_status_value(uint8_t index, uint8_t &status)
{
    int ret = 0;
    uint8_t m_status = 0;
    CHECK_INDEX_RETURN(index);
    if (DLP_4052 == this->dlpType)
    {
        ret = m_dlp_write_data(index, {0x7D, m_status});
        do {
            usleep(200000);
            uint8_t recvBuf[2] = {0};
            ret = m_dlp_read_data(index, {0x7D}, recvBuf, 2);
            if(ret<0)
            {
                DEBUG_LOG(APP, ERROR, " ioctl failed\r\n");
                break;
            }
            m_status = recvBuf[1];
        }while (m_status & 0x80);
        status = m_status;
    }
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_get_pattern_trigger_mode_value(uint8_t index, uint8_t &mode)
{
    int ret = 0;
    CHECK_INDEX_RETURN(index);
    if (DLP_4052 == this->dlpType)
    {
        uint8_t recvBuf[2] = {0};
        ret = m_dlp_read_data(index, {0x70}, recvBuf, 2);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, " ioctl DLP_COMMAND_CONTROL failed\r\n");
            return RTN_FAIL;
        }
        if (recvBuf[0])
        {
            mode = recvBuf[1];
        }
        else
        {
            DEBUG_LOG(APP, ERROR, "ACK check failed!\r\n");
            return RTN_FAIL;
        }
    }
    return RTN_INVALID_ARG;
}

DEV_RTN DEV_DLP::dev_dlp_set_on_off_value(uint8_t index, bool onOff)
{
    int ret = 0;
    CHECK_INDEX_RETURN(index);
    if (DLP_4052 == this->dlpType)
    {
        DEBUG_LOG(APP, INFO, "index:%d  onOff:%d\r\n", index, onOff);
        uint8_t val = (true == onOff) ? 0x02 : 0x00;
        ret = m_dlp_write_data(index, {0x65, val});
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
    }
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_set_reboot_value(uint8_t index, uint32_t &ver)
{
    int ret = 0;
    CHECK_INDEX_RETURN(index);
    if (DLP_4052 == this->dlpType)
    {
        ret = m_dlp_write_data(index, {0x13, 0x01});
        usleep(2000000);

        uint8_t recvBuf[4] = {0};
        ret = m_dlp_read_data(index, {0x00}, recvBuf, 4);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, " ioctl failed\r\n");
            return RTN_FAIL;
        }
        ver = ((uint32_t)recvBuf[3] << 0) |
              ((uint32_t)recvBuf[2] << 8) |
              ((uint32_t)recvBuf[1] << 16) |
              ((uint32_t)recvBuf[0] << 24);
    }
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_set_lut_value(uint8_t index, const DEV_DLP_PATTERN_ORDER_SET &data)
{
    int ret = 0;
    CHECK_INDEX_RETURN(index);
    // 1. Stop pattern sequence
    ret = m_dlp_write_data(index, {0x65, 0x00});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    // 2. Set trig Source
    ret = m_dlp_write_data(index, {0x6F, 0x03});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    ret = m_dlp_write_data(index, {0x69, 0x01});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    // 3. Set Expo
    uint8_t expoBuf[9] = {
        0x66, // 设置曝光时间和周期
        static_cast<uint8_t>(data.expo_time & 0xFF),
        static_cast<uint8_t>((data.expo_time >> 8) & 0xFF),
        static_cast<uint8_t>((data.expo_time >> 16) & 0xFF),
        static_cast<uint8_t>((data.expo_time >> 24) & 0xFF),
        static_cast<uint8_t>(data.period_time & 0xFF),
        static_cast<uint8_t>((data.period_time >> 8) & 0xFF),
        static_cast<uint8_t>((data.period_time >> 16) & 0xFF),
        static_cast<uint8_t>((data.period_time >> 24) & 0xFF)
    };
    ret = m_dlp_write_data(index, expoBuf, 9);
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    // 4. Pattern Trigger Mode
    ret = m_dlp_write_data(index, {0x70, 0x01});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    // 5. Set LUT entries 0x75
    uint8_t lutBuf[5] = {
        0x75,
        static_cast<uint8_t>((data.pattern_num - 1) & 0xFF),
        0x00,
        static_cast<uint8_t>((data.pattern_num - 1) & 0xFF),
        static_cast<uint8_t>((data.img_num - 1) & 0xFF)
    };
    ret = m_dlp_write_data(index, lutBuf, 5);
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    // 6. set pattern_squ 0x77
    ret = m_dlp_write_data(index, {0x77, 0x02});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    ret = m_dlp_write_data(index, {0x76, 0x00});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    // 6. set pattern_squ 0x78
    DLP_CONTROL patControl = {};
    patControl.length = data.pattern_num * 3 + 1;
    patControl.mode = 0;
    patControl.data[0] = 0x78;
    for (uint8_t i = 0; i < data.pattern_num; ++i)
    {
        patControl.data[i * 3 + 1] = data.pattern_squ[i] << 2;

        if (data.bit_depth[i] == 1)
            patControl.data[i * 3 + 2] = 0x71;
        else if (data.bit_depth[i] == 8)
            patControl.data[i * 3 + 2] = 0x78;

        bool trigger = (i == 0);

        if (data.img_num != 1)
        {
            for (auto num : data.pre_num_squ)
            {
                if (i == num)
                {
                    trigger = true;
                    break;
                }
            }
        }

        patControl.data[i * 3 + 3] = trigger ? 0x06 : 0x02;
    }
    ret = m_dlp_write_data(index, patControl);
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    ret = m_dlp_write_data(index, {0x77, 0x00});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    //7. set image_squ
    ret = m_dlp_write_data(index, {0x77, 0x01});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    ret = m_dlp_write_data(index, {0x76, 0x00});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    DLP_CONTROL imgControl = {};
    imgControl.mode = 0;
    imgControl.data[0] = 0x78;
    if(data.img_num == 2){
        imgControl.length = 1 + data.image_squ.size();
        imgControl.data[1] = data.image_squ[1];
        imgControl.data[2] = data.image_squ[0];
    }else{
        imgControl.length = 1 + data.image_squ.size();
        for(size_t i=0; i<data.image_squ.size(); i++)
        {
            imgControl.data[i+1] = data.image_squ[i];
            printf("data[%zu]:0X%02d\r\n", i, data.image_squ[i]);
        }
    }
    ret = m_dlp_write_data(index, imgControl);
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    ret = m_dlp_write_data(index, {0x77, 0x00});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    //8. Validate
    uint8_t status = 0;
    ret = m_dlp_write_data(index, {0x7D, status});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    // 轮询直到 bit7 = 0（Validate完成）
    do
    {
        uint8_t recvBuf[2] = {0};
        ret = m_dlp_read_data(index, {0x7D}, recvBuf, 2);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, " ioctl failed\r\n");
            break;
        }
        // 注意：一般第二个字节才是 status_
        status = recvBuf[1];
    } while (status & 0x80); // bit7 == 1 → busy
    if (status & 0x01)
        printf("Error: Exposure / Period INVALID\n");
    else
        printf("Exposure / Period OK\n");

    if (status & 0x02)
        printf("Error: LUT INVALID\n");
    else
        printf("LUT OK\n");

    if (status & 0x04)
        printf("Warning: Trigger Out1 issue\n");

    if (status & 0x08)
        printf("Warning: Post Vector issue\n");

    if (status & 0x10)
        printf("Warning: Period - Exposure < 230us\n");
    else
        printf("Period - Exposure OK\n");
    //9. Start
    usleep(500000);
    ret = m_dlp_write_data(index, {0x65, 0x02});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_exp_period_config_value(uint8_t index, bool readOrwrite, DEV_DLP_EXPOSURE_PERIOD_SET &set)
{
    int ret = 0;
    CHECK_INDEX_RETURN(index);
    if (DLP_4052 == this->dlpType)
    {
        if(readOrwrite)//写入配置操作
        {
            if(set.exposure_time < 236)
            {
                return RTN_INVALID_ARG;
            }
            else if(231 > (set.period_time - set.exposure_time))
            {
                return RTN_INVALID_ARG;
            }
            uint8_t writeBuf[9] = {
                0x66,
                static_cast<uint8_t>(set.exposure_time & 0xFF),
                static_cast<uint8_t>((set.exposure_time >> 8) & 0xFF),
                static_cast<uint8_t>((set.exposure_time >> 16) & 0xFF),
                static_cast<uint8_t>((set.exposure_time >> 24) & 0xFF),
                static_cast<uint8_t>(set.period_time & 0xFF),
                static_cast<uint8_t>((set.period_time >> 8) & 0xFF),
                static_cast<uint8_t>((set.period_time >> 16) & 0xFF),
                static_cast<uint8_t>((set.period_time >> 24) & 0xFF)
            };
            ret = m_dlp_write_data(index, writeBuf, 9);
            if (ret < 0)
            {
                DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
                return RTN_FAIL;
            }
        }
        else//读取操作
        {
            uint8_t recvBuf[9] = {0};
            ret = m_dlp_read_data(index, {0x66}, recvBuf, 9);
            if(ret<0)
            {
                DEBUG_LOG(APP, ERROR, " ioctl DLP_COMMAND_CONTROL failed\r\n");
                return RTN_FAIL;
            }
            DEBUG_LOG(APP, INFO, "recvData[0]:0x%x 0x%x\r\n", recvBuf[0], recvBuf[1]);
            if(recvBuf[0])
            {
                set.exposure_time = ((uint32_t)recvBuf[1] << 0)  |
                                    ((uint32_t)recvBuf[2] << 8)  |
                                    ((uint32_t)recvBuf[3] << 16) |
                                    ((uint32_t)recvBuf[4] << 24);
                set.period_time   = ((uint32_t)recvBuf[5] << 0)  |
                                    ((uint32_t)recvBuf[6] << 8)  |
                                    ((uint32_t)recvBuf[7] << 16) |
                                    ((uint32_t)recvBuf[8] << 24);
            }
            else
            {
                DEBUG_LOG(APP, ERROR, "ACK check failed!\r\n");
                return RTN_FAIL;
            }
        }

    }
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_led_config_value(uint8_t index, bool readOrwrite, DEV_DLP_LED_SET &ledSet)
{
    int ret = 0;
    uint8_t data = 0;
    CHECK_INDEX_RETURN(index);
    if (DLP_4052 == this->dlpType)
    {
        if(readOrwrite)//写入配置操作
        {
            data |= ((uint8_t)ledSet.ctlMethod << 3);
            data |= ((uint8_t)ledSet.blue_on   << 2);
            data |= ((uint8_t)ledSet.green_on  << 1);
            data |= ((uint8_t)ledSet.red_on);
            ret = m_dlp_write_data(index, {0x10, data});
            if (ret < 0)
            {
                DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
                return RTN_FAIL;
            }
        }
        else//读取操作
        {
            uint8_t recvBuf[2] = {0};
            ret = m_dlp_read_data(index, {0x10}, recvBuf, 2);
            if(ret<0)
            {
                DEBUG_LOG(APP, ERROR, " ioctl DLP_COMMAND_CONTROL failed\r\n");
                return RTN_FAIL;
            }
            DEBUG_LOG(APP, INFO, "recvData[0]:0x%x 0x%x\r\n", recvBuf[0], recvBuf[1]);
            if(recvBuf[0])
            {
                data = recvBuf[1];
                ledSet.ctlMethod = ((data >> 3) & 0x01) ? true : false;
                ledSet.blue_on   = ((data >> 2) & 0x01) ? true : false;
                ledSet.green_on  = ((data >> 1) & 0x01) ? true : false;
                ledSet.red_on    = (data & 0x01) ? true : false;
            }
            else
            {
                DEBUG_LOG(APP, ERROR, "ACK check failed!\r\n");
                return RTN_FAIL;
            }
        }

    }
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_update_falsh_value(uint8_t index, const char* path, const char* config)
{
	CHECK_INDEX_RETURN(index);
    if(DLP_4710 == this->dlpType)
    {
        return dev_dlp_update_falsh_dlp4710(index, path);
    }
    else if(DLP_4052 == this->dlpType)
    {
        return dev_dlp_update_falsh_dlp4052(index, path, config);
    }
    return RTN_FAIL;
}
int DEV_DLP::m_GetSectorNum(FlashResult *flash_result, unsigned int Addr)
{
    unsigned int i;
    for(i = 0; i < flash_result->sector_count; i++)
    {
        if(flash_result->sector_addr[i] > Addr)
        {
            break;
        }
    }
    return (i == 0) ? 0 : (i - 1);
}
int DEV_DLP::m_calc_one_range(uint8_t index, uint32_t addr, uint32_t size, uint32_t *outChk)
{
    // StartAddress 0x29
    uint8_t addrBuf[5] = {
        0x29,
        static_cast<uint8_t>(addr & 0xFF),
        static_cast<uint8_t>((addr >> 8) & 0xFF),
        static_cast<uint8_t>((addr >> 16) & 0xFF),
        static_cast<uint8_t>((addr >> 24) & 0xFF)
    };
    if (m_dlp_write_data(index, addrBuf, 5, 1) < 0)
    {
        printf("SetFlashAddr failed addr=0x%08X\n", addr);
        return -1;
    }

    // DataSize 0x2C
    uint8_t sizeBuf[5] = {
        0x2C,
        static_cast<uint8_t>(size & 0xFF),
        static_cast<uint8_t>((size >> 8) & 0xFF),
        static_cast<uint8_t>((size >> 16) & 0xFF),
        static_cast<uint8_t>((size >> 24) & 0xFF)
    };
    if (m_dlp_write_data(index, sizeBuf, 5, 1) < 0)
    {
        printf("SetUploadSize failed size=0x%08X\n", size);
        return -1;
    }

    // CalculateChecksum 0x26
    if (m_dlp_write_data(index, {0x26, 0x01}, 1) < 0)
    {
        printf("CalculateFlashChecksum failed\n");
        return -1;
    }
    usleep(1000);
    // WaitForFlashReady + ReadControl(0x15 0x00)
    uint8_t recvBuf[10] = {0};
    while (1)
    {
        if (m_dlp_read_data(index, {0x15, 0x00}, recvBuf, 10) < 0)
        {
            printf("ReadControl failed\n");
            return -1;
        }
        // bit3 = busy
        if ((recvBuf[0] & 0x08) == 0)
            break;
        usleep(10000);
    }
    *outChk = ((uint32_t)recvBuf[6]) |
              ((uint32_t)recvBuf[7] << 8) |
              ((uint32_t)recvBuf[8] << 16) |
              ((uint32_t)recvBuf[9] << 24);
    return 0;
}

DEV_RTN DEV_DLP::dev_dlp_update_falsh_dlp4052(uint8_t index, const char* filepath, const char* flash_param_file)
{

    CHECK_INDEX_RETURN(index);
    int ret = -1;
    FlashResult flashresult;
    printf("DLP Flash Update Start\n");
    int fd = this->dlpSet.fd[index];
    if(fd < 0)
    {
        perror("open dlp device failed");
        return RTN_FAIL;
    }
    printf("DLP4052Test\r\n");
    printf("start update flash...\n");
    uint32_t FLASHTABLE_APP_SIGNATURE = 0x01234567;
    uint32_t flash_table_address = 0x00020000; // 128*1024
    size_t filesize = 0;                         // Firmware size
    uint8_t *file_buf = NULL;                  // Firmware memory address
    //-------------1. Firmware file and verification-------------
    // 使用API传入
    printf("Firmware:%s\n", filepath);  
    struct stat st;
    if (stat(filepath, &st) != 0)
    {
        printf("Firmware file does not exist, read failed.\n");
        return RTN_FAIL;
    }
    filesize = st.st_size;
    printf("Total size of firmware file: %ld Byte\n", filesize);
    // Read firmware file into memory
    FILE *fp = fopen(filepath, "rb");
    if (!fp)
    {
        printf("Firmware file failed to open\n");
        return RTN_FAIL;
    }
    file_buf = new uint8_t[filesize];
    if (!file_buf)
    {
        printf("Memory allocation failed\n");
        // //fclose(fp);
        return RTN_FAIL;
    }
    memset(file_buf, 0, filesize);
    fseek(fp, 0, SEEK_SET);
    // Read the file correctly
    if (fread(file_buf, 1, filesize, fp) != filesize)
    {
        printf("Firmware read failed\n");
        delete[] file_buf;
        // //fclose(fp);
        return RTN_FAIL;
    }
    // Verify file validity
    uint32_t value = 0;
    memcpy(&value, file_buf + flash_table_address, sizeof(value));
    if (value != FLASHTABLE_APP_SIGNATURE)
    {
        printf("File verification failed.\n");
        delete[] file_buf;
        // //fclose(fp);
        return RTN_FAIL;
    }
    //-------------2. Stop trig dlp-------------
    uint8_t recvBuf[2] = {0};
    if (m_dlp_read_data(index, {0x69}, recvBuf, 2) < 0)
    {
        delete[] file_buf;
        return RTN_FAIL;
    }
    if (recvBuf[1] == 0)
    {
        uint8_t m_status = 0;
        printf("Currently in Video mode\n");
        // Switch to pattern mode
        if (m_dlp_write_data(index, {0x69, 0x01}) < 0)
        {
            printf("enter pattern mode failed\n");
            delete[] file_buf;
            return RTN_FAIL;
        }
        ret = m_dlp_write_data(index, {0x7D, 0x00});
        do {
            usleep(200000);
            uint8_t checkBuf[2] = {0};
            ret = m_dlp_read_data(index, {0x7D}, checkBuf, 2);
            if(ret<0)
            {
                break;
            }
            m_status = checkBuf[1];
        }while (m_status & 0x80);
        printf("enter pattern mode success m_status:0x%x\n", m_status);
    }
    else if (recvBuf[1] == 1)
    {
        printf("Currently in Pattern sequence mode\n");
        // stop trig dlp
        if (m_dlp_write_data(index, {0x65, 0x00}) < 0)
        {
            perror("ioctl DLP_IOCTL_SET_DATA failed\n");
            delete[] file_buf;
            return RTN_FAIL;
        }
        else
        {
            printf("DLP Stop successfully\n");
        }
    }
    //-------------3. Enter programming mode-------------
    if (m_dlp_write_data(index, {0x30, 0x01}) < 0)
    {
        printf("enter program mode failed\n");
        delete[] file_buf;
        return RTN_FAIL;
    }
    printf("enter program mode success\n");
    // After entering programming mode, you need to wait for the connection to reconnect.
    // Since the time may not be accurately obtained, just wait.
    usleep(8000000);
    //-------------4. Read Flash information-------------
    uint8_t manID = 0;  // Manufacturer ID
    uint16_t devID = 0; // Device ID
    uint8_t idBuf[10] = {0};
    if (m_dlp_read_data(index, {0x15, 0x0C}, idBuf, 10) < 0)
    {
        printf("Read Manufacturer ID failed\n");
        delete[] file_buf;
        return RTN_FAIL;
    }
    manID = idBuf[6];
    printf("Manufacturer ID = 0x%02X\n", manID);
    
    memset(idBuf, 0, sizeof(idBuf));
    if (m_dlp_read_data(index, {0x15, 0x0D}, idBuf, 10) < 0)
    {
        printf("Read Device ID failed\n");
        delete[] file_buf;
        return RTN_FAIL;
    }
    devID = ((uint16_t)idBuf[7] << 8) | idBuf[6];
    printf("Device ID = 0x%04X\n", devID);
    memset(&flashresult, 0, sizeof(flashresult));
    // Extract the 16-digit ID required for the table
    FILE *fpp = fopen(flash_param_file, "r");
    if (fpp == NULL)
    {
        printf("Open FlashDeviceParameters.txt failed\n");
        return RTN_FAIL;
    }
    char line[4096];
    //--------------------------------------------------
    // Read line by line
    //--------------------------------------------------
    while (fgets(line, sizeof(line), fpp))
    {
        //----------------------------------------------
        // Skip comment line
        //----------------------------------------------
        if (line[0] == '/' || line[0] == '\n')
        {
            continue;
        }
        //----------------------------------------------
        // Parse fixed fields
        //----------------------------------------------
        char mfg_name[64];
        char dev_name[64];
        uint16_t tid;
        uint16_t tdev;
        int mb;
        int alg;
        int sec;
        uint32_t size;
        int n = sscanf(line,
                       "\"%[^\"]\", 0x%hx, \"%[^\"]\", 0x%hx, %d, %d, 0x%x, %d,",
                       mfg_name,
                       &tid,
                       dev_name,
                       &tdev,
                       &mb,
                       &alg,
                       &size,
                       &sec);

        if (n != 8)
        {
            continue;
        }

        //------------------------------------------
        // Compare ID
        //------------------------------------------
        if (tid == manID &&
            tdev == devID)
        {
            printf("\nFlash Match Success!\n");

            printf("Manufacturer : %s\n", mfg_name);

            printf("Device       : %s\n", dev_name);

            flashresult.flash_size = size;
            if (filesize > flashresult.flash_size)
            {
                printf("Firmware size exceeds flash size\n");
                //fclose(fp);
                delete[] file_buf;
                break;
            }

            flashresult.sector_count = sec;

            //--------------------------------------
            // Find sector_count position
            //--------------------------------------

            char *p = line;

            int comma_count = 0;

            while (*p)
            {
                if (*p == ',')
                {
                    comma_count++;

                    // sec 后面的逗号
                    if (comma_count == 8)
                    {
                        p++;
                        break;
                    }
                }

                p++;
            }

            //--------------------------------------
            // Read sector addresses
            //--------------------------------------
            int cnt = 0;

            while (cnt < sec && *p)
            {
                //----------------------------------
                // Skip space
                //----------------------------------
                while (*p == ' ' || *p == '\t')
                {
                    p++;
                }
                //----------------------------------
                // Read decimal 0
                //----------------------------------
                if (*p == '0' &&
                    (*(p + 1) == ',' ||
                     *(p + 1) == ' '))
                {
                    flashresult.sector_addr[cnt] = 0;
                    cnt++;
                }
                else
                {
                    //--------------------------------
                    // Read hex sector address
                    //--------------------------------
                    uint32_t addr;

                    if (sscanf(p, "0x%x", &addr) == 1)
                    {
                        flashresult.sector_addr[cnt] = addr;

                        cnt++;
                    }
                }

                //----------------------------------
                // Next comma
                //----------------------------------
                p = strchr(p, ',');

                if (p == NULL)
                {
                    break;
                }

                p++;
            }

            //--------------------------------------
            // Print Result
            //--------------------------------------
            printf("\nflash_size   = 0x%X\n",
                   flashresult.flash_size);

            printf("sector_count = %d\n",
                   flashresult.sector_count);

            flashresult.Type = alg;
            printf("type = %d\n",
                   flashresult.Type);

            for (uint32_t i = 0; i < flashresult.sector_count; i++)
            {
                printf("sector_addr[%d] = 0x%08X\n",
                       i,
                       flashresult.sector_addr[i]);
            }
        }
    }
    fclose(fpp);
    //-------------5. Erase sector  -------------
    // 设置flash类型
    if (m_dlp_write_data(index, {0x2F, flashresult.Type}, 1) < 0)
    {
        printf("set type failed\n");
        delete[] file_buf;
        return RTN_FAIL;
    }
    printf("set type success\n");

    unsigned int start_sector = 0, end_sector = 0;

    start_sector = m_GetSectorNum(&flashresult, flash_table_address);
    end_sector = m_GetSectorNum(&flashresult, filesize);

    if (filesize == flashresult.sector_addr[end_sector]) // If perfectly aligned with last sector start addr, no need to erase last sector.
        end_sector -= 1;

    printf("start_sector: %d    end_sector: %d  \n", start_sector, end_sector);
    for (uint32_t i = start_sector; i <= end_sector; i++)
    {
        uint8_t startAddrBuf[5] = {
            0x29, // 设置起始地址
            static_cast<uint8_t>(flashresult.sector_addr[i] & 0xFF),
            static_cast<uint8_t>((flashresult.sector_addr[i] >> 8) & 0xFF),
            static_cast<uint8_t>((flashresult.sector_addr[i] >> 16) & 0xFF),
            static_cast<uint8_t>((flashresult.sector_addr[i] >> 24) & 0xFF)
        };
        if (m_dlp_write_data(index, startAddrBuf, 5, 1) < 0)
        {
            printf("设置起始地址失败\n");
            delete[] file_buf;
            break;
        }
        printf("g_FlashDevice.SectorArr[%d] = 0x%08X\n", i, flashresult.sector_addr[i]);
        if (m_dlp_write_data(index, {0x28, 0x00}, 1) < 0)
        {
            printf("Erase failed\n");
            delete[] file_buf;
            return RTN_FAIL;
        }
        int k = 10;
        do
        {
            usleep(100000);
            uint8_t statusBuf[10] = {0};
            ret = m_dlp_read_data(index, {0x15, 0x00}, statusBuf, 10);
            if(ret != 0)
            {
                k--;
                if(k<0)
                {
                    printf("Read status failed\n");
                    delete[] file_buf;
                    return RTN_FAIL;
                }
            }
            if((statusBuf[0] & 0x08) == 0)
                break;
        } while(1);
    }
    printf("erase success\n");
    //-------------6. Write data  -------------
    uint8_t flashAddrBuf[5] = {
        0x29, // 设置起始地址
        static_cast<uint8_t>(flash_table_address & 0xFF),
        static_cast<uint8_t>((flash_table_address >> 8) & 0xFF),
        static_cast<uint8_t>((flash_table_address >> 16) & 0xFF),
        static_cast<uint8_t>((flash_table_address >> 24) & 0xFF)
    };
    if (m_dlp_write_data(index, flashAddrBuf, 5, 1) < 0)
    {
        printf("Set Upload Size failed\n");
        delete[] file_buf;
        return RTN_FAIL;
    }
    printf("Set Flash Address = 0x%08X\n", flash_table_address);

    long long image_size = filesize - flash_table_address;

    uint8_t uploadSizeBuf[5] = {
        0x2C, // 设置文件大小
        static_cast<uint8_t>(image_size & 0xFF),
        static_cast<uint8_t>((image_size >> 8) & 0xFF),
        static_cast<uint8_t>((image_size >> 16) & 0xFF),
        static_cast<uint8_t>((image_size >> 24) & 0xFF)
    };
    if (m_dlp_write_data(index, uploadSizeBuf, 5, 1) < 0)
    {
        printf("Set Upload Size failed\n");
        delete[] file_buf;
        return RTN_FAIL;
    }
    printf("Upload Size = 0x%08llX bytes\n", image_size);

    // 写入数据
    // 分块下载数据 (0x25)
    uint32_t down_load = 504;
    long long dataLen = image_size;

    long long totalSent = 0;
    uint8_t *p_data = file_buf + flash_table_address;

    int kl = 0;
    while (dataLen > 0)
    {
        kl++;
        int chunk = (dataLen > down_load) ? down_load : dataLen;

        DLP_CONTROL dlControl = {};
        dlControl.length = chunk + 1;
        dlControl.mode = 1;
        dlControl.data[0] = 0x25; // Download Data 命令

        memcpy(&dlControl.data[1], p_data + totalSent, chunk);

        if (m_dlp_write_data(index, dlControl) < 0)
        {
            printf("Download data failed @ offset %lld\n", totalSent);
            delete[] file_buf;
            return RTN_FAIL;
        }

        totalSent += chunk;
        dataLen -= chunk;

        int percent = (totalSent * 100) / image_size;
        printf("\rDownloading: %d%% (%lld/%lld)", percent, totalSent, image_size);
        fflush(stdout);
        usleep(1000);
    }
    printf("\nDownload complete!\n");
    //-------------7. Compute checksum-------------
    printf("Checking firmware checksum...\n");

    delete[] file_buf;

    fp = fopen(filepath, "rb");
    if (!fp)
    {
        printf("Firmware file failed to open\n");
        return RTN_FAIL;
    }
    if (stat(filepath, &st) != 0)
    {
        printf("Firmware file does not exist, read failed.\n");
        return RTN_FAIL;
    }
    filesize = st.st_size;

    if (flash_table_address > 0)
    {
        file_buf = new uint8_t[flash_table_address];
        fread(file_buf, 1, flash_table_address, fp);
        delete[] file_buf;
    }

    unsigned int expectedChecksum = 0;
    unsigned int checksum = 0;
    unsigned int tempChkSum = 0;
    size_t n = 0;
    unsigned int i = 0;

    /* =========================================================
     * 1) 计算文件期望 checksum（跳过 bootloader）
     * ========================================================= */
    file_buf = new uint8_t[1024];
    if (!file_buf)
    {
        return RTN_FAIL;
    }

    while ((n = fread(file_buf, 1, 1024, fp)) > 0)
    {
        for (size_t j = 0; j < n; j++)
        {
            expectedChecksum += file_buf[j];
        }
    }
    printf("Expected Checksum = 0x%08X\n", expectedChecksum);

    // StartAddress 0x29
    uint8_t chkAddrBuf[5] = {
        0x29,
        static_cast<uint8_t>(flash_table_address & 0xFF),
        static_cast<uint8_t>((flash_table_address >> 8) & 0xFF),
        static_cast<uint8_t>((flash_table_address >> 16) & 0xFF),
        static_cast<uint8_t>((flash_table_address >> 24) & 0xFF)
    };
    if (m_dlp_write_data(index, chkAddrBuf, 5, 1) < 0)
    {
        printf("SetFlashAddr failed addr=0x%08X\n", flash_table_address);
        delete[] file_buf;
        return RTN_FAIL;
    }

    // DataSize 0x2C
    uint32_t chkLen = filesize - flash_table_address;
    uint8_t chkLenBuf[5] = {
        0x2C,
        static_cast<uint8_t>(chkLen & 0xFF),
        static_cast<uint8_t>((chkLen >> 8) & 0xFF),
        static_cast<uint8_t>((chkLen >> 16) & 0xFF),
        static_cast<uint8_t>((chkLen >> 24) & 0xFF)
    };
    if (m_dlp_write_data(index, chkLenBuf, 5, 1) < 0)
    {
        printf("SetUploadSize failed size=0x%08X\n", chkLen);
        delete[] file_buf;
        return RTN_FAIL;
    }

    delete[] file_buf;
    //fclose(fp);

    /* =========================================================
     * 2) 对齐到 bootloader 之后的第一个 sector
     * ========================================================= */
    checksum = 0;
    tempChkSum = 0;
    i = 0;

    if (flash_table_address)
    {
        while (flashresult.sector_addr[i] < flash_table_address)
        {
            i++;
        }

        if (i >= flashresult.sector_count)
        {
            printf("Sector size is not aligned with bootloader size\n");
            return RTN_FAIL;
        }
    }

    /* =========================================================
     * 3) 完全按官方逻辑处理 sector
     * ========================================================= */
    while ((i < flashresult.sector_count - 2) && (flashresult.sector_addr[i + 1] < filesize))
    {
        uint32_t addr = flashresult.sector_addr[i];
        uint32_t size = flashresult.sector_addr[i + 1] - flashresult.sector_addr[i];
        printf("Checksum sector %u: addr=0x%08X size=0x%08X\n", i, addr, size);
        // if (calc_one_range(fd,&m_dlpWriteValue,&m_dlpReadValue,addr,size,&tempChkSum) < 0)
        if (m_calc_one_range(index,addr,size,&tempChkSum) < 0)
            return RTN_FAIL;
        checksum += tempChkSum;
        i++;
        printf("  checksum = 0x%08X\n", checksum);
    }

    /* =========================================================
     * 4) 官方尾部逻辑：处理最后 1~2 个 sector
     * ========================================================= */
    if (i == (flashresult.sector_count - 2))
    {
        if (flashresult.sector_addr[i + 1] < filesize)
        {
            uint32_t addr = flashresult.sector_addr[i];
            uint32_t size = flashresult.sector_addr[i + 1] - flashresult.sector_addr[i];
            printf("Checksum sector %u: addr=0x%08X size=0x%08X\n", i, addr, size);
            if (m_calc_one_range(index,addr,size,&tempChkSum) < 0)
                return RTN_FAIL;
            checksum += tempChkSum;
            i++;
            printf("  checksum = 0x%08X\n", checksum);
        }
        if (flashresult.sector_addr[i] < filesize)
        {
            uint32_t addr = flashresult.sector_addr[i];
            uint32_t size = filesize - flashresult.sector_addr[i];

            printf("Checksum sector %u: addr=0x%08X size=0x%08X\n", i, addr, size);

            if (m_calc_one_range(index,addr,size,&tempChkSum) < 0)
                return RTN_FAIL;

            checksum += tempChkSum;
            printf("  checksum = 0x%08X\n", checksum);
        }
    }
    else
    {
        uint32_t addr = flashresult.sector_addr[i];
        uint32_t size = filesize - flashresult.sector_addr[i];

        printf("Checksum tail sector %u: addr=0x%08X size=0x%08X\n", i, addr, size);

        if (m_calc_one_range(index,addr,size,&tempChkSum)< 0)
            return RTN_FAIL;

        checksum += tempChkSum;
        printf("  checksum = 0x%08X\n", checksum);
    }

    printf("Expected Checksum = 0x%08X\n", expectedChecksum);

    if (checksum != expectedChecksum)
    {
        printf("Checksum mismatch!\n");
        return RTN_FAIL;
    }

    printf("Checksum verify PASS\n");
    //-------------8. exit programming mode -------------
    sleep(1);
    if (m_dlp_write_data(index, {0x30, 0x01}, 1) < 0)
    {
        perror("exit program mode failed");
        return RTN_FAIL;
    }
    printf("exit program mode success\n");
    return RTN_OKAY;
}



DEV_RTN DEV_DLP::dev_dlp_update_falsh_dlp4710(uint8_t index, const char* path)
{
    CHECK_INDEX_RETURN(index);
    DEV_RTN retStatus = RTN_OKAY;
    int ret = 0;
    int filesize = 0;
    uint8_t *buffer = nullptr;
    uint16_t m_quotient = 0;
    uint16_t m_remainder = 0;
    uint8_t precheckSend[5] = {0};
    uint8_t precheckRecv[1] = {0};
    uint8_t eraseRecv[1] = {0};
    DLP_CONTROL step5Ctrl = {};
    DLP_CONTROL blkCtrl = {};
    DLP_CONTROL remCtrl = {};
    uint8_t step6Recv[256] = {0};
    uint8_t entryBuf[25] = {0};
    uint8_t ackBuf[1] = {0};

    if(nullptr == path)
    {
        DEBUG_LOG(APP, ERROR, "path is NULL \r\n");
        return RTN_INVALID_ARG;
    }
    std::ifstream file(path, std::ios::binary | std::ios::in);
    if (!file) {
        DEBUG_LOG(APP, ERROR, "open file %s failed! \r\n", path);
        return RTN_FAIL;
    }
    file.seekg(0, std::ios::end);
    filesize = static_cast<int>(file.tellg());
    file.seekg(0, std::ios::beg);

    if (1024 > filesize)
    {
        DEBUG_LOG(APP, ERROR, "%s file too small (<1024), not normal!\r\n", path);
        return RTN_FAIL;
    }
    buffer = new uint8_t[filesize];
    file.read((char*)buffer, filesize);
    file.close();
    // ---------- Step1: Stop internal patterns ----------
    ret = this->m_dlp_write_data(index, {0x9E, 0x01, 0x00});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    // ---------- Step2: Select flash type ----------
    ret = this->m_dlp_write_data(index, {0xDE, 0xD0});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    // ---------- Step3: Pre-check (DDh) ----------
    precheckSend[0] = 0xDD;
    precheckSend[1] = static_cast<uint8_t>(filesize & 0xFF);
    precheckSend[2] = static_cast<uint8_t>((filesize >> 8) & 0xFF);
    precheckSend[3] = static_cast<uint8_t>((filesize >> 16) & 0xFF);
    precheckSend[4] = static_cast<uint8_t>((filesize >> 24) & 0xFF);

    ret = m_dlp_read_data(index, precheckSend, 5, precheckRecv, 1);
    if (ret < 0 || precheckRecv[0] != 0x00) {
        DEBUG_LOG(APP, ERROR, "Precheck failed, ret=%d val=0x%x \r\n", ret, precheckRecv[0]);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    ret = this->m_dlp_write_data(index, {0xDE, 0xD0});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    // ---------- Step4: Erase flash ----------
    ret = this->m_dlp_write_data(index, {0xE0, 0xAA, 0xBB, 0xCC, 0xDD});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    // Wait erase complete
    for (uint8_t i = 0; i < 200; i++)
    {
        ret = m_dlp_read_data(index, {0xD0}, eraseRecv, 1);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            retStatus = RTN_FAIL;
            goto ERROR_EXIT;
        }
        if (eraseRecv[0] == 0x81)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        if (i == 199)
        {
            DEBUG_LOG(APP, ERROR, "Erase timeout \r\n");
            retStatus = RTN_FAIL;
            goto ERROR_EXIT;
        }
    }
    ret = this->m_dlp_write_data(index, {0xDE, 0xD0});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    // ---------- Step5: Write data using DF/E1/E2 ----------
    m_quotient = filesize / 1024;
    m_remainder = filesize % 1024;
    ret = this->m_dlp_write_data(index, {0xDF, 0x00, 0x04});// DFh 设置块地址和长度
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    memset(&step5Ctrl, 0, sizeof(step5Ctrl));
    step5Ctrl.length = 1025;
    step5Ctrl.data[0] = 0xE1;
    for(int i = 0; i < 1024; i++){
        step5Ctrl.data[i + 1] = buffer[i];
    }
    ret = m_dlp_write_data(index, step5Ctrl);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    
    DEBUG_LOG(APP, INFO, "Write data 1024 success, quotient:%u remainder:%u\r\n", m_quotient, m_remainder);

    for (uint16_t i = 1; i < m_quotient; i++)
    {
        memset(&blkCtrl, 0, sizeof(blkCtrl));
        blkCtrl.length = 1025;
        blkCtrl.data[0] = 0xE2;
        
        for (uint16_t j = 0; j < 1024; j++) {
            blkCtrl.data[j + 1] = buffer[i * 1024 + j];
        }
        ret = m_dlp_write_data(index, blkCtrl);
        if(ret < 0){
            DEBUG_LOG(APP, ERROR, "Write data 1024... failed \r\n");
            retStatus = RTN_FAIL;
            goto ERROR_EXIT;
        }
        DEBUG_LOG(APP, DEBUG,"Write data 1024... success \r\n");
    }

    if (m_remainder != 0)
    {
        m_dlp_write_data(index, {0xDF, static_cast<uint8_t>(m_remainder & 0xFF),
                                 static_cast<uint8_t>((m_remainder >> 8) & 0xFF)});
             
        memset(&remCtrl, 0, sizeof(remCtrl));
        remCtrl.length = m_remainder + 1;
        remCtrl.data[0] = 0xE2;
        for (int j = 0; j < m_remainder; j++) {
            remCtrl.data[j + 1] = buffer[m_quotient * 1024 + j];
        }
        ret = m_dlp_write_data(index, remCtrl);
        if(ret < 0){
            DEBUG_LOG(APP, ERROR, "Write data last block failed \r\n");
            retStatus = RTN_FAIL;
            goto ERROR_EXIT;
        }
        DEBUG_LOG(APP, DEBUG,"Write last block success\r\n");
    }
    //**********************************************************************************************************
    ret = m_dlp_write_data(index, {0xDE, 0xD0});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }

    ret = m_dlp_write_data(index, {0xDF, 0x00, 0x01});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    // ---------- Step6: Read back for verification (E3/E4) ----------
    m_quotient = filesize / 256;
    m_remainder = filesize % 256;
    
    ret = m_dlp_read_data(index, {0xE3}, step6Recv, 256);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }

    for (uint16_t i = 1; i < m_quotient; i++)
    {
        ret = m_dlp_read_data(index, {0xE4}, step6Recv, 256);
        if (ret < 0) {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            retStatus = RTN_FAIL;
            goto ERROR_EXIT;
        }
    }

    if (m_remainder != 0)
    {
        m_dlp_write_data(index, {0xDF, static_cast<uint8_t>(m_remainder & 0xFF),
                                 static_cast<uint8_t>((m_remainder >> 8) & 0xFF)});
             
        ret = m_dlp_read_data(index, {0xE4}, step6Recv, m_remainder);
        if (ret < 0) {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            retStatus = RTN_FAIL;
            goto ERROR_EXIT;
        }
    }

    memset(entryBuf, 0, sizeof(entryBuf));
    entryBuf[0] = WRITE_PATTERN_ORDER_TABLE_ENTRY;
    entryBuf[1] = 0x02;
    ret = m_dlp_write_data(index, entryBuf, 25);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    ret = m_dlp_read_data(index, {0x06}, ackBuf, 1);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    if(0x04 == ackBuf[0]){
        DEBUG_LOG(APP, INFO, "Flash write and verification success\n");
    }else{
        retStatus = RTN_FAIL;
    }
ERROR_EXIT:
    delete[] buffer;
    buffer = nullptr;
    return retStatus;
}

DEV_RTN DEV_DLP::dev_dlp_set_load_timing_value(uint8_t index, uint8_t image_index, uint8_t num)
{
    int ret = 0;
    CHECK_INDEX_RETURN(index);
    if (DLP_4052 == this->dlpType)
    {
        ret = m_dlp_write_data(index, {0x65, 0x00}, 0);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
        usleep(100000);  // 等停止完成
        ret = m_dlp_write_data(index, {0x61, image_index, num}, 0);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
    }
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_get_load_timing_value(uint8_t index, uint32_t& load_time)
{
    int ret = 0;
    CHECK_INDEX_RETURN(index);
    if (DLP_4052 != this->dlpType)
        return RTN_INVALID_ARG;
    uint8_t recvBuf[5];
    ret = m_dlp_read_data(index, {0x0C}, recvBuf, 2);
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    DEBUG_LOG(APP, INFO, "0x0C response:%d %d \r\n", recvBuf[0], recvBuf[1]);

    ret = m_dlp_read_data(index, {0x61}, recvBuf, 5);
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    DEBUG_LOG(APP, INFO, "0x61 response:%d %d %d %d %d\r\n", recvBuf[0], recvBuf[1], recvBuf[2], recvBuf[3],recvBuf[4]);
    if(0x01 != recvBuf[0])
    {
        DEBUG_LOG(APP, ERROR, "recvBuf[0]:%d \r\n", recvBuf[0]);
        return RTN_FAIL;
    }
    load_time = (recvBuf[1]) | (recvBuf[2] << 8) | (recvBuf[3] << 16) | (recvBuf[4] << 24);

    DEBUG_LOG(APP, INFO, "image load timing raw:%f \r\n", static_cast<double>(load_time) / 18667.0);
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_get_status_value(uint8_t index, uint8_t &hwStatus, uint8_t &sysStatus, uint8_t &mainStatus)
{
    CHECK_INDEX_RETURN(index);
    if (DLP_4052 != this->dlpType)
        return RTN_INVALID_ARG;
    int ret = 0;
    const uint8_t commands[3] = {0x20, 0x21, 0x22};
    uint8_t statusValues[3] = {0, 0, 0};
    uint8_t recvBuf[2];
    for (uint8_t i = 0; i < 3; ++i)
    {
        ret = m_dlp_read_data(index, {commands[i]}, recvBuf, 2);
        if (ret < 0 || (0x01 != recvBuf[0]))
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            return RTN_FAIL;
        }
        statusValues[i] = recvBuf[1];
    }
    hwStatus = statusValues[0];
    sysStatus = statusValues[1];
    mainStatus = statusValues[2];
    DEBUG_LOG(APP, INFO, "hwStatus:%d sysStatus:%d mainStatus:%d \r\n", hwStatus, sysStatus, mainStatus);
    return RTN_OKAY;
}