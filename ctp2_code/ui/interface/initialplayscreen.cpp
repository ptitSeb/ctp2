//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Main menu screen
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
// ACTIVISION_ORIGINAL
// - When defined, generates the original Activision code.
// - When not defined, generates the modified Apolyton code.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Shifted buttons from the "Single Player" subscreen into this one to
//   simplify the interface.
//   (JJB)
//
//----------------------------------------------------------------------------

#include "c3.h"

#include "civ3_main.h"
#include "civapp.h"

#include "aui.h"
#include "aui_ldl.h"
#include "aui_uniqueid.h"
#include "aui_stringtable.h"
#include "aui_textfield.h"
#include "c3ui.h"
#include "c3window.h"
#include "c3_static.h"
#include "c3_button.h"
#include "c3_dropdown.h"
#include "c3_listitem.h"
#include "CivPaths.h"
#include "uiutils.h"

#include "netshell.h"

#include "screenutils.h"

#if defined(ACTIVISION_ORIGINAL)
// No longer need this include since the SP screen has been removed
// from the interface
#include "spwindow.h"
#endif

#include "loadsavewindow.h"
#include "initialplayWindow.h"
#include "scenariowindow.h"

#if !defined(ACTIVISION_ORIGINAL)
// Need more includes for the actions of the new buttons
#include "optionswindow.h"
#endif

#include "gameinit.h"
#include "profileDB.h"

#include "ctp2_listbox.h"
#include "ctp2_listitem.h"
#include "ctp2_dropdown.h"

extern  C3UI				*g_c3ui;
extern  CivApp				*g_civApp;
extern  CivPaths			*g_civPaths;
extern	ProfileDB			*g_theProfileDB;


static C3Window				*s_initplayWindow		= NULL;
static MBCHAR				*s_initplayWindowLDLBlock = "InitPlayWindow";





sint32	initialplayscreen_displayMyWindow()
{
	extern bool g_e3Demo;
	if(g_e3Demo)
		return 0;

	sint32 retval=0;
	if(!s_initplayWindow) { retval = initialplayscreen_Initialize(); }

	g_c3ui->AddWindow(s_initplayWindow);

	
	return retval;
}
sint32 initialplayscreen_removeMyWindow(uint32 action)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return 0;

	AUI_ERRCODE auiErr;

	auiErr = g_c3ui->RemoveWindow( s_initplayWindow->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );

	return 1;
}



AUI_ERRCODE initialplayscreen_Initialize( void )
{
	AUI_ERRCODE errcode;

	if ( s_initplayWindow ) 
		return AUI_ERRCODE_OK; 

	
	s_initplayWindow = (C3Window *)aui_Ldl::BuildHierarchyFromRoot(s_initplayWindowLDLBlock);
	Assert (s_initplayWindow != NULL);
	if (s_initplayWindow == NULL) 
		return AUI_ERRCODE_INVALIDPARAM;

	
	errcode = aui_Ldl::SetActionFuncAndCookie(s_initplayWindowLDLBlock, "SpriteTestButton", 
											spritetest_spPress, NULL);
	Assert(errcode == AUI_ERRCODE_OK);

#ifndef _DEBUG
	AUI_ERRCODE *retval=0;
	ctp2_Button *spriteTest = (ctp2_Button *)aui_Ldl::GetObject(s_initplayWindowLDLBlock, "SpriteTestButton");
 	spriteTest->Hide();
#endif

#if defined(ACTIVISION_ORIGINAL)
	// Cut out this code since the button is gone
	errcode = aui_Ldl::SetActionFuncAndCookie(s_initplayWindowLDLBlock, "SPButton", 
											initialplayscreen_spPress, NULL);
	Assert(errcode == AUI_ERRCODE_OK);
#endif

	errcode = aui_Ldl::SetActionFuncAndCookie(s_initplayWindowLDLBlock, "EmailButton", 
											initialplayscreen_emailPress, NULL);
	Assert(errcode == AUI_ERRCODE_OK);

	errcode = aui_Ldl::SetActionFuncAndCookie(s_initplayWindowLDLBlock, "HotseatButton", 
											initialplayscreen_hotseatPress, NULL);
	Assert(errcode == AUI_ERRCODE_OK);

	errcode = aui_Ldl::SetActionFuncAndCookie(s_initplayWindowLDLBlock, "MPButton", 
											initialplayscreen_mpPress, NULL);
	Assert(errcode == AUI_ERRCODE_OK);

	errcode = aui_Ldl::SetActionFuncAndCookie(s_initplayWindowLDLBlock, "QuitButton", 
											initialplayscreen_quitPress, NULL);
	Assert(errcode == AUI_ERRCODE_OK);

	errcode = aui_Ldl::SetActionFuncAndCookie(s_initplayWindowLDLBlock, "CreditsButton", 
											initialplayscreen_creditsPress, NULL);
	Assert(errcode == AUI_ERRCODE_OK);

#if !defined(ACTIVISION_ORIGINAL)
	// Code for new buttons, just copy/pasted and altered from the above

	errcode = aui_Ldl::SetActionFuncAndCookie(s_initplayWindowLDLBlock, "NewGameButton", 
											initialplayscreen_newgamePress, NULL);
	Assert(errcode == AUI_ERRCODE_OK);

	errcode = aui_Ldl::SetActionFuncAndCookie(s_initplayWindowLDLBlock, "LoadGameButton", 
											initialplayscreen_loadgamePress, NULL);
	Assert(errcode == AUI_ERRCODE_OK);

	errcode = aui_Ldl::SetActionFuncAndCookie(s_initplayWindowLDLBlock, "TutorialButton", 
											initialplayscreen_tutorialPress, NULL);
	Assert(errcode == AUI_ERRCODE_OK);

	errcode = aui_Ldl::SetActionFuncAndCookie(s_initplayWindowLDLBlock, "OptionsButton", 
											initialplayscreen_optionsPress, NULL);
	Assert(errcode == AUI_ERRCODE_OK);
#endif // !defined(ACTIVISION_ORIGINAL)

	return AUI_ERRCODE_OK;
}



