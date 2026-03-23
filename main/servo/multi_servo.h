/*
 * multi_servo.h
 * Multi-servo controller with lerobot-compatible angle conversion
 * 
 * Supports synchronized control of multiple STS servos with per-servo
 * calibration data and pre-recorded action playback.
 */

#ifndef MULTI_SERVO_H
#define MULTI_SERVO_H

#include "driver/uart.h"
#include "st_servo/SMS_STS.h"
#include <stdint.h>

// Maximum number of servos supported
#define MULTI_SERVO_MAX 5

// Servo configuration (per-servo calibration data)
struct ServoConfig {
    uint8_t id;            // Servo ID on the bus (1-5)
    uint16_t range_min;    // Minimum position register value
    uint16_t range_max;    // Maximum position register value
    int16_t homing_offset; // Homing offset (for EEPROM write only)
};

class MultiServo {
public:
    MultiServo();
    ~MultiServo();

    // Initialize UART bus for servo communication
    bool Init(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate);

    // Set configuration for a specific servo index (0-based)
    void SetServoConfig(uint8_t index, const ServoConfig& config);

    // Load all servo configs at once
    void SetAllConfigs(const ServoConfig configs[], int count);

    // Apply homing offsets to servo EEPROM (only needed once per servo)
    int ApplyCalibration();

    // Move a single servo to target angle (lerobot-compatible degrees)
    bool MoveTo(uint8_t index, float angle, int speed = 0, int acc = 0);

    // Synchronously move all servos to target angles
    bool SyncMoveTo(const float angles[], int count, int speed = 0, int acc = 0);

    // Play a pre-recorded action sequence
    // data: flat array of int16_t angles, MULTI_SERVO_MAX values per frame
    // frame_count: number of frames
    // interval_ms: time between frames in milliseconds
    bool PlayAction(const int16_t data[], int frame_count, int interval_ms = 50);

    // Stop action playback (if running in a task)
    void StopAction();

    // Enable/disable torque for all servos
    bool EnableTorqueAll(bool enable);

    // Get servo count
    int GetServoCount() const { return servo_count_; }

    // Check if initialized
    bool IsInitialized() const { return initialized_; }

    // Read current 5 joint angles (degrees), order: [base_yaw, base_pitch, elbow_pitch, wrist_roll, wrist_pitch].
    // Returns false if any servo read failed (failed joints are filled with 0).
    bool ReadCurrentAngles(float out[5]);

private:
    SMS_STS servo_;
    ServoConfig configs_[MULTI_SERVO_MAX];
    int servo_count_;
    bool initialized_;
    volatile bool stop_requested_;

    // Lerobot-compatible angle to position conversion
    int16_t AngleToPosition(float angle, uint8_t index);

    // Position to angle (inverse conversion)
    float PositionToAngle(int16_t position, uint8_t index);
};

#endif // MULTI_SERVO_H






