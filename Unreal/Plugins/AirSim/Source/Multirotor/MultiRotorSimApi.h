#pragma once

#include "CoreMinimal.h"
#include "VehicleSimApi.h"
#include "vehicles/multirotor/MultiRotor.hpp"
#include "vehicles/multirotor/MultiRotorParams.hpp"
#include "physics//Kinematics.hpp"
#include "common/Common.hpp"
#include "common/CommonStructs.hpp"
#include "ManualPoseController.h"
#include <chrono>
#include "SimJoyStick/SimJoyStick.h"
#include <future>


class MultirotorSimApi : public VehicleSimApi
{
public:
    typedef msr::airlib::real_T real_T;
    typedef msr::airlib::Utils Utils;
    typedef msr::airlib::MultiRotor MultiRotor;
    typedef msr::airlib::StateReporter StateReporter;
    typedef msr::airlib::UpdatableObject UpdatableObject;
    typedef msr::airlib::Pose Pose;


public:
    virtual ~MultirotorSimApi() = default;

    //VehicleSimApiBase interface
    //implements game interface to update pawn
    MultirotorSimApi(const msr::airlib::GeoPoint& home_geopoint, msr::airlib::MultiRotorParams* vehicle_params, 
        UManualPoseController* manual_pose_controller, APawn* pawn, const std::vector<APIPCamera*>& cameras, const std::string& vehicle_name, 
        const Config& config = Config());
    virtual void updateRenderedState(float dt) override;
    virtual void updateRendering(float dt) override;

    //PhysicsBody interface
    //this just wrapped around MultiRotor physics body
    virtual void reset() override;
    virtual void update() override;
    virtual void reportState(StateReporter& reporter) override;
    virtual UpdatableObject* getPhysicsBody() override;
    virtual msr::airlib::RCData getRCData() const override;

    virtual void setPose(const Pose& pose, bool ignore_collision) override;


private:
    void detectUsbRc();

private:
    msr::airlib::MultirotorApiBase* vehicle_api_;
    msr::airlib::MultiRotorParams* vehicle_params_;
    UManualPoseController* manual_pose_controller_;

    std::unique_ptr<MultiRotor> phys_vehicle_;

    struct RotorInfo {
        real_T rotor_speed = 0;
        int rotor_direction = 0;
        real_T rotor_thrust = 0;
        real_T rotor_control_filtered = 0;
    };
    unsigned int rotor_count_;
    std::vector<RotorInfo> rotor_info_;

    CollisionResponseInfo collision_response_info;

    mutable msr::airlib::RCData rc_data_;
    mutable SimJoyStick joystick_;
    mutable SimJoyStick::State joystick_state_;

    bool pending_pose_collisions_;
    enum class PendingPoseStatus {
        NonePending, RenderStatePending, RenderPending
    } pending_pose_status_;
    Pose pending_pose_; //force new pose through API

    //reset must happen while World is locked so its async task initiated from API thread
    bool reset_pending_;
    bool did_reset_;
    std::packaged_task<void()> reset_task_;

    Pose last_pose_; //for trace lines showing vehicle path
    std::vector<std::string> vehicle_api_messages_;

};
