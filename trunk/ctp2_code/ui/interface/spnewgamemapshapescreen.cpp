//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Handling of user preferences.
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
// - Addion by Martin G�hmann: Two more world shape options, 
//   flat world and Neptun world
//
//----------------------------------------------------------------------------

#include "c3.h"
#include "c3window.h"
#include "c3_popupwindow.h"
#include "c3_button.h"
#include "c3_listitem.h"
#include "c3_dropdown.h"
#include "c3_static.h"
#include "c3slider.h"
#include "c3ui.h"
#include "aui_Radio.h"
#include "aui_SwitchGroup.h"
#include "aui_uniqueid.h"
#include "XY_Coordinates.h"
#include "World.h"

#include "profileDB.h"

#include "custommapscreen.h"

#include "spnewgamewindow.h"
#include "spnewgamemapshapescreen.h"

#include "keypress.h"

extern C3UI			*g_c3ui;
extern ProfileDB	*g_theProfileDB;
extern World		*g_theWorld;

#if defined(ACTIVISION_ORIGINAL)
#define k_NUM_MAPSHAPEBOXES	2
#else
//Allow more shape options by Martin G�hmann
#define k_NUM_MAPSHAPEBOXES	4
#endif

static c3_PopupWindow	*s_spNewGameMapShapeScreen	= NULL;



static aui_SwitchGroup	*s_group		= NULL;
static aui_Radio	**s_checkBox;
static c3_Static	*s_ewLabel			= NULL; // Earth world
static c3_Static	*s_dwLabel			= NULL; // Doughnut world
#if !defined(ACTIVISION_ORIGINAL)
//Added by Martin G�hmann
static c3_Static	*s_nwLabel			= NULL; // Neptun world
#endif
static c3_Static	*s_fwLabel			= NULL; // Flat world

static MBCHAR	checknames[k_NUM_MAPSHAPEBOXES][50] = {
#if defined(ACTIVISION_ORIGINAL)
	"MapShapeOne",
	"MapShapeTwo"
#else
	//Added two more shapes for more shape options by Martin G�hmann
	"MapShapeOne",   //Earth world (West-East wrap world)
	"MapShapeTwo",   //Doughnut world
	"MapShapeThree", //Neptun world (North-South wrap world)
	"MapShapeFour"   //Flat world
#endif //ACTIVISION_ORIGINAL
};

static sint32 s_useMode = 0;


static sint32 s_mapShapeIndex = 0;
sint32 spnewgamemapshapescreen_getMapShapeIndex( void )
{
	return s_mapShapeIndex;
}






void spnewgamemapshapescreen_setMapShapeIndex( sint32 index )
{
	
	if ( index < 0 || index >= k_NUM_MAPSHAPEBOXES )
		return;

	for (sint32 i = 0;i < k_NUM_MAPSHAPEBOXES;i++ )
		s_checkBox[ i ]->SetState( 0 );
	s_checkBox[ index ]->SetState( 1 );

	switch ( s_mapShapeIndex = index ) {
	case 0:
		
		g_theProfileDB->SetXWrap( TRUE );
		g_theProfileDB->SetYWrap( FALSE );
		break;
	case 1:
		
		g_theProfileDB->SetXWrap( TRUE );
		g_theProfileDB->SetYWrap( TRUE );
		break;
	case 2:

		#if !defined(ACTIVISION_ORIGINAL) // Added by Martin G�hmann
		g_theProfileDB->SetXWrap( FALSE );
		g_theProfileDB->SetYWrap( TRUE );
		#endif

		break;
	#if !defined(ACTIVISION_ORIGINAL) // Added by Martin G�hmann
	case 3:

		g_theProfileDB->SetXWrap( FALSE );
		g_theProfileDB->SetYWrap( FALSE );
		break;
	#endif

	default:
		
		Assert( FALSE );
		break;
	}
}





sint32	spnewgamemapshapescreen_displayMyWindow(BOOL viewMode, sint32 useMode)
{
	sint32 retval=0;
	if(!s_spNewGameMapShapeScreen) { retval = spnewgamemapshapescreen_Initialize(); }

	AUI_ERRCODE auiErr;

	
	for (sint32 i = 0;i < k_NUM_MAPSHAPEBOXES;i++ )
		s_checkBox[ i ]->Enable( !viewMode );

	s_useMode = useMode;

	auiErr = g_c3ui->AddWindow(s_spNewGameMapShapeScreen);
	keypress_RegisterHandler(s_spNewGameMapShapeScreen);

	Assert( auiErr == AUI_ERRCODE_OK );

	return retval;
}
sint32 spnewgamemapshapescreen_removeMyWindow(uint32 action)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return 0;

	uint32 id = s_group->WhichIsSelected();

	if ( id ) {
		for ( sint32 i = 0;i < k_NUM_MAPSHAPEBOXES;i ++ ) {
			if ( id == s_checkBox[i]->Id() ) {
				spnewgamemapshapescreen_setMapShapeIndex( i );
			}
		}
	}

	AUI_ERRCODE auiErr;

	auiErr = g_c3ui->RemoveWindow( s_spNewGameMapShapeScreen->Id() );
	keypress_RemoveHandler(s_spNewGameMapShapeScreen);

	Assert( auiErr == AUI_ERRCODE_OK );

	if ( s_useMode == 1 ) {
		custommapscreen_displayMyWindow( FALSE, 1 );
	}

	spnewgamescreen_update();

	return 1;
}



