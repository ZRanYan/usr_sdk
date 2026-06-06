#include "test_func.h"

static std::string format_string(const char *fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return std::string(buf);
}
void print_system_status(bool is_dual,
                         EMBEDDED_DEVICE_TYPE device_type,
                         USR_DEBUG_TEST_TYPE debug_type, std::ofstream *ofs)
{
    // 选择输出目标
    bool use_file = (ofs && ofs->is_open());
    // 封装一个小宏（避免重复写判断）
#define LOG_OUT(fmt, ...)                                  \
    do {                                                   \
        if (use_file)                                      \
            (*ofs) << format_string(fmt, ##__VA_ARGS__);   \
        else                                               \
            printf(fmt, ##__VA_ARGS__);                    \
    } while (0)

    // 辅助格式化函数（下面实现）
    auto dual_str = is_dual ? "true (双目)" : "false (单目)";

    const char *device_str = "UNKNOWN DEVICE TYPE";
    switch (device_type)
    {
        case ONE_SENSOR_ONE_MACHINE:
            device_str = "ONE_SENSOR_ONE_MACHINE (单目单光机)";
            break;
        case ONE_SENSOR_FOUR_MACHINE:
            device_str = "ONE_SENSOR_FOUR_MACHINE (单目四光机)";
            break;
        case TWO_SENSOR_ONE_MACHINE:
            device_str = "TWO_SENSOR_ONE_MACHINE (双目单光机)";
            break;
        default:
            break;
    }

    const char *debug_str = "UNKNOWN DEBUG TYPE";
    switch (debug_type)
    {
        case NORMAL_OPERATING_MODE:
            debug_str = "NORMAL_OPERATING_MODE (正常操作模式)";
            break;
        case DLP_RGB_HYBRID_FLASH:
            debug_str = "DLP_RGB_HYBRID_FLASH (混合闪)";
            break;
        case DLP_FLASH_SEPARATE:
            debug_str = "DLP_FLASH_SEPARATE (DLP单独闪)";
            break;
        case DLP_SELF_TEST:
            debug_str = "DLP_SELF_TEST (半成品自动化测试)";
            break;
        default:
            break;
    }
    LOG_OUT("========== System Status ==========\n");
    LOG_OUT("g_is_dual            : %s\n", dual_str);
    LOG_OUT("g_device_type        : %s\n", device_str);
    LOG_OUT("g_is_debug_test_flag : %s\n", debug_str);
    LOG_OUT("===================================\n");

#undef LOG_OUT
}

void dev_print_version_info(CAM_DEV *dev, DEV_VERSION_INFO_STRUCT &ver, bool isPrint)
{
    dev->dev_get_version(ver);
    if (true == isPrint)
    {
        cout << "Commit ID : " << ver.commitId << "\n";
        cout << "Branch    : " << ver.gitBranch << "\n";
        cout << "Date      : " << ver.date << "\n";
        cout << "Version   : " << ver.version << "\n";
        cout << "orin_sn   : " << ver.orin_sn << "\n";
        cout << "ssd_sn    : " << ver.ssd_sn << "\n";
    }
    return;
}
void dev_set_log_print(CAM_DEV *dev)
{
    DEV_LOG_SET_PARAM log = {
        .level = LOG_DEBUG,
        .module = LOG_ALL,
        // .log_storage = LOG_PRINT_FLASH|LOG_PRINT_TERMINAL,
        .log_storage = LOG_PRINT_FLASH,
    };
    dev->dev_set_debug_log(log);
}

void dev_get_sensor_ver(CAM_DEV *dev, string &ver, bool isPrint)
{
    dev->dev_sensor_get_version(ver);
    if(true == isPrint)
    {
        cout<<"sensor Version : "<<ver<<endl;
    }
}
void sensor_set_gain(CAM_DEV *dev, BOARD_TYPE type, uint16_t gain)
{
    if(CAM_TWO == type)
    {
        dev->dev_sensor_set_gain(CAM_INDEX_ALL,gain);
    }
    else
    {
        dev->dev_sensor_set_gain(CAM_INDEX_ONE,gain);
    }
    cout<<endl;
}

void sensor_set_hv_flip(CAM_DEV *dev, BOARD_TYPE type, bool isEnable)
{
    if(CAM_TWO == type)
    {
        dev->dev_sensor_set_hflip(CAM_INDEX_ALL, isEnable);
        dev->dev_sensor_set_vflip(CAM_INDEX_ALL, isEnable);
    }
    else
    {
        dev->dev_sensor_set_hflip(CAM_INDEX_ONE, isEnable);
        dev->dev_sensor_set_vflip(CAM_INDEX_ONE, isEnable);
    }
    cout<<endl;
}

void sensor_set_black(CAM_DEV *dev, BOARD_TYPE type, uint16_t value)
{
    dev->dev_sensor_set_black(CAM_INDEX_ALL, value);
    if(CAM_TWO == type)
    {
        dev->dev_sensor_set_black(CAM_INDEX_ALL, value);
    }
    else
    {
        dev->dev_sensor_set_black(CAM_INDEX_ONE, value);
    }
}

void sensor_get_temp_value(CAM_DEV *dev)
{
    float temp = 0.0f;
    dev->dev_sensor_get_temp(CAM_INDEX_ONE, temp);
    cout<<"temp return:"<<temp<<endl;
}

int producerPut(RingBuffer* rb, const uint8_t* rawData, size_t dataLen, char *fileName)
{
    pthread_mutex_lock(&rb->queue_mutex);
    while (rb->count == rb->info.buffNum) {
        pthread_cond_wait(&rb->cond_full, &rb->queue_mutex);
    }
    int idx = rb->writeIndex;
    size_t frameSize = (size_t)rb->info.stride * rb->info.height;
    if (dataLen != frameSize) { //检查存放数据的大小是否合适
        pthread_mutex_unlock(&rb->queue_mutex);
        return -1;
    }
    pthread_mutex_lock(&rb->buffers[idx].slot_mutex);
    rb->buffers[idx].state = BUFFER_WRITING;
    pthread_mutex_unlock(&rb->queue_mutex);        // 重要：提前释放队列锁
    // ============ 真正的数据拷贝（此时其他线程可操作其他槽位） ============
    memcpy(rb->buffers[idx].data, rawData, frameSize);
    memcpy(rb->buffers[idx].fileName, fileName, 256);
    rb->buffers[idx].state = BUFFER_FULL;
    pthread_mutex_unlock(&rb->buffers[idx].slot_mutex);
    // 更新队列信息
    pthread_mutex_lock(&rb->queue_mutex);
    rb->writeIndex = (rb->writeIndex + 1) % rb->info.buffNum;
    rb->count++;
    pthread_cond_signal(&rb->cond_empty);
    pthread_mutex_unlock(&rb->queue_mutex);
    return 0;
}

int m_get_count_wildcard(const char *str)
{
    glob_t glob_result;
    int count = 0;
    int ret = glob(str, 0, NULL, &glob_result);
    if (ret != 0) {
        globfree(&glob_result);
        return 0;  // 没有匹配到
    }
    count = glob_result.gl_pathc; // 匹配到的数量
    globfree(&glob_result);
    return count;
}
RingBuffer* initRingBuffer(FrameInfo *mParam)
{
    RingBuffer* rb = (RingBuffer*)malloc(sizeof(RingBuffer));
    if (!rb) return NULL;
    memcpy(&rb->info, mParam, sizeof(FrameInfo));
    rb->buffers = (BufferSlot*)malloc(rb->info.buffNum * sizeof(BufferSlot));
    if (!rb->buffers) {
        free(rb);
        return NULL;
    }
    size_t frameSize = (size_t)(rb->info.stride * rb->info.height); //计算单帧大小
    for (int i = 0; i < rb->info.buffNum; i++) {
        rb->buffers[i].data = (uint8_t*)malloc(frameSize);
        rb->buffers[i].state = BUFFER_EMPTY;
        pthread_mutex_init(&rb->buffers[i].slot_mutex, NULL);
        if (!rb->buffers[i].data) {  // 错误处理：释放已分配内存
            for (int j = 0; j < i; j++)
            {
                pthread_mutex_destroy(&rb->buffers[j].slot_mutex);
                free(rb->buffers[j].data);
            }
            free(rb->buffers);
            free(rb);
            return NULL;
        }
    }
    rb->writeIndex = 0;
    rb->readIndex = 0;
    rb->count = 0;
    pthread_mutex_init(&rb->queue_mutex, NULL);
    pthread_cond_init(&rb->cond_full, NULL);
    pthread_cond_init(&rb->cond_empty, NULL);
    rb->consumerRunning = false;
    printf("RingBuffer initialized: %dx%d, %dbit, %d buffers, frameSize=%zu bytes\n",
           rb->info.stride, rb->info.height, rb->info.bit_mode, rb->info.buffNum, frameSize);
    return rb;
}
void destroyRingBuffer(RingBuffer* rb)
{
    if (!rb) return;
    rb->consumerRunning = false;
    pthread_cond_broadcast(&rb->cond_empty);
    if (rb->consumerThread) 
        pthread_join(rb->consumerThread, NULL);
    for (int i = 0; i < rb->info.buffNum; i++) {
        pthread_mutex_destroy(&rb->buffers[i].slot_mutex);
        free(rb->buffers[i].data);
    }
    free(rb->buffers);
    pthread_mutex_destroy(&rb->queue_mutex);
    pthread_cond_destroy(&rb->cond_full);
    pthread_cond_destroy(&rb->cond_empty);
    free(rb);
}

static void processOneFrame(RingBuffer* rb, int idx)
{
    uint16_t width  = rb->info.stride / 2;   // ✔ 修正关键点
    uint16_t height = rb->info.height;
    uint16_t bit    = rb->info.bit_mode;
    cv::Mat rawMat;
    if (bit == 8)
    {
        rawMat = cv::Mat(height, width, CV_8UC1, rb->buffers[idx].data);
    }
    else if (bit == 10 || bit == 12)
    {
        rawMat = cv::Mat(height, width, CV_16UC1, rb->buffers[idx].data);
    }
    else
    {
        return;
    }
    cv::Mat rawCopy = rawMat.clone();
    cv::Mat displayMat;
    displayMat = rawCopy;
    cv::imwrite(rb->buffers[idx].fileName, displayMat);
}

static void* consumerThreadFunc(void* arg)
{
    RingBuffer* rb = (RingBuffer*)arg;
    while (rb->consumerRunning) {
        pthread_mutex_lock(&rb->queue_mutex);
        while (rb->count == 0 && rb->consumerRunning) {
            pthread_cond_wait(&rb->cond_empty, &rb->queue_mutex);
        }
        if (!rb->consumerRunning) {
            pthread_mutex_unlock(&rb->queue_mutex);
            break;
        }
        int idx = rb->readIndex;
        // 检查并锁定槽位
        pthread_mutex_lock(&rb->buffers[idx].slot_mutex);
        if (rb->buffers[idx].state != BUFFER_FULL) {
            pthread_mutex_unlock(&rb->buffers[idx].slot_mutex);
            pthread_mutex_unlock(&rb->queue_mutex);
            // msleep(10); //睡眠10ms时间再进行见擦汗
            usleep(2000);
            continue;
        }
        rb->buffers[idx].state = BUFFER_READING;
        pthread_mutex_unlock(&rb->buffers[idx].slot_mutex);
        pthread_mutex_unlock(&rb->queue_mutex);   // 关键：长时间处理前释放队列锁
        // ==================== 耗时处理（不持有任何锁） ====================
        processOneFrame(rb, idx);
        // 处理完成，标记为空
        pthread_mutex_lock(&rb->buffers[idx].slot_mutex);
        rb->buffers[idx].state = BUFFER_EMPTY;
        pthread_mutex_unlock(&rb->buffers[idx].slot_mutex);

        pthread_mutex_lock(&rb->queue_mutex);
        rb->readIndex = (rb->readIndex + 1) % rb->info.buffNum;
        rb->count--;
        pthread_cond_signal(&rb->cond_full);
        pthread_mutex_unlock(&rb->queue_mutex);
    }
    return NULL;
}

int startConsumerThread(RingBuffer* rb)
{
    if (rb->consumerRunning) 
        return 0;
    rb->consumerRunning = true;
    if (pthread_create(&rb->consumerThread, NULL, consumerThreadFunc, rb) != 0) {
        rb->consumerRunning = false;
        return -1;
    }
    return 0;
}

void printFile(const std::string& filename)
{
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    std::string line;
    while (std::getline(ifs, line)) {
        std::cout << line << std::endl;
    }
    ifs.close();
}

void print_net_info(const std::string &ifname, std::ofstream &ofs)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        ofs << "socket create failed\n";
        return;
    }
    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);
    // ========= MAC =========
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0)
    {
        unsigned char *mac = reinterpret_cast<unsigned char *>(ifr.ifr_hwaddr.sa_data);

        ofs << "MAC: ";
        for (int i = 0; i < 6; ++i)
        {
            ofs << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(mac[i]);
            if (i != 5)
                ofs << ":";
        }
        ofs << std::dec << "\n";
    }
    else
    {
        ofs << "Get MAC failed\n";
    }
    // ========= IP =========
    if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
    {
        auto *ipaddr = reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr);
        ofs << "IP: " << inet_ntoa(ipaddr->sin_addr) << "\n";
    }
    else
    {
        ofs << "Get IP failed\n";
    }
    // ========= NETMASK =========
    if (ioctl(fd, SIOCGIFNETMASK, &ifr) == 0)
    {
        auto *netmask = reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_netmask);
        ofs << "Netmask: " << inet_ntoa(netmask->sin_addr) << "\n";
    }
    else
    {
        ofs << "Get Netmask failed\n";
    }
    close(fd);
}
void dev_get_soc_status(CAM_DEV *dev, DEV_XSTATUS_VALUE &data, bool isPrint)
{
    dev->dev_xstatus_get_cam_status(data);
    if(true == isPrint)
    {
        cout<<"cpu temp : "<<data.cpuTemp<<endl;
        cout<<"gpu temp : "<<data.gpuTemp<<endl;
        cout<<"cpu mmey : "<<data.cpuMem<<"%"<<endl;
        cout<<"gpu mmey : "<<data.gpuMem<<"%"<<endl;
        cout<<"tmp117 temp : "<<data.exterTemp<<endl;
    }
}
/**
 * @brief 
 * 
 * @param dev 
 * @param index 输出管脚下坐标
 * @param value 脉冲宽度时间参数，单位毫秒
 */
