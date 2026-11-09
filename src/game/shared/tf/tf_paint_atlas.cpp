//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Paint atlas + gel shader.
//
// The atlas is the paintmap: one chart per plane, texels the size of a
// lightmap luxel (default 16, the scale Portal 2 gel was authored for).
// Last gel wins inside a splat. Erase punches the chart back to empty.
// Alpha is solid in the interior and falls off across one luxel, which is
// the splatter threshold from the talk.
//
// The shader (TFPaintShader_Eval) is the surface-embedded sprite from
// Bronwen Grimes, GDC 2011:
//   1. Tile a layout of UV islands over the surface.
//   2. Build a 2x2 matrix in tangent space from the camera side and the
//      in-plane view forward. Orthonormal, truncated so it stays a 2x2.
//   3. Islands are centered at 0, then the matrix spins the interior so
//      the bubble faces the camera. A porthole clip keeps the island edge.
//   4. Two bubble layers (the talk doubled that cost on purpose).
//   5. Fade the coat at grazing angles.
//   6. Clip before the bubble work when coverage is under the threshold.
// Reflections are a fresnel added in the same function. TF2 cannot bind
// this as a real paintmap sampler, so the result is vertex color on
// tftoo/paint/gel.
//
//=============================================================================
#include "cbase.h"
#include "tf_paint_atlas.h"

#ifdef CLIENT_DLL
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#endif

#include "tier0/memdbgon.h"

ConVar tf_paint_luxel( "tf_paint_luxel", "16", FCVAR_REPLICATED,
	"Gel atlas texel size, in units. Portal 2's paintmap uses lightmap scale; 16 is the gel scale." );

#ifdef CLIENT_DLL
ConVar tf_paint_draw_dist( "tf_paint_draw_dist", "1800", FCVAR_CLIENTDLL,
	"Gel coats farther than this are not drawn. The atlas still affects movement." );
#endif

struct TFPaintPage_t
{
	Vector normal;
	Vector tangent;
	Vector bitangent;
	float planeDist;
	float luxel;
	int u0, v0;
	int w, h;
	CUtlVector<unsigned char> tex; // [type, alpha] per luxel

	TFPaintPage_t()
	{
		Reset();
	}

	void Reset()
	{
		normal.Init();
		tangent.Init();
		bitangent.Init();
		planeDist = 0.f;
		luxel = 16.f;
		u0 = v0 = w = h = 0;
		tex.Purge();
	}
};

CTFPaintAtlas::CTFPaintAtlas()
	: m_pPages( NULL )
	, m_nPages( 0 )
{
}

CTFPaintAtlas::~CTFPaintAtlas()
{
	delete[] m_pPages;
	m_pPages = NULL;
	m_nPages = 0;
}

void CTFPaintAtlas::Ensure()
{
	if ( m_pPages )
		return;
	m_pPages = new TFPaintPage_t[PAGE_MAX];
	m_nPages = 0;
}

void CTFPaintAtlas::Clear()
{
	if ( !m_pPages )
		return;
	for ( int i = 0; i < m_nPages; ++i )
		m_pPages[i].Reset();
	m_nPages = 0;
}

static void BasisFromNormal( const Vector &normal, Vector &tangent, Vector &bitangent )
{
	Vector up( 0.f, 0.f, 1.f );
	if ( fabsf( normal.z ) > 0.9f )
		up.Init( 0.f, 1.f, 0.f );
	tangent = CrossProduct( up, normal );
	VectorNormalize( tangent );
	bitangent = CrossProduct( normal, tangent );
}

static float LuxelSize()
{
	float luxel = tf_paint_luxel.GetFloat();
	if ( luxel < 8.f )
		luxel = 8.f;
	if ( luxel > 64.f )
		luxel = 64.f;
	return luxel;
}

static int PaintRankAtlas( int type )
{
	switch ( type )
	{
	case BOUNCE_POWER:	return 4;
	case SPEED_POWER:	return 3;
	case PORTAL_POWER:	return 2;
	case FIFTH_POWER:	return 1;
	default:			return 0;
	}
}

