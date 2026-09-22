# ==============================================================================
# Makefile for ARKI (Visual C++ / NMAKE / Make)
# Usage:
#   nmake               (builds Debug x64 by default)
#   nmake CONF=Release  (builds Release x64)
#   nmake clean         (cleans build artifacts)
# ==============================================================================

!IFNDEF CONF
CONF = Debug
!ENDIF

# Tools
CXX     = cl.exe
RC      = rc.exe
LINK    = link.exe

# Directories
TOOLS_DIR     = ..\tools
D3DX_DIR      = packages\Microsoft.DXSDK.D3DX.9.29.952.8\build\native
OUTDIR        = build\$(CONF)
TARGET        = $(OUTDIR)\ARKI.exe

# Includes
INCLUDES = \
    /I"." \
    /I"$(D3DX_DIR)\include" \
    /I"$(TOOLS_DIR)\bullet3-3.25\src" \
    /I"$(TOOLS_DIR)\imgui-1.92.5" \
    /I"$(TOOLS_DIR)\imgui-1.92.5\backends" \
    /I"$(TOOLS_DIR)\json\include" \
    /I"$(TOOLS_DIR)\tweeny-3.2.1\include" \
    /I"$(TOOLS_DIR)\tinyobjloader-2.0-rc1"

# Common Compiler Flags
CXXFLAGS_COMMON = \
    /nologo \
    /W3 \
    /EHsc \
    /openmp \
    /fp:fast \
    /D_WINDOWS \
    /DUNICODE \
    /D_UNICODE \
    /DBT_THREADSAFE=1 \
    /DBT_USE_DOUBLE_PRECISION \
    $(INCLUDES)

# Debug vs Release settings
!IF "$(CONF)" == "Debug"
CXXFLAGS = $(CXXFLAGS_COMMON) /MTd /Od /Zi /D_DEBUG /D_DEBUG=1 /Fd"$(OUTDIR)\vc.pdb"
D3DX_LIB_DIR = $(D3DX_DIR)\debug\lib\x64
D3DX_LIB     = d3dx9d.lib
BULLET_EXT   = _vs2010_x64_debug.lib
LDFLAGS      = /nologo /DEBUG /SUBSYSTEM:WINDOWS /NODEFAULTLIB:MSVCRT.lib
!ELSE
CXXFLAGS = $(CXXFLAGS_COMMON) /MT /O2 /DNDEBUG /Fd"$(OUTDIR)\vc.pdb"
D3DX_LIB_DIR = $(D3DX_DIR)\release\lib\x64
D3DX_LIB     = d3dx9.lib
BULLET_EXT   = _vs2010_x64_release.lib
LDFLAGS      = /nologo /INCREMENTAL:NO /SUBSYSTEM:WINDOWS /NODEFAULTLIB:MSVCRT.lib
!ENDIF

# Libraries
LIBPATHS = \
    /LIBPATH:"$(D3DX_LIB_DIR)" \
    /LIBPATH:"$(TOOLS_DIR)\bullet3-3.25\bin"

LIBS = \
    d3d9.lib \
    $(D3DX_LIB) \
    dinput8.lib \
    dxguid.lib \
    winmm.lib \
    xaudio2.lib \
    BulletDynamics$(BULLET_EXT) \
    BulletCollision$(BULLET_EXT) \
    LinearMath$(BULLET_EXT)

# Object Files
OBJS = \
    $(OUTDIR)\ArkiGame.obj \
    $(OUTDIR)\BezierTessellator.obj \
    $(OUTDIR)\BulletManager.obj \
    $(OUTDIR)\CArkiBall.obj \
    $(OUTDIR)\CArkiBlock.obj \
    $(OUTDIR)\CArkiBomb.obj \
    $(OUTDIR)\CArkiCliff.obj \
    $(OUTDIR)\CArkiLevel.obj \
    $(OUTDIR)\CArkiPlayer.obj \
    $(OUTDIR)\CBSPlevel.obj \
    $(OUTDIR)\CBulletDebugDrawer.obj \
    $(OUTDIR)\CEnemyBullet.obj \
    $(OUTDIR)\CFlyingEnemy.obj \
    $(OUTDIR)\CFPSPlayer.obj \
    $(OUTDIR)\CGrid.obj \
    $(OUTDIR)\CHL1BSP.obj \
    $(OUTDIR)\CHUD.obj \
    $(OUTDIR)\COrbitCamera.obj \
    $(OUTDIR)\CParticleSystem.obj \
    $(OUTDIR)\CQ3BSP.obj \
    $(OUTDIR)\CQuatCamera.obj \
    $(OUTDIR)\CRibbonRenderer.obj \
    $(OUTDIR)\CRigidBody.obj \
    $(OUTDIR)\CSkybox.obj \
    $(OUTDIR)\CSpriteBatch.obj \
    $(OUTDIR)\CSpriteFont.obj \
    $(OUTDIR)\CXAudio.obj \
    $(OUTDIR)\D3DCamera.obj \
    $(OUTDIR)\D3DRender.obj \
    $(OUTDIR)\EnemySpawner.obj \
    $(OUTDIR)\Logger.obj \
    $(OUTDIR)\main.obj \
    $(OUTDIR)\MovementStrategy.obj \
    $(OUTDIR)\Sounds.obj \
    $(OUTDIR)\stdafx.obj \
    $(OUTDIR)\TextureManager.obj \
    $(OUTDIR)\WADViewer.obj \
    $(OUTDIR)\XMesh.obj \
    $(OUTDIR)\imgui.obj \
    $(OUTDIR)\imgui_demo.obj \
    $(OUTDIR)\imgui_draw.obj \
    $(OUTDIR)\imgui_tables.obj \
    $(OUTDIR)\imgui_widgets.obj \
    $(OUTDIR)\imgui_impl_dx9.obj \
    $(OUTDIR)\imgui_impl_win32.obj \
    $(OUTDIR)\ARKI.res

# Build Rules
all: $(OUTDIR) $(TARGET)

$(OUTDIR):
	@if not exist "$(OUTDIR)" mkdir "$(OUTDIR)"

$(TARGET): $(OBJS)
	$(LINK) $(LDFLAGS) $(LIBPATHS) $(OBJS) $(LIBS) /OUT:"$@"
	@echo [BUILD COMPLETE] Generated: $@

# Pattern Rules for Game Sources
{}.cpp{$(OUTDIR)}.obj::
	$(CXX) $(CXXFLAGS) /c /Fo"$(OUTDIR)\\" $<

# Pattern Rules for ImGui Sources
{$(TOOLS_DIR)\imgui-1.92.5}.cpp{$(OUTDIR)}.obj::
	$(CXX) $(CXXFLAGS) /c /Fo"$(OUTDIR)\\" $<

{$(TOOLS_DIR)\imgui-1.92.5\backends}.cpp{$(OUTDIR)}.obj::
	$(CXX) $(CXXFLAGS) /c /Fo"$(OUTDIR)\\" $<

# Resource File
$(OUTDIR)\ARKI.res: ARKI.rc
	$(RC) /nologo /fo "$@" ARKI.rc

clean:
	@if exist "$(OUTDIR)" rmdir /s /q "$(OUTDIR)"
	@echo Cleaned $(OUTDIR)