void dev_set_io_output_set(CAM_DEV *dev, int index, int value)
{
    DEV_IO_OUT_INDEX mParam;
    if(0 == index)
    {
        mParam = IO_SET_OUT_SYNC;
    }
    else if(1 == index)
    {
        mParam = IO_SET_OUT_AC;
    }
    else if(2 == index)
    {
        mParam = IO_SET_OUT_CC;
    }
    else
    {
        return;
    }
    dev->dev_io_trig_out(mParam, value*1000, 1);
}

void dev_set_io_input_param_set(CAM_DEV *dev, int index)
{
    if(0 == index)
    {
        dev->dev_io_set_in(IO_INPUT_1, 0, 10);
    }
    else if(1 == index)
    {
        dev->dev_io_set_in(IO_INPUT_2, 0, 10);
    }
    else
    {
        cout<<"输入参数有误!!!!!!!"<<endl;
    }
}

void dev_set_io_ext_internal_mode(CAM_DEV *dev, int mode, int value, int value_2)
{
    cout<<"set mode:"<<mode<<" "<<value<<" "<<value_2<<endl;
    if(0 == mode)
    {
        dev->dev_io_ctrl_ext2d_intern_ctl(3, value * 1000, value_2*1000);
    }
    else if(1 == mode)
    {
        dev->dev_io_set_in(IO_INPUT_1, 0, 10);
        dev->dev_io_ctrl_ext2d_exterl_ctl(10,value * 1000, value_2*1000);
    }
}
void dev_set_dlp_status(CAM_DEV *dev, bool isPrint)
{
    float temp = 0.0;
    uint16_t pwm_value;
    int dlp_num[DLP_NUM];
    dev->dev_dlp_find(dlp_num);
    cout << "dlp_num : " << dlp_num[0] << " : " << dlp_num[1] << " : " << dlp_num[2] << " : " << dlp_num[3] << endl;
    for (int i = 0; i < DLP_NUM; i++)
    {
        if (1 == dlp_num[i])
        {
            // dev->dev_dlp_set_delay_invert(i, false, 0); //思特威的sensor使用上升沿触发
            dev->dev_dlp_get_temp(i, temp);
            dev->dev_dlp_get_current(i, pwm_value);
            dev->dev_dlp_set_current_all(10);
            if (true == isPrint)
            {
                cout << "dlp id : " << i << " get temp : " << temp << endl;
                cout << "dlp id : " << i << " get pwm_value : " << pwm_value << endl;
                cout << "dlp trigonce!" << endl;
            }
            dev->dev_dlp_trigonce(i);
            sleep(1);
        }
    }
}
void dev_sensor_debug_test(CAM_DEV *dev, int opt, int addr, int val)
{
    DEV_SENSOR_REG_PARAM reg;
    memset(&reg, 0, sizeof(reg));
    reg.opt = (uint8_t)opt;
    reg.addr = (uint16_t)addr;
    reg.value = (uint8_t)val;
    dev->dev_sensor_reg_set(CAM_INDEX_ONE, reg);
    printf("reg addr:0x%x opt:%d value:%d -> 0x%x\r\n", reg.addr, reg.opt, reg.value, reg.value);
    return;
}

