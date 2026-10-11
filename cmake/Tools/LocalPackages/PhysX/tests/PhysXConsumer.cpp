/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <PxPhysicsAPI.h>
#include <characterkinematic/PxCapsuleController.h>
#include <characterkinematic/PxControllerManager.h>
#include <vehicle/PxVehicleSDK.h>
#include <vehicle/PxVehicleUtilSetup.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace JE::Tests
{
using namespace physx;
void Require(bool value, const char* message)
{
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
class Errors final : public PxErrorCallback
{
public:
    unsigned count = 0;
    void reportError(PxErrorCode::Enum code, const char* message, const char*, int) override
    {
        ++count;
        std::fprintf(stderr, "PhysX diagnostic %u: %s\n", static_cast<unsigned>(code), message);
    }
};
struct SDK
{
    PxDefaultAllocator allocator;
    Errors errors;
    PxFoundation* foundation;
    PxPvd* pvd;
    PxPhysics* physics;
    PxCooking* cooking;
    SDK()
    {
        foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errors);
        Require(foundation != nullptr, "Foundation creation");
        pvd = PxCreatePvd(*foundation);
        Require(pvd != nullptr, "PVD SDK creation without a network transport");
        physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), true, pvd);
        Require(physics != nullptr && PxInitExtensions(*physics, pvd), "Physics/Extensions creation");
        cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, PxCookingParams(physics->getTolerancesScale()));
        Require(cooking != nullptr, "Cooking creation");
    }
    ~SDK()
    {
        const unsigned errorsBefore = errors.count;
        cooking->release();
        PxCloseExtensions();
        physics->release();
        pvd->release();
        foundation->release();
        Require(errors.count == errorsBefore, "SDK teardown produces no additional errors");
    }
};
PxTriangleMesh* FloorMesh(SDK& sdk)
{
    const PxVec3 vertices[] = { {-10,0,-10}, {10,0,-10}, {-10,0,10}, {10,0,10} };
    const PxU32 triangles[] = {0,2,1, 1,2,3};
    PxTriangleMeshDesc desc;
    desc.points.count = 4; desc.points.stride = sizeof(PxVec3); desc.points.data = vertices;
    desc.triangles.count = 2; desc.triangles.stride = 3 * sizeof(PxU32); desc.triangles.data = triangles;
    PxDefaultMemoryOutputStream output;
    Require(sdk.cooking->cookTriangleMesh(desc, output) && output.getSize() > 0, "Triangle mesh CPU cooking");
    PxDefaultMemoryInputData input(output.getData(), output.getSize());
    PxTriangleMesh* mesh = sdk.physics->createTriangleMesh(input);
    Require(mesh && mesh->getNbVertices() == 4 && mesh->getNbTriangles() == 2, "Cooked triangle mesh readback");
    return mesh;
}
struct World
{
    SDK& sdk;
    PxDefaultCpuDispatcher* dispatcher;
    PxScene* scene;
    PxMaterial* material;
    PxTriangleMesh* mesh;
    PxRigidStatic* floor;
    explicit World(SDK& owner) : sdk(owner)
    {
        dispatcher = PxDefaultCpuDispatcherCreate(1);
        Require(dispatcher != nullptr, "CPU dispatcher");
        PxSceneDesc desc(sdk.physics->getTolerancesScale());
        desc.gravity = PxVec3(0,-9.81f,0); desc.cpuDispatcher = dispatcher;
        desc.filterShader = PxDefaultSimulationFilterShader;
        scene = sdk.physics->createScene(desc);
        Require(scene != nullptr, "CPU scene creation");
        material = sdk.physics->createMaterial(0.5f,0.5f,0.0f);
        Require(material != nullptr, "Material creation");
        mesh = FloorMesh(sdk);
        floor = PxCreateStatic(*sdk.physics, PxTransform(PxIdentity), PxTriangleMeshGeometry(mesh), *material);
        Require(floor != nullptr, "WhiteBox-style triangle-mesh collider");
        scene->addActor(*floor);
    }
    ~World()
    {
        floor->release(); scene->release(); dispatcher->release(); mesh->release(); material->release();
    }
};
void Cooking(SDK& sdk)
{
    PxTriangleMesh* mesh = FloorMesh(sdk); mesh->release();
    const PxVec3 points[] = { {0,0,0}, {1,0,0}, {0,1,0}, {0,0,1} };
    PxConvexMeshDesc desc;
    desc.points.count = 4; desc.points.stride = sizeof(PxVec3); desc.points.data = points;
    // Preserve O3DE's cooking request; this does not enable GPU simulation.
    desc.flags = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eGPU_COMPATIBLE;
    PxDefaultMemoryOutputStream output;
    Require(sdk.cooking->cookConvexMesh(desc, output) && output.getSize() > 0, "Convex GPU-compatible CPU cooking");
    PxDefaultMemoryInputData input(output.getData(), output.getSize());
    PxConvexMesh* convex = sdk.physics->createConvexMesh(input);
    Require(convex && convex->getNbVertices() == 4, "Convex mesh readback"); convex->release();
}
void Scene(SDK& sdk)
{
    World world(sdk);
    PxRigidDynamic* body = PxCreateDynamic(*sdk.physics, PxTransform(PxVec3(0,3,0)), PxSphereGeometry(0.5f), *world.material, 1.0f);
    Require(body != nullptr, "Dynamic body creation"); world.scene->addActor(*body);
    for (unsigned frame = 0; frame < 120; ++frame) { world.scene->simulate(1.0f/60); Require(world.scene->fetchResults(true), "Simulation completion"); }
    Require(body->getGlobalPose().p.y > 0.45f && body->getGlobalPose().p.y < 0.6f, "Dynamic body rests on cooked collider");
    PxRaycastBuffer hit;
    Require(world.scene->raycast(PxVec3(5,2,5), PxVec3(0,-1,0), 4, hit) && hit.hasBlock && hit.block.actor == world.floor,
        "Scene raycast returns the cooked collider");
    Require(std::fabs(hit.block.distance - 2.0f) < 0.001f, "Raycast distance"); body->release();
}
void Character(SDK& sdk)
{
    World world(sdk);
    PxControllerManager* manager = PxCreateControllerManager(*world.scene);
    Require(manager != nullptr, "Controller manager creation");
    PxCapsuleControllerDesc desc;
    desc.height = 1; desc.radius = 0.5f; desc.position = PxExtendedVec3(3,3,3); desc.material = world.material;
    desc.stepOffset = 0.2f; desc.contactOffset = 0.05f;
    PxController* controller = manager->createController(desc);
    Require(controller != nullptr, "Capsule controller creation");
    const auto collision = controller->move(PxVec3(0,-5,0), 0.001f, 1.0f/60, PxControllerFilters());
    Require(collision.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN), "Controller collides with cooked floor");
    Require(controller->getPosition().y > 0 && controller->getPosition().y < 3, "Controller movement readback");
    controller->release(); manager->release();
}
void Vehicle(SDK& sdk)
{
    Require(PxInitVehicleSDK(*sdk.physics), "Vehicle SDK initialization");
    const PxVec3 supports[] = { {-1,0,-1}, {1,0,-1}, {-1,0,1}, {1,0,1} };
    PxReal masses[4] = {};
    PxVehicleComputeSprungMasses(4, supports, PxVec3(0), 100, 1, masses);
    for (float mass : masses) { Require(std::fabs(mass - 25.0f) < 0.001f, "Symmetric sprung-mass computation"); }
    PxCloseVehicleSDK();
}
void Invalid(SDK& sdk)
{
    PxTriangleMeshDesc invalid;
    PxDefaultMemoryOutputStream output;
    const unsigned char sentinel[] = {0x4a,0x45,0x21}; output.write(sentinel, sizeof(sentinel));
    const unsigned errorsBefore = sdk.errors.count;
    Require(!sdk.cooking->cookTriangleMesh(invalid, output), "Invalid mesh rejected");
    Require(output.getSize() == sizeof(sentinel) && std::memcmp(output.getData(), sentinel, sizeof(sentinel)) == 0,
        "Invalid cooking leaves caller output unchanged");
    Require(sdk.errors.count > errorsBefore, "Invalid cooking reports a diagnostic");
}
}
int main(int argc, char** argv)
{
    using namespace JE::Tests;
    Require(argc == 2, "One case name required");
    Require(PX_PHYSICS_VERSION_MAJOR == 5 && PX_PHYSICS_VERSION_MINOR == 1 && PX_PHYSICS_VERSION_BUGFIX == 1, "Exact PhysX SDK 5.1.1 API");
    {
    SDK sdk;
    if (!std::strcmp(argv[1], "lifecycle")) { Require(sdk.physics->getTolerancesScale().isValid(), "SDK tolerances"); }
    else if (!std::strcmp(argv[1], "cooking")) { Cooking(sdk); }
    else if (!std::strcmp(argv[1], "scene")) { Scene(sdk); }
    else if (!std::strcmp(argv[1], "character")) { Character(sdk); }
    else if (!std::strcmp(argv[1], "vehicle")) { Vehicle(sdk); }
    else if (!std::strcmp(argv[1], "invalid")) { Invalid(sdk); }
    else { Require(false, "Unknown case"); }
    if (std::strcmp(argv[1], "invalid")) { Require(sdk.errors.count == 0, "Valid operations produce no PhysX errors"); }
    }
    std::printf("CASE %s completed against PhysX 5.1.1 CPU\n", argv[1]);
    return 0;
}
