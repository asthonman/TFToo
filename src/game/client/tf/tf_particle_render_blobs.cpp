//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF2's particles library is built without USE_BLOBULATOR, so render_blobs
// was never registered. This installs that renderer. It draws the particle
// material as sprites (SpriteCard, including particle/paintblobs/*.vmt).
// It does not use Portal 2's blob_surface shader.
//
//===========================================================================//
#include "cbase.h"
#include "particles/particles.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "tier1/strtools.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar r_render_blobs( "r_render_blobs", "1", FCVAR_ARCHIVE, "Draw particles that use the render_blobs renderer. Off in the stock TF2 particle library." );

class C_OP_RenderBlobs : public CParticleRenderOperatorInstance
{
	DECLARE_PARTICLE_OPERATOR( C_OP_RenderBlobs );

public:
	float m_cubeWidth;
	float m_cutoffRadius;
	float m_renderRadius;

	uint32 GetWrittenAttributes( void ) const
	{
		return 0;
	}

	uint32 GetReadAttributes( void ) const
	{
		return PARTICLE_ATTRIBUTE_XYZ_MASK | PARTICLE_ATTRIBUTE_RADIUS_MASK |
			PARTICLE_ATTRIBUTE_TINT_RGB_MASK | PARTICLE_ATTRIBUTE_ALPHA_MASK |
			PARTICLE_ATTRIBUTE_ROTATION_MASK | PARTICLE_ATTRIBUTE_YAW_MASK |
			PARTICLE_ATTRIBUTE_SEQUENCE_NUMBER_MASK;
	}

	virtual void Render( IMatRenderContext *pRenderContext, CParticleCollection *pParticles, void *pContext ) const;

	virtual bool IsBatchable() const
	{
		return false;
	}
};

DEFINE_PARTICLE_OPERATOR( C_OP_RenderBlobs, "render_blobs", OPERATOR_SINGLETON );

BEGIN_PARTICLE_RENDER_OPERATOR_UNPACK( C_OP_RenderBlobs )
	DMXELEMENT_UNPACK_FIELD( "cube_width", "1.0f", float, m_cubeWidth )
	DMXELEMENT_UNPACK_FIELD( "cutoff_radius", "3.3f", float, m_cutoffRadius )
	DMXELEMENT_UNPACK_FIELD( "render_radius", "1.3f", float, m_renderRadius )
END_PARTICLE_OPERATOR_UNPACK( C_OP_RenderBlobs )

static unsigned char BlobByte( float fl )
{
	if ( fl <= 0.f )
		return 0;
	if ( fl >= 1.f )
		return 255;
	return (unsigned char)( fl * 255.f );
}

// paint_blob_sheet_1.vtf is a 2x2 sheet. Sequences 0-3 are those cells.
static void BlobSheetUV( int sequence, float &u0, float &v0, float &u1, float &v1 )
{
	static const float s_cell[4][4] =
	{
		{ 0.00390625f, 0.00390625f, 0.49609375f, 0.49609375f },
		{ 0.50390625f, 0.00390625f, 0.99609375f, 0.49609375f },
		{ 0.00390625f, 0.50390625f, 0.49609375f, 0.99609375f },
		{ 0.50390625f, 0.50390625f, 0.99609375f, 0.99609375f },
	};
	const int cell = sequence & 3;
	u0 = s_cell[cell][0];
	v0 = s_cell[cell][1];
	u1 = s_cell[cell][2];
	v1 = s_cell[cell][3];
}

