/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "DelaunayTriangulationSop.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "delaunator-cpp/include/delaunator.hpp"

// These functions are basic C function, which the DLL loader can find
// much easier than finding a C++ Class.
// The DLLEXPORT prefix is needed so the compile exports these functions from the .dll
// you are creating
extern "C"
{

	DLLEXPORT
	void
	FillSOPPluginInfo(SOP_PluginInfo *info)
	{
		// Always return SOP_CPLUSPLUS_API_VERSION in this function.
		info->apiVersion = SOPCPlusPlusAPIVersion;

		// The opType is the unique name for this TOP. It must start with a 
		// capital A-Z character, and all the following characters must lower case
		// or numbers (a-z, 0-9)
		info->customOPInfo.opType->setString("Delaunaytriangulationsop");

		// The opLabel is the text that will show up in the OP Create Dialog
		info->customOPInfo.opLabel->setString("Delaunay Triangulation SOP");

		// Will be turned into a 3 letter icon on the nodes
		info->customOPInfo.opIcon->setString("DTR");

		// Information about the author of this OP
		info->customOPInfo.authorName->setString("Colas Fiszman");
		info->customOPInfo.authorEmail->setString("colas.fiszman@gmail.com");

		// This SOP works with 0 or 1 inputs
		info->customOPInfo.minInputs = 1;
		info->customOPInfo.maxInputs = 1;

	}

	DLLEXPORT
	SOP_CPlusPlusBase*
	CreateSOPInstance(const OP_NodeInfo* info)
	{
		// Return a new instance of your class every time this is called.
		// It will be called once per SOP that is using the .dll
		return new DelaunayTriangulationSop(info);
	}

	DLLEXPORT
	void
	DestroySOPInstance(SOP_CPlusPlusBase* instance)
	{
		// Delete the instance here, this will be called when
		// Touch is shutting down, when the SOP using that instance is deleted, or
		// if the SOP loads a different DLL
		delete (DelaunayTriangulationSop*)instance;
	}

};


DelaunayTriangulationSop::DelaunayTriangulationSop(const OP_NodeInfo* info) : myNodeInfo(info)
{
}

DelaunayTriangulationSop::~DelaunayTriangulationSop()
{

}

void
DelaunayTriangulationSop::getGeneralInfo(SOP_GeneralInfo* ginfo, const OP_Inputs* inputs, void* reserved)
{
	// This will cause the node to cook every frame
	ginfo->cookEveryFrameIfAsked = true;

	//if direct to GPU loading:
	ginfo->directToGPU = false;

}
float
DelaunayTriangulationSop::getLimitedValue(const Position* ptArr, size_t numPoints, Axis axis, LimitMode mode) {
	float value = 0;
	float average = 0;

	if (mode == LimitMode::zero) {
		return 0.0f;
	}
	
	// convert the position pointer to a float pointer
	const float* positions = reinterpret_cast<const float*>(ptArr);

	switch (mode) {

		// find the smallest value for the specified axis
		case LimitMode::min:
			value = positions[axis];
			for (size_t i = axis; i < numPoints / 3; i += 3) {
				value = std::min(positions[i], value);
			}
			break;

		// find the average value for the specified axis
		case LimitMode::center:
			for (size_t i = axis; i < numPoints / 3; i += 3) {
				average += positions[i];
			}
			value = average / numPoints;
			break;

		// find the maximum value for the speficied axis
		case LimitMode::max:
			value = positions[axis];
			for (size_t i = axis; i < numPoints / 3; i += 3) {
				value = std::max(value, positions[i]);
			}
			break;
	}
	return value;
}

void DelaunayTriangulationSop::build2dCoordsVector(std::vector<double>& coords,
												   const Position* ptArr,
												   size_t numPoints,
												   Axis limitedAxis) {
	// convert the position pointer to a float pointer
	const float* positions = reinterpret_cast<const float*>(ptArr);

	size_t j = 0;
	for (size_t i = 0; i < numPoints * 3; i++) {

		// we need to skip the value of the axis we limit
		if ((i % 3) == limitedAxis) {
			continue;
		}

		// add the values of the other axis to the vector
		coords[j] = positions[i];
		j++;
	}
}


