﻿using System.Numerics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace EraEngine.Components;

public sealed class BoxCharacterController : CharacterController
{
    /// <summary>
    /// X
    /// </summary>
    public float Height
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private set;
    }

    /// <summary>
    /// Y
    /// </summary>
    public float Length
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private set;
    }

    /// <summary>
    /// Z
    /// </summary>
    public float Width
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private set;
    }

    internal override void InitializeComponentInternal(params object[] args)
    {
        if (args.Length < 3)
            throw new ArgumentException("Runtime> You must put at least 3 arguments!");

        Height = (float)args[0];
        Length = (float)args[1];
        Width = (float)args[2];
        Type = CharacterControllerType.Box;
        initializeBoxCCT(Entity.Id, Height, Length, Width);
    }

    public override void Move(Vector3 position)
    {
        // TODO: More complex logic
        base.Move(position);
    }

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void initializeBoxCCT(int id, float x, float y, float z);
}