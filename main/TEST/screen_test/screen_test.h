#ifndef SCREEN_TEST_H
#define SCREEN_TEST_H

#include "display.h"

/**
 * @brief 屏幕测试类
 * 
 * 提供各种屏幕功能测试
 */
class ScreenTest {
public:
    /**
     * @brief 运行所有屏幕测试
     * @param display Display指针
     */
    static void RunAllTests(Display* display);

    /**
     * @brief 测试基本显示功能
     * @param display Display指针
     */
    static void TestBasicDisplay(Display* display);

    /**
     * @brief 测试颜色显示
     * @param display Display指针
     */
    static void TestColors(Display* display);

    /**
     * @brief 测试文本显示
     * @param display Display指针
     */
    static void TestTextDisplay(Display* display);

    /**
     * @brief 测试图像显示
     * @param display Display指针
     */
    static void TestImageDisplay(Display* display);

    /**
     * @brief 测试刷新率
     * @param display Display指针
     */
    static void TestRefreshRate(Display* display);

    /**
     * @brief 测试背光控制
     * @param display Display指针
     */
    static void TestBacklight(Display* display);

private:
    ScreenTest() = default;
};

#endif // SCREEN_TEST_H






