void
DelaunayTriangulationSop::execute(SOP_Output* output, const OP_Inputs* inputs, void* reserved)
{

	if (inputs->getNumInputs() > 0)
	{
		// get the sop connected to the first input
		const OP_SOPInput	*sinput = inputs->getInputSOP(0);

		// get the position of the points
		const Position* ptArr = sinput->getPointPositions();

		// get the orientation of the plane on which we will project the points on
		const char* planeOrientation = inputs->getParString("Planeorientation");

		// store wich axis we will limit to create the place
		Axis limitedAxis = limitedAxis = Axis::z;
		if (strcmp(planeOrientation, "XY") == 0) limitedAxis = Axis::z;
		else if (strcmp(planeOrientation, "YZ") == 0) limitedAxis = Axis::x;
		else if (strcmp(planeOrientation, "ZX") == 0) limitedAxis = Axis::y;

		// get how we will limit the axis to put all the points on the same plane
		const char* limitMethod = inputs->getParString("Limitmode");

		// store the method to limit the position
		LimitMode limitMode = LimitMode::min;
		if (strcmp(limitMethod, "Min") == 0) limitMode = LimitMode::min;
		else if (strcmp(limitMethod, "Center") == 0) limitMode = LimitMode::center;
		else if (strcmp(limitMethod, "Max") == 0) limitMode = LimitMode::max;
		else if (strcmp(limitMethod, "Zero") == 0) limitMode = LimitMode::zero;

		// get the limited value for the selected axis
		float limitedValue = getLimitedValue(ptArr,
			                                 sinput->getNumPoints(),
			                                 limitedAxis,
			                                 limitMode);

		// generate the array of 2d point we will triangulate
		std::vector<double> coords(static_cast<size_t>(sinput->getNumPoints()) * 2);
		build2dCoordsVector(coords, ptArr, sinput->getNumPoints(), limitedAxis);

		// do the delaunay triangulation
		delaunator::Delaunator delaunator(coords);

		for (std::size_t i = 0; i < delaunator.triangles.size(); i += 3) {
			Position pointPosA;
			Position pointPosB;
			Position pointPosC;

			switch (limitedAxis) {

			case Axis::x :
				pointPosA = Position(limitedValue,
					static_cast<float>(delaunator.coords[2 * delaunator.triangles[i]]),
					static_cast<float>(delaunator.coords[2 * delaunator.triangles[i] + 1]));
				pointPosB = Position(limitedValue,
					static_cast<float>(delaunator.coords[2 * delaunator.triangles[i + 1]]),
					static_cast<float>(delaunator.coords[2 * delaunator.triangles[i + 1] + 1]));
				pointPosC = Position(limitedValue,
					static_cast<float>(delaunator.coords[2 * delaunator.triangles[i + 2]]),
					static_cast<float>(delaunator.coords[2 * delaunator.triangles[i + 2] + 1]));
				break;

			case Axis::y:
				pointPosA = Position(static_cast<float>(delaunator.coords[2 * delaunator.triangles[i]]),
					limitedValue,
					static_cast<float>(delaunator.coords[2 * delaunator.triangles[i] + 1]));
				pointPosB = Position(static_cast<float>(delaunator.coords[2 * delaunator.triangles[i + 1]]),
					limitedValue,
					static_cast<float>(delaunator.coords[2 * delaunator.triangles[i + 1] + 1]));
				pointPosC = Position(static_cast<float>(delaunator.coords[2 * delaunator.triangles[i + 2]]),
					limitedValue,
					static_cast<float>(delaunator.coords[2 * delaunator.triangles[i + 2] + 1]));
				break;

			case Axis::z:
				pointPosA = Position(static_cast<float>(delaunator.coords[2 * delaunator.triangles[i]]),
					static_cast<float>(delaunator.coords[2 * delaunator.triangles[i] + 1]),
					limitedValue);
				pointPosB = Position(static_cast<float>(delaunator.coords[2 * delaunator.triangles[i + 1]]),
					static_cast<float>(delaunator.coords[2 * delaunator.triangles[i + 1] + 1]),
					limitedValue);
				pointPosC = Position(static_cast<float>(delaunator.coords[2 * delaunator.triangles[i + 2]]),
					static_cast<float>(delaunator.coords[2 * delaunator.triangles[i + 2] + 1]),
					limitedValue);
				break;
			}

			int indexA = output->addPoint(pointPosA);
			int indexB = output->addPoint(pointPosB);
			int indexC = output->addPoint(pointPosC);
			output->addTriangle(indexA, indexB, indexC);
		}
	}
}

void
DelaunayTriangulationSop::executeVBO(SOP_VBOOutput* output,
						const OP_Inputs* inputs,
						void* reserved)
{
}

//-----------------------------------------------------------------------------------------------------
//								CHOP, DAT, and custom parameters
//-----------------------------------------------------------------------------------------------------

int32_t
DelaunayTriangulationSop::getNumInfoCHOPChans(void* reserved)
{
	// We return the number of channel we want to output to any Info CHOP
	// connected to the CHOP. In this example we are just going to send 4 channels.
	return 0;
}

void
DelaunayTriangulationSop::getInfoCHOPChan(int32_t index,
								OP_InfoCHOPChan* chan, void* reserved)
{
}

bool
DelaunayTriangulationSop::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved)
{
	return false;
}

void
DelaunayTriangulationSop::getInfoDATEntries(int32_t index,
								int32_t nEntries,
								OP_InfoDATEntries* entries,
								void* reserved)
{
}



void
DelaunayTriangulationSop::setupParameters(OP_ParameterManager* manager, void* reserved)
{
	// limit mode
	{
		OP_StringParameter	sp;

		sp.name = "Planeorientation";
		sp.label = "Plane Orientation";

		sp.defaultValue = "XY";

		const char* names[] = { "XY", "YZ", "ZX" };
		const char* labels[] = { "XY Plane", "YZ Plane", "ZX Plane" };

		OP_ParAppendResult res = manager->appendMenu(sp, 3, names, labels);
		assert(res == OP_ParAppendResult::Success);
	}

	// limit mode
	{
		OP_StringParameter	sp;

		sp.name = "Limitmode";
		sp.label = "Limit Mode";

		sp.defaultValue = "Center";

		const char* names[] = { "Min", "Center", "Max", "Zero" };
		const char* labels[] = { "Min", "Center", "Max", "Zero" };

		OP_ParAppendResult res = manager->appendMenu(sp, 4, names, labels);
		assert(res == OP_ParAppendResult::Success);
	}
}

void
DelaunayTriangulationSop::pulsePressed(const char* name, void* reserved)
{
}

