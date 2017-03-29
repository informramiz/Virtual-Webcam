#pragma once

#include <cv.h>
#include <opencv2\opencv.hpp>
#include "FaceTracker.h"

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

EXTERN_C const GUID CLSID_VirtualCam;

class CVCamStream;
class CVCam : public CSource
{
public:
    //////////////////////////////////////////////////////////////////////////
    //  IUnknown
    //////////////////////////////////////////////////////////////////////////
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);

    IFilterGraph *GetGraph() {return m_pGraph;}

private:
    CVCam(LPUNKNOWN lpunk, HRESULT *phr);
};

class CVCamStream : public CSourceStream, public IAMStreamConfig, public IKsPropertySet
{
public:

    //////////////////////////////////////////////////////////////////////////
    //  IUnknown
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }                                                          \
    STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

    //////////////////////////////////////////////////////////////////////////
    //  IQualityControl
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

    //////////////////////////////////////////////////////////////////////////
    //  IAMStreamConfig
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC);

    //////////////////////////////////////////////////////////////////////////
    //  IKsPropertySet
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData);
    HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void *pInstanceData,DWORD cbInstanceData, void *pPropData, DWORD cbPropData, DWORD *pcbReturned);
    HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);
    
    //////////////////////////////////////////////////////////////////////////
    //  CSourceStream
    //////////////////////////////////////////////////////////////////////////
    CVCamStream(HRESULT *phr, CVCam *pParent, LPCWSTR pPinName);
    ~CVCamStream();

    HRESULT FillBuffer(IMediaSample *pms);
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc, ALLOCATOR_PROPERTIES *pProperties);
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT OnThreadCreate(void);
	HRESULT OnThreadDestroy();
	HRESULT OnThreadStartPlay();
private:
	//---------------------------------------------------------------------------- 
	// GetVideoInfoParameters 
	// 
	// Helper function to get the important information out of a VIDEOINFOHEADER 
	//----------------------------------------------------------------------------- 

	void GetVideoInfoParameters(
		const VIDEOINFOHEADER *pvih, // Pointer to the format header. 
		BYTE  * const pbData,        // Pointer to the first address in the buffer. 
		DWORD *pdwWidth,         // Returns the width in pixels. 
		DWORD *pdwHeight,        // Returns the height in pixels. 
		LONG  *plStrideInBytes,  // Add this to a row to get the new row down 
		BYTE **ppbTop,           // Returns pointer to the first byte in the top row of pixels. 
		bool bYuv                // Is this a YUV format? (true = YUV, false = RGB) 
		)
	{
		LONG lStride;


		//  For 'normal' formats, biWidth is in pixels.  
		//  Expand to bytes and round up to a multiple of 4. 
		if (pvih->bmiHeader.biBitCount != 0 &&
			0 == (7 & pvih->bmiHeader.biBitCount))
		{
			lStride = (pvih->bmiHeader.biWidth * (pvih->bmiHeader.biBitCount / 8) + 3) & ~3;
		}
		else   // Otherwise, biWidth is in bytes. 
		{
			lStride = pvih->bmiHeader.biWidth;
		}

		//  If rcTarget is empty, use the whole image. 
		if (IsRectEmpty(&pvih->rcTarget))
		{
			*pdwWidth = (DWORD)pvih->bmiHeader.biWidth;
			*pdwHeight = (DWORD)(abs(pvih->bmiHeader.biHeight));

			if (pvih->bmiHeader.biHeight < 0 || bYuv)   // Top-down bitmap.  
			{
				*plStrideInBytes = lStride; // Stride goes "down" 
				*ppbTop = pbData; // Top row is first. 
			}
			else        // Bottom-up bitmap 
			{
				*plStrideInBytes = -lStride;    // Stride goes "up" 
				*ppbTop = pbData + lStride * (*pdwHeight - 1);  // Bottom row is first. 
			}
		}
		else   // rcTarget is NOT empty. Use a sub-rectangle in the image. 
		{
			*pdwWidth = (DWORD)(pvih->rcTarget.right - pvih->rcTarget.left);
			*pdwHeight = (DWORD)(pvih->rcTarget.bottom - pvih->rcTarget.top);

			if (pvih->bmiHeader.biHeight < 0 || bYuv)   // Top-down bitmap. 
			{
				// Same stride as above, but first pixel is modified down 
				// and and over by the target rectangle. 
				*plStrideInBytes = lStride;
				*ppbTop = pbData +
					lStride * pvih->rcTarget.top +
					(pvih->bmiHeader.biBitCount * pvih->rcTarget.left) / 8;
			}
			else  // Bottom-up bitmap. 
			{
				*plStrideInBytes = -lStride;
				*ppbTop = pbData +
					lStride * (pvih->bmiHeader.biHeight - pvih->rcTarget.top - 1) +
					(pvih->bmiHeader.biBitCount * pvih->rcTarget.left) / 8;
			}
		}
	}


    CVCam *m_pParent;
    REFERENCE_TIME m_rtLastTime;
    HBITMAP m_hLogoBmp;
    CCritSec m_cSharedState;
    IReferenceClock *m_pClock;

	//opencv stuff
	cv::VideoCapture video_capture;
	FaceTracker face_tracker;
};