void C_OP_RenderBlobs::Render( IMatRenderContext *pRenderContext, CParticleCollection *pParticles, void *pContext ) const
{
	(void)pContext;
	if ( !r_render_blobs.GetBool() || !pParticles || !pParticles->m_pDef )
		return;

	IMaterial *pMaterial = pParticles->m_pDef->GetMaterial();
	if ( !pMaterial || pMaterial->IsErrorMaterial() )
		return;

	CParticleVisibilityData vis;
	vis.m_flAlphaVisibility = 1.f;
	vis.m_flRadiusVisibility = 1.f;
	vis.m_flCameraBias = 0.f;
	vis.m_bUseVisibility = false;

	int nParticles = 0;
	const ParticleRenderData_t *pSortList = pParticles->GetRenderList( pRenderContext, false, &nParticles, &vis );
	if ( nParticles <= 0 || !pSortList )
		return;

	size_t xyzStride = 0, rgbStride = 0, rotStride = 0, yawStride = 0, seqStride = 0;
	const fltx4 *xyz = pParticles->GetM128AttributePtr( PARTICLE_ATTRIBUTE_XYZ, &xyzStride );
	const fltx4 *rgb = pParticles->GetM128AttributePtr( PARTICLE_ATTRIBUTE_TINT_RGB, &rgbStride );
	const fltx4 *rot = pParticles->GetM128AttributePtr( PARTICLE_ATTRIBUTE_ROTATION, &rotStride );
	const fltx4 *yaw = pParticles->GetM128AttributePtr( PARTICLE_ATTRIBUTE_YAW, &yawStride );
	const fltx4 *seq = pParticles->GetM128AttributePtr( PARTICLE_ATTRIBUTE_SEQUENCE_NUMBER, &seqStride );

	const bool bBlobSheet = V_stristr( pMaterial->GetName(), "paint_blob_sheet" ) != NULL;
	pRenderContext->Bind( pMaterial );

	if ( pMaterial->IsSpriteCard() )
	{
		const int nBatch = 64;
		while ( nParticles > 0 )
		{
			const int nDraw = MIN( nParticles, nBatch );
			nParticles -= nDraw;

			IMesh *pMesh = pRenderContext->GetDynamicMesh( true );
			CMeshBuilder mb;
			mb.Begin( pMesh, MATERIAL_TRIANGLES, nDraw * 4, nDraw * 6 );
			int vertex = 0;
			const bool bSecond = mb.TextureCoordinateSize( 4 ) > 0;

			for ( int i = 0; i < nDraw; ++i )
			{
				const ParticleRenderData_t *pRender = --pSortList;
				const int hParticle = pRender->m_nIndex;
				const int nGroup = hParticle / 4;
				const int nOffset = hParticle & 3;

				const int xyzIndex = nGroup * (int)xyzStride;
				const float x = SubFloat( xyz[xyzIndex], nOffset );
				const float y = SubFloat( xyz[xyzIndex + 1], nOffset );
				const float z = SubFloat( xyz[xyzIndex + 2], nOffset );

				const int rgbIndex = nGroup * (int)rgbStride;
				const unsigned char rc = BlobByte( SubFloat( rgb[rgbIndex], nOffset ) );
				const unsigned char gc = BlobByte( SubFloat( rgb[rgbIndex + 1], nOffset ) );
				const unsigned char bc = BlobByte( SubFloat( rgb[rgbIndex + 2], nOffset ) );
				const unsigned char ac = pRender->m_nAlpha;

				float rad = pRender->m_flRadius;
				if ( !( rad > 0.5f ) )
					rad = m_renderRadius > 0.f ? m_renderRadius : 8.f;

				const float flRot = SubFloat( rot[nGroup * (int)rotStride], nOffset );
				const float flYaw = SubFloat( yaw[nGroup * (int)yawStride], nOffset );
				const int sequence = (int)SubFloat( seq[nGroup * (int)seqStride], nOffset );

				float u0 = 0.f, v0 = 0.f, u1 = 1.f, v1 = 1.f;
				if ( bBlobSheet )
					BlobSheetUV( sequence, u0, v0, u1, v1 );

				static const float s_corner[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
				for ( int c = 0; c < 4; ++c )
				{
					mb.Position3f( x, y, z );
					mb.Color4ub( rc, gc, bc, ac );
					mb.TexCoord4f( 0, u0, v0, u1, v1 );
					mb.TexCoord4f( 1, 0.f, 0.f, 0.f, 0.f );
					mb.TexCoord4f( 2, 0.f, flRot, rad, flYaw );
					if ( mb.TextureCoordinateSize( 3 ) > 0 )
						mb.TexCoord2f( 3, s_corner[c][0], s_corner[c][1] );
					if ( bSecond )
						mb.TexCoord4f( 4, 0.f, 0.f, 0.f, 0.f );
					mb.AdvanceVertex();
				}
				mb.FastIndex( vertex );
				mb.FastIndex( vertex + 1 );
				mb.FastIndex( vertex + 2 );
				mb.FastIndex( vertex );
				mb.FastIndex( vertex + 2 );
				mb.FastIndex( vertex + 3 );
				vertex += 4;
			}

			mb.End();
			pMesh->Draw();
			g_pParticleSystemMgr->TallyParticlesRendered( nDraw * 4, nDraw * 6 );
		}
		return;
	}

	Vector camForward, camRight, camUp;
	pRenderContext->GetWorldSpaceCameraVectors( &camForward, &camRight, &camUp );
	(void)camForward;

	IMesh *pMesh = pRenderContext->GetDynamicMesh( true );
	CMeshBuilder mb;
	mb.Begin( pMesh, MATERIAL_TRIANGLES, nParticles * 2 );
	for ( int i = 0; i < nParticles; ++i )
	{
		const ParticleRenderData_t *pRender = --pSortList;
		const int hParticle = pRender->m_nIndex;
		const int nGroup = hParticle / 4;
		const int nOffset = hParticle & 3;
		const int xyzIndex = nGroup * (int)xyzStride;
		Vector c( SubFloat( xyz[xyzIndex], nOffset ), SubFloat( xyz[xyzIndex + 1], nOffset ), SubFloat( xyz[xyzIndex + 2], nOffset ) );
		float rad = pRender->m_flRadius;
		if ( !( rad > 0.5f ) )
			rad = m_renderRadius > 0.f ? m_renderRadius : 8.f;

		const int rgbIndex = nGroup * (int)rgbStride;
		const unsigned char rc = BlobByte( SubFloat( rgb[rgbIndex], nOffset ) );
		const unsigned char gc = BlobByte( SubFloat( rgb[rgbIndex + 1], nOffset ) );
		const unsigned char bc = BlobByte( SubFloat( rgb[rgbIndex + 2], nOffset ) );
		const unsigned char ac = pRender->m_nAlpha;

		Vector p0 = c - camRight * rad - camUp * rad;
		Vector p1 = c + camRight * rad - camUp * rad;
		Vector p2 = c + camRight * rad + camUp * rad;
		Vector p3 = c - camRight * rad + camUp * rad;

		mb.Position3fv( p0.Base() ); mb.Color4ub( rc, gc, bc, ac ); mb.TexCoord2f( 0, 0, 1 ); mb.AdvanceVertex();
		mb.Position3fv( p1.Base() ); mb.Color4ub( rc, gc, bc, ac ); mb.TexCoord2f( 0, 1, 1 ); mb.AdvanceVertex();
		mb.Position3fv( p2.Base() ); mb.Color4ub( rc, gc, bc, ac ); mb.TexCoord2f( 0, 1, 0 ); mb.AdvanceVertex();
		mb.Position3fv( p0.Base() ); mb.Color4ub( rc, gc, bc, ac ); mb.TexCoord2f( 0, 0, 1 ); mb.AdvanceVertex();
		mb.Position3fv( p2.Base() ); mb.Color4ub( rc, gc, bc, ac ); mb.TexCoord2f( 0, 1, 0 ); mb.AdvanceVertex();
		mb.Position3fv( p3.Base() ); mb.Color4ub( rc, gc, bc, ac ); mb.TexCoord2f( 0, 0, 0 ); mb.AdvanceVertex();
	}
	mb.End();
	pMesh->Draw();
}

void AddTFBlobRenderer( void )
{
	static bool s_bAdded = false;
	if ( s_bAdded )
		return;
	s_bAdded = true;
	REGISTER_PARTICLE_OPERATOR( FUNCTION_RENDERER, C_OP_RenderBlobs );
}
