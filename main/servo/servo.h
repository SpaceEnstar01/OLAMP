#ifndef SERVO_H
#define SERVO_H
#include "driver/uart.h"  // 在文件开头添加
#include "st_servo/SMS_STS.h"

class Servo {
public:
    Servo();
    ~Servo();

    // 初始化舵机
    // uart_num: UART编号 (如 UART_NUM_1)
    // tx_pin: 发送引脚
    // rx_pin: 接收引脚
    // baud_rate: 波特率 (STS舵机默认1M)
    // servo_id: 舵机ID (默认1)
    bool Init(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate, int servo_id = 1);

    // 移动到指定角度
    // degree: 目标角度，范围 -180 到 180 度
    // speed: 速度参数，0为最大速度 (可选，默认0)
    // acc: 加速度参数，0为最大加速度 (可选，默认0)
    // 返回: true表示命令发送成功，false表示失败
    bool MoveTo(float degree, int speed = 0, int acc = 0);

    // 获取当前角度
    // 返回: 当前角度，如果读取失败返回 NaN
    float GetPosition();

    // 使能/禁用舵机力矩
    // enable: true使能，false禁用
    // 返回: true表示成功，false表示失败
    bool EnableTorque(bool enable);

    // 检查是否已初始化
    bool IsInitialized() const { return initialized_; }

    // 获取错误码
    int GetError() const { return error_code_; }

    // Ping舵机检查通信
    // 返回: 舵机ID表示成功，-1表示失败
    int Ping();

private:
    SMS_STS servo_;
    bool initialized_;
    int servo_id_;
    int error_code_;

    // 角度转寄存器值
    static int DegreeToReg(float degree);
    
    // 寄存器值转角度
    static float RegToDegree(int reg);
};

#endif // SERVO_H