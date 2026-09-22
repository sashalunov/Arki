cmake_minimum_required(VERSION 3.20)
project(ARKI LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the build type (Debug or Release)" FORCE)
endif()

# Paths to external tools and packages
set(TOOLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../tools")
set(D3DX_PACKAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/packages/Microsoft.DXSDK.D3DX.9.29.952.8/build/native")

# Source files
set(ARKI_SOURCES
    ArkiGame.cpp
    BezierTessellator.cpp
    BulletManager.cpp
    CArkiBall.cpp
    CArkiBlock.cpp
    CArkiBomb.cpp
    CArkiCliff.cpp
    CArkiLevel.cpp
    CArkiPlayer.cpp
    CBSPlevel.cpp
    CBulletDebugDrawer.cpp
    CEnemyBullet.cpp
    CFlyingEnemy.cpp
    CFPSPlayer.cpp
    CGrid.cpp
    CHL1BSP.cpp
    CHUD.cpp
    COrbitCamera.cpp
    CParticleSystem.cpp
    CQ3BSP.cpp
    CQuatCamera.cpp
    CRibbonRenderer.cpp
    CRigidBody.cpp
    CSkybox.cpp
    CSpriteBatch.cpp
    CSpriteFont.cpp
    CXAudio.cpp
    D3DCamera.cpp
    D3DRender.cpp
    EnemySpawner.cpp
    Logger.cpp
    main.cpp
    MovementStrategy.cpp
    Sounds.cpp
    stdafx.cpp
    TextureManager.cpp
    WADViewer.cpp
    XMesh.cpp
    ARKI.rc
)

set(IMGUI_SOURCES
    "${TOOLS_DIR}/imgui-1.92.5/imgui.cpp"
    "${TOOLS_DIR}/imgui-1.92.5/imgui_demo.cpp"
    "${TOOLS_DIR}/imgui-1.92.5/imgui_draw.cpp"
    "${TOOLS_DIR}/imgui-1.92.5/imgui_tables.cpp"
    "${TOOLS_DIR}/imgui-1.92.5/imgui_widgets.cpp"
    "${TOOLS_DIR}/imgui-1.92.5/backends/imgui_impl_dx9.cpp"
    "${TOOLS_DIR}/imgui-1.92.5/backends/imgui_impl_win32.cpp"
)

add_executable(ARKI WIN32
    ${ARKI_SOURCES}
    ${IMGUI_SOURCES}
)

# Include directories
target_include_directories(ARKI PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${D3DX_PACKAGE_DIR}/include"
    "${TOOLS_DIR}/bullet3-3.25/src"
    "${TOOLS_DIR}/imgui-1.92.5"
    "${TOOLS_DIR}/imgui-1.92.5/backends"
    "${TOOLS_DIR}/json/include"
    "${TOOLS_DIR}/tweeny-3.2.1/include"
    "${TOOLS_DIR}/tinyobjloader-2.0-rc1"
)

# Compiler defines
target_compile_definitions(ARKI PRIVATE
    _WINDOWS
    UNICODE
    _UNICODE
    BT_THREADSAFE=1
    BT_USE_DOUBLE_PRECISION
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Debug>:_DEBUG=1>
    $<$<CONFIG:Release>:NDEBUG>
)

# MSVC-specific compiler and linker options
if(MSVC)
    target_compile_options(ARKI PRIVATE
        /W3
        /EHsc
        /openmp
        /fp:fast
        /permissive-
    )

    # Use Static Runtime (/MT /MTd) matching the pre-compiled Bullet libraries
    set_property(TARGET ARKI PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )

    # Library search directories
    target_link_directories(ARKI PRIVATE
        "$<$<CONFIG:Debug>:${D3DX_PACKAGE_DIR}/debug/lib/x64>"
        "$<$<CONFIG:Release>:${D3DX_PACKAGE_DIR}/release/lib/x64>"
        "${TOOLS_DIR}/bullet3-3.25/bin"
    )

    # Link libraries
    target_link_libraries(ARKI PRIVATE
        d3d9
        $<$<CONFIG:Debug>:d3dx9d>
        $<$<CONFIG:Release>:d3dx9>
        dinput8
        dxguid
        winmm
        xaudio2
        $<$<CONFIG:Debug>:BulletDynamics_vs2010_x64_debug>
        $<$<CONFIG:Release>:BulletDynamics_vs2010_x64_release>
        $<$<CONFIG:Debug>:BulletCollision_vs2010_x64_debug>
        $<$<CONFIG:Release>:BulletCollision_vs2010_x64_release>
        $<$<CONFIG:Debug>:LinearMath_vs2010_x64_debug>
        $<$<CONFIG:Release>:LinearMath_vs2010_x64_release>
    )

    # Ignore default library MSVCRT when linking static runtime
    target_link_options(ARKI PRIVATE
        "/NODEFAULTLIB:MSVCRT.lib"
        "/SUBSYSTEM:WINDOWS"
    )
endif()

