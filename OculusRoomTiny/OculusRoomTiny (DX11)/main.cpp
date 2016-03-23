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
#include <algorithm>

// Include DirectX
#include "Win32_DirectXAppUtil.h"

// Include the Oculus SDK
#include "OVR_CAPI_D3D.h"

#include <ovrvision_pro.h>	//Ovrvision SDK
//#include <ovrvision_ar.h>

#include <opencv2/aruco.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

extern int InitializeCamPlane(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, int w, int h, float zsize);
extern int RendererCamPlane(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
extern int SetCamImage(ID3D11DeviceContext* DeviceContext, unsigned char* camImage, unsigned int imagesize);
extern int CleanCamPlane();

OVR::OvrvisionPro ovrvision;
//OVR::OvrvisionAR* pOvrAR;
int ovWidth = 0;
int ovHeight = 0;
int ovPixelsize = 4;

cv::Mat img_left;
cv::Mat img_right;
cv::Mat img_up;
std::vector<cv::Mat > img_exps;
cv::Mat img_exp_composite;
std::vector<cv::Mat > img_l;
std::vector<cv::Mat > img_r;

cv::Mat text_overlay;

void rotateCorners(std::vector<cv::Point2f> &rotatedCorners);

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

#define MISTAKE_NONE 0
#define MISTAKE_EVENODD 1
#define MISTAKE_BIGLITTLE 2
#define MISTAKE_SUITPLUS 4
#define MISTAKE_SUITMINUS 8

int visTypeMapping[] = {
	VIS_ARROWS_ON_CARD, //Marker 60, EXP_4
	VIS_ARROWS_ON_CARD,
	VIS_REASONING_ON_CARD,
	VIS_REASONING_ON_CARD //Marker 63, EXP_1
};

//Index is card number, NOT order in which subject will see cards
int mistakes[] = {
	//First, mistakes for experiment 63, which uses deck A
	MISTAKE_SUITPLUS, //0 - 7th card in order -- Change spade to heart
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_SUITMINUS, //7 - 31st
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_SUITMINUS, //12 - 24th
	MISTAKE_NONE,
	MISTAKE_EVENODD, //14 - 16th card seen
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_SUITMINUS, //23 - 19th
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_EVENODD, //26 - 25th
	MISTAKE_EVENODD, //27 - 11th card seen 
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_EVENODD, //31 - 28th
	//Mistakes for experiment 60, deck B
	MISTAKE_NONE, //0
	MISTAKE_SUITMINUS, //1 - 28th card seen
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_EVENODD, //5 - 19th card seen
	MISTAKE_NONE,
	MISTAKE_EVENODD, //7 - 31st card seen
	MISTAKE_BIGLITTLE, //8 - 25th card seen
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_SUITPLUS, //11 - 7th card seen
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_BIGLITTLE, //21 - 10th card seen
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_SUITPLUS, //26 - 21st card seen
	MISTAKE_NONE,
	MISTAKE_EVENODD, //28 - 14th card seen
	MISTAKE_NONE,
	MISTAKE_NONE,
	MISTAKE_NONE
};

int g_currentExperiment = EXP_1_ID;
int g_currentCard = 0; //2 of spades
int g_visType = VIS_REASONING_ON_CARD;

bool g_imgExpCompDirty = true;

//From www.blackpawn.com/texts/pointinpoly
bool sameSide(cv::Point2f &p1, cv::Point2f &p2, cv::Point2f &a, cv::Point2f &b) {
	double cp1 = (b - a).cross(p1 - a);
	double cp2 = (b - a).cross(p2 - a);

	return cp1*cp2 >= 0;
}

bool pointInTriangle(cv::Point2f &p, cv::Point2f &a, cv::Point2f &b, cv::Point2f &c) {
	return sameSide(p, a, b, c) && sameSide(p, b, a, c) && sameSide(p, c, a, b);
}

bool cardGoesLeft(int cardId, int experimentId) {
	int cardNum = (cardId % 8) + 2; //2-9 of each suit
	int suit = cardId / 8;

	switch (experimentId) {
	case EXP_1_ID:
	default:
		//Left: Odd heart and spade, even space and diamond
		return (cardNum % 2 == 1 && (suit == SUIT_HEART || suit == SUIT_SPADE)) ||
			(cardNum % 2 == 0 && (suit == SUIT_SPADE || suit == SUIT_DIAMOND));

	case EXP_2_ID:
		//Left: Heart and Spade Odd, or Club and Diamond 2-5
		return (cardNum % 2 == 1 && (suit == SUIT_HEART || suit == SUIT_SPADE)) ||
			(cardNum <= 5 && (suit == SUIT_CLUB || suit == SUIT_DIAMOND));

	case EXP_3_ID:
		//Left: Odd 2-5, or Even heart or club
		return (cardNum % 2 == 1 && cardNum <= 5) ||
			(cardNum % 2 == 0 && (suit == SUIT_HEART || suit == SUIT_CLUB));

	case EXP_4_ID:
		//Left: 2-5 diamon and club, or 6-9 even
		return (cardNum <= 5 && (suit == SUIT_DIAMOND || suit == SUIT_CLUB)) ||
			(cardNum >= 6 && cardNum % 2 == 0);
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

/* Scan marker ids to see which experiment we are doing */
void updateExperiment(std::vector< int > &markerId, std::vector< std::vector<cv::Point2f> > &markerCorners) {
	for (unsigned int i = 0; i < markerId.size(); i++) {
		float mArea = markerAreaApprox(markerCorners[i]);
		float mEccentricity = markerEccentricity(markerCorners[i]);

		if (markerId[i] >= EXP_4_ID && markerId[i] <= EXP_1_ID && mEccentricity > 0.5 && mArea > 250.0f && mArea < 7500.0f) {
			if (g_currentExperiment != markerId[i]) {
				g_imgExpCompDirty = true;
			}
			g_currentExperiment = markerId[i];

			text_overlay.setTo(cv::Scalar(255, 255, 255));
			cv::putText(text_overlay, "Exp " + std::to_string(g_currentExperiment), cv::Point(0, 24), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255.0, 0.0, 0.0));
		}
	}

	if (g_currentExperiment >= EXP_4_ID && g_currentExperiment <= EXP_1_ID) {
		g_visType = visTypeMapping[g_currentExperiment - EXP_4_ID];
	}
}

int applyError(int realCardNum) {
	int fakeCardNum = realCardNum;
	int suit = fakeCardNum / 8;
	int val = fakeCardNum % 8;
	int offset = 0;
	if (g_currentExperiment == 60) {
		offset = 32;
	}

	if (g_currentExperiment == EXP_1_ID || g_currentExperiment == EXP_4_ID) {
		switch (mistakes[realCardNum + offset]) {
		case MISTAKE_BIGLITTLE:
			fakeCardNum = fakeCardNum ^ 4; //Flips the high order bit of the card number ... so 0 becomes 4, 5 becomes 1, and so on.
			break;
		case MISTAKE_EVENODD:
			fakeCardNum = fakeCardNum ^ 1; //Flips even to odd, odd to even
			break;
		case MISTAKE_SUITMINUS:
			suit = (suit + 3) % 4;
			fakeCardNum = suit * 8 + val;
			break;
		case MISTAKE_SUITPLUS:
			suit = (suit + 1) % 4;
			fakeCardNum = suit * 8 + val;
			break;
		default:
			//do nothing
			break;
		}

	}
	return fakeCardNum;
}

/* Scan marker ids to see which card we are looking at.
 * Return the index where the card was found, for use in lookup into markerCorners */
int updateCard(std::vector< int > &markerId, std::vector< std::vector<cv::Point2f> > &markerCorners) {
	int index = -1;
	for (unsigned int i = 0; i < markerId.size(); i++) {
		float mArea = markerAreaApprox(markerCorners[i]);
		float mEccentricity = markerEccentricity(markerCorners[i]);

		if (markerId[i] < 32 &&  mEccentricity > 0.5 && mArea > 250.0f && mArea < 7500.0f) {
			if (g_currentCard != applyError(markerId[i])) {
				g_imgExpCompDirty = true;
			}
			g_currentCard = applyError(markerId[i]);
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

cv::Point2f origPts[] = { 
	cv::Point2f(0, 0),
	cv::Point2f(1, 0),
	cv::Point2f(1, 1),
	cv::Point2f(0, 1) 
};

void fillMarkerWithImage(unsigned char* target, cv::Mat &source, int ovWidth, int ovHeight, std::vector<cv::Point2f> &corners, float expansion = 1.0f, bool clipTop = false) {
	if (corners.size() >= 4) {
		//First, find the Region Of Interest that might need to be filled
		float minX = corners[0].x;
		float maxX = corners[0].x;
		float minY = corners[0].y;
		float maxY = corners[0].y;
		for (unsigned int i = 1; i < corners.size(); i++) {
			minX = std::min(minX, corners[i].x);
			maxX = std::max(maxX, corners[i].x);
			minY = std::min(minY, corners[i].y);
			maxY = std::max(maxY, corners[i].y);
		}
		float diffX = maxX - minX;
		float diffY = maxY - minY;
		minY = minY - (expansion / 2)*diffY;
		maxY = maxY + (expansion / 2)*diffY;
		minX = minX - (expansion / 2)*diffX;
		maxX = maxX + (expansion / 2)*diffX;

		//Compute perspective projection here
		cv::Mat perspProj = getPerspectiveTransform(corners.data(), origPts);
		cv::Mat invPerspProj = getPerspectiveTransform(origPts, corners.data());

		static std::vector<cv::Point2f> eCorners;
		static std::vector<cv::Point2f> eOrigPts;
		eCorners.clear();
		eOrigPts.clear();

		eOrigPts.push_back(cv::Point2f(0.5f - expansion / 2, 0.5f - expansion / 2));
		eOrigPts.push_back(cv::Point2f(0.5f + expansion / 2, 0.5f - expansion / 2));
		eOrigPts.push_back(cv::Point2f(0.5f + expansion / 2, 0.5f + expansion / 2));
		eOrigPts.push_back(cv::Point2f(0.5f - expansion / 2, 0.5f + expansion / 2));

		cv::perspectiveTransform(eOrigPts, eCorners, invPerspProj);

		for (int y = (int)minY; y < (int)maxY; y++) {
			for (int x = (int)minX; x < (int)maxX; x++) {
				static std::vector< cv::Point2f> testPoint;
				testPoint.clear();
				testPoint.push_back(cv::Point2f((float)x, (float)y));
				if (pointInTriangle(testPoint[0], eCorners[0], eCorners[1], eCorners[2]) ||
					pointInTriangle(testPoint[0], eCorners[2], eCorners[3], eCorners[0])) {
					int target_index = 4 * (x + ovWidth*y);

					static std::vector< cv::Point2f> srcPoint;
					cv::perspectiveTransform(testPoint, srcPoint, perspProj);

					int src_x = (int)((((expansion / 2 - 0.5f) + srcPoint[0].x) / expansion)*(source.cols - 1));
					int src_y = (int)((((expansion / 2 - 0.5f) + srcPoint[0].y) / expansion)*(source.rows - 1));
					int src_index = 3 * (src_x + src_y*source.cols);

					if (src_x >= source.cols || src_y >= source.rows || 
						(clipTop && src_y < source.rows * 0.35) || target_index < 0 || src_index < 0) {
						continue;
					}

					for (int offset = 0; offset < 4; offset++) {
						target[target_index + offset] = (3*(int)source.data[src_index + offset] + 1*(int)target[target_index + offset]) / 4;
					}
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

bool checkExpCondition(int experimentNum, int cardNum, int suitNum, int colNum, int rowNum) {
	switch (experimentNum) {
	case EXP_1_ID:
	default:
		if (rowNum == 0 && cardNum % 2 == 1) return true;
		if (rowNum == 2 && cardNum % 2 == 0) return true;
		if (rowNum == 1 && colNum == 0 && (suitNum == SUIT_HEART || suitNum == SUIT_SPADE)) return true;
		if (rowNum == 1 && colNum == 1 && (suitNum == SUIT_CLUB || suitNum == SUIT_DIAMOND)) return true;
		if (rowNum == 3 && colNum == 0 && (suitNum == SUIT_DIAMOND || suitNum == SUIT_SPADE)) return true;
		if (rowNum == 3 && colNum == 1 && (suitNum == SUIT_CLUB || suitNum == SUIT_HEART)) return true;
		return false;

	case EXP_2_ID:
		if (rowNum == 0 && (suitNum == SUIT_HEART || suitNum == SUIT_SPADE)) return true;
		if (rowNum == 2 && (suitNum == SUIT_DIAMOND || suitNum == SUIT_CLUB)) return true;
		if (rowNum == 1 && colNum == 0 && cardNum % 2 == 1) return true;
		if (rowNum == 1 && colNum == 1 && cardNum % 2 == 0) return true;
		if (rowNum == 3 && colNum == 0 && cardNum <= 5) return true;
		if (rowNum == 3 && colNum == 1 && cardNum >= 6) return true;
		return false;

	case EXP_3_ID:
		if (rowNum == 0 && cardNum % 2 == 1) return true;
		if (rowNum == 2 && cardNum % 2 == 0) return true;
		if (rowNum == 1 && colNum == 0 && cardNum <= 5) return true;
		if (rowNum == 1 && colNum == 1 && cardNum >= 6) return true;
		if (rowNum == 3 && colNum == 0 && (suitNum == SUIT_HEART || suitNum == SUIT_CLUB)) return true;
		if (rowNum == 3 && colNum == 1 && (suitNum == SUIT_DIAMOND || suitNum == SUIT_SPADE)) return true;
		return false;

	case EXP_4_ID:
		if (rowNum == 0 && cardNum <= 5) return true;
		if (rowNum == 2 && cardNum >= 6) return true;
		if (rowNum == 1 && colNum == 0 && (suitNum == SUIT_DIAMOND || suitNum == SUIT_CLUB)) return true;
		if (rowNum == 1 && colNum == 1 && (suitNum == SUIT_SPADE || suitNum == SUIT_HEART)) return true;
		if (rowNum == 3 && colNum == 0 && cardNum % 2 == 0) return true;
		if (rowNum == 3 && colNum == 1 && cardNum % 2 == 1) return true;
		return false;
	}

	return false;
}

void addOverlay(unsigned char* p, int ovWidth, int ovHeight, cv::Mat &overlay) {
	for (int row = 0; row < ovHeight && row < overlay.rows; row++) {
		for (int col = 0; col < ovWidth && col < overlay.cols; col++) {
			int pindex = (ovWidth/2 - overlay.cols/2 + col) + (64+row)*ovWidth;
			int oindex = col + row*overlay.cols;

			for (int i = 0; i < 3; i++) {
				p[4 * pindex + i] = overlay.data[3 * oindex + i];
			}
		}
	}
}

void processMarkers(unsigned char* p, int ovWidth, int ovHeight, std::vector< int > &markerIds, std::vector< std::vector<cv::Point2f> > &markerCorners) {
	updateExperiment(markerIds, markerCorners); //Switch experiments, if necessary
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
		
		if (g_visType == VIS_ARROWS_ON_CARD) {
			fillMarkerWithImage(p, goLeft ? img_left : img_right, ovWidth, ovHeight, rotatedCorners);
		}
		else {
			if (g_imgExpCompDirty) { //Will only do this if something has changed in our state
				img_exps[g_currentExperiment % 60].copyTo(img_exp_composite);
				if (g_visType == VIS_REASONING_ON_CARD) {
					//First, shade based on whether should go left or right
					if (goLeft) {
						img_exp_composite = img_exp_composite.mul(img_r[0], 1.0 / 255.0);
					}
					else {
						img_exp_composite = img_exp_composite.mul(img_l[0], 1.0 / 255.0);
					}

					for (int row = 0; row < 4; row++) {
						if (!checkExpCondition(g_currentExperiment, 2 + g_currentCard % 8, g_currentCard / 8, 0, row)) {
							img_exp_composite = img_exp_composite.mul(img_l[row + 1], 1.0 / 255.0);
						}
						if (!checkExpCondition(g_currentExperiment, 2 + g_currentCard % 8, g_currentCard / 8, 1, row)) {
							img_exp_composite = img_exp_composite.mul(img_r[row + 1], 1.0 / 255.0);
						}
					}
				}
				g_imgExpCompDirty = false;
			}
			fillMarkerWithImage(p, img_exp_composite, ovWidth, ovHeight, rotatedCorners, 4.0f, true);
		}
	}
	else if (g_visType == VIS_ARROWS_ON_BOX && ((goLeft && boxIndices.first != -1) || (!goLeft && boxIndices.second != -1))) {
		int index = goLeft ? boxIndices.first : boxIndices.second;
		std::vector<cv::Point2f> rotatedCorners = markerCorners[index];

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
	
	img_exp_composite = cv::imread("exp60.png", CV_LOAD_IMAGE_COLOR);
	
	img_l.push_back(cv::imread("L1.png", CV_LOAD_IMAGE_COLOR));
	img_l.push_back(cv::imread("L2.png", CV_LOAD_IMAGE_COLOR));
	img_l.push_back(cv::imread("L3.png", CV_LOAD_IMAGE_COLOR));
	img_l.push_back(cv::imread("L4.png", CV_LOAD_IMAGE_COLOR));
	img_l.push_back(cv::imread("L5.png", CV_LOAD_IMAGE_COLOR));

	img_r.push_back(cv::imread("R1.png", CV_LOAD_IMAGE_COLOR));
	img_r.push_back(cv::imread("R2.png", CV_LOAD_IMAGE_COLOR));
	img_r.push_back(cv::imread("R3.png", CV_LOAD_IMAGE_COLOR));
	img_r.push_back(cv::imread("R4.png", CV_LOAD_IMAGE_COLOR));
	img_r.push_back(cv::imread("R5.png", CV_LOAD_IMAGE_COLOR));

	text_overlay.create(32, 150, CV_8UC3);
	text_overlay.setTo(cv::Scalar(255, 255, 255));
	cv::putText(text_overlay, "Exp " + std::to_string(g_currentExperiment), cv::Point(0, 24), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255.0, 0.0, 0.0));

	{
		std::vector< int > markerIds;
		std::vector< std::vector<cv::Point2f> > markerCorners;//, rejectedCandidates;
        cv::Ptr<cv::aruco::DetectorParameters> parameters = new cv::aruco::DetectorParameters(); 
		parameters->doCornerRefinement = true;
		cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_ARUCO_ORIGINAL);
		static cv::Mat grey;
		if (grey.dims != 2) {
			grey.create(ovHeight, ovWidth, CV_8UC1);//, ovrvision.GetCamImageBGRA(OVR::Cameye::OV_CAMEYE_LEFT), );
		}

		InitializeCamPlane(DIRECTX.Device, DIRECTX.Context, ovWidth, ovHeight, 1.0f);

		// Main loop
		while (DIRECTX.HandleMessages())
		{
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

					//Camera View
					if (eye == 0) {
						unsigned char* p = ovrvision.GetCamImageBGRA(OVR::Cameye::OV_CAMEYE_LEFT);
						//Convert to greyscale
						for (int i = 0; i < ovWidth*ovHeight; i++) {
							unsigned char average = (((int)p[4 * i] + (int)p[4 * i + 1] + (int)p[4 * i + 2]) / 3);
							grey.data[i] = average;
						}
						cv::aruco::detectMarkers(grey, dictionary, markerCorners, markerIds, parameters);//, parameters, rejectedCandidates);

						processMarkers(p, ovWidth, ovHeight, markerIds, markerCorners);
						addOverlay(p, ovWidth, ovHeight, text_overlay);
						SetCamImage(DIRECTX.Context, p, ovWidth*ovPixelsize);
					}
					else {
						unsigned char* p = ovrvision.GetCamImageBGRA(OVR::Cameye::OV_CAMEYE_RIGHT);
						//Convert to greyscale
						for (int i = 0; i < ovWidth*ovHeight; i++) {
							unsigned char average = (((int)p[4 * i] + (int)p[4 * i + 1] + (int)p[4 * i + 2]) / 3);
							grey.data[i] = average;
						}
						cv::aruco::detectMarkers(grey, dictionary, markerCorners, markerIds, parameters);//, parameters, rejectedCandidates);

						processMarkers(p, ovWidth, ovHeight, markerIds, markerCorners);
						addOverlay(p, ovWidth, ovHeight, text_overlay);
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
