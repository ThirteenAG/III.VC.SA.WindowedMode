#pragma once
#include <stdint.h>
#include "d3d8/d3d8.h"
#include "d3d8/dinput.h"

class Interval
{
private:
	unsigned int initial_;

public:
	inline Interval() : initial_(GetTickCount()) { }

	virtual ~Interval() { }

	inline unsigned int value() const
	{
		return GetTickCount() - initial_;
	}
};

class Fps
{
protected:
	unsigned int m_fps;
	unsigned int m_fpscount;
	Interval m_fpsinterval;

public:
	Fps() : m_fps(0), m_fpscount(0) { }

	void update()
	{
		m_fpscount++;

		if (m_fpsinterval.value() > 1000)
		{
			m_fps = m_fpscount;
			m_fpscount = 0;
			m_fpsinterval = Interval();
		}
	}

	unsigned int get() const
	{
		return m_fps;
	}
};

typedef struct _D3DPRESENT_PARAMETERS_D3D9_
{
	UINT                BackBufferWidth;
	UINT                BackBufferHeight;
	D3DFORMAT           BackBufferFormat;
	UINT                BackBufferCount;
	D3DMULTISAMPLE_TYPE MultiSampleType;
	DWORD               MultiSampleQuality;
	D3DSWAPEFFECT       SwapEffect;
	HWND                hDeviceWindow;
	BOOL                Windowed;
	BOOL                EnableAutoDepthStencil;
	D3DFORMAT           AutoDepthStencilFormat;
	DWORD               Flags;
	UINT                FullScreen_RefreshRateInHz;
	UINT                FullScreen_PresentationInterval;
} D3DPRESENT_PARAMETERS_D3D9;

struct RwVideoMode
{
	int32_t width;
	int32_t height;
	int32_t depth;
	int32_t flags;
};

struct RsGlobalType
{
	char *AppName;
	int MaximumWidth;
	int MaximumHeight;
	int frameLimit;
	int quit;
	int ps;
};

enum RwRasterType
{
	rwRASTERTYPENORMAL = 0x00,
	rwRASTERTYPEZBUFFER = 0x01,
	rwRASTERTYPECAMERA = 0x02,
	rwRASTERTYPETEXTURE = 0x04,
	rwRASTERTYPECAMERATEXTURE = 0x05,
	rwRASTERTYPEMASK = 0x07,
	rwRASTERPALETTEVOLATILE = 0x40,
	rwRASTERDONTALLOCATE = 0x80,
	rwRASTERTYPEFORCEENUMSIZEINT = ((int32_t)((~((uint32_t)0)) >> 1))
};

struct IDirectInputDeviceA;
struct IDirectInputA;
struct IDirect3DSwapChain8;

struct GameDxInput
{
	unsigned char __pad00[24];
	IDirectInputA* pInput;
	IDirectInputDeviceA* pInputDevice;
	unsigned char __pad04[8];
};

typedef float RwReal;
struct RwV2d
{
	RwReal x;
	RwReal y;
};

struct RwV3d
{
	RwReal x;
	RwReal y;
	RwReal z;
};

struct RwPlane
{
	RwV3d normal;
	RwReal distance;
};

struct RwFrustumPlane
{
	RwPlane plane;
	uint8_t closestX;
	uint8_t closestY;
	uint8_t closestZ;
	uint8_t pad;
};

struct RwBBox
{
	RwV3d sup;
	RwV3d inf;
};

struct RwRaster
{
	RwRaster* pParent;
	uint8_t* pPixels;
	uint8_t* pPalette;
	int32_t nWidth;
	int32_t nHeight;
	int32_t nDepth;
	int32_t nStride;
	int16_t nOffsetX;
	int16_t nOffsetY;
	uint8_t cType;
	uint8_t cFlags;
	uint8_t cPrivateFlags;
	uint8_t cFormat;
	uint8_t* pOriginalPixels;
	int32_t nOriginalWidth;
	int32_t nOriginalHeight;
	int32_t nOriginalStride;
};

struct RwCamera
{
	char RwObjectHasFrame[20];
	uint32_t RwCameraProjection;
	uint32_t RwCameraBeginUpdateFunc;
	uint32_t RwCameraEndUpdateFunc;
	char RwMatrix[64];
	RwRaster           *frameBuffer;
	RwRaster           *zBuffer;
	RwV2d               viewWindow;
	RwV2d               recipViewWindow;
	RwV2d               viewOffset;
	RwReal              nearPlane;
	RwReal              farPlane;
	RwReal              fogPlane;
	RwReal              zScale, zShift;
	RwFrustumPlane      frustumPlanes[6];
	RwBBox              frustumBoundBox;
	RwV3d               frustumCorners[8];
};

struct DisplayMode
{
	int nWidth;
	int nHeight;
	unsigned char __pad00[12];
};