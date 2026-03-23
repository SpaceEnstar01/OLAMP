#include "lamp_led.h"
#include "mcp_server.h"
#include <esp_log.h>
#include <esp_err.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sstream>
#include <iomanip>

#define TAG "LampLed"

// LEDC配置常量
constexpr ledc_timer_t LampLed::LAMP_LEDC_TIMER;
constexpr ledc_channel_t LampLed::LAMP_LEDC_CHANNEL;
constexpr ledc_timer_bit_t LampLed::LAMP_LEDC_RESOLUTION;
constexpr uint32_t LampLed::LAMP_LEDC_FREQ_HZ;
constexpr uint32_t LampLed::LAMP_LEDC_MAX_DUTY;

LampLed::LampLed(gpio_num_t gpio, bool output_invert)
    : gpio_(gpio),
      output_invert_(output_invert),
      is_on_(false),
      brightness_(0),
      ledc_initialized_(false) {
    
    ESP_LOGI(TAG, "Initializing LampLed on GPIO %d", gpio_);
    
    // 初始化LEDC PWM
    InitLedc();
    
    // 初始状态：关灯
    SetBrightness(0, 0);
    
    // 注册MCP工具
    RegisterMcpTools();
    
    // 设置全局实例（用于关键词直接控制）
    SetLampLed(this);
    
    ESP_LOGI(TAG, "LampLed initialized successfully");
}

LampLed::~LampLed() {
    if (ledc_initialized_) {
        ledc_stop(LEDC_LOW_SPEED_MODE, LAMP_LEDC_CHANNEL, 0);
    }
}

void LampLed::InitLedc() {
    // 先重置GPIO，确保没有被其他功能占用
    gpio_reset_pin(gpio_);
    vTaskDelay(pdMS_TO_TICKS(10));  // 等待GPIO重置完成
    
    ESP_LOGI(TAG, "GPIO %d reset, starting LEDC initialization", gpio_);
    
    // 配置LEDC定时器（参考成功测试代码）
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.timer_num = LAMP_LEDC_TIMER;
    timer_conf.duty_resolution = LAMP_LEDC_RESOLUTION;
    timer_conf.freq_hz = LAMP_LEDC_FREQ_HZ;
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(err));
        return;
    }
    
    // 配置LEDC通道（参考成功测试代码）
    ledc_channel_config_t ch_conf = {};
    ch_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    ch_conf.channel = LAMP_LEDC_CHANNEL;
    ch_conf.timer_sel = LAMP_LEDC_TIMER;
    ch_conf.intr_type = LEDC_INTR_DISABLE;
    ch_conf.gpio_num = gpio_;
    ch_conf.duty = 0;  // 初始关灯
    ch_conf.hpoint = 0;
    ch_conf.flags.output_invert = output_invert_ ? 1 : 0;
    
    err = ledc_channel_config(&ch_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel: %s", esp_err_to_name(err));
        return;
    }
    
    // 保存通道配置（用于后续操作）
    ledc_channel_.channel = LAMP_LEDC_CHANNEL;
    ledc_channel_.gpio_num = gpio_;
    ledc_channel_.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_channel_.timer_sel = LAMP_LEDC_TIMER;
    
    ledc_initialized_ = true;
    ESP_LOGI(TAG, "LEDC initialized: Timer=%d, Channel=%d, GPIO=%d, Resolution=8 bit", 
             LAMP_LEDC_TIMER, LAMP_LEDC_CHANNEL, gpio_);
}

void LampLed::SetDuty(uint32_t duty, uint32_t fade_ms) {
    if (!ledc_initialized_) {
        ESP_LOGE(TAG, "LEDC not initialized, cannot set duty!");
        return;
    }
    
    // 限制占空比范围
    if (duty > LAMP_LEDC_MAX_DUTY) {
        duty = LAMP_LEDC_MAX_DUTY;
    }
    
    ESP_LOGI(TAG, "Setting duty: %lu/%lu", duty, LAMP_LEDC_MAX_DUTY);
    
    // 直接设置占空比，不使用fade
    esp_err_t err1 = ledc_set_duty(LEDC_LOW_SPEED_MODE, LAMP_LEDC_CHANNEL, duty);
    if (err1 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set duty: %s", esp_err_to_name(err1));
        return;
    }
    
    esp_err_t err2 = ledc_update_duty(LEDC_LOW_SPEED_MODE, LAMP_LEDC_CHANNEL);
    if (err2 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update duty: %s", esp_err_to_name(err2));
    } else {
        ESP_LOGI(TAG, "Duty updated successfully to %lu", duty);
    }
}

uint32_t LampLed::BrightnessToDuty(uint8_t brightness) const {
    if (brightness > 100) {
        brightness = 100;
    }
    // 将0-100的亮度值转换为0-255的占空比（8位分辨率）
    return (LAMP_LEDC_MAX_DUTY * brightness) / 100;
}