static void RebuildPage( TFPaintPage_t &page, int u0, int v0, int w, int h )
{
	if ( w < 1 )
		w = 1;
	if ( h < 1 )
		h = 1;
	if ( w > 96 )
		w = 96;
	if ( h > 96 )
		h = 96;

	CUtlVector<unsigned char> neu;
	neu.SetCount( w * h * 2 );
	Q_memset( neu.Base(), 0, w * h * 2 );

	for ( int y = 0; y < page.h; ++y )
	{
		for ( int x = 0; x < page.w; ++x )
		{
			int nx = ( page.u0 + x ) - u0;
			int ny = ( page.v0 + y ) - v0;
			if ( nx < 0 || ny < 0 || nx >= w || ny >= h )
				continue;
			int si = ( y * page.w + x ) * 2;
			int di = ( ny * w + nx ) * 2;
			neu[di] = page.tex[si];
			neu[di + 1] = page.tex[si + 1];
		}
	}

	page.tex.Swap( neu );
	page.u0 = u0;
	page.v0 = v0;
	page.w = w;
	page.h = h;
}

int CTFPaintAtlas::FindOrAdd( const Vector &pos, const Vector &normal, float radius )
{
	Ensure();

	Vector n = normal;
	if ( n.LengthSqr() < 0.01f )
		n.Init( 0.f, 0.f, 1.f );
	VectorNormalize( n );

	int best = -1;
	int bestGrow = 1 << 30;
	int bestU0 = 0, bestV0 = 0, bestW = 0, bestH = 0;

	for ( int i = 0; i < m_nPages; ++i )
	{
		TFPaintPage_t &page = m_pPages[i];
		if ( DotProduct( n, page.normal ) < 0.985f )
			continue;
		if ( fabsf( DotProduct( pos, page.normal ) - page.planeDist ) > 8.f )
			continue;

		int iu = (int)floorf( DotProduct( pos, page.tangent ) / page.luxel );
		int iv = (int)floorf( DotProduct( pos, page.bitangent ) / page.luxel );
		int radT = (int)ceilf( radius / page.luxel ) + 1;
		int su0 = iu - radT;
		int sv0 = iv - radT;
		int su1 = iu + radT;
		int sv1 = iv + radT;

		int u0 = page.u0 < su0 ? page.u0 : su0;
		int v0 = page.v0 < sv0 ? page.v0 : sv0;
		int u1 = ( page.u0 + page.w - 1 ) > su1 ? ( page.u0 + page.w - 1 ) : su1;
		int v1 = ( page.v0 + page.h - 1 ) > sv1 ? ( page.v0 + page.h - 1 ) : sv1;
		int nw = u1 - u0 + 1;
		int nh = v1 - v0 + 1;
		if ( nw > PAGE_DIM_MAX || nh > PAGE_DIM_MAX )
			continue;

		int grow = nw * nh - page.w * page.h;
		if ( grow < bestGrow )
		{
			bestGrow = grow;
			best = i;
			bestU0 = u0;
			bestV0 = v0;
			bestW = nw;
			bestH = nh;
		}
	}

	if ( best >= 0 )
	{
		TFPaintPage_t &page = m_pPages[best];
		if ( bestU0 != page.u0 || bestV0 != page.v0 || bestW != page.w || bestH != page.h )
			RebuildPage( page, bestU0, bestV0, bestW, bestH );
		return best;
	}

	if ( m_nPages >= PAGE_MAX )
		return -1;

	Vector tangent, bitangent;
	BasisFromNormal( n, tangent, bitangent );

	TFPaintPage_t &page = m_pPages[m_nPages];
	page.Reset();
	page.normal = n;
	page.tangent = tangent;
	page.bitangent = bitangent;
	page.planeDist = DotProduct( pos, n );
	page.luxel = LuxelSize();

	int iu = (int)floorf( DotProduct( pos, tangent ) / page.luxel );
	int iv = (int)floorf( DotProduct( pos, bitangent ) / page.luxel );
	int radT = (int)ceilf( radius / page.luxel ) + 1;
	int w = radT * 2 + 2;
	int h = w;
	int u0 = iu - radT;
	int v0 = iv - radT;
	if ( w > PAGE_DIM_MAX )
	{
		w = h = PAGE_DIM_MAX;
		u0 = iu - PAGE_DIM_MAX / 2;
		v0 = iv - PAGE_DIM_MAX / 2;
	}
	page.u0 = u0;
	page.v0 = v0;
	page.w = w;
	page.h = h;
	page.tex.SetCount( w * h * 2 );
	Q_memset( page.tex.Base(), 0, w * h * 2 );

	++m_nPages;
	return m_nPages - 1;
}

