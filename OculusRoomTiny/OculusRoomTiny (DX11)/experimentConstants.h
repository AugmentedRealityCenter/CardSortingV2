#pragma once

constexpr int VIS_ARROWS_ON_CARD = 0;
constexpr int VIS_REASONING_ON_CARD = 1;
//constexpr int VIS_ARROWS_ON_BOX = 2


constexpr int SUIT_SPADE = 0;
constexpr int SUIT_HEART = 1;
constexpr int SUIT_CLUB  = 2;
constexpr int SUIT_DIAMOND = 3;

constexpr int EXP_1_ID = 63;
constexpr int EXP_2_ID = 62;
constexpr int EXP_3_ID = 61;
constexpr int EXP_4_ID = 60;

constexpr int CRITERIA_1 = 63;
constexpr int CRITERIA_2 = 60;

//constexpr int LEFT_BOX_ID = 58
//constexpr inte RIGHT_BOX_ID = 59

constexpr int MISTAKE_NONE = 0;
constexpr int MISTAKE_EVENODD = 1;
constexpr int MISTAKE_BIGLITTLE = 2;
constexpr int MISTAKE_SUITPLUS = 4;
constexpr int MISTAKE_SUITMINUS = 8;

//Index is card number, NOT order in which subject will see cards
constexpr int MISTAKES[] = {
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

//First item is code on the Mode card, the second is the criterion/deck number, and third is the vis number
const int MODE_CRIT_DECK_LUT[4][3] =
{
	{ EXP_4_ID, CRITERIA_2, VIS_ARROWS_ON_CARD },
	{ EXP_3_ID, CRITERIA_2, VIS_REASONING_ON_CARD },
	{ EXP_2_ID, CRITERIA_1, VIS_ARROWS_ON_CARD },
	{ EXP_1_ID, CRITERIA_1, VIS_REASONING_ON_CARD }
};
constexpr int LUT_START = 60;


const cv::Point2f ORIG_PTS[] = {
	cv::Point2f(0, 0),
	cv::Point2f(1, 0),
	cv::Point2f(1, 1),
	cv::Point2f(0, 1)
};
