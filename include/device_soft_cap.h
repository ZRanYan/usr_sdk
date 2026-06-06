
#ifndef DEVICE_SOFT_CAP_H_
#define DEVICE_SOFT_CAP_H_

typedef unsigned char uint8_t;

typedef struct 
{
    bool isEnableSensor;
    bool isEnableDlp;
    bool isEnableIo;
    bool isEnableXst;
    bool isEnableLed;
}DEV_SOFT_CAPABILITY;

static DEV_SOFT_CAPABILITY orin_dev = 
{
    .isEnableSensor = 1,
    .isEnableDlp = 1,
    .isEnableIo = 1,
    .isEnableXst = 1,
    .isEnableLed = 1,
};

static DEV_SOFT_CAPABILITY nxp_dev = 
{
    .isEnableSensor = 1,
    .isEnableDlp = 1,
    .isEnableIo = 1,
    .isEnableXst = 1,
    .isEnableLed = 1,
};

static DEV_SOFT_CAPABILITY xilinx_dev = 
{
    .isEnableSensor = 1,
    .isEnableDlp = 0,
    .isEnableIo = 1,
    .isEnableXst = 1,
    .isEnableLed = 1
};

static DEV_SOFT_CAPABILITY orin_ext_dev = 
{
    .isEnableSensor = 1,
    .isEnableDlp = 0,
    .isEnableIo = 0,
    .isEnableXst = 0,
    .isEnableLed = 0
};

DEV_SOFT_CAPABILITY *g_cap = &orin_dev;





#endif