void dev_set_two_sensor_trig(CAM_DEV *dev)
{
    int i = 10;
    while(i--)
    {
        if(i<0)
            break;
        dev->dev_io_trig_out(IO_SET_RES_TRIG_SENSOR, 11, 1);//单位是毫秒
        usleep(200000);
    }
}
void dev_set_io_rgb_status(CAM_DEV *dev, uint8_t set)
{
    const DEV_IO_RGB_PARAM mParam = 
    {
        .r_expo = 50000,
        .g_expo = 50000,
        .b_expo = 50000,
        .w_expo = 50000,
        .period = 100000,
        .rgbPwm = 90,
    };
    DEV_IO_RGB_TRIG_SET mSet = 
    {
        .rgbw_on = { true, true, true, true }
    };
    dev->dev_io_set_rgb_status(mParam);
    if(set<16)
    {
        mSet.rgbw_on[0] = (set >> 0) & 0x1; // R
        mSet.rgbw_on[1] = (set >> 1) & 0x1; // G
        mSet.rgbw_on[2] = (set >> 2) & 0x1; // B
        mSet.rgbw_on[3] = (set >> 3) & 0x1; // W
         printf("Apply Config: "
               "R=%d G=%d B=%d W=%d\n",
               mSet.rgbw_on[0],
               mSet.rgbw_on[1],
               mSet.rgbw_on[2],
               mSet.rgbw_on[3]);

        dev->dev_io_rgb_signal_trig(mSet);
    }
}

