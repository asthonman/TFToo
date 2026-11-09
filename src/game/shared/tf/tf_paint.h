//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Mobility gels (Emulsion / Portal 2 paint), stored in the game DLL.
// TF2 has no paintmap sampler, so the stamps are a network log and the coat
// itself lives in a lightmap-scale atlas (see tf_paint_atlas).
//
//=============================================================================
#ifndef TF_PAINT_H
#define TF_PAINT_H
#ifdef _WIN32
#pragma once
#endif

// Same order as emulsion-sdk paint_enum.h. Do not renumber.
enum PaintPowerType
{
	BOUNCE_POWER = 0,
	REFLECT_POWER,
	SPEED_POWER,
	PORTAL_POWER,		// adhesion / stick gel (Emulsion's green gel)
	NO_POWER,			// erase
	FIFTH_POWER,
	SIXTH_POWER,
	SEVENTH_POWER,
	PAINT_POWER_TYPE_COUNT = SEVENTH_POWER
};

#define TF_PAINT_STAMP_MAX	80
#define TF_PAINT_BLOB_MAX	48

struct TFPaintContact_t
{
	int		type;
	Vector	pos;
	Vector	normal;
	bool	painted;

	TFPaintContact_t()
	{
		type = NO_POWER;
		pos.Init();
		normal.Init( 0.f, 0.f, 1.f );
		painted = false;
	}
};

extern ConVar tf_paint_enabled;
extern ConVar tf_paint_trace_radius;
extern ConVar tf_paint_speed;
extern ConVar tf_paint_speed_accel;
extern ConVar tf_paint_speed_friction;
extern ConVar tf_paint_stick_speed;
extern ConVar tf_paint_bounce_min;
extern ConVar tf_paint_bounce_wall;
extern ConVar tf_paint_splat_radius;

void TFPaint_GetColor( int type, int &r, int &g, int &b );
const char *TFPaint_Name( int type );

TFPaintContact_t TFPaint_Query( const Vector &origin, float extra );

void TFPaint_AddBlob( const Vector &pos, const Vector &vel, int type );
void TFPaint_PaintSphere( const Vector &pos, const Vector &normal, int type, float radius );
void TFPaint_EraseSphere( const Vector &pos, float radius );
void TFPaint_PlaySound( CBaseEntity *pEnt, const char *pszSound );

#endif // TF_PAINT_H
