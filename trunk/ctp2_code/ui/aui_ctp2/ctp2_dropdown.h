

#pragma once

#ifndef __CTP2_DROPDOWN_H__
#define __CTP2_DROPDOWN_H__

#include "aui_dropdown.h"
#include "patternbase.h"


class ctp2_ListItem;

class ctp2_DropDown : public aui_DropDown, public PatternBase
{
public:
	
	ctp2_DropDown(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = NULL,
		void *cookie = NULL );
	ctp2_DropDown(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR *pattern,
		sint32 buttonSize = 0,
		sint32 windowSize = 0,
		ControlActionCallback *ActionFunc = NULL,
		void *cookie = NULL );
	virtual ~ctp2_DropDown() {};

	void	Clear(void);
	
	
	virtual AUI_ERRCODE Draw(aui_Surface *surface, sint32 x, sint32 y);

	virtual AUI_ERRCODE DrawThis(
						aui_Surface *surface = NULL,
						sint32 x = 0,
						sint32 y = 0 );

	
	
	AUI_ERRCODE		AddItem(ctp2_ListItem *item);

	
	
	void			BuildListStart(void);

	
	
	void			BuildListEnd(void);


protected:
	ctp2_DropDown() : aui_DropDown() {}
	AUI_ERRCODE		InitCommonLdl( MBCHAR *ldlBlock );
	AUI_ERRCODE		InitCommon( sint32 buttonSize, sint32 windowSize );
	AUI_ERRCODE		CreateComponents( MBCHAR *ldlBlock = NULL );

protected:
	virtual AUI_ERRCODE	RepositionButton( void );
	virtual AUI_ERRCODE	RepositionListBoxWindow( void );
};

#endif