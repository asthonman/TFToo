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

#include "bspflags.h"

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
#define TF_PAINT_ENTITY_COAT_MAX	16

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

class CBaseEntity;

extern ConVar tf_paint_enabled;
extern ConVar tf_paint_trace_radius;
extern ConVar tf_paint_speed;
extern ConVar tf_paint_speed_accel;
extern ConVar tf_paint_speed_friction;
extern ConVar tf_paint_stick_speed;
extern ConVar tf_paint_bounce_min;
extern ConVar tf_paint_bounce_wall;
extern ConVar tf_paint_splat_radius;
extern ConVar tf_paint_grease_lifetime;
extern ConVar tf_paint_ice_time;
extern ConVar tf_paint_ice_friction;
extern ConVar tf_paint_ice_accel;
extern ConVar tf_paint_glue_time;
extern ConVar tf_paint_repulse_enemy_time;
extern ConVar tf_paint_repulse_ally_time;

void TFPaint_GetColor( int type, int &r, int &g, int &b, int team = 0 );
const char *TFPaint_Name( int type );

// Gel decals stick to world brushes and static props. The trace is MASK_SHOT
// with no hitboxes, so players and bullet-pass materials (CONTENTS_GRATE)
// are not a surface. Dynamic props and func_brush are not either.
#define TF_PAINT_VIS_MASK ( MASK_SHOT & ~( CONTENTS_HITBOX ) )

TFPaintContact_t TFPaint_Query( const Vector &origin, float extra );

// Team that painted the nearest stamp of this type covering origin. 0 if none.
int TFPaint_QueryStampTeam( const Vector &origin, int type );

struct TFPaintStampInfo_t
{
	bool	found;
	int		team;
	float	die;		// absolute time, 0 = until erased

	TFPaintStampInfo_t()
	{
		found = false;
		team = 0;
		die = 0.f;
	}
};

// extra is added to the stamp radius. 28 matches QueryStampTeam.
TFPaintStampInfo_t TFPaint_QueryStampInfo( const Vector &origin, int type, float extra = 28.f );

// flCoatLife is how long the splat stays. 0 lasts until something erases it.
// Grease gun passes tf_paint_grease_lifetime. pOwner is who fired the blob.
// iTeam is stored on the stamp so grease can speed allies and ice enemies.
void TFPaint_AddBlob( const Vector &pos, const Vector &vel, int type, CBaseEntity *pOwner = NULL, float flCoatLife = 0.f );
void TFPaint_PaintSphere( const Vector &pos, const Vector &normal, int type, float radius, float flCoatLife = 0.f, int iTeam = 0, bool bForce = false );
void TFPaint_EraseSphere( const Vector &pos, float radius );
void TFPaint_PlaySound( CBaseEntity *pEnt, const char *pszSound );

// Timed chemist coats. Repulsion is BOUNCE_POWER. Nitric fluid is FIFTH_POWER.
void TFPaint_AddTimedCoat( const Vector &pos, const Vector &normal, int type, float radius, float lifetime, bool bNitric, CBaseEntity *pOwner );
void TFPaint_EraseCoatsNear( const Vector &pos, float radius );
void TFPaint_IgniteNitricSegment( const Vector &start, const Vector &end );
// Any impact (bullet, projectile, melee, blast) within extra units of a nitric coat lights it.
void TFPaint_IgniteNitricAt( const Vector &pos, float extra );

#ifdef GAME_DLL
// Brushes and static props only. Players, dynamic props, and func_brush are skipped.
void TFPaint_TraceDecal( const Vector &start, const Vector &end, const Vector &mins, const Vector &maxs, trace_t &tr );
#endif

// Bounce a projectile off repulsion gel instead of its normal surface hit.
// Speed is kept. nOutward points out of the solid. Returns true if it bounced.
// bForce skips the world-gel test so a coated player, building, or prop can bounce it.
bool TFPaint_ReflectProjectile( CBaseEntity *pEnt, const Vector &hitPos, const Vector &nOutward, bool bForce = false );

// True while this player, building, or physics prop is wearing repulsion gel.
bool TFPaint_IsRepulsionCoated( CBaseEntity *pEnt );

// Bounce pProj off a coated body. Returns false if pHit is not coated or the
// projectile is already leaving. A miss still means "do not apply the hit."
bool TFPaint_ReflectOffCoated( CBaseEntity *pProj, CBaseEntity *pHit, const Vector &hitPos );

#ifdef GAME_DLL
// Enemy players last tf_paint_repulse_enemy_time. Allies, enemy buildings,
// and physics props last tf_paint_repulse_ally_time. Allied buildings are skipped.
bool TFPaint_CoatWithRepulsion( CBaseEntity *pEnt, CBaseEntity *pPainter );

// Launch a moveable physics prop off repulsion gel. nOutward points out of the
// surface. preVelocity is the incoming speed (zero uses the object's current
// velocity). bForce skips the world-gel test for a coated prop or coated body.
bool TFPaint_BouncePhysicsObject( CBaseEntity *pProp, const Vector &contact, const Vector &nOutward, const Vector &preVelocity, bool bForce = false );
#endif

#ifdef GAME_DLL
// HL2DM player linear impact table, interpolated by speed squared. Min 150 ups.
// Damages enemies of pAttacker. Cooldown is per mover and victim.
void TFPaint_SmashEnemies( CBaseEntity *pMover, const Vector &origin, const Vector &velocity, float radius, CBaseEntity *pAttacker );
#endif

#endif // TF_PAINT_H
