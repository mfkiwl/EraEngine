﻿using System.Runtime.CompilerServices;

namespace EraEngine;

public class BackgroundServiceSystem : IESystem
{
    public ESystemPriority Priority
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public BackgroundServiceSystem()
    {
        Priority = ESystemPriority.Low;
    }

    public void Update(World world, float dt)
    {
    }
}