void dev_set_io_led_status(CAM_DEV *dev, int index)
{
    dev->dev_io_set_status_led((DEV_LED_INDEX)index, 1000, 8);
}
void dev_set_io_enable_dlp_usb(CAM_DEV *dev, int index)
{
    cout<<"输入开启的dlp的usb:"<<index<<endl;
    if(0xff == index)
    {
        dev->dev_io_set_dlp_enable_usb(1, DLP_ALL);
    }
    else
    {
        switch (index)
        {
            case 0:
                dev->dev_io_set_dlp_enable_usb(1, DLP_0);
                break;
            case 1:
                dev->dev_io_set_dlp_enable_usb(1, DLP_1);
                break;
            case 2:
                dev->dev_io_set_dlp_enable_usb(1, DLP_2);
                break;
            case 3:
                dev->dev_io_set_dlp_enable_usb(1, DLP_3);
                break;
            default:
                cout<<"输入的参数是错误的:"<<index<<endl;
                break;
        }
    }
}

void dev_get_dlp_ver(CAM_DEV *dev, string &ver, bool isPrint)
{
    dev->dev_dlp_get_version(ver);
    if(true == isPrint)
    {
        cout<<"dlp Version : "<<ver<<endl;
    }
}

void dev_get_io_ver(CAM_DEV *dev, string &ver, bool isPrint)
{
    dev->dev_io_get_version(ver);
    if(true == isPrint)
    {
        cout<<"io Version : "<<ver<<endl;
    }
}

void sensor_set_roi(CAM_DEV *dev, DEV_CAM_INDEX type)
{
    DEV_ROI mParam = 
    {
        .bitMode = SENSOR_8BIT,
        .binningMode = SENSOR_ROI,
        .x = 0,
        .y = 0,
        .w = 2856,
        .h = 2848,
        .alignWidth = 0,
    };

    dev->dev_sensor_set_roi(type, mParam);
    cout<<endl;
}
/**
 * @brief 会将raw数据转成png保存
 * 
 * @param frame 
 * @param usr_data 
 */
