#define _USE_MATH_DEFINES	// For M_PI, etc.
#include <cmath>
#include <memory>
#include <unordered_map>
#include <unordered_set> 
#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <array>
#include "moments.h"
#include "contour.h"

// Required by the reduction function
#include "../roi_data.h"

Contour::Contour()
{
	contour_pixels.reserve(100);
}

//
// Uses Moore's algorithm
//
void Contour::calculate (const ImageMatrix & im)
{
	//==== Pad the image

	int width = im.width, 
		height = im.height;
	
	readOnlyPixels image = im.ReadablePixels();

	int paddingColor = 0;
	std::vector<PixIntens> paddedImage((height + 2) * (width + 2), paddingColor); 
	for (int x = 0; x < width + 2; x++)
		for (int y = 0; y < height + 2; y++)
		{
			if (x == 0 || y == 0 || x == width + 1 || y == height + 1)
			{
				paddedImage[x + y * (width + 2)] = paddingColor;
			}
			else
			{
				paddedImage[x + y * (width + 2)] = image[x - 1 + (y - 1) * width];
			}
		}

	const int WHITE = 0;
	contour_pixels.clear();

	bool inside = false;
	int pos = 0;

	//==== Prepare the contour image
	std::vector<PixIntens> borderImage ((height + 2) * (width + 2), 0);

	// Set entire image to WHITE
	for (int y = 0; y < (height + 2); y++)
		for (int x = 0; x < (width + 2); x++)
		{
			borderImage [x + y * (width + 2)] = WHITE;
		}


	//==== Scan 
	for (int y = 0; y < (height + 2); y++)
		for (int x = 0; x < (width + 2); x++)
		{
			pos = x + y * (width + 2);

				
			// Scan for BLACK pixel
			if (borderImage[pos] != 0 /*borderImage[pos] == BLACK*/ && !inside)		// Entering an already discovered border
			{
				inside = true;
			}
			else if (paddedImage[pos] != 0 /*paddedImage[pos] == BLACK*/ && inside)	// Already discovered border point
			{
				continue;
			}
			else if (paddedImage[pos] == WHITE && inside)	// Leaving a border
			{
				inside = false;
			}
			else if (paddedImage[pos] != 0 /*paddedImage[pos] == BLACK*/ && !inside)	// Undiscovered border point
				{
					borderImage[pos] = paddedImage[pos]; /*BLACK*/

				int checkLocationNr = 1;	// The neighbor number of the location we want to check for a new border point
				int checkPosition;			// The corresponding absolute array address of checkLocationNr
				int newCheckLocationNr; 	// Variable that holds the neighborhood position we want to check if we find a new border at checkLocationNr
				int startPos = pos;			// Set start position
				int counter = 0; 			// Counter is used for the jacobi stop criterion
				int counter2 = 0; 			// Counter2 is used to determine if the point we have discovered is one single point

				// Defines the neighborhood offset position from current position and the neighborhood
				// position we want to check next if we find a new border at checkLocationNr
				int neighborhood[8][2] = {
						{-1,7},
						{-3 - width,7},
						{-width - 2,1},
						{-1 - width,1},
						{1,3},
						{3 + width,3},
						{width + 2,5},
						{1 + width,5}
				};
				// Trace around the neighborhood
				while (true)
				{
					checkPosition = pos + neighborhood[checkLocationNr - 1][0];
					newCheckLocationNr = neighborhood[checkLocationNr - 1][1];

					if (paddedImage[checkPosition] != 0 /*paddedImage[checkPosition] == BLACK*/) // Next border point found
					{
						if (checkPosition == startPos)
						{
							counter++;

							// Stopping criterion (jacob)
							if (newCheckLocationNr == 1 || counter >= 3)
							{
								// Close loop
								inside = true; // Since we are starting the search at were we first started we must set inside to true
								break;
							}
						}

						checkLocationNr = newCheckLocationNr; // Update which neighborhood position we should check next
						pos = checkPosition;
						counter2 = 0; 						// Reset the counter that keeps track of how many neighbors we have visited
						borderImage[checkPosition] = paddedImage[checkPosition]; /*BLACK*/
					}
					else
					{
						// Rotate clockwise in the neighborhood
						checkLocationNr = 1 + (checkLocationNr % 8);
						if (counter2 > 8)
						{
							// If counter2 is above 8 we have traced around the neighborhood and
							// therefor the border is a single black pixel and we can exit
							counter2 = 0;
							break;
						}
						else
						{
							counter2++;
						}
					}
				}
			}
		}

	//==== Remove padding and save the countour image as a vector of contour-onlu pixels
	AABB bb = im.original_aabb;
	int base_x = bb.get_xmin(),
		base_y = bb.get_ymin();
	contour_pixels.clear();
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			size_t idx = x + 1 + (y + 1) * (width + 2);
			// clippedBorderImage[x + y * width] = borderImage [idx];
			auto inte = borderImage[idx];
			if (inte)
			{

				Pixel2 p (x + base_x, y + base_y, inte);
				contour_pixels.push_back (p);
			}
		}
	}
}

StatsInt Contour::get_roi_perimeter()
{
    return (StatsInt)contour_pixels.size();
}

StatsReal Contour::get_diameter_equal_perimeter()
{
    StatsReal retval = get_roi_perimeter() / M_PI;
    return retval;
}

std::tuple<StatsReal, StatsReal, StatsReal, StatsReal> Contour::get_min_max_mean_stddev_intensity()
{
	Moments4 m;
	for (auto px : contour_pixels)
	{
		m.add(px.inten);
	}

	StatsReal min_ = m.min__(),
		max_ = m.max__(),
		mean_ = m.mean(),
		stddev_ = m.std();

	return {min_, max_, mean_, stddev_};
}

namespace Nyxus
{
	void calcRoiContour(LR& r)
	{
		if (r.roi_disabled)
			return;

		//==== Calculate ROI's image matrix
		r.aux_image_matrix.use_roi(r.raw_pixels, r.aabb);

		//==== Contour, ROI perimeter, equivalent circle diameter
		r.contour.calculate(r.aux_image_matrix);
		r.fvals[PERIMETER][0] = r.contour.get_roi_perimeter();
		r.fvals[EQUIVALENT_DIAMETER][0] = r.contour.get_diameter_equal_perimeter();
		auto [cmin, cmax, cmean, cstddev] = r.contour.get_min_max_mean_stddev_intensity();
		r.fvals[EDGE_MEAN_INTENSITY][0] = cmean;
		r.fvals[EDGE_STDDEV_INTENSITY][0] = cstddev;
		r.fvals[EDGE_MAX_INTENSITY][0] = cmax;
		r.fvals[EDGE_MIN_INTENSITY][0] = cmin;

		//==== IntegratedIntensityEdge, MaxIntensityEdge, MinIntensityEdge, etc (namely - EDGE_INTEGRATEDINTENSITY, EDGE_MAXINTENSITY, EDGE_MININTENSITY, EDGE_MEANINTENSITY, EDGE_STDDEVINTENSITY)
		r.reduce_edge_intensity_features();	
	}

	void parallelReduceContour (size_t start, size_t end, std::vector<int>* ptrLabels, std::unordered_map <int, LR>* ptrLabelData)
	{
		for (auto i = start; i < end; i++)
		{
			int lab = (*ptrLabels)[i];
			LR& r = (*ptrLabelData)[lab];
			calcRoiContour(r);
		}
	}
}
