//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Sprite Group List
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2 
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
// 
// _DEBUG
// - Generate debug version when set.
//
// ACTIVISION_ORIGINAL		
// - When defined, generates the original Activision code.
// - When not defined, generates the modified Apolyton code.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Increased sprite limit from 200 to 500 by Martin G�hmann
//
//----------------------------------------------------------------------------

#pragma once
#ifndef __SPRITEGROUPLIST_H__
#define __SPRITEGROUPLIST_H__

#if defined(ACTIVISION_ORIGINAL)
//Removed by Martin G�hmann
#define k_MAX_SPRITES		200
#else
//Added by Martin G�hmann
#define k_MAX_SPRITES		500
#endif

#include "SpriteList.h"
#include "SpriteGroup.h"

class SpriteGroup;

class SpriteGroupList : public SpriteList {
public:
	SpriteGroupList();
	virtual ~SpriteGroupList();

	SPRITELISTERR			LoadSprite(uint32 index, GROUPTYPE type, LOADTYPE loadType,GAME_ACTION action);
	SPRITELISTERR			PurgeSprite(uint32 index);

	SpriteGroup				*GetSprite(uint32 index, GROUPTYPE type, LOADTYPE loadType,GAME_ACTION action);
	BOOL					ReleaseSprite(uint32 index, LOADTYPE loadType);

	void					RefreshBasicLoads(GROUPTYPE groupType);

private:
	SpriteGroup				*m_spriteList[k_MAX_SPRITES];
};

void spritegrouplist_Initialize(void);
void spritegrouplist_Cleanup(void);

#endif