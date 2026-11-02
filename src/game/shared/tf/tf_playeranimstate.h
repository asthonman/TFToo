//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef TF_PLAYERANIMSTATE_H
#define TF_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "../Multiplayer/multiplayer_animstate.h"

#if defined( CLIENT_DLL )
class C_TFPlayer;
#define CTFPlayer C_TFPlayer
#else
class CTFPlayer;
#endif

// ------------------------------------------------------------------------------------------------ //
// CPlayerAnimState declaration.
// ------------------------------------------------------------------------------------------------ //
class CTFPlayerAnimState : public CMultiPlayerAnimState
{
public:
	
	DECLARE_CLASS( CTFPlayerAnimState, CMultiPlayerAnimState );

	CTFPlayerAnimState();
	CTFPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData );
	~CTFPlayerAnimState();

	void InitTF( CTFPlayer *pPlayer );
	CTFPlayer *GetTFPlayer( void )							{ return m_pTFPlayer; }

	virtual void ClearAnimationState();
	virtual Activity TranslateActivity( Activity actDesired );
	Activity ActivityOverride( Activity baseAct, bool *pRequired );
	virtual void Update( float eyeYaw, float eyePitch );

	void	DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	virtual void CheckStunAnimation();
	virtual Activity CalcMainActivity();
	virtual void ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr );

	void CheckPasstimeThrowAnimation();
	void CheckCYOAPDAAnimtion();
	void HoldBrickSlam();

	virtual float GetCurrentMaxGroundSpeed();
	virtual float GetGesturePlaybackRate( void );

	bool	HandleMoving( Activity &idealActivity );
	bool	HandleJumping( Activity &idealActivity );
	bool	HandleDucking( Activity &idealActivity );
	bool	HandleSwimming( Activity &idealActivity );

	virtual bool ShouldUpdateAnimState();

	virtual void GetOuterAbsVelocity( Vector& vel );

	bool	IsItemTestingBot( void );

	virtual void RestartGesture( int iGestureSlot, Activity iGestureActivity, bool bAutoKill = true );

	void	SetRenderangles( const QAngle& angles ) { m_angRender = angles; }

	void	Vehicle_LeanAccel( float flInAccel );
	// Cached body_pitch / body_yaw indices are latched once. If that lookup
	// ran before the studiohdr was ready, the server stores 0 and the torso
	// never leaves the reference pose. Re-resolve by name after the feet yaw.
	void	ApplyAimPoseParameters( CStudioHdr *pStudioHdr );
	// Bone setup uses its own copy of the pose array. Fill that copy too,
	// so a latched zero pose cannot win over the eyes.
	void	StampAimPoseParameters( CStudioHdr *pStudioHdr, float *pPoseParameters, int nPoseParameters );
private:
	void Taunt_ComputePoseParam_MoveX( CStudioHdr *pStudioHdr );
	void Taunt_ComputePoseParam_MoveY( CStudioHdr *pStudioHdr );
	void Vehicle_ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr );
	void Vehicle_ComputePoseParam_AccelLean( CStudioHdr *pStudioHdr );
	
	CTFPlayer   *m_pTFPlayer;
	bool		m_bInAirWalk;
	float		m_flHoldDeployedPoseUntilTime;
	float		m_flTauntMoveX;
	float		m_flTauntMoveY;
	float		m_flVehicleLeanVel;
	float		m_flVehicleLeanPos;
	Vector		m_vecSmoothedUp;

	typedef std::pair< int, float > CachedPoseParam_t;
	CUtlVector< CachedPoseParam_t > m_PlayerPoseParams;
};

CTFPlayerAnimState *CreateTFPlayerAnimState( CTFPlayer *pPlayer );

#endif // TF_PLAYERANIMSTATE_H
