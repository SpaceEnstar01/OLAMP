/*
 * servo_manager.h
 * Servo module facade - the ONLY external interface for servo control
 *
 * Usage from Application:
 *   ServoManager::GetInstance().Init();
 *   ServoManager::GetInstance().PlayAction("smile");
 *   ServoManager::GetInstance().Stop();
 */

#ifndef SERVO_MANAGER_H
#define SERVO_MANAGER_H

#include "multi_servo.h"
#include "freertos/semphr.h"

// Read current 5 joint angles (degrees) for the active lamp model.
// Convenience wrapper around ServoManager::GetCurrentAngles.
// Order: [base_yaw, base_pitch, elbow_pitch, wrist_roll, wrist_pitch].
// Returns false on failure (out will be filled with 0).
bool ReadLampJointAngles(float out_angles[5]);

class ServoManager {
public:
    static ServoManager& GetInstance();

    // One-call initialization: UART + calibration + torque enable + soft start
    // Returns true if all servos are ready
    bool Init();

    // Play a named action (e.g. "smile")
    // Runs in a background FreeRTOS task so it won't block the caller
    bool PlayAction(const char* name);

    // Stop current action playback
    void Stop();

    // Check if an action is currently playing
    bool IsPlaying() const { return is_playing_; }

    // Check if the servo system is initialized
    bool IsInitialized() const { return multi_servo_.IsInitialized(); }

    // Direct access to MultiServo for advanced use (e.g. single servo control)
    MultiServo& GetMultiServo() { return multi_servo_; }

    // High-level helper: move all 5 joints to target angles over given duration.
    // Angles are lerobot-compatible degrees in the order:
    // [base_yaw, base_pitch, elbow_pitch, wrist_roll, wrist_pitch].
    // This call is blocking and performs a smooth multi-step move.
    bool MoveToAngles(const float angles[5], int duration_ms);

    // Lightweight single-joint move (single WritePosEx path).
    // joint_index: 0..4
    // target_angle_deg: lerobot-compatible degrees
    // duration_ms: desired move duration (used to derive speed parameter)
    bool MoveJointToAngle(uint8_t joint_index, float target_angle_deg, int duration_ms);

    // Read current 5 joint angles (degrees) from servo bus.
    // Order: [base_yaw, base_pitch, elbow_pitch, wrist_roll, wrist_pitch].
    // Returns false if read fails.
    bool GetCurrentAngles(float out[5]);

    // Get angles used for motion planning.
    // Priority:
    // 1) last successfully commanded/read cache
    // 2) one-shot servo read
    // Returns false if both unavailable.
    bool GetAnglesForMotion(float out[5]);

    // Update last-commanded cache after a successful move.
    void UpdateLastCommandedAngles(const float angles[5]);
    // Set startup baseline angles for no-feedback mode.
    void SetInitialAngles(const float angles[5]);

private:
    ServoManager();
    ~ServoManager();
    ServoManager(const ServoManager&) = delete;
    ServoManager& operator=(const ServoManager&) = delete;

    MultiServo multi_servo_;
    volatile bool is_playing_;
    float last_commanded_angles_[5];
    bool last_commanded_valid_;
    bool no_feedback_mode_;
    SemaphoreHandle_t servo_bus_mutex_;

    // Background task for action playback
    static void PlaybackTask(void* param);

    // Playback context passed to the FreeRTOS task
    struct PlaybackContext {
        ServoManager* manager;
        const int16_t* data;
        int frame_count;
        int interval_ms;
    };
};

#endif // SERVO_MANAGER_H

