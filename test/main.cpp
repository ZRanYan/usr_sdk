#include "test_func.h"
int main(int argc, const char *argv[])
{
    DEV_SENSOR_ATTRIBUTE sensor =
    {
        .index = CAM_INDEX_ALL,
        .type = SENSOR_OG02C1B,
        .x = 0,
        .y = 0,
        .w = 0,
        .h = 0,
        .sensorNum = 2,
        .v4l_sub_slot = 4, //gmsl相机上配置4
        .bufferNum = 8,
        .oot = 2500,
        .testPicMode = 0, //
        .usr_data = nullptr
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
    // dev_judgment_node(ctx.m_pram, sensor); //根据驱动节点判断下位机类型
    printf("sensorNum:%d  type:%d\r\n", sensor.sensorNum, sensor.type);
    auto cmdTable = build_cmd_table();
    if(1 < argc)
    {
        std::string arg = argv[1];
        sensor.usr_data = &ctx.m_pram;
        dev->dev_init(sensor, ORIN_DEV);
        if(arg == "0")
        {
            ctx.m_pram.g_is_debug_test_flag = DLP_SELF_TEST;
            save_test_log_result(dev, &ctx.m_pram);
        }
        else if(arg == "d")
        {
            ctx.m_pram.g_is_debug_test_flag = USR_DEBUG_TEST;
            ctx.argc = 1;
            ctx.argv.clear();
            ctx.argv.push_back("21");
            execute_cmd(cmdTable, ctx);
            // ctx.argc = 2;
            // ctx.argv.clear();
            // ctx.argv.push_back("15");
            // ctx.argv.push_back("288");
            // execute_cmd(cmdTable, ctx);
            sleep(3);
            ctx.argc = 1;
            ctx.argv.clear();
            ctx.argv.push_back("24");
            execute_cmd(cmdTable, ctx);
            while(1)
            {
                sleep(20);
            }
        }
        delete dev;
        dev = nullptr;
        exit(0);
    }

    ctx.m_pram.g_rb = initRingBuffer(&ctx.m_pram.g_frame_set);
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
