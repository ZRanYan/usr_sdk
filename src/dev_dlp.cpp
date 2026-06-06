
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
    DEBUG_LOG(APP, INFO, "total dlp num:%d \r\n", dlpSet.fdNum);
    return ((0 == dlpSet.fdNum)?RTN_FAIL:RTN_OKAY);
}

int DEV_DLP::m_dlp_write_data(uint8_t index,DLP_WRITE& val,std::initializer_list<uint8_t> list)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return -1;
    }

    if (list.size() > sizeof(val.data))
        return -1;
    memset(&val, 0, sizeof(val));
    val.length = static_cast<uint16_t>(list.size());
    memcpy(val.data, list.begin(), val.length);
    return ioctl(this->dlpSet.fd[index], DLP_IOCTL_SET_DATA, &val);
}

void DEV_DLP::dev_dlp_get_ver(std::string& ver)
{
    ver = m_ver;
    return;
}
DEV_RTN DEV_DLP::dev_dlp_get_temp_value(uint8_t index, float& temp)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return RTN_INVALID_ARG;
    }

    int ret = 0;
    uint16_t raw = 0;
    memset(&m_dlpReadValue, 0, sizeof(m_dlpReadValue));
    
    m_dlpReadValue.sendLength=1;
    m_dlpReadValue.sendData[0]=0xD6;
    m_dlpReadValue.recvLength = 2;
    ret = ioctl(dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "error ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    raw = (m_dlpReadValue.recvData[1] << 8) | m_dlpReadValue.recvData[0];
    temp = (raw & 0x07FF) / 10.0f;
    temp *= ((1 == ((raw >> 11) & 0x01))?-1:1);
    DEBUG_LOG(APP, INFO, "temp:%f \r\n", temp);
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_set_current_value(uint8_t index, uint16_t current)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return RTN_INVALID_ARG;
    }

    int ret = 0;
    m_dlpReadValue.sendLength = 1;
    m_dlpReadValue.sendData[0] = READ_RGB_LED_PWM;
    m_dlpReadValue.recvLength = 6;
    ret = ioctl(dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    m_dlpReadValue.recvData[4] = (current & 0xFF);
    m_dlpReadValue.recvData[5] = current >> 8;
    m_dlpWriteValue.length = 7;
    m_dlpWriteValue.data[0] = WRITE_RGB_LED_PWM;
    memcpy(&m_dlpWriteValue.data[1], &m_dlpReadValue.recvData[0], m_dlpWriteValue.length - 1);
    ret = ioctl(dlpSet.fd[index], DLP_IOCTL_SET_DATA, &m_dlpWriteValue);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_set_current_all_value(uint16_t current)
{
    DEV_RTN ret = RTN_OKAY;
    uint8_t i=0;
    for(i=0;i<DLP_MAX_DEVICES;i++)
    {
        if(1 == this->dlpSet.id[i])
        {
            ret = dev_dlp_set_current_value(i, current);
            if(ret != RTN_OKAY)
            {
                DEBUG_LOG(APP, ERROR, "set failed!\r\n");
                break;
            }
        }
    }
    return ret;
}


DEV_RTN DEV_DLP::dev_dlp_get_pwm_value(uint8_t index, uint16_t& pwm)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return RTN_INVALID_ARG;
    }

    memset(&m_dlpReadValue, 0, sizeof(m_dlpReadValue));
    m_dlpReadValue.sendLength = 1;
    m_dlpReadValue.sendData[0] = READ_RGB_LED_PWM;
    m_dlpReadValue.recvLength = 6;
    int ret = ioctl(dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    pwm=(m_dlpReadValue.recvData[5]<<8) | m_dlpReadValue.recvData[4];
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_set_group_info_value(uint8_t index, uint8_t write_ctl,const DEV_DLP_PATTERN_GROUP_INFO& sel_pack)
{
    memset(&m_dlpWriteValue, 0, sizeof(m_dlpWriteValue));
    this->m_dlpWriteValue.length = 25;
    this->m_dlpWriteValue.data[0] = WRITE_PATTERN_ORDER_TABLE_ENTRY;
    this->m_dlpWriteValue.data[1] = write_ctl;              // 1
    this->m_dlpWriteValue.data[2] = (sel_pack.group_id & 0xFF); // 1
    this->m_dlpWriteValue.data[3] = sel_pack.num_pattern;         // 4
    this->m_dlpWriteValue.data[4] = 0x04;
    this->m_dlpWriteValue.data[5] = 0x00;
    this->m_dlpWriteValue.data[6] = 0x00;
    this->m_dlpWriteValue.data[7] = 0x00;
    this->m_dlpWriteValue.data[8] = 0x00;
    this->m_dlpWriteValue.data[9] = 0x00;
    this->m_dlpWriteValue.data[10] = 0x00;
    this->m_dlpWriteValue.data[11] = 0x00;
    this->m_dlpWriteValue.data[12] = 0x00;
    this->m_dlpWriteValue.data[13] = sel_pack.exp_s.exp & 0xFF;
    this->m_dlpWriteValue.data[14] = (sel_pack.exp_s.exp >> 8) & 0xFF;
    this->m_dlpWriteValue.data[15] = (sel_pack.exp_s.exp >> 16) & 0xFF;
    this->m_dlpWriteValue.data[16] = (sel_pack.exp_s.exp >> 24) & 0xFF;
    this->m_dlpWriteValue.data[17] = sel_pack.exp_s.pre_exp & 0xFF;
    this->m_dlpWriteValue.data[18] = (sel_pack.exp_s.pre_exp >> 8) & 0xFF;
    this->m_dlpWriteValue.data[19] = (sel_pack.exp_s.pre_exp >> 16) & 0xFF;
    this->m_dlpWriteValue.data[20] = (sel_pack.exp_s.pre_exp >> 24) & 0xFF;
    this->m_dlpWriteValue.data[21] = sel_pack.exp_s.post_exp & 0xFF;
    this->m_dlpWriteValue.data[22] = (sel_pack.exp_s.post_exp >> 8) & 0xFF;
    this->m_dlpWriteValue.data[23] = (sel_pack.exp_s.post_exp >> 16) & 0xFF;
    this->m_dlpWriteValue.data[24] = (sel_pack.exp_s.post_exp >> 24) & 0xFF;
    int ret = ioctl(dlpSet.fd[index], DLP_IOCTL_SET_DATA, &this->m_dlpWriteValue);
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
    memset(&m_dlpReadValue, 0, sizeof(m_dlpReadValue));
    m_dlpReadValue.sendLength = 2;
    m_dlpReadValue.sendData[0] = READ_PATTERN_ORDER_TABLE_ENTRY;
    m_dlpReadValue.sendData[1] = param.group_id;
    m_dlpReadValue.recvLength = 24;
    ret = ioctl(dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    param.num_pattern=m_dlpReadValue.recvData[1];
    param.exp_s.exp=(m_dlpReadValue.recvData[14]<<24) | (m_dlpReadValue.recvData[13]<<16) |(m_dlpReadValue.recvData[12]<<8) | (m_dlpReadValue.recvData[11]);
    param.exp_s.pre_exp=(m_dlpReadValue.recvData[18]<<24) | (m_dlpReadValue.recvData[17]<<16) |(m_dlpReadValue.recvData[16]<<8) | (m_dlpReadValue.recvData[15]);
    param.exp_s.post_exp=(m_dlpReadValue.recvData[22]<<24) | (m_dlpReadValue.recvData[21]<<16) |(m_dlpReadValue.recvData[20]<<8) | (m_dlpReadValue.recvData[19]);
    if(0 == param.exp_s.exp)
    {
        return RTN_FAIL;
    }
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_trigonce_value(uint8_t index)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return RTN_INVALID_ARG;
    }
    int ret = 0;
    ret = this->m_dlp_write_data(index, this->m_dlpWriteValue, {WRITE_INTERNAL_PATTERN_CONTROL,0x00,0x00});
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_trigonce_new_set(uint8_t index)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return RTN_INVALID_ARG;
    }
    int ret = 0;
    ret = ioctl(this->dlpSet.fd[index], DLP_TRIG_ONCE_ASSIGN, NULL);
    if (ret < 0)
    {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_get_min_expo_value(uint8_t index, DEV_DLP_EXP& param)
{
    int ret;
    memset(&m_dlpReadValue, 0, sizeof(m_dlpReadValue));
    m_dlpReadValue.sendLength = 7;
    m_dlpReadValue.sendData[0] = READ_MIN_EXPO;
    m_dlpReadValue.sendData[1] = 0x01;
    m_dlpReadValue.sendData[2] = 0x02;
    m_dlpReadValue.sendData[3] = (param.exp&0xFF);
    m_dlpReadValue.sendData[4] = (param.exp>>8)&0xFF;
    m_dlpReadValue.sendData[5] = (param.exp>>16)&0xFF;
    m_dlpReadValue.sendData[6] = (param.exp>>24)&0xFF;
    m_dlpReadValue.recvLength = 13;
    ret = ioctl(dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
    if(ret<0){
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    param.exp=(m_dlpReadValue.recvData[4]<<24) | (m_dlpReadValue.recvData[3]<<16) |(m_dlpReadValue.recvData[2]<<8) | (m_dlpReadValue.recvData[1]);
    param.pre_exp=(m_dlpReadValue.recvData[8]<<24) | (m_dlpReadValue.recvData[7]<<16) |(m_dlpReadValue.recvData[6]<<8) | (m_dlpReadValue.recvData[5]);
    param.post_exp=(m_dlpReadValue.recvData[12]<<24) | (m_dlpReadValue.recvData[11]<<16) |(m_dlpReadValue.recvData[10]<<8) | (m_dlpReadValue.recvData[9]);
    if(0 == param.exp)
    {
        DEBUG_LOG(APP, ERROR, "param.exp : 0 \r\n");
        return RTN_FAIL;
    }
    return RTN_OKAY;
}
DEV_RTN DEV_DLP::dev_dlp_set_delay_invert_value(uint8_t index, bool isInvert, uint32_t delayUs)
{
	if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return RTN_INVALID_ARG;
    }
    memset(&m_dlpWriteValue, 0, sizeof(m_dlpWriteValue));
    m_dlpWriteValue.length=6;
    m_dlpWriteValue.data[0]=0x92;
    if(isInvert){
        m_dlpWriteValue.data[1]=0x07;
    }else{
        m_dlpWriteValue.data[1]=0x03;
    }
    if(0 == delayUs){
        m_dlpWriteValue.data[2]=0x00;
        m_dlpWriteValue.data[3]=0x00;
        m_dlpWriteValue.data[4]=0x00;
        m_dlpWriteValue.data[5]=0x00;
    }else{
        m_dlpWriteValue.data[2]=(delayUs) & 0xFF;
        m_dlpWriteValue.data[3]=(delayUs >> 8) & 0xFF;
        m_dlpWriteValue.data[4]=(delayUs >> 16) & 0xFF;
        m_dlpWriteValue.data[5]=(delayUs >> 24) & 0xFF;
    }
    int ret = ioctl(dlpSet.fd[index], DLP_IOCTL_SET_DATA, &m_dlpWriteValue);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_set_long_short_flip_value(uint8_t index, bool long_flip, bool short_flip)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return RTN_INVALID_ARG;
    }

    int ret = 0;
    uint8_t result  = 0;
    result |= (static_cast<int>(short_flip) << 2);  // 放到第三位（bit 2）
    result |= (static_cast<int>(long_flip) << 1); // 放到第二位（bit 1）
    ret = m_dlp_write_data(index, m_dlpWriteValue, {0x14, result});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        return RTN_FAIL;
    }
    return RTN_OKAY;
}

DEV_RTN DEV_DLP::dev_dlp_update_falsh_value(uint8_t index, const char* path)
{
    if (index >= DLP_MAX_DEVICES)
    {
        DEBUG_LOG(APP, ERROR, "invalid index: %u (>= DLP_MAX_DEVICES %u)\n", index, DLP_MAX_DEVICES);
        return RTN_INVALID_ARG;
    }

    DEV_RTN retStatus = RTN_OKAY;
    int ret = 0;
    int filesize = 0;
    uint8_t *buffer = nullptr;
    uint16_t m_quotient;
    uint16_t m_remainder;
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
    ret = this->m_dlp_write_data(index, m_dlpWriteValue,{0x9E,0x01,0x00});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    // ---------- Step2: Select flash type ----------
    ret = this->m_dlp_write_data(index, m_dlpWriteValue,{0xDE,0xD0});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    // ---------- Step3: Pre-check (DDh) ----------
    memset(&m_dlpReadValue, 0, sizeof(m_dlpReadValue));
    m_dlpReadValue.sendLength = 5;
    m_dlpReadValue.sendData[0] = 0xDD;
    m_dlpReadValue.sendData[1] = filesize & 0xFF;
    m_dlpReadValue.sendData[2] = (filesize >> 8) & 0xFF;
    m_dlpReadValue.sendData[3] = (filesize >> 16) & 0xFF;
    m_dlpReadValue.sendData[4] = (filesize >> 24) & 0xFF;
    m_dlpReadValue.recvLength = 1;
    ret = ioctl(dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
    if (ret < 0 || m_dlpReadValue.recvData[0] != 0x00) {
        DEBUG_LOG(APP, ERROR, "Precheck failed, ret=%d val=0x%x \r\n", ret, m_dlpReadValue.recvData[0]);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    ret = this->m_dlp_write_data(index, m_dlpWriteValue,{0xDE,0xD0});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    // ---------- Step4: Erase flash ----------
    ret = this->m_dlp_write_data(index, m_dlpWriteValue, {0xE0, 0xAA, 0xBB, 0xCC, 0xDD});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    // Wait erase complete
    for (uint8_t i = 0; i < 200; i++)
    {
        memset(&m_dlpReadValue, 0, sizeof(m_dlpReadValue));
        m_dlpReadValue.sendLength = 1;
        m_dlpReadValue.sendData[0] = 0xD0;
        m_dlpReadValue.recvLength = 1;
        ret = ioctl(dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
        if (ret < 0)
        {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            retStatus = RTN_FAIL;
            goto ERROR_EXIT;
        }
        if (m_dlpReadValue.recvData[0] == 0x81)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        if (i == 199)
        {
            DEBUG_LOG(APP, ERROR, "Erase timeout \r\n");
            retStatus = RTN_FAIL;
            goto ERROR_EXIT;
        }
    }
    ret = this->m_dlp_write_data(index, m_dlpWriteValue, {0xDE, 0xD0});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    // ---------- Step5: Write data using DF/E1/E2 ----------
    m_quotient = filesize / 1024;
    m_remainder = filesize % 1024;
    ret = this->m_dlp_write_data(index, m_dlpWriteValue, {0xDF, 0x00, 0x04});// DFh 设置块地址和长度
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    memset(&m_dlpWriteValue,0,sizeof(m_dlpWriteValue));
    m_dlpWriteValue.length=1025;
    m_dlpWriteValue.data[0]=0xE1;
    for(int i=0;i<1024;i++){
        m_dlpWriteValue.data[i+1]=buffer[i];
    }
    ret = ioctl(this->dlpSet.fd[index], DLP_IOCTL_SET_DATA, &m_dlpWriteValue);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    
    DEBUG_LOG(APP, INFO, "Write data 1024 success, quotient:%u remainder:%u\r\n", m_quotient, m_remainder);

    for (uint16_t i = 1; i < m_quotient; i++)
    {
        memset(&m_dlpWriteValue, 0, sizeof(m_dlpWriteValue));
        m_dlpWriteValue.length = 1025;
        m_dlpWriteValue.data[0] = 0xE2;
        
        for (uint16_t j = 0; j < 1024; j++) {
            m_dlpWriteValue.data[j+1]=buffer[i*1024+j];
        }
        ret = ioctl(dlpSet.fd[index], DLP_IOCTL_SET_DATA, &m_dlpWriteValue);
        if(ret<0){
            DEBUG_LOG(APP, ERROR, "Write data 1024... failed \r\n");
            retStatus = RTN_FAIL;
            goto ERROR_EXIT;
        }
        DEBUG_LOG(APP, DEBUG,"Write data 1024... success \r\n");
    }

    if (m_remainder != 0)
    {
        m_dlp_write_data(index, m_dlpWriteValue,
            {0xDF, static_cast<uint8_t>(m_remainder & 0xFF),
             static_cast<uint8_t>((m_remainder >> 8) & 0xFF)});
             
        memset(&m_dlpWriteValue, 0, sizeof(m_dlpWriteValue));
        m_dlpWriteValue.length = m_remainder + 1;
        m_dlpWriteValue.data[0] = 0xE2;
        for (int j = 0; j < m_remainder; j++) {
            m_dlpWriteValue.data[j+1] = buffer[m_quotient*1024 + j];
        }
        ret = ioctl(dlpSet.fd[index], DLP_IOCTL_SET_DATA, &m_dlpWriteValue);
        if(ret<0){
            DEBUG_LOG(APP, ERROR, "Write data last block failed \r\n");
            retStatus = RTN_FAIL;
            goto ERROR_EXIT;
        }
        DEBUG_LOG(APP, DEBUG,"Write last block success\r\n");
    }
    //**********************************************************************************************************
    ret = m_dlp_write_data(index, m_dlpWriteValue, {0xDE, 0xD0});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }

    ret = m_dlp_write_data(index, m_dlpWriteValue, {0xDF, 0x00, 0x01});
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    // ---------- Step6: Read back for verification (E3/E4) ----------
    m_quotient = filesize / 256;
    m_remainder = filesize % 256;
    memset(&m_dlpReadValue, 0, sizeof(m_dlpReadValue));
    m_dlpReadValue.sendLength = 1;
    m_dlpReadValue.sendData[0] = 0xE3;
    m_dlpReadValue.recvLength = 256;
    ret = ioctl(dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }

    for (uint16_t i = 1; i < m_quotient; i++)
    {
        memset(&m_dlpReadValue, 0, sizeof(m_dlpReadValue));
        m_dlpReadValue.sendLength = 1;
        m_dlpReadValue.sendData[0] = 0xE4;
        m_dlpReadValue.recvLength = 256;
        
        ret = ioctl(dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
        if (ret < 0) {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            retStatus = RTN_FAIL;
            goto ERROR_EXIT;
        }
    }

    if (m_remainder != 0)
    {
        m_dlp_write_data(index, m_dlpWriteValue,
            {0xDF, static_cast<uint8_t>(m_remainder & 0xFF),
             static_cast<uint8_t>((m_remainder >> 8) & 0xFF)});
             
        memset(&m_dlpReadValue, 0, sizeof(m_dlpReadValue));
        m_dlpReadValue.sendLength = 1;
        m_dlpReadValue.sendData[0] = 0xE4;
        m_dlpReadValue.recvLength = m_remainder;
        ret = ioctl(dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
        if (ret < 0) {
            DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
            retStatus = RTN_FAIL;
            goto ERROR_EXIT;
        }
    }

    memset(&m_dlpWriteValue, 0, sizeof(m_dlpWriteValue));
    m_dlpWriteValue.length = 25;
    m_dlpWriteValue.data[0] = WRITE_PATTERN_ORDER_TABLE_ENTRY;
    m_dlpWriteValue.data[1] = 0x02;
    ret = ioctl(dlpSet.fd[index], DLP_IOCTL_SET_DATA, &m_dlpWriteValue);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    memset(&m_dlpReadValue, 0, sizeof(m_dlpReadValue));
    m_dlpReadValue.sendLength = 1;
    m_dlpReadValue.sendData[0] = 0x06;
    m_dlpReadValue.recvLength = 1;
    ret = ioctl(dlpSet.fd[index], DLP_COMMAND_CONTROL, &m_dlpReadValue);
    if (ret < 0) {
        DEBUG_LOG(APP, ERROR, "ret:%d \r\n", ret);
        retStatus = RTN_FAIL;
        goto ERROR_EXIT;
    }
    if(0x04 == m_dlpReadValue.recvData[0]){
        DEBUG_LOG(APP, INFO, "Flash write and verification success\n");
    }else{
        retStatus = RTN_FAIL;
    }
ERROR_EXIT:
    delete[] buffer;
    buffer = nullptr;
    return retStatus;
}