
#include "../include/dev_xstatus.h"


DEV_XSTATUS::DEV_XSTATUS()
{
    isHaveTmp117 = false;
}

DEV_XSTATUS::~DEV_XSTATUS()
{
    if(-1 != this->fd_tmp117_temp && this->isHaveTmp117)
    {
        close(this->fd_tmp117_temp);
        this->fd_tmp117_temp = -1;
    }
}

DEV_RTN DEV_XSTATUS::dev_xstatus_init()
{
    this->isHaveTmp117 = (access("/dev/tmp117_1", F_OK) == 0);
    if (this->isHaveTmp117)
    {
        this->fd_tmp117_temp = open(EXTERNAL_DEV_TEMP, O_RDWR);
        if (0 > this->fd_tmp117_temp)
        {
            DEBUG_LOG(APP, ERROR, " open %s failed! \r\n", EXTERNAL_DEV_TEMP);
            return RTN_FAIL;
        }
        else
        {
            DEBUG_LOG(APP, INFO, " open %s ok \r\n", EXTERNAL_DEV_TEMP);
        }
    }
    return RTN_OKAY;
}

int DEV_XSTATUS::m_read_cpu_temp(float& value)
{
    FILE *fp = fopen(CPU_TEMP_PATH, "r");
    if (!fp) {
        perror("Failed to open temperature file");
        return -1;
    }
    int temp_milli = 0;
    fscanf(fp, "%d", &temp_milli);
    fclose(fp);
    value = (float)(temp_milli / 1000.0f);
    return 0;
}

int DEV_XSTATUS::m_read_gpu_temp(float& value)
{
    FILE *fp = fopen(GPU_TEMP_PATH, "r");
    if (!fp) {
        perror("Failed to open temperature file");
        return -1;
    }
    int temp_milli = 0;
    fscanf(fp, "%d", &temp_milli);
    fclose(fp);
    value = (float)(temp_milli / 1000.0f);  // 转换为摄氏度
    return 0;
}
int DEV_XSTATUS::m_read_cpu_memy(float& data)
{
    FILE *fp = fopen(CPU_MEMY_PATH, "r");  // /proc/meminfo
    if (!fp) {
        perror("Failed to open /proc/meminfo");
        return -1;
    }

    long mem_total = 0, mem_free = 0, buffers = 0, cached = 0;
    char label[32];
    long value;

    while (fscanf(fp, "%31s %ld kB\n", label, &value) == 2) {
        if (strcmp(label, "MemTotal:") == 0) mem_total = value;
        else if (strcmp(label, "MemFree:") == 0) mem_free = value;
        else if (strcmp(label, "Buffers:") == 0) buffers = value;
        else if (strcmp(label, "Cached:") == 0) cached = value;
    }
    fclose(fp);
    if (mem_total == 0) return -1; // 避免除零
    long used = mem_total - (mem_free + buffers + cached);
    data = (float)used / (float)mem_total * 100.0f;  // 百分比
    return 0;
}
int DEV_XSTATUS::m_read_gpu_memy(float& value)
{
    FILE* fp_max = fopen(GPU_MEMY_PATH_1, "r");
    FILE* fp_cur = fopen(GPU_MEMY_PATH_2, "r");
    if (!fp_max || !fp_cur) { perror("Failed to open GPU freq files");
        if(fp_max) fclose(fp_max);
        if(fp_cur) fclose(fp_cur);
        return -1;
    }
    long max_freq = 0, cur_freq = 0;
    fscanf(fp_max, "%ld", &max_freq);
    fscanf(fp_cur, "%ld", &cur_freq);
    fclose(fp_max);
    fclose(fp_cur);
    if (max_freq == 0)
        return -1;
    value =  (float)cur_freq / (float)max_freq * 100.0f; // 百分比
    return 0;
}

int DEV_XSTATUS::m_read_tmp117_temp(float &value)
{
    TEMP_RESULT_DATA set;
    if (this->isHaveTmp117)
    {
        if (0 != ioctl(this->fd_tmp117_temp, TEMP_IOCTL_GET_TEMPERATURE, &set))
        {
            return -1;
        }
        value = set.temp_value / 1000.0f;
    }
    else
    {
        value = 0.00f;
    }
    return 0;
}

int DEV_XSTATUS::dev_xstatus_get_temp_value(DEV_XSTATUS_TYPE type, float& value)
{
    int ret = 0;
    switch(type)
    {
        case CPU_TEMP_TYPE:
            ret = this->m_read_cpu_temp(value);
            break;
        case GPU_TEMP_TYPE:
            ret = this->m_read_gpu_temp(value);
            break;
        case CPU_MEMY_TYPE:
            ret = this->m_read_cpu_memy(value);
            break;
        case GPU_MEMY_TYPE:
            ret = this->m_read_gpu_memy(value);
            break;
        case TMP117_TEMP_TYPE:
            ret = this->m_read_tmp117_temp(value);
            break;
        default:
            ret = -1;
            break;
    }
    return ret;
}

void DEV_XSTATUS::dev_xstatus_get_orin_sn(std::string& sn)
{
    std::size_t offset = 0x14;
    std::size_t length = 18;
    std::ifstream file(ORIN_EEPROM_PATCH, std::ios::binary);
    if (!file)
    {
        return;
    }
    file.seekg(offset, std::ios::beg);
    if (!file)
    {
        return;
    }
    std::vector<char> buffer(length);
    file.read(buffer.data(), length);
    std::size_t bytesRead = file.gcount();
    sn = std::string(buffer.data(), bytesRead);
    return;
}

void DEV_XSTATUS::dev_xstatus_get_ssd_sn(std::string& sn)
{
    std::ifstream file(SDD_SN_PATCH);
    if (!file)
    {
        return;
    }
    std::getline(file, sn);
    return;
}
