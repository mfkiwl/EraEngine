﻿using EraScriptingCore.Extensions;
using System.Numerics;
using System.Runtime.InteropServices;

namespace EraScriptingCore.Domain.Components;

public enum ForceMode : uint
{
    None,
    Force,
    Impulse
}

public class RigidbodyComponent : EComponent
{
    private int _mass = 0;

    public void AddForce(Vector3 force, ForceMode mode) 
    {
        IntPtr vec = Memory.Vector3ToIntPtr(force);
        AddForce_Impl(Entity.Id, (uint)mode, vec);
        Memory.ReleaseIntPtr(vec);
    }

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void AddForce_Impl(uint id, uint mode, IntPtr force);
}
