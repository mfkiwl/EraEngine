﻿namespace EraEngine.Core;

public class CollisionHandler : IEHandler
{
    [UnmanagedCaller]
    public static void HandleCollision(int id1, int id2)
    {
        if (EWorld.SceneWorld.Entities.TryGetValue(id1, out var entity1)
            && EWorld.SceneWorld.Entities.TryGetValue(id2, out var entity2))
        {
            var res1 = Parallel.ForEach(entity1.Components.Values, c => c.OnCollisionEnter(entity2));
            var res2 = Parallel.ForEach(entity2.Components.Values, c => c.OnCollisionEnter(entity1));

            if (!res1.IsCompleted || !res2.IsCompleted)
                Debug.LogError("Failed to call OnCollisionEnter.");
        }
    }
}
