#include "test_func.h"
int main(int argc, const char *argv[])
{   
    DEV_SENSOR_ATTRIBUTE sensor = 
    {
        .index = CAM_INDEX_ALL,
        .type = SENSOR_SC535,
        .sensorNum = 1,
        .bufferNum = 8,
        .oot       = 10,
        .usr_data = nullptr
    };
    FrameInfo mFrameInfo = { //唤醒队列需要保存的数据
        .stride = 4928,
        .height = 2048,
        .bit_mode = 10,
        .buffNum = 42
    };
    CAM_DEV *dev = new CAM_DEV();
    CmdContext ctx;
    ctx.dev = dev;
    ctx.boardType = 1;
    ctx.m_pram.SET_NUM = 21;
    ctx.m_pram.g_dlp_index[0] = 0;
    ctx.m_pram.g_dlp_index[1] = 0;
    ctx.m_pram.g_pic_index[0] = 0;
    ctx.m_pram.g_pic_index[1] = 0;
    ctx.m_pram.SET_PIC_NUM = 21;
    ctx.m_pram.g_device_type = ONE_SENSOR_ONE_MACHINE;
    dev_judgment_node(ctx.m_pram, sensor); //根据驱动节点判断下位机类型
    auto cmdTable = build_cmd_table();
    if(2 == argc)
    {
        std::string arg = argv[1];
        if(arg == "0")
        {
            ctx.m_pram.g_is_debug_test_flag = DLP_SELF_TEST;
            sensor.usr_data = &ctx.m_pram;
            dev->dev_init(sensor, ORIN_DEV);
            save_test_log_result(dev, &ctx.m_pram);
            delete dev;   
            dev = nullptr;
            exit(0);
        }
    }

    ctx.m_pram.g_rb = initRingBuffer(&mFrameInfo);
    if (!ctx.m_pram.g_rb) {
        printf("初始化失败\n");
        return -1;
    }
    startConsumerThread(ctx.m_pram.g_rb);
    sensor.usr_data = &ctx.m_pram;
    dev->dev_init(sensor, ORIN_DEV);
    while(1)
    {
        cmd_print_help(cmdTable);
        cout<<":";
        string line;
        getline(cin, line);
        stringstream ss(line);
        vector<string> argv;
        string temp;
        while (ss >> temp)
        {
            argv.push_back(temp);
        }
        ctx.argc = argv.size();
        if (ctx.argc == 0)
            continue;
        ctx.argv = argv;
        if(-1 == stoi(argv[0]))
            break;
        execute_cmd(cmdTable, ctx);
    }
    cout<<"退出测试程序"<<endl;
    destroyRingBuffer(ctx.m_pram.g_rb);
    if(NULL != dev)
    {
        delete dev;   
        dev = nullptr; 
    }
    return 0;
}
