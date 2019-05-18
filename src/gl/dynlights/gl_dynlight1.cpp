// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2002-2016 Christoph Oelckers
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//
/*
** gl_dynlight1.cpp
** dynamic light application
**
**/

#include "gl/system/gl_system.h"
#include "c_dispatch.h"
#include "p_local.h"
#include "vectors.h"
#include "gl/gl_functions.h"
#include "g_level.h"
#include "actorinlines.h"
#include "a_dynlight.h"

#include "gl/system/gl_interface.h"
#include "gl/renderer/gl_renderer.h"
#include "gl/renderer/gl_lightdata.h"
#include "gl/data/gl_data.h"
#include "gl/dynlights/gl_dynlight.h"
#include "gl/scene/gl_drawinfo.h"
#include "gl/scene/gl_portal.h"
#include "gl/shaders/gl_shader.h"
#include "gl/textures/gl_material.h"


//==========================================================================
//
// Light related CVARs
//
//==========================================================================

CVAR (Bool, gl_lights_checkside, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG);
CVAR (Bool, gl_light_sprites, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG);
CVAR (Bool, gl_light_particles, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG);


//==========================================================================
//
// Sets up the parameters to render one dynamic light onto one plane
//
//==========================================================================
bool gl_GetLight(int group, Plane & p, FDynamicLight * light, bool checkside, FDynLightData &ldata)
{
	DVector3 pos = light->PosRelative(group);
	float radius = (light->GetRadius());

	auto dist = fabs(p.DistToPoint((float)pos.X, (float)pos.Z, (float)pos.Y));

	if (radius <= 0.f) return false;
	if (dist > radius) return false;
	if (checkside && gl_lights_checkside && p.PointOnSide((float)pos.X, (float)pos.Z, (float)pos.Y))
	{
		return false;
	}

	gl_AddLightToList(group, light, ldata);
	return true;
}

//==========================================================================
//
// Add one dynamic light to the light data list
//
//==========================================================================
void gl_AddLightToList(int group, FDynamicLight * light, FDynLightData &ldata)
{
	int i = 0;

	DVector3 pos = light->PosRelative(group);
	float radius = light->GetRadius();

	float cs;
	if (light->IsAdditive()) 
	{
		cs = 0.2f;
		i = 2;
	}
	else 
	{
		cs = 1.0f;
	}

	float r = light->GetRed() / 255.0f * cs;
	float g = light->GetGreen() / 255.0f * cs;
	float b = light->GetBlue() / 255.0f * cs;

	if (light->IsSubtractive())
	{
		DVector3 v(r, g, b);
		float length = (float)v.Length();
		
		r = length - r;
		g = length - g;
		b = length - b;
		i = 1;
	}

	float shadowIndex = light->mShadowmapIndex + 1.0f;

    // Store attenuate flag in the sign bit of the float.
	if (light->IsAttenuated()) shadowIndex = -shadowIndex;

	float lightType = 0.0f;
	float spotInnerAngle = 0.0f;
	float spotOuterAngle = 0.0f;
	float spotDirX = 0.0f;
	float spotDirY = 0.0f;
	float spotDirZ = 0.0f;
	if (light->IsSpot())
	{
		lightType = 1.0f;
		spotInnerAngle = (float)light->pSpotInnerAngle->Cos();
		spotOuterAngle = (float)light->pSpotOuterAngle->Cos();

		DAngle negPitch = -*light->pPitch;
		DAngle Angle = light->target->Angles.Yaw;
		double xzLen = negPitch.Cos();
		spotDirX = float(-Angle.Cos() * xzLen);
		spotDirY = float(-negPitch.Sin());
		spotDirZ = float(-Angle.Sin() * xzLen);
	}

	float *data = &ldata.arrays[i][ldata.arrays[i].Reserve(16)];
	data[0] = float(pos.X);
	data[1] = float(pos.Z);
	data[2] = float(pos.Y);
	data[3] = radius;
	data[4] = r;
	data[5] = g;
	data[6] = b;
	data[7] = shadowIndex;
	data[8] = spotDirX;
	data[9] = spotDirY;
	data[10] = spotDirZ;
	data[11] = lightType;
	data[12] = spotInnerAngle;
	data[13] = spotOuterAngle;
	data[14] = 0.0f; // unused
	data[15] = 0.0f; // unused
}
