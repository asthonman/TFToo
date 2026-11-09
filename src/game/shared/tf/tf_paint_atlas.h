//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Lightmap-space gel atlas.
//
// Portal 2 stores gel in the lightmap parameterization (Bronwen Grimes,
// GDC 2011, "Non-standard Texture Maps"). TF2 will not give the game DLL
// those pages or a paint sampler, so each paintable plane gets its own
// chart at a luxel size of 16 and the charts are what movement samples.
//
//=============================================================================
#ifndef TF_PAINT_ATLAS_H
#define TF_PAINT_ATLAS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_paint.h"

class CTFPaintAtlas
{
public:
	CTFPaintAtlas();
	~CTFPaintAtlas();

	void Clear();
	void Stamp( const Vector &pos, const Vector &normal, int type, float radius );
	void Erase( const Vector &pos, float radius );
	TFPaintContact_t Query( const Vector &origin, float extra ) const;

#ifdef CLIENT_DLL
	// Shades every covered luxel with the gel shader and draws the coat.
	void Draw( const Vector &eye ) const;
#endif

private:
	enum
	{
		PAGE_MAX = 64,
		PAGE_DIM_MAX = 96
	};

	struct TFPaintPage_t *m_pPages;
	int m_nPages;

	void Ensure();
	int FindOrAdd( const Vector &pos, const Vector &normal, float radius );
	void Raster( struct TFPaintPage_t &page, const Vector &pos, int type, float radius, bool bErase );

	// Pages own a raw texel buffer. Entities are not copied.
	CTFPaintAtlas( const CTFPaintAtlas & );
	CTFPaintAtlas &operator=( const CTFPaintAtlas & );
};

#endif // TF_PAINT_ATLAS_H
