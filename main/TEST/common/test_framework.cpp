#include "test_framework.h"
#include <esp_log.h>
#include <cstdio>

#define TAG "TestFramework"

void TestFramework::Initialize() {
    ESP_LOGI(TAG, "Initializing test framework...");
    results_.clear();
    passed_count_ = 0;
    failed_count_ = 0;
    ESP_LOGI(TAG, "Test framework initialized");
}

void TestFramework::RunTest(const std::string& test_name) {
    current_test_name_ = test_name;
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Running test: %s", test_name.c_str());
    ESP_LOGI(TAG, "========================================");
}

void TestFramework::ReportResult(const std::string& test_name, bool passed, const std::string& message) {
    TestResult result;
    result.test_name = test_name.empty() ? current_test_name_ : test_name;
    result.passed = passed;
    result.message = message;

    results_.push_back(result);

    if (passed) {
        passed_count_++;
        ESP_LOGI(TAG, "[PASS] %s", result.test_name.c_str());
        if (!message.empty()) {
            ESP_LOGI(TAG, "  Message: %s", message.c_str());
        }
    } else {
        failed_count_++;
        ESP_LOGE(TAG, "[FAIL] %s", result.test_name.c_str());
        if (!message.empty()) {
            ESP_LOGE(TAG, "  Error: %s", message.c_str());
        }
    }
}

void TestFramework::PrintSummary() {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Test Summary");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Total tests: %d", (int)results_.size());
    ESP_LOGI(TAG, "Passed: %d", passed_count_);
    ESP_LOGI(TAG, "Failed: %d", failed_count_);
    ESP_LOGI(TAG, "Success rate: %.1f%%", 
             results_.size() > 0 ? (passed_count_ * 100.0f / results_.size()) : 0.0f);
    ESP_LOGI(TAG, "========================================");

    if (failed_count_ > 0) {
        ESP_LOGW(TAG, "Failed tests:");
        for (const auto& result : results_) {
            if (!result.passed) {
                ESP_LOGW(TAG, "  - %s: %s", result.test_name.c_str(), result.message.c_str());
            }
        }
    }
    ESP_LOGI(TAG, "");
}

void TestFramework::Cleanup() {
    PrintSummary();
    ESP_LOGI(TAG, "Test framework cleanup completed");
}
