void sensor_get_raw_data(DEV_IMG_DEF frame, void *usr_data)
{
    char buffer[256];
    struct DEV_Param *mParam = (struct DEV_Param *)usr_data;
    int cnt = ++mParam->g_dlp_index[frame.sensor_index];
    int cnt_pic = ++mParam->g_pic_index[frame.sensor_index];
    std::snprintf(buffer, sizeof(buffer),
                  "./pic/index_%d_frame_i_%d_d_%d_w_%d_h_%d_s_%d_length_%d.png",
                  frame.sensor_index, cnt_pic,cnt,
                  frame.width,
                  frame.height,
                  frame.stride,
                  frame.all_bytes);
    std::string current_frame_string(buffer);
    std::cout<<current_frame_string<<std::endl;
    producerPut(mParam->g_rb, (const uint8_t*)frame.data, frame.all_bytes, buffer);
}
void thread_dlp(CAM_DEV *dev, struct DEV_Param *param, int index)
{
    unsigned int i = 0;
    while (1)
    {
        std::unique_lock<std::mutex> lock(param->g_mtx1);
        param->g_cv1.wait(lock, [&]{
            if (param->g_is_dual)
            {
                return param->g_dlp_index[0].load() >= param->SET_NUM &&
                    param->g_dlp_index[1].load() >= param->SET_NUM;
            }
            else
            {
                return param->g_dlp_index[0].load() >= param->SET_NUM;
            }
        });
        if (param->g_is_dual)
        {
            param->g_dlp_index[0] = 0;
            param->g_dlp_index[1] = 0;
        }
        else
        {
            param->g_dlp_index[0] = 0;
        }
        printf("thread_dlp:i:%d ret:%d\r\n", i++, dev->dev_dlp_trigonce(index));
    }
}
void thread_pic(CAM_DEV *dev, struct DEV_Param *param)
{
    unsigned int i = 0;
    while (1)
    {
        std::unique_lock<std::mutex> lock(param->g_mtx2);
        param->g_cv2.wait(lock, [&] {
            if (param->g_is_dual)
            {
                return param->g_pic_index[0].load() >= param->SET_NUM &&
                    param->g_pic_index[1].load() >= param->SET_NUM;
            }
            else
            {
                return param->g_pic_index[0].load() >= param->SET_NUM;
            }
        });
        if (param->g_is_dual)
        {
            param->g_pic_index[0] = 0;
            param->g_pic_index[1] = 0;
        }
        else
        {
            param->g_pic_index[0] = 0;
        }
        int ret = dev->dev_io_clear_trig_count();
        if (RTN_OKAY != ret)
        {
            std::cout << "error dev_io_clear_trig_count:" << ret << std::endl;
        }
        printf("thread_pic:i:%d \r\n", i++);
    }
}
static bool m_file_exists(const char* filename)
{
    return access(filename, F_OK) == 0;
}

DEV_RTN dev_test_dlp_updata_flash(CAM_DEV *dev, uint8_t index, bool isPrint)
{
    DEV_RTN ret;
    int dlp_num[DLP_NUM];
    dev->dev_dlp_find(dlp_num);
    cout << "dlp_num : " << dlp_num[0] << " : " << dlp_num[1] << " : " << dlp_num[2] << " : " << dlp_num[3] << endl;
    if (false == m_file_exists("./data.bin"))
    {
        if (isPrint)
        {
            cout << "当前目录没有升级文件data.bin!" << endl;
        }
        return RTN_NOT_ALLOWED;
    }
    for (int i = 0; i < DLP_NUM; i++)
    {
        if(1 ==dlp_num[i] && index == i)
        {
            ret = dev->dev_dlp_updata_falsh(index, "./data.bin");
            if(RTN_OKAY != ret)
            {
                if (isPrint)
                {
                    cout << "测试升级data.bin文件失败" << endl;
                }
                return RTN_FAIL;
            }
            else
            {
               if (isPrint)
                {
                    cout << "测试升级data.bin文件成功" << endl;
                }
                return ret;
            }
        }
    }
    if (isPrint)
    {
        cout<<"输入的index参数非法"<<endl;
    }
    return RTN_INVALID_ARG;
}
/**
 * @brief 配置dlp的连续触发
 * 
 * @param dev 
 * @param num ：dlp的下坐标，单目旦光机是1，双目
 * @param isPrint 
 */
