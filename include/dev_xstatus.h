
#ifndef _DEV_XSTATUS_H_
#define _DEV_XSTATUS_H_
#include <sys/ioctl.h>
#include <fstream>
#include <string>
#include <vector>
#include "debug.h"
#include "dev_common.h"

#define PS_TEMP "/sys/bus/iio/devices/iio:device0/in_temp0_ps_temp_raw"
#define PL_TEMP "/sys/bus/iio/devices/iio:device0/in_temp2_pl_temp_raw"

//CPU_GPU------temp
#define CPU_TEMP_PATH "/sys/devices/virtual/thermal/thermal_zone1/temp"
#define GPU_TEMP_PATH "/sys/devices/virtual/thermal/thermal_zone2/temp"
//CPU_GPU------memy
#define CPU_MEMY_PATH "/proc/meminfo"

#define GPU_MEMY_PATH_1 "/sys/class/devfreq/17000000.gpu/max_freq"
#define GPU_MEMY_PATH_2 "/sys/class/devfreq/17000000.gpu/cur_freq"

//orin的核心板的SN编号
#define ORIN_EEPROM_PATCH "/sys/bus/i2c/devices/0-0050/eeprom"
//固态硬盘的SN编号
#define SDD_SN_PATCH    "/sys/block/nvme0n1/device/serial"


#define PROCSTATFILE "/proc/stat"
#define PROCMEMINFOFILE "/proc/meminfo"
#define MAX_NAME 128
#define String_startsWith(s, match) (strstr((s), (match)) == (s))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
typedef struct CPUData_
{
    unsigned long long int totaltime;
    unsigned long long int total_used;
    unsigned long long int systemtime;
    unsigned long long int nicetime;
    unsigned long long int usertime;
    unsigned long long int irqtime;
    unsigned long long int softirq;
} CPUData;

typedef struct
{
    uint16_t  temp_result; //读取寄存器的值
    uint16_t  temp_offset; //读取偏移量
    int32_t  temp_value; //最终的温度值，精确到小数点后3
}TEMP_RESULT_DATA;

#define TEMP_IOCTL_GET_TEMPERATURE _IOW('l', 1, TEMP_RESULT_DATA)//获取参数
#define EXTERNAL_DEV_TEMP "/dev/tmp117_1"

class DEV_XSTATUS
{
public:
    DEV_XSTATUS();
    ~DEV_XSTATUS();

public:
    DEV_RTN dev_xstatus_init();
    int dev_xstatus_get_temp_value(DEV_XSTATUS_TYPE, float&);
    void dev_xstatus_get_orin_sn(std::string&);
    void dev_xstatus_get_ssd_sn(std::string&);


private:
    int m_read_cpu_temp(float&);
    int m_read_gpu_temp(float&);
    int m_read_cpu_memy(float&);
    int m_read_gpu_memy(float&);
    int m_read_tmp117_temp(float&);
private:
    bool isHaveTmp117 = false;
    int fd_ps_temp = -1;
    int fd_pl_temp = -1;
    int fd_sensor_temp = -1;
    int fd_tmp117_temp=-1;
};




#endif
