//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseprojectile.h"
#include "basecombatweapon_shared.h"
#include "collisionutils.h"

#ifdef GAME_DLL
	#include "iscorer.h"
	#include "physics_shared.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( BaseProjectile, DT_BaseProjectile )

BEGIN_NETWORK_TABLE( CBaseProjectile, DT_BaseProjectile )
#if !defined( CLIENT_DLL )
	SendPropEHandle( SENDINFO( m_hOriginalLauncher ) ),
#else
	RecvPropEHandle( RECVINFO( m_hOriginalLauncher ) ),
#endif // CLIENT_DLL
END_NETWORK_TABLE()


#ifndef CLIENT_DLL
IMPLEMENT_AUTO_LIST( IBaseProjectileAutoList );
#endif // !CLIENT_DLL


#ifdef TF_DLL
CBaseEntity* GetAttackerEntity( CBaseProjectile* pProjectile )
{
	CBaseEntity *pAttacker = pProjectile->GetOriginalLauncher();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
	if ( pScorerInterface )
	{
		pAttacker = pScorerInterface->GetScorer();
	}
	else if ( pAttacker && pAttacker->GetOwnerEntity() )
	{
		pAttacker = pAttacker->GetOwnerEntity();
	}

	return pAttacker;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CBaseProjectile::CBaseProjectile()
{
#ifdef GAME_DLL
	m_iDestroyableHitCount = 0;
	m_bCanCollideWithTeammates = false;
#endif
	m_hOriginalLauncher = NULL;

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseProjectile::~CBaseProjectile()
{
#ifdef TF_DLL
	IGameEvent *event = gameeventmanager->CreateEvent( "projectile_removed" );
	if ( event )
	{
		item_definition_index_t ownerWeaponDefIndex = INVALID_ITEM_DEF_INDEX;

		CBaseCombatWeapon *pWeapon = dynamic_cast< CBaseCombatWeapon * >( GetOriginalLauncher() );
		if ( pWeapon )
		{
			ownerWeaponDefIndex = pWeapon->GetAttributeContainer()->GetItem()->GetItemDefIndex();
		}

		CBaseEntity *pAttacker = GetAttackerEntity( this );

		if ( !pAttacker || ownerWeaponDefIndex == INVALID_ITEM_DEF_INDEX )
		{
			delete event;
			return;
		}

		event->SetInt( "attacker", pAttacker->entindex() );
		event->SetInt( "weapon_def_index", ownerWeaponDefIndex );
		event->SetInt( "num_hit", m_vecEntsHit.Count() );
		event->SetInt( "num_direct_hit", m_vecEntsDirectHit.Count() );

		gameeventmanager->FireEvent( event, true );
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProjectile::SetLauncher( CBaseEntity *pLauncher )
{
	if ( m_hOriginalLauncher == NULL )
	{
		m_hOriginalLauncher = pLauncher;
	}

#ifdef GAME_DLL
	ResetCollideWithTeammates();
#endif // GAME_DLL
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProjectile::Spawn()
{
	BaseClass::Spawn();

#ifdef GAME_DLL
	ResetCollideWithTeammates();
#endif // GAME_DLL
}


#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// A model's bounding box is not collision. Brushes, players and a real
// collision mesh are. Rockets already trace that way; this is the same test.
//-----------------------------------------------------------------------------
static bool ProjectileShouldCollideWith( CBaseEntity *pOther, const Vector &vecStart, const Vector &vecEnd )
{
	if ( !pOther )
		return false;

	if ( pOther->IsWorld() || pOther->GetSolid() == SOLID_BSP )
		return true;

	if ( pOther->IsPlayer() || pOther->IsBaseObject() || pOther->IsCombatCharacter() || pOther->IsCombatItem() || pOther->IsProjectileCollisionTarget() )
		return true;

	vcollide_t *pCollide = modelinfo->GetVCollide( pOther->GetModelIndex() );
	if ( !pCollide || pCollide->solidCount <= 0 || !pCollide->solids )
		return false;

	for ( int i = 0; i < pCollide->solidCount; ++i )
	{
		if ( !pCollide->solids[i] )
			continue;

		trace_t tr;
		physcollision->TraceBox( vecStart, vecEnd, vec3_origin, vec3_origin, pCollide->solids[i], pOther->GetAbsOrigin(), pOther->GetAbsAngles(), &tr );
		if ( tr.DidHit() )
			return true;
	}

	return false;
}

class CTraceFilterProjectileRealCollision : public CTraceFilterSimple
{
public:
	CTraceFilterProjectileRealCollision( CBaseEntity *pPass, int collisionGroup, const Vector &start, const Vector &end )
		: CTraceFilterSimple( pPass, collisionGroup ), m_vecStart( start ), m_vecEnd( end )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandle, int contentsMask ) OVERRIDE
	{
		if ( !CTraceFilterSimple::ShouldHitEntity( pHandle, contentsMask ) )
			return false;

		CBaseEntity *pEnt = EntityFromEntityHandle( pHandle );
		// Static props have no CBaseEntity. The engine trace already uses their mesh.
		if ( !pEnt )
			return true;

		return ProjectileShouldCollideWith( pEnt, m_vecStart, m_vecEnd );
	}

private:
	Vector m_vecStart;
	Vector m_vecEnd;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProjectile::CollideWithTeammatesThink()
{
	m_bCanCollideWithTeammates = true;
}

//-----------------------------------------------------------------------------
// Purpose: Lots of stuff uses this, so centralize here
//-----------------------------------------------------------------------------
bool CBaseProjectile::ShouldTouchNonWorldSolid( CBaseEntity *pOther, const trace_t *pTrace )
{
	if ( !pOther || !pTrace )
		return false;

	Vector vecStart = pTrace->startpos;
	Vector vecEnd = pTrace->endpos;
	if ( vecStart.DistToSqr( vecEnd ) < 1.0f )
	{
		Vector vecDir = GetAbsVelocity();
		if ( vecDir.LengthSqr() < 1.0f )
			vecDir = -pTrace->plane.normal;
		if ( vecDir.LengthSqr() < 0.01f )
			vecDir = Vector( 1.0f, 0.0f, 0.0f );
		VectorNormalize( vecDir );
		vecStart -= vecDir * 8.0f;
		vecEnd = vecStart + vecDir * 48.0f;
	}

	return ProjectileShouldCollideWith( pOther, vecStart, vecEnd );
}

//-----------------------------------------------------------------------------
// The sweep stopped on a bounding box. Finish the move, ignoring every model
// that doesn't have a collision mesh in the way, and stop on real geometry.
//-----------------------------------------------------------------------------
void CBaseProjectile::FlyThroughBoundingBoxHit( const trace_t &trace, const Vector &vecMove )
{
	Vector vecDir = vecMove;
	float flMove = VectorNormalize( vecDir );
	if ( flMove < 0.01f )
	{
		vecDir = GetAbsVelocity() * gpGlobals->frametime;
		flMove = VectorNormalize( vecDir );
	}
	if ( flMove < 0.01f )
		return;

	Vector vecEnd = trace.startpos + vecDir * flMove;
	CTraceFilterProjectileRealCollision filter( this, GetCollisionGroup(), trace.startpos, vecEnd );
	unsigned int nMask = PhysicsSolidMaskForEntity();

	Vector vecStart = GetAbsOrigin();
	trace_t tr;
	UTIL_TraceHull( vecStart, vecEnd, WorldAlignMins(), WorldAlignMaxs(), nMask, &filter, &tr );

	if ( tr.startsolid && trace.m_pEnt )
	{
		Vector vecMins, vecMaxs;
		trace.m_pEnt->CollisionProp()->WorldSpaceAABB( &vecMins, &vecMaxs );
		float flPad = ( WorldAlignMaxs() - WorldAlignMins() ).Length() + 1.0f;
		vecMins -= Vector( flPad, flPad, flPad );
		vecMaxs += Vector( flPad, flPad, flPad );

		float flLeftSolid = 1.0f;
		CBaseTrace boxTrace;
		Vector vecDelta = vecDir * ( ( vecMaxs - vecMins ).Length() + flMove );
		IntersectRayWithBox( GetAbsOrigin(), vecDelta, vecMins, vecMaxs, 0.0f, &boxTrace, &flLeftSolid );
		vecStart = GetAbsOrigin() + vecDelta * flLeftSolid + vecDir;
		vecEnd = vecStart + vecDir * flMove;
		UTIL_TraceHull( vecStart, vecEnd, WorldAlignMins(), WorldAlignMaxs(), nMask, &filter, &tr );
	}

	if ( !tr.startsolid )
		SetAbsOrigin( tr.endpos );
}

//-----------------------------------------------------------------------------
// Purpose: Don't park on a bounding box. Real hits keep the old fly resolution
// (rockets explode from their touch, and never get here still alive).
//-----------------------------------------------------------------------------
void CBaseProjectile::ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity )
{
	if ( GetModelName() != NULL_STRING && trace.m_pEnt && !ShouldTouchNonWorldSolid( trace.m_pEnt, &trace ) )
	{
		FlyThroughBoundingBoxHit( trace, vecVelocity );
		return;
	}

	BaseClass::ResolveFlyCollisionCustom( trace, vecVelocity );
}

extern float g_flServerCurTime;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProjectile::ResetCollideWithTeammates()
{
	// Don't collide with players on the owner's team for the first bit of our life
	m_bCanCollideWithTeammates = false;

	// Josh: This now uses a new g_flServerCurTime which is not affected by rollback, etc
	// as Thinks like this should be in server time space, not client rollback time space.
	// This fixes a bug where clients with higher ping would get shorter teammate collisions,
	// as it was thinking for less time, and low ping local players would end up getting longer collisions,
	// and due to the way rollback is implemented in Source (can do roll-forward), weapons like the
	// Crucader's Crossbow with a 0 delay, would end up having some, due to the client-local curtime,
	// being above the server's curtime.
	// It is worth noting that rollback does not affect position/velocity of the spawned projectile (only hitscan).
	// There is also a special case for the 0 delay, because we don't need to think in that instance anyway!
	// ( but it was tested with that too :> )
	if ( GetCollideWithTeammatesDelay() == 0.0f )
		m_bCanCollideWithTeammates = true;
	else
		SetContextThink( &CBaseProjectile::CollideWithTeammatesThink, g_flServerCurTime + GetCollideWithTeammatesDelay(), "CollideWithTeammates" );
}

#endif // GAME_DLL


#ifdef TF_DLL
//-----------------------------------------------------------------------------
// Purpose: Fire an event that we hit someone, and tally up how many we've hit
//			so we can fire another event when we're deleted that says how many
//			we've hit in our life
//-----------------------------------------------------------------------------
void CBaseProjectile::RecordEnemyPlayerHit( const CBaseEntity* pHitPlayer, bool bDirect )
{
	Assert( pHitPlayer->IsPlayer() );
	if ( pHitPlayer->GetTeamNumber() == GetTeamNumber() )
		return;

	// Record another hit
	if ( m_vecEntsHit.Find( pHitPlayer->entindex() ) == m_vecEntsHit.InvalidIndex() )
	{
		m_vecEntsHit.AddToTail( pHitPlayer->entindex() );
	}

	if ( bDirect )
	{
		// Record another direct hit
		if ( m_vecEntsDirectHit.Find( pHitPlayer->entindex() ) == m_vecEntsHit.InvalidIndex() )
		{
			m_vecEntsDirectHit.AddToTail( pHitPlayer->entindex() );
		}

		// Fire an event about us direct hitting
		IGameEvent *event = gameeventmanager->CreateEvent( "projectile_direct_hit" );
		if ( event )
		{
			item_definition_index_t ownerWeaponDefIndex = INVALID_ITEM_DEF_INDEX;
			CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon *>(GetOriginalLauncher());
			if (pWeapon)
			{
				ownerWeaponDefIndex = pWeapon->GetAttributeContainer()->GetItem()->GetItemDefIndex();
			}

			CBaseEntity *pAttacker = GetAttackerEntity( this );

			if ( !pAttacker )
			{
				delete event;
				return;
			}

			event->SetInt( "attacker", pAttacker->entindex() );
			event->SetInt( "victim", pHitPlayer->entindex() );
			event->SetInt( "weapon_def_index", ownerWeaponDefIndex );

			gameeventmanager->FireEvent( event, true );
		}
	}
}
#endif // TF_DLL