AUI_ERRCODE initialplayscreen_Cleanup()
{
	if ( !s_initplayWindow  ) return AUI_ERRCODE_OK; 

	g_c3ui->RemoveWindow( s_initplayWindow->Id() );

	AUI_ERRCODE		errcode;

	errcode = aui_Ldl::DeleteHierarchyFromRoot(s_initplayWindowLDLBlock);
	Assert(errcode == AUI_ERRCODE_OK);

	
	s_initplayWindow = NULL;

	return errcode;
}



#if defined(ACTIVISION_ORIGINAL)
// Button removed
void
initialplayscreen_spPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	if(initialplayscreen_removeMyWindow(action))
		spscreen_displayMyWindow();
}
#endif



void
spritetest_spPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	if(initialplayscreen_removeMyWindow(action))
	{
			
			MBCHAR fieldText[k_MAX_NAME_LEN];

			sprintf(fieldText,"Jerry");
				
			
  
  			g_theProfileDB->SetLeaderName(fieldText);
				
			
			g_theProfileDB->SetSaveNote("");
				
			
			
			g_theProfileDB->SetTutorialAdvice(FALSE);
				
			g_theProfileDB->SetFogOfWar(false);

			g_civApp->PostSpriteTestAction();
	} 
}


void
initialplayscreen_mpPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	
	control->Enable(FALSE);

	NetShell::Enter( k_NS_FLAGS_CREATE );
}


void initialplayscreen_hotseatPress(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != (uint32)AUI_BUTTON_ACTION_EXECUTE) return;

	if(initialplayscreen_removeMyWindow(action)) {
		g_startHotseatGame = TRUE;
		spnewgamescreen_displayMyWindow();
	}
}

void initialplayscreen_emailPress(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != (uint32)AUI_BUTTON_ACTION_EXECUTE) return;

	if(initialplayscreen_removeMyWindow(action)) {
		g_startEmailGame = TRUE;
		spnewgamescreen_displayMyWindow();
	}
}	

void
initialplayscreen_loadPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;


		loadsavescreen_displayMyWindow(LSS_LOAD_GAME);
}
void
initialplayscreen_continuePress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;
}
void
initialplayscreen_instantPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	initialplayscreen_removeMyWindow(action);
	g_civApp->PostStartGameAction();
}
void
initialplayscreen_mapeditorPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;
}

void
initialplayscreen_quitPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	initialplayscreen_removeMyWindow(action);
	
	ExitGame();
}
void
initialplayscreen_creditsPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	open_CreditsScreen();
}

#if !defined(ACTIVISION_ORIGINAL)
// Code for new buttons taked from spscreen.cpp (and altered)
void initialplayscreen_newgamePress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;
	if(initialplayscreen_removeMyWindow(action)) 
		spnewgamescreen_displayMyWindow();
}

void initialplayscreen_loadgamePress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	loadsavescreen_displayMyWindow(LSS_LOAD_GAME);
}

void initialplayscreen_tutorialPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;
	
	if (c3files_HasLegalCD()) {
		if(initialplayscreen_removeMyWindow(action)) {
			g_theProfileDB->SetTutorialAdvice(TRUE);
			g_civApp->PostStartGameAction();
		}
	}
}

void initialplayscreen_optionsPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	optionsscreen_displayMyWindow(0);
}
#endif //!defined(ACTIVISION_ORIGINAL)

C3Window *GetInitialPlayScreen()
{
	if (s_initplayWindow)
		return s_initplayWindow;

	return NULL;
}