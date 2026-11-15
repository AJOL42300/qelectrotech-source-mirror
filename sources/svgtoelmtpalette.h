/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef SVGTOELMTPALETTE_H
#define SVGTOELMTPALETTE_H

/**
	The colours an element part can have, by the name its "style"
	attribute uses for the "filling" and the "color" (the pen), with
	their value as the element editor draws them. The same list, minus
	the duplicates of a name, as CustomElementGraphicPart::applyStylesToQPainter().
*/
namespace SvgToElmtPalette
{
	struct Entry {
		const char *name;
		int red;
		int green;
		int blue;
	};

	inline constexpr Entry colors[] = {
	{"black", 0, 0, 0},
	{"white", 255, 255, 255},
	{"green", 0, 255, 0},
	{"red", 255, 0, 0},
	{"blue", 0, 0, 255},
	{"gray", 160, 160, 164},
	{"brun", 97, 44, 0},
	{"yellow", 255, 255, 0},
	{"cyan", 0, 255, 255},
	{"magenta", 255, 0, 255},
	{"lightgray", 192, 192, 192},
	{"orange", 255, 128, 0},
	{"purple", 136, 28, 168},
	{"HTMLPinkPink", 255, 192, 203},
	{"HTMLPinkLightPink", 255, 182, 193},
	{"HTMLPinkHotPink", 255, 105, 180},
	{"HTMLPinkDeepPink", 255, 20, 147},
	{"HTMLPinkPaleVioletRed", 219, 112, 147},
	{"HTMLPinkMediumVioletRed", 199, 21, 133},
	{"HTMLRedLightSalmon", 255, 160, 122},
	{"HTMLRedSalmon", 250, 128, 114},
	{"HTMLRedDarkSalmon", 233, 150, 122},
	{"HTMLRedLightCoral", 240, 128, 128},
	{"HTMLRedIndianRed", 205, 92, 92},
	{"HTMLRedCrimson", 220, 20, 60},
	{"HTMLRedFirebrick", 178, 34, 34},
	{"HTMLRedDarkRed", 139, 0, 0},
	{"HTMLOrangeOrangeRed", 255, 69, 0},
	{"HTMLOrangeTomato", 255, 99, 71},
	{"HTMLOrangeCoral", 255, 127, 80},
	{"HTMLOrangeDarkOrange", 255, 140, 0},
	{"HTMLOrangeOrange", 255, 165, 0},
	{"HTMLYellowLightYellow", 255, 255, 224},
	{"HTMLYellowLemonChiffon", 255, 250, 205},
	{"HTMLYellowLightGoldenrodYellow", 250, 250, 210},
	{"HTMLYellowPapayaWhip", 255, 239, 213},
	{"HTMLYellowMoccasin", 255, 228, 181},
	{"HTMLYellowPeachPuff", 255, 218, 185},
	{"HTMLYellowPaleGoldenrod", 238, 232, 170},
	{"HTMLYellowKhaki", 240, 230, 140},
	{"HTMLYellowDarkKhaki", 189, 183, 107},
	{"HTMLYellowGold", 255, 215, 0},
	{"HTMLBrownCornsilk", 255, 248, 220},
	{"HTMLBrownBlanchedAlmond", 255, 235, 205},
	{"HTMLBrownBisque", 255, 228, 196},
	{"HTMLBrownNavajoWhite", 255, 222, 173},
	{"HTMLBrownWheat", 245, 222, 179},
	{"HTMLBrownBurlywood", 222, 184, 135},
	{"HTMLBrownTan", 210, 180, 140},
	{"HTMLBrownRosyBrown", 188, 143, 143},
	{"HTMLBrownSandyBrown", 244, 164, 96},
	{"HTMLBrownGoldenrod", 218, 165, 32},
	{"HTMLBrownDarkGoldenrod", 184, 134, 11},
	{"HTMLBrownPeru", 205, 133, 63},
	{"HTMLBrownChocolate", 210, 105, 30},
	{"HTMLBrownSaddleBrown", 139, 69, 19},
	{"HTMLBrownSienna", 160, 82, 45},
	{"HTMLBrownBrown", 165, 42, 42},
	{"HTMLBrownMaroon", 128, 0, 0},
	{"HTMLGreenDarkOliveGreen", 85, 107, 47},
	{"HTMLGreenOlive", 128, 128, 0},
	{"HTMLGreenOliveDrab", 107, 142, 35},
	{"HTMLGreenYellowGreen", 154, 205, 50},
	{"HTMLGreenLimeGreen", 50, 205, 50},
	{"HTMLGreenLawnGreen", 124, 252, 0},
	{"HTMLGreenChartreuse", 127, 255, 0},
	{"HTMLGreenGreenYellow", 173, 255, 47},
	{"HTMLGreenSpringGreen", 0, 255, 127},
	{"HTMLGreenMediumSpringGreen", 0, 250, 154},
	{"HTMLGreenLightGreen", 144, 238, 144},
	{"HTMLGreenPaleGreen", 152, 251, 152},
	{"HTMLGreenDarkSeaGreen", 143, 188, 143},
	{"HTMLGreenMediumAquamarine", 102, 205, 170},
	{"HTMLGreenMediumSeaGreen", 60, 179, 113},
	{"HTMLGreenSeaGreen", 46, 139, 87},
	{"HTMLGreenForestGreen", 34, 139, 34},
	{"HTMLGreenGreen", 0, 128, 0},
	{"HTMLGreenDarkGreen", 0, 100, 0},
	{"HTMLCyanLightCyan", 224, 255, 255},
	{"HTMLCyanPaleTurquoise", 175, 238, 238},
	{"HTMLCyanAquamarine", 127, 255, 212},
	{"HTMLCyanTurquoise", 64, 224, 208},
	{"HTMLCyanMediumTurquoise", 72, 209, 204},
	{"HTMLCyanDarkTurquoise", 0, 206, 209},
	{"HTMLCyanLightSeaGreen", 32, 178, 170},
	{"HTMLCyanCadetBlue", 95, 158, 160},
	{"HTMLCyanDarkCyan", 0, 139, 139},
	{"HTMLCyanTeal", 0, 128, 128},
	{"HTMLBlueLightSteelBlue", 176, 196, 222},
	{"HTMLBluePowderBlue", 176, 224, 230},
	{"HTMLBlueLightBlue", 173, 216, 230},
	{"HTMLBlueSkyBlue", 135, 206, 235},
	{"HTMLBlueLightSkyBlue", 135, 206, 250},
	{"HTMLBlueDeepSkyBlue", 0, 191, 255},
	{"HTMLBlueDodgerBlue", 30, 144, 255},
	{"HTMLBlueCornflowerBlue", 100, 149, 237},
	{"HTMLBlueSteelBlue", 70, 130, 180},
	{"HTMLBlueRoyalBlue", 65, 105, 225},
	{"HTMLBlueMediumBlue", 0, 0, 205},
	{"HTMLBlueDarkBlue", 0, 0, 139},
	{"HTMLBlueNavy", 0, 0, 128},
	{"HTMLBlueMidnightBlue", 25, 25, 112},
	{"HTMLPurpleLavender", 230, 230, 250},
	{"HTMLPurpleThistle", 216, 191, 216},
	{"HTMLPurplePlum", 221, 160, 221},
	{"HTMLPurpleViolet", 238, 130, 238},
	{"HTMLPurpleOrchid", 218, 112, 214},
	{"HTMLPurpleMediumOrchid", 186, 85, 211},
	{"HTMLPurpleMediumPurple", 147, 112, 219},
	{"HTMLPurpleBlueViolet", 138, 43, 226},
	{"HTMLPurpleDarkViolet", 148, 0, 211},
	{"HTMLPurpleDarkOrchid", 153, 50, 204},
	{"HTMLPurpleDarkMagenta", 139, 0, 139},
	{"HTMLPurplePurple", 128, 0, 128},
	{"HTMLPurpleIndigo", 75, 0, 130},
	{"HTMLPurpleDarkSlateBlue", 72, 61, 139},
	{"HTMLPurpleSlateBlue", 106, 90, 205},
	{"HTMLPurpleMediumSlateBlue", 123, 104, 238},
	{"HTMLWhiteSnow", 255, 250, 250},
	{"HTMLWhiteHoneydew", 240, 255, 240},
	{"HTMLWhiteMintCream", 245, 255, 250},
	{"HTMLWhiteAzure", 240, 255, 255},
	{"HTMLWhiteAliceBlue", 240, 248, 255},
	{"HTMLWhiteGhostWhite", 248, 248, 255},
	{"HTMLWhiteWhiteSmoke", 245, 245, 245},
	{"HTMLWhiteSeashell", 255, 245, 238},
	{"HTMLWhiteBeige", 245, 245, 220},
	{"HTMLWhiteOldLace", 253, 245, 230},
	{"HTMLWhiteFloralWhite", 255, 250, 240},
	{"HTMLWhiteIvory", 255, 255, 240},
	{"HTMLWhiteAntiqueWhite", 250, 235, 215},
	{"HTMLWhiteLinen", 250, 240, 230},
	{"HTMLWhiteLavenderBlush", 255, 240, 245},
	{"HTMLWhiteMistyRose", 255, 228, 225},
	{"HTMLGrayGainsboro", 220, 220, 220},
	{"HTMLGrayLightGray", 211, 211, 211},
	{"HTMLGrayDarkGray", 169, 169, 169},
	{"HTMLGrayGray", 128, 128, 128},
	{"HTMLGrayDimGray", 105, 105, 105},
	{"HTMLGrayLightSlateGray", 119, 136, 153},
	{"HTMLGraySlateGray", 112, 128, 144},
	{"HTMLGrayDarkSlateGray", 47, 79, 79},
	};
}

#endif // SVGTOELMTPALETTE_H
