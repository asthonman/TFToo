//========= Copyright Valve Corporation, All rights reserved. ============//
// Party and matchmaking queue are unused. These stubs keep the existing
// callers linking and report that the local player is never queued.

#include "cbase.h"
#include "tf_partyclient.h"
#include "tf_gc_client.h"
#include "tf_matchmaking_shared.h"

extern CTFPartyClient *g_pTFPartyClient;

static void OnPartyClientPrefConVarChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	NoteUnused( pConVar );
	NoteUnused( pOldString );
	NoteUnused( flOldValue );

	CTFPartyClient *pPartyClient = GTFPartyClient();
	if ( pPartyClient )
	{
		pPartyClient->ReadConVarPreferences();
	}
}

ConVar tf_mm_custom_ping_enabled( "tf_mm_custom_ping_enabled", "0", FCVAR_ARCHIVE,
                                  "Unused. Matchmaking is not available.",
                                  false, 0.f, false, 0.f, OnPartyClientPrefConVarChanged );
ConVar tf_mm_custom_ping( "tf_mm_custom_ping", "100", FCVAR_ARCHIVE,
                          "Unused. Matchmaking is not available.",
                          true, (float)CUSTOM_PING_TOLERANCE_MIN, true, (float)CUSTOM_PING_TOLERANCE_MAX,
                          OnPartyClientPrefConVarChanged );
ConVar tf_party_join_request_mode( "tf_party_join_request_mode", "-1", FCVAR_ARCHIVE,
                                   "Unused. Matchmaking is not available.",
                                   true, -1.f,
                                   true, (float)CTFPartyClient::k_ePartyJoinRequestMode_Max,
                                   OnPartyClientPrefConVarChanged );
ConVar tf_party_ignore_invites( "tf_party_ignore_invites", "1", FCVAR_ARCHIVE,
                                "Unused. Matchmaking is not available.",
                                OnPartyClientPrefConVarChanged );
ConVar tf_party_keep_on_same_team( "tf_party_keep_on_same_team", "0", FCVAR_ARCHIVE );

CTFPartyClient::CTFPartyClient()
{
	g_pTFPartyClient = this;
}

CTFPartyClient::~CTFPartyClient()
{
}

bool CTFPartyClient::Init()
{
	m_bInit = true;
	ReadConVarPreferences();
	return true;
}

void CTFPartyClient::Think()
{
}

int CTFPartyClient::GetNumPartyMembers() const
{
	return 1;
}

CTFPartyClient::MemberStatus_t CTFPartyClient::GetPartyMemberStatus( int idx ) const
{
	NoteUnused( idx );
	return MemberStatus_t{ true, false };
}

int CTFPartyClient::CountNumOnlinePartyMembers() const
{
	return 1;
}

CSteamID CTFPartyClient::GetPartyMember( int i ) const
{
	NoteUnused( i );
	return CSteamID();
}

int CTFPartyClient::GetNumPendingMembers() const
{
	return 0;
}

int CTFPartyClient::GetMaxPartyMembers() const
{
	return MAX_PARTY_SIZE;
}

void CTFPartyClient::LeaveActiveParty()
{
}

void CTFPartyClient::SetLocalUIState( ETFSyncedMMMenuStep eMenuStep, ETFMatchGroup eMatchGroup )
{
	m_localUIState.set_menu_step( eMenuStep );
	m_localUIState.set_match_group( eMatchGroup );
}

const TFSyncedMMUIState &CTFPartyClient::GetLeaderUIState() const
{
	return m_localUIState;
}

CTFPartyClient::EPartyJoinRequestMode CTFPartyClient::GetPartyJoinRequestMode() const
{
	return m_ePrefPartyJoinRequestMode;
}

void CTFPartyClient::SetPartyJoinRequestMode( EPartyJoinRequestMode eMode )
{
	if ( m_ePrefPartyJoinRequestMode == eMode )
		return;

	m_ePrefPartyJoinRequestMode = eMode;
	tf_party_join_request_mode.SetValue( (int)eMode );
}