void dev_set_dlp_rgb_xtrig_status( CAM_DEV *dev, struct DEV_Param *devParam, uint32_t num)
{  
    uint8_t index = 1;
    // struct DEV_Param *devParam = &ctx.m_pram;
    // CAM_DEV *dev = ctx.dev;
    DEV_IO_DLP_RGB_SYNC_PARAM mParam;
    mParam.mode = 1; //开启rgb和dlp的联动触发
    if(num < 4)
    {
        index = num;
    }
    mParam.syncDlpNum = devParam->SET_NUM;
    mParam.syncRgbNum = devParam->SET_NUM-4;
    mParam.pwmValue = 90;
    mParam.rgbExpoTime[0] = 400;
    mParam.rgbExpoTime[1] = 500;
    mParam.rgbExpoTime[2] = 600;
    mParam.rgbExpoTime[3] = 800;
    dev->dev_io_set_get_dlp_rgb_sync_states(mParam); //配置参数
    dev->dev_io_clear_trig_count();
    dev->dev_dlp_trigonce(index);
    devParam->g_dlp_index[0] = 0;
    devParam->g_dlp_index[1] = 0;
    devParam->g_pic_index[0] = 0;
    devParam->g_pic_index[1] = 0;
    std::thread t1(thread_dlp, dev, devParam, index);
    std::thread t2(thread_pic, dev, devParam);
    t1.join();
    t2.join();
}
void sensor_image_self_test_save(DEV_IMG_DEF frame, void *usr_data)
{
    char buffer[128];
    struct DEV_Param *mParam = (struct DEV_Param *)usr_data;
    int cnt = ++mParam->g_dlp_index[frame.sensor_index];
    int cnt_pic = ++mParam->g_pic_index[frame.sensor_index];
    std::snprintf(buffer, sizeof(buffer),
                  "index_%d_frame_i_%d_d_%d_w_%d_h_%d_s_%d_length_%d.raw",
                  frame.sensor_index, cnt_pic,cnt,
                  frame.width,
                  frame.height,
                  frame.stride,
                  frame.all_bytes);
    std::string current_frame_string(buffer);
    std::cout<<current_frame_string<<std::endl;
    if((1 == cnt && 0 == frame.sensor_index) || (2 == cnt && 1 == frame.sensor_index))
    {
        std::cout<<current_frame_string<<std::endl;
        std::ofstream file(current_frame_string, std::ios::binary);
        if (!file.is_open()) return;
        file.write((const char* )frame.data, frame.all_bytes);
        file.close();
    }
}
void save_test_log_result(CAM_DEV *dev, struct DEV_Param *devParam)
{
    int i;
    int dlp_num[DLP_NUM];
    string m_string;
    float temp = 0.0f;
    uint16_t dlpPwm = 0;
    DEV_XSTATUS_VALUE status;
    DEV_VERSION_INFO_STRUCT ver;
    const std::string logFile = "deviceSelfTest.txt"; // 定义文件名
    std::ofstream ofs(logFile);                // 默认覆盖模式
    if (!ofs.is_open())
    {
        std::cerr << "Failed to open log file: " << logFile << "\n";
        return;
    }
    dev->dev_io_set_status_led(LED_2, 0, 0);
    dev->dev_io_set_status_led(LED_3, 0, 0);
    dev_print_version_info(dev, ver, false);
    print_system_status(devParam->g_is_dual, devParam->g_device_type, devParam->g_is_debug_test_flag, &ofs);
    ofs << "--- DEV Version Info ---\n";
    ofs << "Commit ID : " << ver.commitId << "\n";
    ofs << "Branch    : " << ver.gitBranch << "\n";
    ofs << "Date      : " << ver.date << "\n";
    ofs << "Version   : " << ver.version << "\n";
    ofs << "orin_sn   : " << ver.orin_sn << "\n";
    ofs << "ssd_sn    : " << ver.ssd_sn << "\n";
    dev_get_sensor_ver(dev, m_string, false);
    ofs << "sensor driver Version : " << m_string << "\n";
    dev_get_dlp_ver(dev, m_string, false);
    ofs << "dlp driver Version : " << m_string << "\n";
    dev_get_io_ver(dev, m_string, false);
    ofs << "iomanager driver Version : " << m_string << "\n";
    ofs << "--- dlp test result ---\n";
    //注册相机回调函数
    dev->dev_register_image_callback(CAM_INDEX_ALL, sensor_image_self_test_save);
    dev->dev_dlp_find(dlp_num);
    for (i = 0; i < DLP_NUM; i++)
    {
        if (1 == dlp_num[i])
        {
            dev->dev_dlp_set_delay_invert(i, true, 0);
            dev->dev_dlp_get_temp(i, temp);
            ofs << "dlp_"<<i<<" temp value  : " << temp << "\n";
            dev->dev_dlp_get_current(i, dlpPwm);
            ofs << "dlp_"<<i<<" pwm value   : " << dlpPwm << "\n";
            dev->dev_dlp_trigonce(i);
            sleep(1);
        }
    }
    dev->dev_xstatus_get_cam_status(status);
    ofs << "--- soc test result ---\n";
    ofs<<"cpu temp : "<<status.cpuTemp<<endl;
    ofs<<"gpu temp : "<<status.gpuTemp<<endl;
    ofs<<"cpu mmey : "<<status.cpuMem<<"%"<<endl;
    ofs<<"gpu mmey : "<<status.gpuMem<<"%"<<endl;
    ofs<<"tmp117 temp : "<<status.exterTemp<<endl;
    ofs << "--- net test result ---\n";
    print_net_info("eth0", ofs);
    ofs.close();
    dev_set_io_rgb_status(dev, 0x0F);
    printFile(logFile);
}

/**
 * @brief 只统计sensor的数量
 * 
 * @param frame 
 * @param usr_data 
 */
void sensor_image_count(DEV_IMG_DEF frame, void *usr_data)
{
    char buffer[128];
    struct DEV_Param *mParam = (struct DEV_Param *)usr_data;
    int cnt = ++mParam->g_dlp_index[frame.sensor_index];
    int cnt_pic = ++mParam->g_pic_index[frame.sensor_index];
    std::snprintf(buffer, sizeof(buffer),
                  "index_%d_frame_i_%d_d_%d_w_%d_h_%d_s_%d_length_%d.raw",
                  frame.sensor_index, cnt_pic,cnt,
                  frame.width,
                  frame.height,
                  frame.stride,
                  frame.all_bytes);
    std::string current_frame_string(buffer);
    std::cout<<current_frame_string<<std::endl;
#if 1
    if(cnt == mParam->SET_NUM) //dlp事件触发一次
    {
        if(0 == frame.sensor_index)
            std::cout<<current_frame_string<<std::endl;
        mParam->g_cv1.notify_one();
    }
    if(cnt_pic == mParam->SET_PIC_NUM)//图像计数清零触发一次
    {
        if(1 == frame.sensor_index)
            std::cout<<current_frame_string<<std::endl;
        mParam->g_cv2.notify_one();  //
    }
#endif
}
static void event_callback(DEV_FLAG_TYPE event)
{
    static int i = 0;
    printf("in %d -----count: %d\r\n", event, i++);
    return;
}

void cmd_print_help(const vector<CmdItem> &cmdTable)
{
    cout<<"----------------------------->>"<<endl;
    cout<<"-1 退出程序"<<endl;
    for(const auto &item : cmdTable)
    {
        printf("%-2s %s\n", item.usage, item.desc);
    }
}