void LampLed::TurnOn(uint32_t fade_ms) {
    (void)fade_ms;  // 不使用fade，保留参数以兼容接口
    ESP_LOGI(TAG, "TurnOn called, ledc_initialized=%d", ledc_initialized_);
    std::lock_guard<std::mutex> lock(mutex_);
    
    brightness_ = 100;
    is_on_ = true;
    uint32_t duty = BrightnessToDuty(100);
    ESP_LOGI(TAG, "TurnOn: setting duty to %lu", duty);
    SetDuty(duty, 0);  // 立即切换，不使用fade
    
    ESP_LOGI(TAG, "Lamp turned ON");
}

void LampLed::TurnOff(uint32_t fade_ms) {
    (void)fade_ms;  // 不使用fade，保留参数以兼容接口
    std::lock_guard<std::mutex> lock(mutex_);
    
    brightness_ = 0;
    is_on_ = false;
    ESP_LOGI(TAG, "TurnOff: setting duty to 0");
    SetDuty(0, 0);  // 立即切换，不使用fade
    
    ESP_LOGI(TAG, "Lamp turned OFF");
}

void LampLed::Toggle(uint32_t fade_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_on_) {
        TurnOff(fade_ms);
    } else {
        TurnOn(fade_ms);
    }
}

void LampLed::SetBrightness(uint8_t brightness, uint32_t fade_ms) {
    (void)fade_ms;  // 不使用fade，保留参数以兼容接口
    std::lock_guard<std::mutex> lock(mutex_);
    
    ESP_LOGI(TAG, "SetBrightness called: brightness=%d, ledc_initialized=%d", 
             brightness, ledc_initialized_);
    
    if (brightness > 100) {
        brightness = 100;
    }
    
    brightness_ = brightness;
    is_on_ = (brightness_ > 0);
    
    uint32_t duty = BrightnessToDuty(brightness_);
    ESP_LOGI(TAG, "Calculated duty: %lu (from brightness %d)", duty, brightness_);
    SetDuty(duty, 0);  // 立即切换，不使用fade
    
    ESP_LOGI(TAG, "Brightness set to %d%% (duty=%lu)", brightness_, duty);
}

std::string LampLed::GetStateJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "{\"power\":" << (is_on_ ? "true" : "false") 
        << ",\"brightness\":" << static_cast<int>(brightness_) << "}";
    return oss.str();
}

void LampLed::RegisterMcpTools() {
    auto& mcp_server = McpServer::GetInstance();
    
    // 获取台灯状态
    mcp_server.AddTool("self.lamp.get_state",
        "Get the current state of the lamp (power and brightness).\n"
        "Returns JSON with 'power' (true/false) and 'brightness' (0-100).",
        PropertyList(),
        [this](const PropertyList& properties) -> ReturnValue {
            return this->GetStateJson();
        });
    
    // 开灯
    mcp_server.AddTool("self.lamp.turn_on",
        "Turn on the lamp to maximum brightness.\n"
        "When user says '开灯', '打开台灯', '亮灯' etc., call this tool.",
        PropertyList(),
        [this](const PropertyList& properties) -> ReturnValue {
            this->TurnOn(500);  // 500ms渐变
            return this->GetStateJson();
        });
    
    // 关灯
    mcp_server.AddTool("self.lamp.turn_off",
        "Turn off the lamp (set brightness to 0).\n"
        "When user says '关灯', '关闭台灯', '熄灯' etc., call this tool.",
        PropertyList(),
        [this](const PropertyList& properties) -> ReturnValue {
            this->TurnOff(500);  // 500ms渐变
            return this->GetStateJson();
        });
    
    // 切换开关
    mcp_server.AddTool("self.lamp.toggle",
        "Toggle the lamp on/off state.\n"
        "When user says '切换台灯', '开关灯' etc., call this tool.",
        PropertyList(),
        [this](const PropertyList& properties) -> ReturnValue {
            this->Toggle(500);
            return this->GetStateJson();
        });
    
    // 设置亮度
    mcp_server.AddTool("self.lamp.set_brightness",
        "Set the lamp brightness level (0-100).\n"
        "When user says '调亮一点', '调暗一点', '设置亮度为50%' etc., call this tool.\n"
        "Args:\n"
        "  `brightness`: Brightness level from 0 to 100 (0=off, 100=maximum)",
        PropertyList({
            Property("brightness", kPropertyTypeInteger, 50, 0, 100)
        }),
        [this](const PropertyList& properties) -> ReturnValue {
            int brightness = properties["brightness"].value<int>();
            this->SetBrightness(static_cast<uint8_t>(brightness), 300);  // 300ms渐变
            return this->GetStateJson();
        });
    
    ESP_LOGI(TAG, "MCP tools registered: self.lamp.*");
}

// 全局LampLed实例指针（用于关键词直接控制）
static LampLed* g_lamp_led_instance = nullptr;

LampLed* GetLampLed() {
    return g_lamp_led_instance;
}

void SetLampLed(LampLed* lamp_led) {
    g_lamp_led_instance = lamp_led;
    if (lamp_led) {
        ESP_LOGI(TAG, "Global LampLed instance set");
    }
}