bool CTFPartyClient::GetIgnorePartyInvites() const
{
	return true;
}

void CTFPartyClient::SetIgnorePartyInvites( bool bIgnore )
{
	m_bPrefIgnoreInvites = bIgnore;
	if ( tf_party_ignore_invites.GetBool() != bIgnore )
	{
		tf_party_ignore_invites.SetValue( bIgnore );
	}
}

void CTFPartyClient::ReadConVarPreferences()
{
	if ( !m_bInit )
		return;

	bool bEnabled = tf_mm_custom_ping_enabled.GetBool();
	uint32 unValue = bEnabled ? (uint32)Max( 0, tf_mm_custom_ping.GetInt() ) : 0u;
	m_localGroupCriteria.SetCustomPingTolerance( unValue );

	int nRawJoinRequestMode = tf_party_join_request_mode.GetInt();
	if ( nRawJoinRequestMode < (int)k_ePartyJoinRequestMode_Min || nRawJoinRequestMode > (int)k_ePartyJoinRequestMode_Max )
	{
		m_ePrefPartyJoinRequestMode = k_ePartyJoinRequestMode_Current_Default;
	}
	else
	{
		m_ePrefPartyJoinRequestMode = (EPartyJoinRequestMode)nRawJoinRequestMode;
	}

	m_bPrefIgnoreInvites = true;
}

bool CTFPartyClient::BCanInvitePlayer( CSteamID steamID )
{
	NoteUnused( steamID );
	return false;
}

bool CTFPartyClient::BInvitePlayerToParty( CSteamID steamID, bool bExpectingExistingRequestToJoin )
{
	NoteUnused( steamID );
	NoteUnused( bExpectingExistingRequestToJoin );
	return false;
}

void CTFPartyClient::CancelOutgoingInviteOrIncomingJoinRequest( CSteamID steamIDTarget )
{
	NoteUnused( steamIDTarget );
}

void CTFPartyClient::CancelOutgoingJoinRequestOrIncomingInvite( CSteamID steamIDTarget )
{
	NoteUnused( steamIDTarget );
}

bool CTFPartyClient::BPromoteToLeader( CSteamID steamID )
{
	NoteUnused( steamID );
	return false;
}

bool CTFPartyClient::BCanRequestToJoinPlayer( CSteamID steamID )
{
	NoteUnused( steamID );
	return false;
}

bool CTFPartyClient::BKickPartyMember( CSteamID steamID )
{
	NoteUnused( steamID );
	return false;
}

bool CTFPartyClient::BRequestJoinPlayer( CSteamID steamID, bool bExpectingExistingInvite )
{
	NoteUnused( steamID );
	NoteUnused( bExpectingExistingInvite );
	return false;
}

void CTFPartyClient::SendPartyChat( const char *pszMessage )
{
	NoteUnused( pszMessage );
}

bool CTFPartyClient::BInQueueForMatchGroup( ETFMatchGroup eMatchGroup ) const
{
	NoteUnused( eMatchGroup );
	return false;
}

int CTFPartyClient::GetNumQueuedMatchGroups() const
{
	return 0;
}

ETFMatchGroup CTFPartyClient::GetQueuedMatchGroupByIdx( int idx ) const
{
	NoteUnused( idx );
	return k_eTFMatchGroup_Invalid;
}

bool CTFPartyClient::BInStandbyQueue() const
{
	return false;
}

bool CTFPartyClient::BCanQueueForStandby() const
{
	return false;
}

bool CTFPartyClient::BCurrentMatchOrInviteDisallowsQueuing( bool bCheckParty ) const
{
	NoteUnused( bCheckParty );
	return true;
}

bool CTFPartyClient::BCanQueueForMatch( ETFMatchGroup eGroup, CUtlVector< QueueEligibilityData_t > &vecReasons ) const
{
	NoteUnused( eGroup );
	QueueEligibilityData_t reason;
	reason.m_eReason = k_eDisabledType_System;
	vecReasons.AddToTail( reason );
	return false;
}