vector<CmdItem> build_cmd_table()
{
    vector<CmdItem> table = \
    {   
        {0,"0","HS系列硬件功能检测结果输入到文档里面",1,1,[](CmdContext &ctx){save_test_log_result(ctx.dev, &ctx.m_pram);}},
        {1,"1","打印当前版本信息",1,1,[](const CmdContext &ctx){cmd_dev_print_version_info(ctx.dev, true);}},
        {2,"2","开启内部的debug打印",1,1,[](const CmdContext &ctx){dev_set_log_print(ctx.dev);}},
        {10,"10","打印sensor的版本信息",1,1,[](const CmdContext &ctx){
                                                    string m_string; \
                                                    dev_get_sensor_ver(ctx.dev, m_string, true);}},
        {11,"11 [gain]","配置sensor的gain参数",2,2,[](const CmdContext &ctx){ctx.dev->dev_sensor_set_gain(CAM_INDEX_ALL,stoi(ctx.argv[1]));}},
        {12,"12 [0/1]","配置sensor的水平垂直翻转参数",2,2,[](const CmdContext &ctx){
                                                    ctx.dev->dev_sensor_set_hflip(CAM_INDEX_ALL, stoi(ctx.argv[1]));
                                                    ctx.dev->dev_sensor_set_vflip(CAM_INDEX_ALL, stoi(ctx.argv[1]));}},
        {13,"13 [black]","配置sensor的黑电平参数",2,2,[](const CmdContext &ctx){ctx.dev->dev_sensor_set_black(CAM_INDEX_ALL,stoi(ctx.argv[1]));}},
        {14,"14","获取sensor的最高帧率值",1,1,[](const CmdContext &ctx){
                                                    float rate = 0.0f;
                                                    ctx.dev->dev_sensor_get_frame_rate(CAM_INDEX_ONE, rate);
                                                    cout << "sensor_0 max frame rate : " << rate << "fps."<<endl;}},
        {15,"15","配置sensor的roi参数",1,1,[](const CmdContext &ctx){sensor_set_roi(ctx.dev, CAM_INDEX_ALL);}},
        {16,"16","配置sensor的回调函数",1,1,[](const CmdContext &ctx){ctx.dev->dev_register_image_callback(CAM_INDEX_ONE,sensor_get_raw_data);}},
        {17,"17","获取sensor的温度信息",1,1,[](const CmdContext &ctx){
                                                    float temp = 0.0f;
                                                    ctx.dev->dev_sensor_get_temp(CAM_INDEX_ONE, temp);
                                                    cout<<"temp return:"<<temp<<endl;}},
        {20,"20","获取dlp驱动程序版本信息",1,1,[](const CmdContext &ctx){
                                                    string m_string; \
                                                    dev_get_dlp_ver(ctx.dev, m_string, true);}},
        {21,"21","测试配置dlp的参数",1,1,[](const CmdContext &ctx){dev_set_dlp_status(ctx.dev, true);}},
        {22,"22 [dlp_index]","测试DLP和RGB联动触发配置,后跟配置dlp的下坐标参数:0-3",2,2,[](CmdContext &ctx){
                                                    ctx.dev->dev_register_image_callback(CAM_INDEX_ALL, sensor_image_count);
                                                    dev_set_dlp_rgb_xtrig_status(ctx.dev, &ctx.m_pram, 1);}},
        {23,"23 [index]","测试DLP的刷写图卡,烧录文件时data.bin,后跟DLP的index:测试那个dlp的烧录",2,2,[](CmdContext &ctx){
                                                    dev_test_dlp_updata_flash(ctx.dev, stoi(ctx.argv[1]), true);}},
        {30,"30","获取iomanager驱动程序版本信息",1,1,[](const CmdContext &ctx){
                                                    string m_string; \
                                                    dev_get_io_ver(ctx.dev, m_string, true);}},
        {31,"31 [index]","测试io配置dlp的usb连接,后跟dlp光机的index值(0-3),0xff(255)为全开",2,2,[](const CmdContext &ctx){
                                                    dev_set_io_enable_dlp_usb(ctx.dev, stoi(ctx.argv[1]));}},
        {32,"32 [index]","测试io配置led闪烁,后跟参数0-代表闪烁led_1,1-代表闪烁led_2,2-代表闪烁led_3",2,2,[](const CmdContext &ctx){
                                                    dev_set_io_led_status(ctx.dev, stoi(ctx.argv[1]));}},
        {33,"33 [index]","测试io配置rgb灯闪烁,和22测试项功能互斥,后跟参数0-15(低4位代表那个灯rgbw亮)",2,2,[](const CmdContext &ctx){
                                                    dev_set_io_rgb_status(ctx.dev, stoi(ctx.argv[1]));}},
        {34,"34 [mode] [ms] [ms]","测试外部光源2D的出图方式,后跟mode参数,0-定时出图,1-配合2D管脚输入出图,第二个参数配置sensor的曝光时间(单位毫秒)，第三个参数出图周期时间/SYNC管脚脉冲信号的时间(单位毫秒)"
                                                    ,4,4,[](const CmdContext &ctx){dev_set_io_ext_internal_mode(ctx.dev, stoi(ctx.argv[1]), stoi(ctx.argv[2]),stoi(ctx.argv[3]));}},
        {35,"35 [index] [ms]","测试io输出口的脉冲波输出或者电平输出,后跟参数1:DEV_IO_OUT_INDEX,参数2:0-代表电平输出,xx代表输出一个脉冲时间值(单位毫秒)",3,3,[](const CmdContext &ctx){
                                                    dev_set_io_output_set(ctx.dev, stoi(ctx.argv[1]), stoi(ctx.argv[2]));}},
        {36,"36 [index]","开启io输入测试,接收来自2D,3D管脚的输入信息,打印中断的输入回调函数,第一个参数:0-代表2D,1-代表3D",2,2,[](const CmdContext &ctx){
                                                    ctx.dev->dev_register_event_callback(event_callback);
                                                    ctx.dev->dev_io_set_in((SOURCE_IO)stoi(ctx.argv[1]), 1, 0);}},
        {40,"40","获取SOC的测温和内存信息",1,1,[](const CmdContext &ctx){
                                                    DEV_XSTATUS_VALUE xStatus;
                                                    dev_get_soc_status(ctx.dev, xStatus, true);}}, 
        {50,"50","测试双目的sensor触发抓拍信号",1,1,[](const CmdContext &ctx){dev_set_two_sensor_trig(ctx.dev);}},
        {60,"60","sensor的寄存器参数读取和写入,第一个传参reg的地址(0xXXXX)，第二个参数写入的值(0xXX),第二个参数省略代表读取",2,3,[](const CmdContext &ctx){
                                                    dev_set_two_sensor_trig(ctx.dev);
                                                    if(2 == ctx.argc)
                                                    {
                                                        printf("addr 0x%x %d \r\n",stoi(ctx.argv[1], nullptr, 0), stoi(ctx.argv[1], nullptr, 0));
                                                        dev_sensor_debug_test(ctx.dev, 0, stoi(ctx.argv[1], nullptr, 0), 0);
                                                    }
                                                    else if(3 == ctx.argc)
                                                    {
                                                        printf("addr 0x%x %d \r\n",stoi(ctx.argv[1], nullptr, 0), stoi(ctx.argv[1], nullptr, 0));
                                                        printf("val 0x%x %d \r\n",stoi(ctx.argv[2], nullptr, 0), stoi(ctx.argv[2], nullptr, 0));
                                                        dev_sensor_debug_test(ctx.dev, 1, stoi(ctx.argv[1], nullptr, 0), stoi(ctx.argv[2], nullptr, 0));
                                                    }
                                                }},
    };
    return table;
}

