#ifndef CAMERA_TEST_H
#define CAMERA_TEST_H

#include "camera.h"

/**
 * @brief 摄像头测试类
 * 
 * 提供各种摄像头功能测试
 */
class CameraTest {
public:
    /**
     * @brief 运行所有摄像头测试
     * @param camera Camera指针
     */
    static void RunAllTests(Camera* camera);

    /**
     * @brief 测试摄像头初始化
     * @param camera Camera指针
     */
    static void TestCameraInit(Camera* camera);

    /**
     * @brief 测试图像捕获
     * @param camera Camera指针
     */
    static void TestImageCapture(Camera* camera);

    /**
     * @brief 测试不同分辨率
     * @param camera Camera指针
     */
    static void TestResolutions(Camera* camera);

    /**
     * @brief 测试图像质量
     * @param camera Camera指针
     */
    static void TestImageQuality(Camera* camera);

    /**
     * @brief 测试帧率
     * @param camera Camera指针
     */
    static void TestFrameRate(Camera* camera);

    /**
     * @brief 测试图像翻转
     * @param camera Camera指针
     */
    static void TestImageFlip(Camera* camera);

private:
    CameraTest() = default;
};

#endif // CAMERA_TEST_H
















