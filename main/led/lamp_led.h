#ifndef _LAMP_LED_H_
#define _LAMP_LED_H_

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <cstdint>
#include <mutex>
#include <string>

/**
 * LampLed - 台灯PWM控制类
 * 
 * 功能：
 * - 使用LEDC PWM控制亮度（0-100%）
 * - 支持渐变效果（fade）
 * - 自动注册MCP工具供语义控制
 * 
 * 使用不同的LEDC资源避免与gpio_led冲突：
 * - Timer: LEDC_TIMER_1
 * - Channel: LEDC_CHANNEL_1
 * - Resolution: 8位 (0-255)
 */
class LampLed {
public:
    /**
     * 构造函数
     * @param gpio GPIO引脚号（默认GPIO 21）
     * @param output_invert 是否反转输出（0=正常，1=反转）
     */
    LampLed(gpio_num_t gpio = GPIO_NUM_21, bool output_invert = false);
    
    /**
     * 析构函数
     */
    ~LampLed();

    /**
     * 开灯（渐变到最大亮度）
     * @param fade_ms 渐变时间（毫秒），0表示立即切换
     */
    void TurnOn(uint32_t fade_ms = 500);

    /**
     * 关灯（渐变到0亮度）
     * @param fade_ms 渐变时间（毫秒），0表示立即切换
     */
    void TurnOff(uint32_t fade_ms = 500);

    /**
     * 切换开关状态
     * @param fade_ms 渐变时间（毫秒）
     */
    void Toggle(uint32_t fade_ms = 500);

    /**
     * 设置亮度
     * @param brightness 亮度值（0-100）
     * @param fade_ms 渐变时间（毫秒），0表示立即切换
     */
    void SetBrightness(uint8_t brightness, uint32_t fade_ms = 300);

    /**
     * 获取当前亮度（0-100）
     */
    uint8_t GetBrightness() const { return brightness_; }

    /**
     * 获取开关状态
     */
    bool IsOn() const { return is_on_; }

    /**
     * 获取状态JSON字符串（供MCP工具使用）
     */
    std::string GetStateJson() const;

private:
    gpio_num_t gpio_;
    bool output_invert_;
    bool is_on_;
    uint8_t brightness_;  // 0-100
    ledc_channel_config_t ledc_channel_;
    bool ledc_initialized_;
    mutable std::mutex mutex_;

    // LEDC配置常量（参考成功测试代码）
    static constexpr ledc_timer_t LAMP_LEDC_TIMER = LEDC_TIMER_1;
    static constexpr ledc_channel_t LAMP_LEDC_CHANNEL = LEDC_CHANNEL_1;
    static constexpr ledc_timer_bit_t LAMP_LEDC_RESOLUTION = LEDC_TIMER_8_BIT;
    static constexpr uint32_t LAMP_LEDC_FREQ_HZ = 5000;  // 5kHz PWM频率
    static constexpr uint32_t LAMP_LEDC_MAX_DUTY = 255;  // 8-bit resolution: 2^8 - 1

    /**
     * 初始化LEDC PWM
     */
    void InitLedc();

    /**
     * 设置PWM占空比（内部方法）
     * @param duty 占空比值（0-LAMP_LEDC_MAX_DUTY）
     * @param fade_ms 渐变时间（毫秒），0表示立即切换
     */
    void SetDuty(uint32_t duty, uint32_t fade_ms = 0);

    /**
     * 亮度值转换为占空比
     * @param brightness 亮度值（0-100）
     * @return 占空比值（0-LAMP_LEDC_MAX_DUTY）
     */
    uint32_t BrightnessToDuty(uint8_t brightness) const;

    /**
     * 注册MCP工具（供语义控制）
     */
    void RegisterMcpTools();
};

/**
 * 获取全局LampLed实例（用于关键词直接控制）
 * 注意：需要在板级初始化文件中先创建LampLed实例
 * @return LampLed指针，如果未初始化则返回nullptr
 */
LampLed* GetLampLed();

/**
 * 设置全局LampLed实例（由LampLed构造函数自动调用）
 */
void SetLampLed(LampLed* lamp_led);

/**
 * 测试函数：启动LED灯测试（亮5秒，灭1秒循环）
 * @param lamp_led LampLed实例指针
 */
void StartLampLedTest(LampLed* lamp_led);

/**
 * 停止LED灯测试
 */
void StopLampLedTest(void);

#endif // _LAMP_LED_H_