void CTFPartyClient::RequestQueueForMatch( ETFMatchGroup eMatchGroup )
{
	NoteUnused( eMatchGroup );
}

void CTFPartyClient::RequestQueueForStandby()
{
}

void CTFPartyClient::CancelMatchQueueRequest( ETFMatchGroup eMatchGroup )
{
	NoteUnused( eMatchGroup );
}

void CTFPartyClient::CancelStandbyQueueRequest()
{
}

const CTFGroupMatchCriteria &CTFPartyClient::GetEffectiveGroupCriteria() const
{
	return m_localGroupCriteria;
}

ConstRefTFGroupMatchCriteria CTFPartyClient::GetLastConfirmedGroupCriteria() const
{
	return ConstRefTFGroupMatchCriteria( m_localGroupCriteria );
}

CTFPerPlayerMatchCriteria &CTFPartyClient::MutLocalPlayerCriteria()
{
	return m_localPerPlayerCriteria;
}

CTFGroupMatchCriteria &CTFPartyClient::MutLocalGroupCriteria()
{
	return m_localGroupCriteria;
}

void CTFPartyClient::SaveCasualCriteria()
{
}

void CTFPartyClient::LoadSavedCasualCriteria()
{
}

int CTFPartyClient::GetNumOutgoingInvites() const
{
	return 0;
}

CSteamID CTFPartyClient::GetOutgoingInvite( int i ) const
{
	NoteUnused( i );
	return CSteamID();
}

bool CTFPartyClient::BHaveOutgoingInviteForSteamID( CSteamID steamID ) const
{
	NoteUnused( steamID );
	return false;
}

int CTFPartyClient::GetNumIncomingJoinRequests() const
{
	return 0;
}

CSteamID CTFPartyClient::GetIncomingJoinRequest( int i ) const
{
	NoteUnused( i );
	return CSteamID();
}

bool CTFPartyClient::BHaveIncomingJoinRequestForSteamID( CSteamID steamID ) const
{
	NoteUnused( steamID );
	return false;
}

int CTFPartyClient::GetNumIncomingInvites() const
{
	return 0;
}

CTFPartyInvite *CTFPartyClient::GetIncomingInvite( int i ) const
{
	NoteUnused( i );
	return NULL;
}

bool CTFPartyClient::BHaveIncomingInviteForSteamID( CSteamID steamID ) const
{
	NoteUnused( steamID );
	return false;
}

int CTFPartyClient::GetNumOutgoingJoinRequests() const
{
	return 0;
}

CTFPartyInvite *CTFPartyClient::GetOutgoingJoinRequest( int i ) const
{
	NoteUnused( i );
	return NULL;
}

bool CTFPartyClient::BHaveOutgoingJoinRequestForSteamID( CSteamID steamID ) const
{
	NoteUnused( steamID );
	return false;
}

void CTFPartyClient::SOCacheSubscribed( const CSteamID &steamIDOwner, GCSDK::ESOCacheEvent eEvent )
{
	NoteUnused( steamIDOwner );
	NoteUnused( eEvent );
}

void CTFPartyClient::SOCreated( const CSteamID &steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	NoteUnused( steamIDOwner );
	NoteUnused( pObject );
	NoteUnused( eEvent );
}

void CTFPartyClient::SOUpdated( const CSteamID &steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	NoteUnused( steamIDOwner );
	NoteUnused( pObject );
	NoteUnused( eEvent );
}

void CTFPartyClient::SODestroyed( const CSteamID &steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	NoteUnused( steamIDOwner );
	NoteUnused( pObject );
	NoteUnused( eEvent );
}

void CTFPartyClient::SOCacheUnsubscribed( const CSteamID &steamIDOwner, GCSDK::ESOCacheEvent eEvent )
{
	NoteUnused( steamIDOwner );
	NoteUnused( eEvent );
	m_pSOCache = NULL;
	m_pActiveParty = NULL;
	m_unActivePartyID = 0u;
}
