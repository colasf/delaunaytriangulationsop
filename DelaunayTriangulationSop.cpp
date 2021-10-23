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
		info->customOPInfo.opType->setString("Simpleshapes");

		// The opLabel is the text that will show up in the OP Create Dialog
		info->customOPInfo.opLabel->setString("Simple Shapes");

		// Will be turned into a 3 letter icon on the nodes
		info->customOPInfo.opIcon->setString("SSP");

		// Information about the author of this OP
		info->customOPInfo.authorName->setString("Author Name");
		info->customOPInfo.authorEmail->setString("email@email.com");

		// This SOP works with 0 or 1 inputs
		info->customOPInfo.minInputs = 0;
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
	myExecuteCount = 0;
	myOffset = 0.0;
	myChop = "";

	myChopChanName = "";
	myChopChanVal = 0;

	myDat = "N/A";
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
	//bool directGPU = inputs->getParInt("Gpudirect") != 0 ? true : false;
	ginfo->directToGPU = false;

}


////-----------------------------------------------------------------------------------------------------
////										Generate a geometry on CPU
////-----------------------------------------------------------------------------------------------------
//
//void
//DelaunayTriangulationSop::cubeGeometry(SOP_Output* output, float scale)
//{
//	// to generate a geometry:
//	// addPoint() is the first function to be called.
//	// then we can add normals, colors, and any custom attributes for the points
//	// last function can be either addParticleSystem() or addTriangle()
//
//	// front
//	output->addPoint(Position(1.0f*scale, -1.0f, 1.0f));
//	output->addPoint(Position(3.0f*scale, -1.0f, 1.0f));
//	output->addPoint(Position(3.0f*scale, 1.0f, 1.0f));
//	output->addPoint(Position(1.0f*scale, 1.0, 1.0));
//	// back
//	output->addPoint(Position(1.0f*scale, -1.0f, -1.0f));
//	output->addPoint(Position(3.0f*scale, -1.0f, -1.0f));
//	output->addPoint(Position(3.0f*scale, 1.0f, -1.0f));
//	output->addPoint(Position(1.0f*scale, 1.0f, -1.0f));
//
//	Vector normal[] = {
//		// front
//		Vector(1.0, 0.0, 0.0),
//		Vector(0.0, 1.0, 0.0),
//		Vector(0.0, 0.0, 1.0),
//		Vector(1.0, 1.0, 1.0),
//		// back
//		Vector(1.0, 0.0, 0.0),
//		Vector(0.0, 1.0, 0.0),
//		Vector(0.0, 0.0, 1.0),
//		Vector(1.0, 1.0, 1.0),
//	};
//
//	Color color[] =
//	{
//		// front colors
//		Color(1.0, 0.0, 0.0, 1.0),
//		Color(0.0, 1.0, 0.0, 1.0),
//		Color(0.0, 0.0, 1.0, 1.0),
//		Color(1.0, 1.0, 1.0, 1.0),
//		// back colors
//		Color(1.0, 0.0, 0.0, 1.0),
//		Color(0.0, 1.0, 0.0, 1.0),
//		Color(0.0, 0.0, 1.0, 1.0),
//		Color(1.0, 1.0, 1.0, 1.0),
//	};
//
//	float color2[] =
//	{
//		// front colors
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		// back colors
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//	};
//
//	// indices of the input vertices
//
//	int32_t vertices[] = {
//		// front
//		0, 1, 2,
//		2, 3, 0,
//		// top
//		1, 5, 6,
//		6, 2, 1,
//		// back
//		7, 6, 5,
//		5, 4, 7,
//		// bottom
//		4, 0, 3,
//		3, 7, 4,
//		// left
//		4, 5, 1,
//		1, 0, 4,
//		// right
//		3, 2, 6,
//		6, 7, 3,
//	};
//
//	int sz = 8;
//
//
//	for (int32_t i = 0; i < sz; ++i)
//	{
//		output->setNormal(normal[i], i);
//		output->setColor(color[i], i);
//	}
//
//	SOP_CustomAttribData cu;
//	cu.name = "CustomColor";
//	cu.attribType = AttribType::Float;
//	cu.floatData = color2;
//	cu.numComponents = 4;
//	output->setCustomAttribute(&cu, sz);
//
//	for (int i = 0; i < 12; i++)
//	{
//		output->addTriangle(vertices[i * 3],
//							vertices[i * 3 + 1],
//							vertices[i * 3 + 2]);
//	}
//}
//
//void
//DelaunayTriangulationSop::lineGeometry(SOP_Output* output)
//{
//	// to generate a geometry:
//	// addPoint() is the first function to be called.
//	// then we can add normals, colors, and any custom attributes for the points
//	// last function to be called is addLines()
//
//	// line 1 = 9 vertices
//	output->addPoint(Position(-0.8f, 0.0f, 1.0f));
//	output->addPoint(Position(-0.6f, 0.4f, 1.0f));
//	output->addPoint(Position(-0.4f, 0.8f, 1.0f));
//	output->addPoint(Position(-0.2f, 0.4f, 1.0f));
//	output->addPoint(Position(0.0f,  0.0f, 1.0f));
//	output->addPoint(Position(0.2f, -0.4f, 1.0f));
//	output->addPoint(Position(0.4f, -0.8f, 1.0f));
//	output->addPoint(Position(0.6f, -0.4f, 1.0f));
//	output->addPoint(Position(0.8f,  0.0f, 1.0f));
//
//	// line 2 = 8 vertices
//	output->addPoint(Position(-0.8f, 0.2f, 1.0f));
//	output->addPoint(Position(-0.6f, 0.6f, 1.0f));
//	output->addPoint(Position(-0.4f, 1.0f, 1.0f));
//	output->addPoint(Position(-0.2f, 0.6f, 1.0f));
//	output->addPoint(Position(0.0f,  0.2f, 1.0f));
//	output->addPoint(Position(0.2f, -0.2f, 1.0f));
//	output->addPoint(Position(0.4f, -0.6f, 1.0f));
//	output->addPoint(Position(0.6f, -0.2f, 1.0f));
//
//	Vector normal[] =
//	{
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//		Vector(1.0f, 1.0f, 1.0f),
//	};
//
//	Color color[] =
//	{
//		Color(1.0f, 0.0f, 0.0f, 1.0f),
//		Color(0.0f, 1.0f, 0.0f, 1.0f),
//		Color(0.0f, 0.0f, 1.0f, 1.0f),
//		Color(1.0f, 1.0f, 1.0f, 1.0f),
//		Color(1.0f, 0.0f, 0.0f, 1.0f),
//		Color(0.0f, 1.0f, 0.0f, 1.0f),
//		Color(0.0f, 0.0f, 1.0f, 1.0f),
//		Color(1.0f, 1.0f, 1.0f, 1.0f),
//		Color(1.0f, 0.0f, 0.0f, 1.0f),
//
//		Color(1.0f, 0.0f, 0.0f, 1.0f),
//		Color(0.0f, 1.0f, 0.0f, 1.0f),
//		Color(0.0f, 0.0f, 1.0f, 1.0f),
//		Color(1.0f, 1.0f, 1.0f, 1.0f),
//		Color(1.0f, 0.0f, 0.0f, 1.0f),
//		Color(0.0f, 1.0f, 0.0f, 1.0f),
//		Color(0.0f, 0.0f, 1.0f, 1.0f),
//		Color(1.0f, 1.0f, 1.0f, 1.0f),
//	};
//
//	float color2[] =
//	{
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//		1.0, 0.0, 0.0, 1.0,
//	};
//
//	// indices of the input vertices
//
//	int32_t vertices[] =
//	{
//		0, 1, 2, 3, 4, 5, 6, 7, 8,
//		9, 10, 11, 12, 13, 14, 15, 16
//	};
//
//	int32_t lineSizes[]
//	{
//		9, 8
//	};
//
//	// the size of all vertices of all lines:
//	int sz = 9 + 8;
//
//	for (int32_t i = 0; i < sz; ++i)
//	{
//		output->setNormal(normal[i], i);
//		output->setColor(color[i], i);
//	}
//
//	SOP_CustomAttribData cu;
//	cu.name = "CustomColor";
//	cu.attribType = AttribType::Float;
//	cu.floatData = color2;
//	cu.numComponents = 4;
//	output->setCustomAttribute(&cu, sz);
//
//	//output->addLine(vertices, sz);
//	output->addLines(vertices, lineSizes, 2);
//}
//
//void
//DelaunayTriangulationSop::triangleGeometry(SOP_Output* output)
//{
//	int32_t vertices[3] = { 0, 1, 2 };
//
//	//int sz = 3;
//	output->addPoint(Position(0.0f, 0.0f, 0.0f));
//	output->addPoint(Position(0.0f, 2.0f, 0.0f));
//	output->addPoint(Position(2.0f, 0.0f, 0.0f));
//
//	Vector normal(0.0f, 0.0f, 1.0f);
//
//	output->setNormal(normal, 0);
//	output->setNormal(normal, 1);
//	output->setNormal(normal, 2);
//
//	output->addTriangle(vertices[0], vertices[1], vertices[2]);
//
//}

