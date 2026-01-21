#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <esp_log.h>
#include <esp_err.h>
#include <driver/gpio.h>
#include <string>
#include <cstdint>

/**
 * @brief 测试工具函数命名空间
 */
namespace TestUtils {

    /**
     * @brief 初始化基础系统
     * 
     * 初始化NVS、事件循环等ESP-IDF基础组件
     */
    void InitializeSystem();

    /**
     * @brief 延迟指定毫秒数
     * @param ms 延迟毫秒数
     */
    void DelayMs(uint32_t ms);

    /**
     * @brief 延迟指定秒数
     * @param seconds 延迟秒数
     */
    void DelaySeconds(uint32_t seconds);

    /**
     * @brief 打印测试信息
     * @param module 模块名称
     * @param info 信息内容
     */
    void PrintTestInfo(const std::string& module, const std::string& info);

    /**
     * @brief 打印测试错误
     * @param module 模块名称
     * @param error 错误信息
     */
    void PrintTestError(const std::string& module, const std::string& error);

    /**
     * @brief 检查GPIO状态
     * @param gpio GPIO编号
     * @param level 期望的电平 (0或1)
     * @return true 如果GPIO状态匹配
     */
    bool CheckGpioState(gpio_num_t gpio, int level);

    /**
     * @brief 设置GPIO输出
     * @param gpio GPIO编号
     * @param level 输出电平
     */
    void SetGpioOutput(gpio_num_t gpio, int level);

    /**
     * @brief 格式化测试结果
     * @param passed 是否通过
     * @param message 消息
     * @return 格式化后的字符串
     */
    std::string FormatResult(bool passed, const std::string& message);

    /**
     * @brief 检查ESP错误码
     * @param err ESP错误码
     * @param operation 操作名称
     * @return true 如果成功
     */
    bool CheckEspError(esp_err_t err, const std::string& operation);

    /**
     * @brief 打印分隔线
     * @param width 分隔线宽度
     */
    void PrintSeparator(int width = 50);

    /**
     * @brief 打印测试标题
     * @param title 标题
     */
    void PrintTestTitle(const std::string& title);

} // namespace TestUtils

#endif // TEST_UTILS_H
















