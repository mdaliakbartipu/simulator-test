
#ifndef msr_AirLibUnitTests_SimpleFlightTest_hpp
#define msr_AirLibUnitTests_SimpleFlightTest_hpp

#include "vehicles/MultiRotorParamsFactory.hpp"
#include "TestBase.hpp"
#include "physics/PhysicsWorld.hpp"
#include "physics/FastPhysicsEngine.hpp"
#include "controllers/DroneControllerCancelable.hpp"
#include "common/SteppableClock.hpp"

namespace msr { namespace airlib {

class SimpleFlightTest : public TestBase
{
public:
    virtual void run() override
    {
        auto clock = std::make_shared<SteppableClock>();
        ClockFactory::get(clock);

        std::unique_ptr<MultiRotorParams> params = MultiRotorParamsFactory::createConfig("SimpleFlight");
        MultiRotor vehicle;
        std::unique_ptr<Environment> environment;
        vehicle.initialize(params.get(), Pose(), 
            GeoPoint(), environment);

        std::vector<UpdatableObject*> vehicles = { &vehicle };
        std::unique_ptr<PhysicsEngineBase> physics_engine(new FastPhysicsEngine());
        PhysicsWorld physics_world(physics_engine.get(), vehicles, 
            static_cast<uint64_t>(clock->getStepSize() * 1E9));

        DroneControllerBase* controller = params->getController();
        testAssert(controller != nullptr, "Controller was null");
        std::string message;
        testAssert(controller->isAvailable(message), message);

        clock->sleep_for(0.04f);
        
        DirectCancelableBase cancellable(controller, &vehicle);
        controller->setOffboardMode(true);
        controller->armDisarm(true, cancellable);
        controller->takeoff(10, cancellable);

        while (true) {
            clock->sleep_for(0.1f);
            controller->getStatusMessages(messages_);
            for (const auto& status_message : messages_) {
                std::cout << status_message << std::endl;
            }
            messages_.clear();

            std::cout << VectorMath::toString(vehicle.getKinematics().pose.position) << std::endl;
        }
    }

private:
    std::vector<std::string> messages_;
    
private:
    class DirectCancelableBase : public CancelableBase
    {
    public:
        DirectCancelableBase(DroneControllerBase* controller, const MultiRotor* vehicle)
            : controller_(controller), vehicle_(vehicle)
        {
        }
        virtual void execute() override
        {}

        virtual bool sleep(double secs) override 
        {
            controller_->getStatusMessages(messages_);
            for (const auto& status_message : messages_) {
                std::cout << status_message << std::endl;
            }
            messages_.clear();

            std::cout << VectorMath::toString(vehicle_->getKinematics().pose.position) << std::endl;

            return CancelableBase::sleep(secs);
        };

    private:
        DroneControllerBase* controller_;
        const MultiRotor* vehicle_;
        std::vector<std::string> messages_;
    };

};


}}
#endif