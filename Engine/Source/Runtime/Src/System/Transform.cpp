//
// Created by johnk on 2025/1/21.
//

#include <Runtime/System/Transform.h>

namespace Runtime {
    TransformSystem::TransformSystem(ECRegistry& inRegistry, const SystemSetupContext& inContext)
        : System(inRegistry, inContext)
        , worldTransformUpdatedObserver(registry.Observer())
        , localTransformUpdatedObserver(registry.Observer())
    {
        worldTransformUpdatedObserver.ObUpdated<WorldTransform>();
        localTransformUpdatedObserver.ObUpdated<LocalTransform>();
    }

    TransformSystem::~TransformSystem() = default;

    void TransformSystem::Tick(float inDeltaTimeSeconds)
    {
        // Step0: classify the updated entities
        std::vector<Entity> pendingUpdateLocalTransforms;
        std::vector<Entity> pendingUpdateChildrenWorldTransforms;
        std::vector<Entity> pendingUpdateSelfAndChildrenWorldTransforms;

        pendingUpdateLocalTransforms.reserve(worldTransformUpdatedObserver.Count());
        pendingUpdateChildrenWorldTransforms.reserve(worldTransformUpdatedObserver.Count());
        worldTransformUpdatedObserver.EachThenClear([&](Entity e) -> void {
            if (registry.Has<LocalTransform>(e) && registry.Has<Hierarchy>(e) && HierarchyOps::HasParent(registry, e)) {
                pendingUpdateLocalTransforms.emplace_back(e);
            }

            if (registry.Has<Hierarchy>(e) && HierarchyOps::HasChildren(registry, e)) {
                pendingUpdateChildrenWorldTransforms.emplace_back(e);
            }
        });

        pendingUpdateSelfAndChildrenWorldTransforms.reserve(localTransformUpdatedObserver.Count());
        localTransformUpdatedObserver.EachThenClear([&](Entity e) -> void {
            if (registry.Has<WorldTransform>(e) && registry.Has<Hierarchy>(e) && HierarchyOps::HasParent(registry, e)) {
                pendingUpdateSelfAndChildrenWorldTransforms.emplace_back(e);
            }
        });

        // Step1: update local transforms
        for (auto e : pendingUpdateLocalTransforms) {
            auto& localTransform = registry.Get<LocalTransform>(e);
            const auto& worldTransform = registry.Get<WorldTransform>(e);
            const auto& hierarchy = registry.Get<Hierarchy>(e);
            const auto& parentWorldTransform = registry.Get<WorldTransform>(hierarchy.parent);

            const auto& localToWorldMatrix = worldTransform.localToWorld.GetTransformMatrix();
            const auto& parentLocalToWorldMatrix = parentWorldTransform.localToWorld.GetTransformMatrix();
            localTransform.localToParent = Common::FTransform(parentLocalToWorldMatrix.Inverse() * localToWorldMatrix);
        }

        // Step2: update world transforms
        const auto updateWorldByLocal = [&](Entity child, Entity parent) -> void {
            if (!registry.Has<LocalTransform>(child) || !registry.Has<WorldTransform>(child) || !registry.Has<WorldTransform>(parent)) {
                return;
            }

            auto& childWorldTransform = registry.Get<WorldTransform>(child);
            const auto& childLocalTransform = registry.Get<LocalTransform>(child);
            const auto& parentWorldTransform = registry.Get<WorldTransform>(parent);

            const auto& parentLocalToWorldMatrix = parentWorldTransform.localToWorld.GetTransformMatrix();
            const auto& childLocalToParentMatrix = childLocalTransform.localToParent.GetTransformMatrix();
            childWorldTransform.localToWorld = Common::FTransform(parentLocalToWorldMatrix * childLocalToParentMatrix);
        };

        for (const auto e : pendingUpdateChildrenWorldTransforms) {
            HierarchyOps::TraverseChildrenRecursively(registry, e, [&](Entity child, Entity parent) -> void {
                updateWorldByLocal(child, parent);
            });
        }
        for (const auto e : pendingUpdateSelfAndChildrenWorldTransforms) {
            const auto& hierarchy = registry.Get<Hierarchy>(e);
            updateWorldByLocal(e, hierarchy.parent);

            HierarchyOps::TraverseChildrenRecursively(registry, e, [&](Entity child, Entity parent) -> void {
                updateWorldByLocal(child, parent);
            });
        }
    }
}