void CTFPaintAtlas::Raster( TFPaintPage_t &page, const Vector &pos, int type, float radius, bool bErase )
{
	if ( page.w < 1 || page.h < 1 || page.tex.Count() < page.w * page.h * 2 )
		return;

	float dist = DotProduct( pos, page.normal ) - page.planeDist;
	Vector onPlane = pos - page.normal * dist;
	float tu = DotProduct( onPlane, page.tangent ) / page.luxel;
	float tv = DotProduct( onPlane, page.bitangent ) / page.luxel;

	int radT = (int)ceilf( radius / page.luxel ) + 1;
	int x0 = (int)floorf( tu ) - page.u0 - radT;
	int y0 = (int)floorf( tv ) - page.v0 - radT;
	int x1 = (int)floorf( tu ) - page.u0 + radT;
	int y1 = (int)floorf( tv ) - page.v0 + radT;
	if ( x0 < 0 )
		x0 = 0;
	if ( y0 < 0 )
		y0 = 0;
	if ( x1 >= page.w )
		x1 = page.w - 1;
	if ( y1 >= page.h )
		y1 = page.h - 1;

	float edge = radius - page.luxel;
	if ( edge < radius * 0.55f )
		edge = radius * 0.55f;
	if ( edge < 0.f )
		edge = 0.f;

	for ( int y = y0; y <= y1; ++y )
	{
		for ( int x = x0; x <= x1; ++x )
		{
			float cx = ( page.u0 + x ) + 0.5f;
			float cy = ( page.v0 + y ) + 0.5f;
			float dx = ( cx - tu ) * page.luxel;
			float dy = ( cy - tv ) * page.luxel;
			float d = sqrtf( dx * dx + dy * dy );
			if ( d > radius )
				continue;

			float cov = 1.f;
			if ( d > edge && radius > edge )
				cov = 1.f - ( d - edge ) / ( radius - edge );
			if ( cov < 0.f )
				cov = 0.f;
			int ia = (int)( cov * 255.f );
			if ( ia < 1 )
				continue;

			int si = ( y * page.w + x ) * 2;
			if ( bErase )
			{
				int na = (int)page.tex[si + 1] - ia;
				if ( na < 24 )
				{
					page.tex[si] = 0;
					page.tex[si + 1] = 0;
				}
				else
				{
					page.tex[si + 1] = (unsigned char)na;
				}
				continue;
			}

			if ( ia < 20 )
				continue;

			// Interior of the splat is a solid coat. The rim stays soft.
			if ( cov > 0.82f )
				ia = 255;

			unsigned char prevType = page.tex[si];
			unsigned char prevA = page.tex[si + 1];
			page.tex[si] = (unsigned char)type;
			if ( prevA > 0 && prevType == (unsigned char)type && prevA > ia )
				ia = prevA;
			page.tex[si + 1] = (unsigned char)ia;
		}
	}
}

void CTFPaintAtlas::Stamp( const Vector &pos, const Vector &normal, int type, float radius )
{
	if ( type == NO_POWER )
	{
		Erase( pos, radius );
		return;
	}
	if ( radius < 4.f )
		radius = 4.f;
	if ( radius > 250.f )
		radius = 250.f;

	int page = FindOrAdd( pos, normal, radius );
	if ( page < 0 )
		return;
	Raster( m_pPages[page], pos, type, radius, false );
}

void CTFPaintAtlas::Erase( const Vector &pos, float radius )
{
	if ( !m_pPages || m_nPages <= 0 )
		return;
	if ( radius < 8.f )
		radius = 8.f;
	if ( radius > 256.f )
		radius = 256.f;

	for ( int i = 0; i < m_nPages; ++i )
	{
		TFPaintPage_t &page = m_pPages[i];
		float dist = DotProduct( pos, page.normal ) - page.planeDist;
		if ( fabsf( dist ) > radius )
			continue;
		// Sphere vs this chart: the disc on the plane is the intersection.
		float planar2 = radius * radius - dist * dist;
		if ( planar2 <= 0.f )
			continue;
		Raster( page, pos, NO_POWER, sqrtf( planar2 ), true );
	}
}