const CmdItem *find_cmd(const vector<CmdItem> &table, int cmd)
{
    for(const auto &item : table)
    {
        if(item.cmd == cmd)
        {
            return &item;
        }
    }
    return nullptr;
}

void execute_cmd(const vector<CmdItem> &table, CmdContext &ctx)
{
    int cmd = stoi(ctx.argv[0]);
    auto item = find_cmd(table, cmd);
    if(item == nullptr)
    {
        printf("unknown cmd:%d\n", cmd);
        cmd_print_help(table);
        return;
    }
    if(ctx.argc < item->minArgc ||
            ctx.argc > item->maxArgc)
    {
        printf("usage: %s 输入参数数量不对!\n",
               item->usage);
        return;
    }
    item->func(ctx);
}

void cmd_dev_print_version_info(CAM_DEV *dev, bool isPrint)
{
    DEV_VERSION_INFO_STRUCT ver;
    dev->dev_get_version(ver);
    if (true == isPrint)
    {
        cout << "Commit ID : " << ver.commitId << "\n";
        cout << "Branch    : " << ver.gitBranch << "\n";
        cout << "Date      : " << ver.date << "\n";
        cout << "Version   : " << ver.version << "\n";
        cout << "orin_sn   : " << ver.orin_sn << "\n";
        cout << "ssd_sn    : " << ver.ssd_sn << "\n";
    }
    return;
}

void dev_judgment_node(struct DEV_Param &mParam, DEV_SENSOR_ATTRIBUTE &sensor)
{
    uint8_t videoNum = 0;
    uint8_t ioNum = 0; //判断iomanager的驱动是否加载
    uint8_t dlpNum = 0;
    ioNum = m_get_count_wildcard("/dev/iomanager");
    if(1 != ioNum)
    {
        cout<<"请加载相关的驱动,比如iomanager.ko驱动程序"<<endl;
    }
    videoNum = m_get_count_wildcard("/dev/video*");
    if(0 != videoNum)
    {
        int sensor_id = 0;
        FILE *fp = fopen("/sys/kernel/debug/nv_imx/sensor_id", "r");
        if (fp && fscanf(fp, "%d", &sensor_id) == 1) sensor.type = (DEV_SENSOR_TYPE)sensor_id;
        if (fp) fclose(fp);
        if(1 == videoNum)
        {
            sensor.sensorNum = 1;
            mParam.g_is_dual = false;
        }
        else if(2 == videoNum)
        {
            sensor.sensorNum = 2;
            mParam.g_device_type = TWO_SENSOR_ONE_MACHINE;
            mParam.g_is_dual = true;
        }
    }
    else
    {
        cout<<"请加载相关的驱动,比如nv_imx.ko驱动程序"<<endl;
    }
    dlpNum = m_get_count_wildcard("/dev/dlp3010_*");
    if(4 == dlpNum)
    {
        mParam.g_device_type = ONE_SENSOR_FOUR_MACHINE;
    }
    else if(1 == dlpNum)
    {
        mParam.g_device_type = ONE_SENSOR_ONE_MACHINE;
    }
    else
    {
        cout<<"请加载相关的驱动,比如dlp3010.ko驱动程序"<<endl;
    }
    mParam.g_is_debug_test_flag = NORMAL_OPERATING_MODE;
}