#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <esp_log.h>
#include <string>
#include <vector>

/**
 * @brief 测试框架类
 * 
 * 提供统一的测试接口和结果管理
 */
class TestFramework {
public:
    struct TestResult {
        std::string test_name;
        bool passed;
        std::string message;
    };

    static TestFramework& GetInstance() {
        static TestFramework instance;
        return instance;
    }

    /**
     * @brief 初始化测试框架
     */
    void Initialize();

    /**
     * @brief 运行测试
     * @param test_name 测试名称
     */
    void RunTest(const std::string& test_name);

    /**
     * @brief 报告测试结果
     * @param test_name 测试名称
     * @param passed 是否通过
     * @param message 测试消息
     */
    void ReportResult(const std::string& test_name, bool passed, const std::string& message = "");

    /**
     * @brief 打印测试摘要
     */
    void PrintSummary();

    /**
     * @brief 清理资源
     */
    void Cleanup();

    /**
     * @brief 获取测试结果列表
     */
    const std::vector<TestResult>& GetResults() const { return results_; }

    /**
     * @brief 获取通过数量
     */
    int GetPassedCount() const { return passed_count_; }

    /**
     * @brief 获取失败数量
     */
    int GetFailedCount() const { return failed_count_; }

private:
    TestFramework() = default;
    ~TestFramework() = default;
    TestFramework(const TestFramework&) = delete;
    TestFramework& operator=(const TestFramework&) = delete;

    std::vector<TestResult> results_;
    int passed_count_ = 0;
    int failed_count_ = 0;
    std::string current_test_name_;
};

/**
 * @brief 测试断言宏
 * @param condition 断言条件
 * @param message 失败消息
 */
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            ESP_LOGE("TEST", "[%s] Assertion failed: %s", __FUNCTION__, message); \
            TestFramework::GetInstance().ReportResult(__FUNCTION__, false, message); \
            return; \
        } \
    } while(0)

/**
 * @brief 测试通过宏
 * @param test_name 测试名称
 */
#define TEST_PASS(test_name) \
    TestFramework::GetInstance().ReportResult(test_name, true, "Test passed")

/**
 * @brief 测试失败宏
 * @param test_name 测试名称
 * @param message 失败消息
 */
#define TEST_FAIL(test_name, message) \
    TestFramework::GetInstance().ReportResult(test_name, false, message)

/**
 * @brief 开始测试宏
 * @param test_name 测试名称
 */
#define TEST_START(test_name) \
    TestFramework::GetInstance().RunTest(test_name)

#endif // TEST_FRAMEWORK_H
