TFPaintContact_t CTFPaintAtlas::Query( const Vector &origin, float extra ) const
{
	TFPaintContact_t best;
	if ( !m_pPages || m_nPages <= 0 )
		return best;

	float reach = extra;
	if ( reach < 8.f )
		reach = 8.f;

	int bestRank = 0;
	float bestPlane = 1.0e12f;

	for ( int i = 0; i < m_nPages; ++i )
	{
		const TFPaintPage_t &page = m_pPages[i];
		float dist = DotProduct( origin, page.normal ) - page.planeDist;
		if ( dist < -8.f || dist > reach + 72.f )
			continue;

		float tu = DotProduct( origin, page.tangent ) / page.luxel;
		float tv = DotProduct( origin, page.bitangent ) / page.luxel;
		int rad = (int)ceilf( reach / page.luxel ) + 1;
		int x0 = (int)floorf( tu ) - page.u0 - rad;
		int y0 = (int)floorf( tv ) - page.v0 - rad;
		int x1 = (int)floorf( tu ) - page.u0 + rad;
		int y1 = (int)floorf( tv ) - page.v0 + rad;
		if ( x0 < 0 )
			x0 = 0;
		if ( y0 < 0 )
			y0 = 0;
		if ( x1 >= page.w )
			x1 = page.w - 1;
		if ( y1 >= page.h )
			y1 = page.h - 1;

		for ( int y = y0; y <= y1; ++y )
		{
			for ( int x = x0; x <= x1; ++x )
			{
				int si = ( y * page.w + x ) * 2;
				int alpha = page.tex[si + 1];
				if ( alpha < 48 )
					continue;
				int type = page.tex[si];
				int rank = PaintRankAtlas( type );
				float plane = fabsf( dist );
				if ( rank > bestRank || ( rank == bestRank && plane < bestPlane ) )
				{
					bestRank = rank;
					bestPlane = plane;
					best.painted = true;
					best.type = type;
					best.normal = page.normal;
					float cu = ( page.u0 + x ) + 0.5f;
					float cv = ( page.v0 + y ) + 0.5f;
					best.pos = page.normal * page.planeDist
						+ page.tangent * ( cu * page.luxel )
						+ page.bitangent * ( cv * page.luxel );
				}
			}
		}
	}

	return best;
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Gel shader. One luxel corner in, color and alpha out.
//-----------------------------------------------------------------------------
static void TFPaintShader_Eval( const Vector &world, const Vector &normal, const Vector &tangent, const Vector &bitangent,
	const Vector &eye, int type, int coverage, int &r, int &g, int &b, int &a )
{
	r = g = b = a = 0;
	// Early clip. The talk bails out of the pixel shader under the threshold.
	if ( coverage < 28 )
		return;

	Vector view = eye - world;
	float vlen = VectorNormalize( view );
	float ndotv = 1.f;
	if ( vlen > 1.f )
		ndotv = DotProduct( normal, view );
	if ( ndotv < 0.f )
		ndotv = 0.f;

	// Grazing fade, so the porthole does not hard-clip at a low angle.
	float fade = ( ndotv - 0.04f ) / 0.28f;
	if ( fade < 0.f )
		fade = 0.f;
	if ( fade > 1.f )
		fade = 1.f;

	float body = ( coverage / 255.f ) * fade;
	if ( body < 0.10f )
		return;

	// Camera matrix in tangent space, truncated to 2x2.
	// v-sub-s is the camera side, v-sub-f is view forward lying in the plane.
	Vector side = CrossProduct( view, normal );
	if ( side.LengthSqr() < 1.0e-6f )
		side = tangent;
	VectorNormalize( side );
	Vector fwd = CrossProduct( normal, side );

	float m00 = DotProduct( tangent, side );
	float m01 = DotProduct( bitangent, side );
	float m10 = DotProduct( tangent, fwd );
	float m11 = DotProduct( bitangent, fwd );

	float su = DotProduct( world, tangent );
	float sv = DotProduct( world, bitangent );

	// Layout island. Centered at 0 so the post-transform lands on 0.5.
	// The porthole test is the pre-transform island, matching the talk.
	float Layer = 0.f;
	float tiles[2] = { 22.f, 13.f };
	float ox[2] = { 0.f, 0.37f };
	float oy[2] = { 0.f, 0.18f };
	for ( int layer = 0; layer < 2; ++layer )
	{
		float lu = su / tiles[layer] + ox[layer];
		float lv = sv / tiles[layer] + oy[layer];
		float cu = lu - floorf( lu ) - 0.5f;
		float cv = lv - floorf( lv ) - 0.5f;
		float edge = fabsf( cu ) > fabsf( cv ) ? fabsf( cu ) : fabsf( cv );
		if ( edge > 0.48f )
			continue;
		float suv = m00 * cu + m01 * cv;
		float svv = m10 * cu + m11 * cv;
		float rad = sqrtf( suv * suv + svv * svv ) * 2.15f;
		float bubble = 1.f - rad;
		if ( bubble < 0.f )
			bubble = 0.f;
		bubble = bubble * bubble;
		float edgeFade = 1.f - ( edge - 0.30f ) / 0.18f;
		if ( edgeFade < 0.f )
			edgeFade = 0.f;
		if ( edgeFade > 1.f )
			edgeFade = 1.f;
		float sample = bubble * edgeFade;
		// First island is the coat. The second fetch only adds, and dimmer.
		if ( layer == 0 )
		{
			if ( sample > Layer )
				Layer = sample;
		}
		else
		{
			Layer += sample * 0.35f;
		}
	}
	if ( Layer > 1.f )
		Layer = 1.f;

	int cr, cg, cb;
	TFPaint_GetColor( type, cr, cg, cb );

	float inv = 1.f - ndotv;
	float fres = inv * inv * inv;
	float thick = 0.74f + 0.40f * Layer;
	float spec = fres * 0.50f + Layer * fres * 0.85f;

	int ir = (int)( cr * thick + 255.f * spec );
	int ig = (int)( cg * thick + 255.f * spec );
	int ib = (int)( cb * thick + 255.f * spec );
	if ( ir > 255 ) ir = 255;
	if ( ig > 255 ) ig = 255;
	if ( ib > 255 ) ib = 255;
	if ( ir < 0 ) ir = 0;
	if ( ig < 0 ) ig = 0;
	if ( ib < 0 ) ib = 0;

	float alpha = body * ( 0.80f + 0.20f * Layer );
	if ( alpha < 0.12f )
		return;

	r = ir;
	g = ig;
	b = ib;
	a = (int)( alpha * 230.f );
	if ( a > 230 )
		a = 230;
	if ( a < 0 )
		a = 0;
}

static Vector PageCenter( const TFPaintPage_t &page, int x, int y )
{
	float cu = ( page.u0 + x ) + 0.5f;
	float cv = ( page.v0 + y ) + 0.5f;
	return page.normal * page.planeDist
		+ page.tangent * ( cu * page.luxel )
		+ page.bitangent * ( cv * page.luxel );
}

struct GelVert_t
{
	Vector p;
	unsigned char c[4];
	float u;
	float v;
};

// Dynamic meshes draw every primitive Begin reserved. A short batch of
// uninitialized verts is garbage on screen, so the flush begins with the
// exact quad count.
static const int kGelBatchQuads = 180;

static void GelFlush( CMatRenderContextPtr &ctx, IMaterial *mat, GelVert_t *verts, int &quads )
{
	if ( quads <= 0 || !mat )
	{
		quads = 0;
		return;
	}

	ctx->Bind( mat );
	IMesh *mesh = ctx->GetDynamicMesh( true, NULL, NULL, mat );
	CMeshBuilder mb;
	mb.Begin( mesh, MATERIAL_TRIANGLES, quads * 2 );
	int n = quads * 6;
	for ( int i = 0; i < n; ++i )
	{
		mb.Position3fv( verts[i].p.Base() );
		mb.Color4ub( verts[i].c[0], verts[i].c[1], verts[i].c[2], verts[i].c[3] );
		mb.TexCoord2f( 0, verts[i].u, verts[i].v );
		mb.AdvanceVertex();
	}
	mb.End();
	mesh->Draw();
	quads = 0;
}

static void GelQuad( CMatRenderContextPtr &ctx, IMaterial *mat, GelVert_t *verts, int &quads, const Vector c[4], const int rgba[4][4] )
{
	if ( !mat )
		return;
	bool any = false;
	for ( int i = 0; i < 4; ++i )
	{
		if ( rgba[i][3] > 0 )
			any = true;
	}
	if ( !any )
		return;

	if ( quads >= kGelBatchQuads )
		GelFlush( ctx, mat, verts, quads );

	int idx[6] = { 0, 1, 2, 0, 2, 3 };
	int base = quads * 6;
	for ( int k = 0; k < 6; ++k )
	{
		int i = idx[k];
		GelVert_t &v = verts[base + k];
		v.p = c[i];
		v.c[0] = (unsigned char)rgba[i][0];
		v.c[1] = (unsigned char)rgba[i][1];
		v.c[2] = (unsigned char)rgba[i][2];
		v.c[3] = (unsigned char)rgba[i][3];
		v.u = ( i == 1 || i == 2 ) ? 1.f : 0.f;
		v.v = ( i >= 2 ) ? 1.f : 0.f;
	}
	++quads;
}

void CTFPaintAtlas::Draw( const Vector &eye ) const
{
	if ( !m_pPages || m_nPages <= 0 )
		return;

	static IMaterial *s_pGel = NULL;
	if ( !s_pGel )
		s_pGel = materials->FindMaterial( "tftoo/paint/gel", TEXTURE_GROUP_CLIENT_EFFECTS, false );
	if ( !s_pGel || s_pGel->IsErrorMaterial() )
		return;

	float maxDist = tf_paint_draw_dist.GetFloat();
	if ( maxDist < 256.f )
		maxDist = 256.f;
	float maxDist2 = maxDist * maxDist;

	int order[PAGE_MAX];
	float key[PAGE_MAX];
	int nOrder = 0;
	for ( int i = 0; i < m_nPages; ++i )
	{
		const TFPaintPage_t &page = m_pPages[i];
		if ( page.w < 1 || page.h < 1 )
			continue;
		float cu = ( page.u0 + page.w * 0.5f ) * page.luxel;
		float cv = ( page.v0 + page.h * 0.5f ) * page.luxel;
		Vector center = page.normal * page.planeDist + page.tangent * cu + page.bitangent * cv;
		Vector toEye = eye - center;
		if ( DotProduct( page.normal, toEye ) < -64.f )
			continue;
		float d2 = toEye.LengthSqr();
		if ( d2 > maxDist2 * 1.2f )
			continue;
		order[nOrder] = i;
		key[nOrder] = d2;
		++nOrder;
	}
	for ( int i = 0; i < nOrder; ++i )
	{
		for ( int j = i + 1; j < nOrder; ++j )
		{
			if ( key[j] < key[i] )
			{
				float tk = key[i];
				key[i] = key[j];
				key[j] = tk;
				int ti = order[i];
				order[i] = order[j];
				order[j] = ti;
			}
		}
	}

	CMatRenderContextPtr ctx( materials );
	static GelVert_t s_verts[kGelBatchQuads * 6];
	int quads = 0;
	int budget = 2800;

	for ( int oi = 0; oi < nOrder && budget > 0; ++oi )
	{
		const TFPaintPage_t &page = m_pPages[order[oi]];
		for ( int y = 0; y < page.h && budget > 0; ++y )
		{
			for ( int x = 0; x < page.w && budget > 0; ++x )
			{
				int si = ( y * page.w + x ) * 2;
				int coverage = page.tex[si + 1];
				int type = page.tex[si];
				if ( coverage < 28 )
					continue;

				Vector center = PageCenter( page, x, y );
				Vector toEye = eye - center;
				float d2 = toEye.LengthSqr();
				if ( d2 > maxDist2 )
					continue;
				if ( DotProduct( page.normal, toEye ) <= 0.f )
					continue;

				// Far coats drop to every other luxel, then every fourth.
				// Grimes shipped a cheaper LOD shader for the same reason.
				int step = 1;
				if ( d2 > 700.f * 700.f )
					step = 2;
				if ( d2 > 1200.f * 1200.f )
					step = 4;
				if ( step > 1 && ( ( x % step ) != 0 || ( y % step ) != 0 ) )
					continue;

				float scale = (float)step;
				Vector lift = page.normal * 0.45f;
				Vector halfU = page.tangent * ( page.luxel * 0.5f * scale );
				Vector halfV = page.bitangent * ( page.luxel * 0.5f * scale );
				Vector c[4];
				c[0] = center - halfU - halfV + lift;
				c[1] = center + halfU - halfV + lift;
				c[2] = center + halfU + halfV + lift;
				c[3] = center - halfU + halfV + lift;

				int rgba[4][4];
				bool any = false;
				for ( int k = 0; k < 4; ++k )
				{
					int rr, gg, bb, aa;
					TFPaintShader_Eval( c[k], page.normal, page.tangent, page.bitangent, eye, type, coverage, rr, gg, bb, aa );
					rgba[k][0] = rr;
					rgba[k][1] = gg;
					rgba[k][2] = bb;
					rgba[k][3] = aa;
					if ( aa > 0 )
						any = true;
				}
				if ( !any )
					continue;

				GelQuad( ctx, s_pGel, s_verts, quads, c, rgba );
				--budget;
			}
		}
	}

	GelFlush( ctx, s_pGel, s_verts, quads );
}

#endif // CLIENT_DLL
