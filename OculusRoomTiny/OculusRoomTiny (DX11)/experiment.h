
#include <iostream>

#include <opencv2/aruco.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

// Include DirectX
#include "Win32_DirectXAppUtil.h"

// Include the Oculus SDK
#include "OVR_CAPI_D3D.h"

#include <ovrvision_pro.h>	//Ovrvision SDK
//#include <ovrvision_ar.h>

#include "OculusTexture.h"

class expr {
public:	

	/**
	@brief	Generate an array of some random numbers

	@param	size	 The size of the output array that we want

	@return An array that contains some randomly generated numbers
	*/
	static std::vector<int> getRandomNumbers(int size);

	//From www.blackpawn.com/texts/pointinpoly
	static bool sameSide(cv::Point2f &p1, cv::Point2f &p2, cv::Point2f &a,
		cv::Point2f &b);

	static bool pointInTriangle(cv::Point2f &p, cv::Point2f &a, cv::Point2f &b,
		cv::Point2f &c);

	/**
	@brief	Decide if the card should go to left box or not.

	@param	cardId	   	Identifier for the card.
	@param	currentCrit	The current criteria.

	@return True if it meets the requirement, false if not
	*/
	static bool cardGoesLeft(int cardId, int currentCrit);

	/**
	@brief	Calculate the distance between two points

	@param	pt1	The first point.
	@param	pt2	The second point.

	@return The distance.
	*/
	static float dist(cv::Point2f pt1, cv::Point2f pt2);

	/**
	@brief	Calculate the approximate area of a marker.

	@param	markerCorners A set of 4 corners of the marker.

	@return The area
	*/
	static float markerAreaApprox(std::vector< cv::Point2f > markerCorners);

	static float markerEccentricity(std::vector< cv::Point2f > markerCorners);

	/**
	@brief	Scan marker ids to see which experiment we are doing

	@param markerId	 	Identifier for the marker.
	@param markerCorners	The marker corners.
	*/
	static void updateExperiment(const std::vector< int > &markerId,
		const std::vector< std::vector<cv::Point2f> > &markerCorners);

	/**
	@brief	Applies error to a card.

	@param	realCardNum	The real ID card number.

	@return  the fake ID number of that card if it is assigned to any error, real ID number if it is not
	*/
	static int applyError(int realCardNum);

	/**
	@brief	Scan marker ids to see which card we are looking at.

	@param [in,out]	p
	@param	markerId	 	Identifier for the marker.
	@param	markerCorners	The marker corners.

	@return	the index where the card was found, for use in lookup into markerCorners.
	*/
	static int updateCard(unsigned char* p, const std::vector< int > &markerId,
		const std::vector< std::vector<cv::Point2f> > &markerCorners);


	/*std::pair<int, int> findBoxes(std::vector< int > &markerId) {
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
	*/


	/**
	@brief	Fill marker with image.

	@param [in,out]	target
	@param	source			  	The image that will be added.
	@param	ovWidth			  	Width of the ovr image.
	@param	ovHeight		  	Height of the ovr image.
	@param	corners			  	The marker corners.
	@param	expansion		  	(Optional)
	@param	clipTop			  	(Optional)
	*/
	static void fillMarkerWithImage(unsigned char* target, const cv::Mat &source, int ovWidth,
		int ovHeight, const std::vector<cv::Point2f> &corners,
		float expansion, bool clipTop);

	/**
	@brief	Rotate corners' position (Clockwise)

	@param [in,out]	rotatedCorners	List of 4 corners
	*/
	static void rotateCorners(std::vector<cv::Point2f> &rotatedCorners);

	/**
	@brief	Check exponent condition.

	@param	currentCrit	The current card criteria.
	@param	cardNum	   	The card number (2-9).
	@param	suitNum	   	The suit number.
	@param	colNum	   	The column number (0 is left, 1 is right)
	@param	rowNum	   	The row number (Each row is equivalent to a specific requirement) (There are 4 rows)

	@return	true if it succeeds, false if it fails.
	*/
	static bool checkExpCondition(int currentCrit, int cardNum, int suitNum,
		int colNum, int rowNum);

	/**
	@brief	Adds an overlay that shows what experiment we are doing.

	@param [in,out]	p
	@param	ovWidth			   	Width of the ovr image.
	@param	ovHeight		   	Height of the ovr image.
	@param overlay	            The overlay.
	*/
	static void addOverlay(unsigned char* p, int ovWidth, int ovHeight,
		const cv::Mat &overlay);

	/**
	@brief	Adds an overlay that shows the reasoning to sort the card of that experiment.

	@param [in,out]	p
	@param	ovWidth			   	Width of the ovr image.
	@param	ovHeight		   	Height of the ovr image.
	@param overlay	            The overlay.
	@param	xoffset
	*/
	static void addOverlay2(unsigned char* p, int ovWidth, int ovHeight,
		const cv::Mat &overlay, int xoffset);

	/**
	@brief	Adds an overlay that shows a notice telling user to double check the card they are holding.

	@param [in,out]	p
	@param	ovWidth			   	Width of the ovr image.
	@param	ovHeight		   	Height of the ovr image.
	@param overlay	            The overlay.
	@param xoffset				Amount to shift the overlay left or right
	*/
	static void expr::addOverlay3(unsigned char* p, int ovWidth, int ovHeight,
		const cv::Mat &overlay, int xoffset);

	/**
	@brief	Process the markers.

	@param [in,out]	p
	@param	ovWidth		 	Width of the ovr image.
	@param	ovHeight	 	Height of the ovr image.
	@param	markerIds	 	Identifiers for the markers.
	@param	markerCorners	The marker corners.
	*/
	static void processMarkers(unsigned char* p, int ovWidth, int ovHeight,
		const std::vector< int > &markerIds,
		const std::vector< std::vector<cv::Point2f> > &markerCorners);

	// return true to retry later (e.g. after display lost)
	static bool MainLoop(bool retryCreate);
};