float
DelaunayTriangulationSop::getLimitedValue(const Position* ptArr, size_t numPoints, Axis axis, LimitMode mode) {
	float value = 0;
	float average = 0;
	// convert the position pointer to a float pointer
	const float* positions = reinterpret_cast<const float*>(ptArr);

	switch (mode) {

		// find the smallest value for the specified axis
		case LimitMode::min:
			for (size_t i = axis; i < numPoints / 3; i += 3) {
				// first step
				if (i < 3) {
					value = positions[i];
				}
				else {
					if (positions[i] < value) {
						value = positions[i];
					}
				}
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
			for (size_t i = axis; i < numPoints / 3; i += 3) {
				// first step
				if (i < 3) {
					value = positions[i];
				}
				else {
					if (positions[i] > value) {
						value = positions[i];
					}
				}
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
		if (((i + limitedAxis) % 3) == 0) {
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
	myExecuteCount++;

	if (inputs->getNumInputs() > 0)
	{
		//inputs->enablePar("Reset", 0);	// not used
		//inputs->enablePar("Shape", 0);	// not used
		//inputs->enablePar("Scale", 0);  // not used

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

		// get the limited value for the selected axis
		float limitedValue = getLimitedValue(ptArr,
			                                 sinput->getNumPoints(),
			                                 limitedAxis,
			                                 limitMode);

		// generate the 2 array of point we will triangulate
		std::vector<double> coords(static_cast<size_t>(sinput->getNumPoints()) * 2);

		build2dCoordsVector(coords, ptArr, sinput->getNumPoints(), limitedAxis);

		delaunator::Delaunator delaunator(coords);

		//for (std::size_t i = 0; i < d.triangles.size(); i += 3) {
		//	printf(
		//		"Triangle points: [[%f, %f], [%f, %f], [%f, %f]]\n",
		//		d.coords[2 * d.triangles[i]],        //tx0
		//		d.coords[2 * d.triangles[i] + 1],    //ty0
		//		d.coords[2 * d.triangles[i + 1]],    //tx1
		//		d.coords[2 * d.triangles[i + 1] + 1],//ty1
		//		d.coords[2 * d.triangles[i + 2]],    //tx2
		//		d.coords[2 * d.triangles[i + 2] + 1] //ty2
		//	     );
		//}

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

		//const Vector* normals = nullptr;
		//const Color* colors = nullptr;
		//const TexCoord* textures = nullptr;
		//int32_t numTextures = 0;

		//if (sinput->hasNormals())
		//{
		//	normals = sinput->getNormals()->normals;
		//}

		//if (sinput->hasColors())
		//{
		//	colors = sinput->getColors()->colors;
		//}

		//if (sinput->getTextures()->numTextureLayers)
		//{
		//	textures = sinput->getTextures()->textures;
		//	numTextures = sinput->getTextures()->numTextureLayers;
		//}

		//for (int i = 0; i < sinput->getNumPoints(); i++)
		//{
		//	output->addPoint(ptArr[i]);

		//	if (normals)
		//	{
		//		output->setNormal(normals[i], i);
		//	}

		//	if (colors)
		//	{
		//		output->setColor(colors[i], i);
		//	}

		//	if (textures)
		//	{
		//		//output->setTexCoord((float*)(textures + (i * numTextures * 3)), numTextures, i);
		//		output->setTexCoord(textures + (i * numTextures), numTextures, i);
		//	}

		//}

		//for (int i = 0; i < sinput->getNumCustomAttributes(); i++)
		//{
		//	const SOP_CustomAttribData* customAttr = sinput->getCustomAttribute(i);

		//	if (customAttr->attribType == AttribType::Float)
		//	{
		//		output->setCustomAttribute(customAttr, sinput->getNumPoints());
		//	}
		//	else
		//	{
		//		output->setCustomAttribute(customAttr, sinput->getNumPoints());
		//	}
		//}


		//for (int i = 0; i < sinput->getNumPrimitives(); i++)
		//{

		//	const SOP_PrimitiveInfo primInfo = sinput->getPrimitive(i);

		//	const int32_t* primVert = primInfo.pointIndices;

		//	// Note: the addTriangle() assumes that the input SOP has triangulated geometry,
		//	// if the input geometry is not a triangle, you need to convert it to triangles first:
		//	output->addTriangle(*(primVert), *(primVert + 1), *(primVert + 2));
		//}

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
	return 4;
}

void
DelaunayTriangulationSop::getInfoCHOPChan(int32_t index,
								OP_InfoCHOPChan* chan, void* reserved)
{
	// This function will be called once for each channel we said we'd want to return
	// In this example it'll only be called once.

	if (index == 0)
	{
		chan->name->setString("executeCount");
		chan->value = (float)myExecuteCount;
	}

	if (index == 1)
	{
		chan->name->setString("offset");
		chan->value = (float)myOffset;
	}

	if (index == 2)
	{
		chan->name->setString(myChop.c_str());
		chan->value = (float)myOffset;
	}

	if (index == 3)
	{
		chan->name->setString(myChopChanName.c_str());
		chan->value = myChopChanVal;
	}
}

bool
DelaunayTriangulationSop::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved)
{
	infoSize->rows = 3;
	infoSize->cols = 2;
	// Setting this to false means we'll be assigning values to the table
	// one row at a time. True means we'll do it one column at a time.
	infoSize->byColumn = false;
	return true;
}

void
DelaunayTriangulationSop::getInfoDATEntries(int32_t index,
								int32_t nEntries,
								OP_InfoDATEntries* entries,
								void* reserved)
{
	char tempBuffer[4096];

	if (index == 0)
	{
		// Set the value for the first column
#ifdef _WIN32
		strcpy_s(tempBuffer, "executeCount");
#else // macOS
		strlcpy(tempBuffer, "executeCount", sizeof(tempBuffer));
#endif
		entries->values[0]->setString(tempBuffer);

		// Set the value for the second column
#ifdef _WIN32
		sprintf_s(tempBuffer, "%d", myExecuteCount);
#else // macOS
		snprintf(tempBuffer, sizeof(tempBuffer), "%d", myExecuteCount);
#endif
		entries->values[1]->setString(tempBuffer);
	}

	if (index == 1)
	{
		// Set the value for the first column
#ifdef _WIN32
		strcpy_s(tempBuffer, "offset");
#else // macOS
		strlcpy(tempBuffer, "offset", sizeof(tempBuffer));
#endif
		entries->values[0]->setString(tempBuffer);

		// Set the value for the second column
#ifdef _WIN32
		sprintf_s(tempBuffer, "%g", myOffset);
#else // macOS
		snprintf(tempBuffer, sizeof(tempBuffer), "%g", myOffset);
#endif
		entries->values[1]->setString(tempBuffer);
	}

	if (index == 2)
	{
		// Set the value for the first column
#ifdef _WIN32
		strcpy_s(tempBuffer, "DAT input name");
#else // macOS
		strlcpy(tempBuffer, "offset", sizeof(tempBuffer));
#endif
		entries->values[0]->setString(tempBuffer);

		// Set the value for the second column
#ifdef _WIN32
		strcpy_s(tempBuffer, myDat.c_str());
#else // macOS
		snprintf(tempBuffer, sizeof(tempBuffer), "%g", myOffset);
#endif
		entries->values[1]->setString(tempBuffer);
	}
}



void
DelaunayTriangulationSop::setupParameters(OP_ParameterManager* manager, void* reserved)
{
	//// CHOP
	//{
	//	OP_StringParameter	np;

	//	np.name = "Chop";
	//	np.label = "CHOP";

	//	OP_ParAppendResult res = manager->appendCHOP(np);
	//	assert(res == OP_ParAppendResult::Success);
	//}

	//// scale
	//{
	//	OP_NumericParameter	np;

	//	np.name = "Scale";
	//	np.label = "Scale";
	//	np.defaultValues[0] = 1.0;
	//	np.minSliders[0] = -10.0;
	//	np.maxSliders[0] = 10.0;

	//	OP_ParAppendResult res = manager->appendFloat(np);
	//	assert(res == OP_ParAppendResult::Success);
	//}

	//// shape
	//{
	//	OP_StringParameter	sp;

	//	sp.name = "Shape";
	//	sp.label = "Shape";

	//	sp.defaultValue = "Cube";

	//	const char *names[] = { "Cube", "Triangle", "Line" };
	//	const char *labels[] = { "Cube", "Triangle", "Line" };

	//	OP_ParAppendResult res = manager->appendMenu(sp, 3, names, labels);
	//	assert(res == OP_ParAppendResult::Success);
	//}

	//// GPU Direct
	//{
	//	OP_NumericParameter np;

	//	np.name = "Gpudirect";
	//	np.label = "GPU Direct";

	//	OP_ParAppendResult res = manager->appendToggle(np);
	//	assert(res == OP_ParAppendResult::Success);
	//}

	//// pulse
	//{
	//	OP_NumericParameter	np;

	//	np.name = "Reset";
	//	np.label = "Reset";

	//	OP_ParAppendResult res = manager->appendPulse(np);
	//	assert(res == OP_ParAppendResult::Success);
	//}
	
	

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

		const char* names[] = { "Min", "Center", "Max" };
		const char* labels[] = { "Min", "Center", "Max" };

		OP_ParAppendResult res = manager->appendMenu(sp, 3, names, labels);
		assert(res == OP_ParAppendResult::Success);
	}

}

void
DelaunayTriangulationSop::pulsePressed(const char* name, void* reserved)
{
	if (!strcmp(name, "Reset"))
	{
		myOffset = 0.0;
	}
}