AUI_ERRCODE spnewgamemapshapescreen_Initialize( aui_Control::ControlActionCallback *callback )
{
	AUI_ERRCODE errcode;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR		switchBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	sint32 i;

	if ( s_spNewGameMapShapeScreen ) return AUI_ERRCODE_OK; 

	strcpy(windowBlock, "SPNewGameMapShapeScreen");

	{ 
		s_spNewGameMapShapeScreen = new c3_PopupWindow( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false);
		Assert( AUI_NEWOK(s_spNewGameMapShapeScreen, errcode) );
		if ( !AUI_NEWOK(s_spNewGameMapShapeScreen, errcode) ) errcode;

		
		s_spNewGameMapShapeScreen->Resize(s_spNewGameMapShapeScreen->Width(),s_spNewGameMapShapeScreen->Height());
		s_spNewGameMapShapeScreen->GrabRegion()->Resize(s_spNewGameMapShapeScreen->Width(),s_spNewGameMapShapeScreen->Height());
		s_spNewGameMapShapeScreen->SetStronglyModal(TRUE);
	}
	
	if ( !callback ) callback = spnewgamemapshapescreen_backPress;

	
	sprintf( controlBlock, "%s.%s", windowBlock, "Name" );
	s_spNewGameMapShapeScreen->AddTitle( controlBlock );
	s_spNewGameMapShapeScreen->AddClose( callback );



	sprintf( controlBlock, "%s.%s", windowBlock, "Group" );
	s_group = new aui_SwitchGroup( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_group, errcode) );
	if ( !AUI_NEWOK(s_group, errcode) ) return errcode;

	s_checkBox = new aui_Radio*[k_NUM_MAPSHAPEBOXES];

	for ( i = 0;i < k_NUM_MAPSHAPEBOXES;i++ ) {
		sprintf( switchBlock, "%s.%s", controlBlock, checknames[i] );
		s_checkBox[i] = new aui_Radio( &errcode, aui_UniqueId(), switchBlock );
		Assert( AUI_NEWOK(s_checkBox[i], errcode) );
		if ( !AUI_NEWOK(s_checkBox[i], errcode) ) return errcode;
		s_group->AddSwitch( (aui_Radio *)s_checkBox[i] );
	
	}

	if ( g_theProfileDB->IsXWrap() ) {
		if ( g_theProfileDB->IsYWrap() ) {
			s_checkBox[1]->SetState(1); //Earth world (x-warp only)
		}
		else {
			s_checkBox[0]->SetState(1); //Doughnut world (x-warp and y-warp)
		}
	}
	#if !defined(ACTIVISION_ORIGINAL) // Added by Martin G�hmann
	else {
		if ( g_theProfileDB->IsYWrap() ) {
			s_checkBox[2]->SetState(1); //Neptun world (y-warp only)
		}
		else {
			s_checkBox[3]->SetState(1); //Flat World (no warp)
		}
	}
	#endif

	s_ewLabel = spNew_c3_Static( &errcode, windowBlock, "EWLabel" );
	s_dwLabel = spNew_c3_Static( &errcode, windowBlock, "DWLabel" );
	#if !defined(ACTIVISION_ORIGINAL) // Added by Martin G�hmann
	s_nwLabel = spNew_c3_Static( &errcode, windowBlock, "NWLabel" );
	s_fwLabel = spNew_c3_Static( &errcode, windowBlock, "FWLabel" );
	#endif


	
	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return AUI_ERRCODE_OK;
}



AUI_ERRCODE spnewgamemapshapescreen_Cleanup()
{
#define mycleanup(mypointer) if(mypointer) { delete mypointer; mypointer = NULL; };

	if ( !s_spNewGameMapShapeScreen  ) return AUI_ERRCODE_OK; 

	g_c3ui->RemoveWindow( s_spNewGameMapShapeScreen->Id() );
	keypress_RemoveHandler(s_spNewGameMapShapeScreen);

	for (sint32 i = 0;i < k_NUM_MAPSHAPEBOXES;i++ ) {
		mycleanup( s_checkBox[i] );
	}

	mycleanup( s_group );
	mycleanup( s_ewLabel );//Earth like world: East-West wrap world
	mycleanup( s_dwLabel );//Doughnut world
#if !defined(ACTIVISION_ORIGINAL)
	//Added by Martin G�hmann
	mycleanup( s_nwLabel );//Neptun like world: North-South wrap world
	mycleanup( s_fwLabel );//Flat world
#endif //ACTIVISION_ORIGINAL




	delete s_spNewGameMapShapeScreen;
	s_spNewGameMapShapeScreen = NULL;

	return AUI_ERRCODE_OK;

#undef mycleanup
}




void spnewgamemapshapescreen_backPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	
	

	spnewgamemapshapescreen_removeMyWindow(action);

}
