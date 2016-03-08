/************************************************************************************
Filename    :   Win32_RoomTiny_Main.cpp
Content     :   First-person view test application for Oculus Rift
Created     :   11th May 2015
Authors     :   Tom Heath
Copyright   :   Copyright 2015 Oculus, Inc. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*************************************************************************************/
/// This is an entry-level sample, showing a minimal VR sample, 
/// in a simple environment.  Use WASD keys to move around, and cursor keys.
/// Dismiss the health and safety warning by tapping the headset, 
/// or pressing any key. 
/// It runs with DirectX11.

#include <iostream>

// Include DirectX
#include "Win32_DirectXAppUtil.h"

// Include the Oculus SDK
#include "OVR_CAPI_D3D.h"

#include <ovrvision_pro.h>	//Ovrvision SDK
//#include <ovrvision_ar.h>

#include "DXUT.h"
#include "DXUTgui.h"
#include "SDKmisc.h"

#include <opencv2/aruco.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

extern int InitializeCamPlane(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, int w, int h, float zsize);
extern int RendererCamPlane(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
extern int SetCamImage(ID3D11DeviceContext* DeviceContext, unsigned char* camImage, unsigned int imagesize);
extern int CleanCamPlane();

//CDXUTDialogResourceManager g_resourceManager = NULL;
CDXUTTextHelper*           g_pTxtHelper = NULL;

OVR::OvrvisionPro ovrvision;
//OVR::OvrvisionAR* pOvrAR;
int ovWidth = 0;
int ovHeight = 0;
int ovPixelsize = 4;

cv::Mat img_left;
cv::Mat img_right;
cv::Mat img_up;
std::vector<cv::Mat > img_exps;

//------------------------------------------------------------
// ovrSwapTextureSet wrapper class that also maintains the render target views
// needed for D3D11 rendering.
struct OculusTexture
{
    ovrHmd                   hmd;
	ovrSwapTextureSet      * TextureSet;
    static const int         TextureCount = 2;
	ID3D11RenderTargetView * TexRtv[TextureCount];

    OculusTexture() :
        hmd(nullptr),
        TextureSet(nullptr)
    {
        TexRtv[0] = TexRtv[1] = nullptr;
    }

    bool Init(ovrHmd _hmd, int sizeW, int sizeH)
	{
        hmd = _hmd;

		D3D11_TEXTURE2D_DESC dsDesc;
		dsDesc.Width = sizeW;
		dsDesc.Height = sizeH;
		dsDesc.MipLevels = 1;
		dsDesc.ArraySize = 1;
        dsDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		dsDesc.SampleDesc.Count = 1;   // No multi-sampling allowed
		dsDesc.SampleDesc.Quality = 0;
		dsDesc.Usage = D3D11_USAGE_DEFAULT;
		dsDesc.CPUAccessFlags = 0;
		dsDesc.MiscFlags = 0;
		dsDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

		ovrResult result = ovr_CreateSwapTextureSetD3D11(hmd, DIRECTX.Device, &dsDesc, ovrSwapTextureSetD3D11_Typeless, &TextureSet);
        if (!OVR_SUCCESS(result))
            return false;

        VALIDATE(TextureSet->TextureCount == TextureCount, "TextureCount mismatch.");

		for (int i = 0; i < TextureCount; ++i)
		{
			ovrD3D11Texture* tex = (ovrD3D11Texture*)&TextureSet->Textures[i];
			D3D11_RENDER_TARGET_VIEW_DESC rtvd = {};
			rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			DIRECTX.Device->CreateRenderTargetView(tex->D3D11.pTexture, &rtvd, &TexRtv[i]);
		}

        return true;
    }

	~OculusTexture()
	{
		for (int i = 0; i < TextureCount; ++i)
        {
            Release(TexRtv[i]);
        }
		if (TextureSet)
        {
            ovr_DestroySwapTextureSet(hmd, TextureSet);
        }
	}

	void AdvanceToNextTexture()
	{
		TextureSet->CurrentIndex = (TextureSet->CurrentIndex + 1) % TextureSet->TextureCount;
	}
};

#define VIS_ARROWS_ON_CARD 0
#define VIS_REASONING_ON_CARD 1
#define VIS_ARROWS_ON_BOX 2
#define VIS_GUIDANCE_ON_CARD 3

#define SUIT_SPADE 0
#define SUIT_HEART 1
#define SUIT_CLUB 2
#define SUIT_DIAMOND 3

#define EXP_1_ID 63
#define EXP_2_ID 62
#define EXP_3_ID 61
#define EXP_4_ID 60

#define LEFT_BOX_ID 58
#define RIGHT_BOX_ID 59

int g_currentExperiment = EXP_1_ID;
int g_currentCard = 0; //2 of spades
int g_visType = VIS_GUIDANCE_ON_CARD;

bool cardGoesLeft(int cardId, int experimentId) {
	int cardNum = (cardId % 8) + 2; //2-9 of each suit
	int suit = cardId / 8;

	switch (experimentId) {
	case 63:
	default:
		//Left: Odd heart and spade, even space and diamond
		return (cardNum % 2 == 1 && (suit == SUIT_HEART || suit == SUIT_SPADE)) ||
			(cardNum % 2 == 0 && (suit == SUIT_SPADE || suit == SUIT_DIAMOND));

	case 62:
		//Left: Heart and Spade Odd, or Club and Diamond 2-5
		return (cardNum % 2 == 1 && (suit == SUIT_HEART || suit == SUIT_SPADE)) ||
			(cardNum <= 5 && (suit == SUIT_CLUB || suit == SUIT_DIAMOND));

	case 61:
		//Left: Odd 2-5, or Even heart or club
		return (cardNum % 2 == 1 && cardNum <= 5) ||
			(cardNum % 2 == 0 && (suit == SUIT_HEART || suit == SUIT_CLUB));

	case 60:
		//Left: 2-5 diamon and club, or 6-9 even
		return (cardNum <= 5 && (suit == SUIT_DIAMOND || suit == SUIT_CLUB)) ||
			(cardNum >= 6 && cardNum % 2 == 0);
	}
}

/* Scan marker ids to see which experiment we are doing */
void updateExperiment(std::vector< int > &markerId) {
	for (unsigned int i = 0; i < markerId.size(); i++) {
		if (markerId[i] >= 60 && markerId[i] <= 63) {
			g_currentExperiment = markerId[i];
		}
	}
}

float dist(cv::Point2f pt1, cv::Point2f pt2) {
	cv::Point2f diff = pt1 - pt2;
	return sqrt(diff.x*diff.x + diff.y*diff.y);
}

float markerAreaApprox(std::vector< cv::Point2f > markerCorners) {
	float distx = dist(markerCorners[0], markerCorners[1]);
	float disty = dist(markerCorners[0], markerCorners[3]);
	return distx*disty;
}

float markerEccentricity(std::vector< cv::Point2f > markerCorners) {
	float distx = dist(markerCorners[0], markerCorners[1]);
	float disty = dist(markerCorners[0], markerCorners[3]);
	float eccentricity = 0;
	if (disty > 0) {
		eccentricity = distx / disty;
	}
	if (eccentricity > 1.0f) {
		eccentricity = 1.0f / eccentricity;
	}
	return eccentricity;
}

void expandMarker(std::vector<cv::Point2f > &markerCorners, float amt) {
	cv::Point2f center;
	for (int i = 0; i < markerCorners.size(); i++) {
		center += markerCorners[i];
	}
	center /= 4.0f;

	for (int i = 0; i < markerCorners.size(); i++) {
		cv::Point2f diff = markerCorners[i] - center;
		diff *= amt;
		markerCorners[i] = diff + center;
	}
}

/* Scan marker ids to see which card we are looking at.
 * Return the index where the card was found, for use in lookup into markerCorners */
int updateCard(std::vector< int > &markerId, std::vector< std::vector<cv::Point2f> > &markerCorners) {
	int index = -1;
	for (unsigned int i = 0; i < markerId.size(); i++) {
		float mArea = markerAreaApprox(markerCorners[i]);
		float mEccentricity = markerEccentricity(markerCorners[i]);

		if (markerId[i] < 32 &&  mEccentricity > 0.5 && mArea > 250.0f && mArea < 7500.0f) {
			g_currentCard = markerId[i];
			index = i;
		}
	}

	return index;
}

std::pair<int, int> findBoxes(std::vector< int > &markerId) {
	int left_index = -1;
	int right_index = -1;
	for (unsigned int i = 0; i < markerId.size(); i++) {
		if (markerId[i] == LEFT_BOX_ID) {
			left_index = i;
		}
		if (markerId[i] == RIGHT_BOX_ID) {
			right_index = i;
		}
	}

	return std::make_pair(left_index, right_index);
}

void fillMarkerWithImage(unsigned char* target, cv::Mat source, int ovWidth, int ovHeight, std::vector<cv::Point2f> &corners, bool clipTop = false) {
	if (corners.size() >= 4) {
		float diagDist = cv::sqrt((corners[0].x - corners[2].x)*(corners[0].x - corners[2].x) +
			(corners[0].y - corners[2].y)*(corners[0].y - corners[2].y));
		
		float res = 1.0f/diagDist;

		for (float y = 0.0f; y < 1.0f; y += res) {
			float left_y = corners[0].y*y + corners[3].y*(1 - y);
			float left_x = corners[0].x*y + corners[3].x*(1 - y);
			float right_y = corners[1].y*y + corners[2].y*(1 - y);
			float right_x = corners[1].x*y + corners[2].x*(1 - y);

			for (float x = 0.0f; x < 1.0f; x += res) {
				float my_y = left_y*x + right_y*(1 - x);
				float my_x = left_x*x + right_x*(1 - x);
				int index = 4 * ((int)my_x + ovWidth*(int)my_y);
				int src_x = (int)(source.cols*x);
				int src_y = (int)(source.rows*y);
				int src_index = 3*(src_x + src_y*source.cols);

				if (clipTop && src_y < source.rows * 0.35) {
					continue;
				}
				for (int offset = 0; offset < 4; offset++) {
					target[index + offset] = source.data[src_index + offset];
				}
			}
		}
	}
}

void rotateCorners(std::vector<cv::Point2f> &rotatedCorners) {
	cv::Point2f t = rotatedCorners[0];
	for (int j = 0;j < 3;j++) {
		rotatedCorners[j] = rotatedCorners[j + 1];
	}
	rotatedCorners[3] = t;
}

void processMarkers(unsigned char* p, int ovWidth, int ovHeight, std::vector< int > &markerIds, std::vector< std::vector<cv::Point2f> > &markerCorners) {
	updateExperiment(markerIds); //Switch experiments, if necessary
	int cardIndex = updateCard(markerIds, markerCorners); //Switch currentCard, if necessary, and get index for rendering
	std::pair<int, int> boxIndices = findBoxes(markerIds);

	//Check which way the current card should go
	bool goLeft = cardGoesLeft(g_currentCard, g_currentExperiment);

	if ((g_visType == VIS_ARROWS_ON_CARD || g_visType == VIS_REASONING_ON_CARD || g_visType == VIS_GUIDANCE_ON_CARD) && cardIndex != -1) {
		std::vector<cv::Point2f> rotatedCorners = markerCorners[cardIndex];

		int mostLeftPt = 0;
		for (int i = 1; i < 4; i++) {
			if (rotatedCorners[i].x < rotatedCorners[mostLeftPt].x) {
				mostLeftPt = i;
			}
		}
		float distUp = rotatedCorners[mostLeftPt].y - rotatedCorners[(mostLeftPt + 1) % 4].y;
		float distDown = rotatedCorners[(mostLeftPt + 3) % 4].y - rotatedCorners[mostLeftPt].y;
		int numRots = distUp < distDown ? mostLeftPt : (mostLeftPt+1)%4;
		
		//If they have the card rotated, keep the arrow pointing the right way
		for (int i = 0; i < numRots; i++) {
			rotateCorners(rotatedCorners);
		}
		//Rotate twice, because I put the images on the markers the wrong way up.
		rotateCorners(rotatedCorners);
		rotateCorners(rotatedCorners);
		
		if (g_visType == VIS_ARROWS_ON_CARD) {
			fillMarkerWithImage(p, goLeft ? img_left : img_right, ovWidth, ovHeight, rotatedCorners);
		}
		else {
			expandMarker(rotatedCorners, 4.0f);
			fillMarkerWithImage(p, img_exps[g_currentExperiment % 60], ovWidth, ovHeight, rotatedCorners, true);
			//TODO Reasoning version
		}
	}
	else if (g_visType == VIS_ARROWS_ON_BOX && ((goLeft && boxIndices.first != -1) || (!goLeft && boxIndices.second != -1))) {
		int index = goLeft ? boxIndices.first : boxIndices.second;
		std::vector<cv::Point2f> rotatedCorners = markerCorners[index];
		
		//Rotate twice, because I put the images on the markers the wrong way up.
		rotateCorners(rotatedCorners);
		rotateCorners(rotatedCorners);

		fillMarkerWithImage(p, img_up, ovWidth, ovHeight, rotatedCorners);
	}
}

// return true to retry later (e.g. after display lost)
static bool MainLoop(bool retryCreate)
{
    // Initialize these to nullptr here to handle device lost failures cleanly
	ovrTexture     * mirrorTexture = nullptr;
	OculusTexture  * pEyeRenderTexture[2] = { nullptr, nullptr };
	DepthBuffer    * pEyeDepthBuffer[2] = { nullptr, nullptr };
    Camera         * mainCam = nullptr;
	D3D11_TEXTURE2D_DESC td = {};

	ovrHmd HMD;
	ovrGraphicsLuid luid;
	ovrResult result = ovr_Create(&HMD, &luid);
    if (!OVR_SUCCESS(result))
        return retryCreate;

    ovrHmdDesc hmdDesc = ovr_GetHmdDesc(HMD);

	// Setup Device and Graphics
	// Note: the mirror window can be any size, for this sample we use 1/2 the HMD resolution
    if (!DIRECTX.InitDevice(hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2, reinterpret_cast<LUID*>(&luid)))
        goto Done;

	// Make the eye render buffers (caution if actual size < requested due to HW limits). 
	ovrRecti         eyeRenderViewport[2];

	for (int eye = 0; eye < 2; ++eye)
	{
		ovrSizei idealSize = ovr_GetFovTextureSize(HMD, (ovrEyeType)eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
		pEyeRenderTexture[eye] = new OculusTexture();
        if (!pEyeRenderTexture[eye]->Init(HMD, idealSize.w, idealSize.h))
        {
            if (retryCreate) goto Done;
	        VALIDATE(OVR_SUCCESS(result), "Failed to create eye texture.");
        }
		pEyeDepthBuffer[eye] = new DepthBuffer(DIRECTX.Device, idealSize.w, idealSize.h);
		eyeRenderViewport[eye].Pos.x = 0;
		eyeRenderViewport[eye].Pos.y = 0;
		eyeRenderViewport[eye].Size = idealSize;
        if (!pEyeRenderTexture[eye]->TextureSet)
        {
            if (retryCreate) goto Done;
            VALIDATE(false, "Failed to create texture.");
        }
	}

	// Create a mirror to see on the monitor.
	td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	td.Width = DIRECTX.WinSizeW;
	td.Height = DIRECTX.WinSizeH;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.SampleDesc.Count = 1;
	td.MipLevels = 1;
    result = ovr_CreateMirrorTextureD3D11(HMD, DIRECTX.Device, &td, 0, &mirrorTexture);
    if (!OVR_SUCCESS(result))
    {
        if (retryCreate) goto Done;
        VALIDATE(false, "Failed to create mirror texture.");
    }

	// Create camera
    mainCam = new Camera(&XMVectorSet(0.0f, 0.0f, 0.0f, 0), &XMQuaternionIdentity());

	// Setup VR components, filling out description
	ovrEyeRenderDesc eyeRenderDesc[2];
	eyeRenderDesc[0] = ovr_GetRenderDesc(HMD, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
	eyeRenderDesc[1] = ovr_GetRenderDesc(HMD, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

    bool isVisible = true;

	int locationID = 0;
	OVR::Camprop cameraMode = OVR::OV_CAMVR_FULL;
	if (__argc > 2) {
		printf("Ovrvisin Pro mode changed.");
		//__argv[0]; ApplicationPath
		locationID = atoi(__argv[1]);
		cameraMode = (OVR::Camprop)atoi(__argv[2]);
	}

	if (ovrvision.Open(locationID, cameraMode)) {
		ovWidth = ovrvision.GetCamWidth();
		ovHeight = ovrvision.GetCamHeight();
		ovPixelsize = ovrvision.GetCamPixelsize();

		ovrvision.SetCameraSyncMode(false);

		InitializeCamPlane(DIRECTX.Device, DIRECTX.Context, ovWidth, ovHeight, 1.0f);
	}

	img_left = cv::imread("left.png", CV_LOAD_IMAGE_COLOR);
	img_right = cv::imread("right.png", CV_LOAD_IMAGE_COLOR);
	img_up = cv::imread("up.png", CV_LOAD_IMAGE_COLOR);
	img_exps.push_back(cv::imread("exp60.png", CV_LOAD_IMAGE_COLOR));
	img_exps.push_back(cv::imread("exp61.png", CV_LOAD_IMAGE_COLOR));
	img_exps.push_back(cv::imread("exp62.png", CV_LOAD_IMAGE_COLOR));
	img_exps.push_back(cv::imread("exp63.png", CV_LOAD_IMAGE_COLOR));

	{
		std::vector< int > markerIds;
		std::vector< std::vector<cv::Point2f> > markerCorners;//, rejectedCandidates;
        //cv::Ptr<cv::aruco::DetectorParameters> parameters = new cv::aruco::DetectorParameters(); 
		cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_ARUCO_ORIGINAL);
		cv::Mat grey;
		grey.create(ovHeight, ovWidth, CV_8UC1);//, ovrvision.GetCamImageBGRA(OVR::Cameye::OV_CAMEYE_LEFT), );

		// Main loop
		while (DIRECTX.HandleMessages())
		{
			XMVECTOR forward = XMVector3Rotate(XMVectorSet(0, 0, -0.05f, 0), mainCam->Rot);
			XMVECTOR right = XMVector3Rotate(XMVectorSet(0.05f, 0, 0, 0), mainCam->Rot);
			if (DIRECTX.Key['W'] || DIRECTX.Key[VK_UP])	  mainCam->Pos = XMVectorAdd(mainCam->Pos, forward);
			if (DIRECTX.Key['S'] || DIRECTX.Key[VK_DOWN]) mainCam->Pos = XMVectorSubtract(mainCam->Pos, forward);
			if (DIRECTX.Key['D'])                         mainCam->Pos = XMVectorAdd(mainCam->Pos, right);
			if (DIRECTX.Key['A'])                         mainCam->Pos = XMVectorSubtract(mainCam->Pos, right);
			static float Yaw = 0;
			if (DIRECTX.Key[VK_LEFT])  mainCam->Rot = XMQuaternionRotationRollPitchYaw(0, Yaw += 0.02f, 0);
			if (DIRECTX.Key[VK_RIGHT]) mainCam->Rot = XMQuaternionRotationRollPitchYaw(0, Yaw -= 0.02f, 0);

			// Get both eye poses simultaneously, with IPD offset already included. 
			ovrPosef         EyeRenderPose[2];
			ovrVector3f      HmdToEyeViewOffset[2] = { eyeRenderDesc[0].HmdToEyeViewOffset,
													   eyeRenderDesc[1].HmdToEyeViewOffset };
			double frameTime = ovr_GetPredictedDisplayTime(HMD, 0);
			// Keeping sensorSampleTime as close to ovr_GetTrackingState as possible - fed into the layer
			double           sensorSampleTime = ovr_GetTimeInSeconds();
			ovrTrackingState hmdState = ovr_GetTrackingState(HMD, frameTime, ovrTrue);
			ovr_CalcEyePoses(hmdState.HeadPose.ThePose, HmdToEyeViewOffset, EyeRenderPose);

			ovrvision.PreStoreCamData(OVR::Camqt::OV_CAMQT_DMSRMP);

			// Render Scene to Eye Buffers
			if (isVisible)
			{
				for (int eye = 0; eye < 2; ++eye)
				{
					// Increment to use next texture, just before writing
					pEyeRenderTexture[eye]->AdvanceToNextTexture();

					// Clear and set up rendertarget
					int texIndex = pEyeRenderTexture[eye]->TextureSet->CurrentIndex;
					DIRECTX.SetAndClearRenderTarget(pEyeRenderTexture[eye]->TexRtv[texIndex], pEyeDepthBuffer[eye]);
					DIRECTX.SetViewport((float)eyeRenderViewport[eye].Pos.x, (float)eyeRenderViewport[eye].Pos.y,
						(float)eyeRenderViewport[eye].Size.w, (float)eyeRenderViewport[eye].Size.h);

					//Get the pose information in XM format
					XMVECTOR eyeQuat = XMVectorSet(EyeRenderPose[eye].Orientation.x, EyeRenderPose[eye].Orientation.y,
						EyeRenderPose[eye].Orientation.z, EyeRenderPose[eye].Orientation.w);
					XMVECTOR eyePos = XMVectorSet(EyeRenderPose[eye].Position.x, EyeRenderPose[eye].Position.y, EyeRenderPose[eye].Position.z, 0);

					// Get view and projection matrices for the Rift camera
					XMVECTOR CombinedPos = XMVectorAdd(mainCam->Pos, XMVector3Rotate(eyePos, mainCam->Rot));
					Camera finalCam(&CombinedPos, &(XMQuaternionMultiply(eyeQuat, mainCam->Rot)));
					XMMATRIX view = finalCam.GetViewMatrix();
					ovrMatrix4f p = ovrMatrix4f_Projection(eyeRenderDesc[eye].Fov, 0.2f, 1000.0f, ovrProjection_RightHanded);
					XMMATRIX proj = XMMatrixSet(p.M[0][0], p.M[1][0], p.M[2][0], p.M[3][0],
						p.M[0][1], p.M[1][1], p.M[2][1], p.M[3][1],
						p.M[0][2], p.M[1][2], p.M[2][2], p.M[3][2],
						p.M[0][3], p.M[1][3], p.M[2][3], p.M[3][3]);
					XMMATRIX prod = XMMatrixMultiply(view, proj);

					InitializeCamPlane(DIRECTX.Device, DIRECTX.Context, ovWidth, ovHeight, 1.0f);

					//Camera View
					if (eye == 0) {
						unsigned char* p = ovrvision.GetCamImageBGRA(OVR::Cameye::OV_CAMEYE_LEFT);
						//Convert to greyscale
						for (int i = 0; i < ovWidth*ovHeight; i++) {
							unsigned char average = (((int)p[4 * i] + (int)p[4 * i + 1] + (int)p[4 * i + 2]) / 3);
							grey.data[i] = average;
						}
						cv::aruco::detectMarkers(grey, dictionary, markerCorners, markerIds);//, parameters, rejectedCandidates);

						processMarkers(p, ovWidth, ovHeight, markerIds, markerCorners);

						SetCamImage(DIRECTX.Context, p, ovWidth*ovPixelsize);
					}
					else {
						unsigned char* p = ovrvision.GetCamImageBGRA(OVR::Cameye::OV_CAMEYE_RIGHT);
						//Convert to greyscale
						for (int i = 0; i < ovWidth*ovHeight; i++) {
							unsigned char average = (((int)p[4 * i] + (int)p[4 * i + 1] + (int)p[4 * i + 2]) / 3);
							grey.data[i] = average;
						}
						cv::aruco::detectMarkers(grey, dictionary, markerCorners, markerIds);//, parameters, rejectedCandidates);

						processMarkers(p, ovWidth, ovHeight, markerIds, markerCorners);

						SetCamImage(DIRECTX.Context, p, ovWidth*ovPixelsize);
					}
					RendererCamPlane(DIRECTX.Device, DIRECTX.Context);
				}
			}

			// Initialize our single full screen Fov layer.
			ovrLayerEyeFov ld = {};
			ld.Header.Type = ovrLayerType_EyeFov;
			ld.Header.Flags = 0;

			for (int eye = 0; eye < 2; ++eye)
			{
				ld.ColorTexture[eye] = pEyeRenderTexture[eye]->TextureSet;
				ld.Viewport[eye] = eyeRenderViewport[eye];
				ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
				ld.RenderPose[eye] = EyeRenderPose[eye];
				ld.SensorSampleTime = sensorSampleTime;
			}

			ovrLayerHeader* layers = &ld.Header;
			result = ovr_SubmitFrame(HMD, 0, nullptr, &layers, 1);
			// exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
			if (!OVR_SUCCESS(result))
				goto Done;

			isVisible = (result == ovrSuccess);

			// Render mirror
			ovrD3D11Texture* tex = (ovrD3D11Texture*)mirrorTexture;
			DIRECTX.Context->CopyResource(DIRECTX.BackBuffer, tex->D3D11.pTexture);
			DIRECTX.SwapChain->Present(0, 0);
		}
	}
	// Release resources
Done:
    delete mainCam;
	if (mirrorTexture) ovr_DestroyMirrorTexture(HMD, mirrorTexture);
    for (int eye = 0; eye < 2; ++eye)
    {
	    delete pEyeRenderTexture[eye];
        delete pEyeDepthBuffer[eye];
    }
	DIRECTX.ReleaseDevice();
	ovr_Destroy(HMD);
	ovrvision.Close();

    // Retry on ovrError_DisplayLost
    return retryCreate || OVR_SUCCESS(result) || (result == ovrError_DisplayLost);
}

//-------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int)
{
	// Initializes LibOVR, and the Rift
	ovrResult result = ovr_Initialize(nullptr);
	VALIDATE(OVR_SUCCESS(result), "Failed to initialize libOVR.");

    VALIDATE(DIRECTX.InitWindow(hinst, L"Oculus Room Tiny (DX11)"), "Failed to open window.");

	int locationID = 0;

    DIRECTX.Run(MainLoop);

	ovr_Shutdown();
	return(0);
}
