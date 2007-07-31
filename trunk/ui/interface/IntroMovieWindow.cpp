

#include "c3.h"

#include "aui_moviebutton.h"
#include "aui_uniqueid.h"
#include "aui_ldl.h"

#include "c3window.h"
#include "Pattern.h"
#include "PatternBase.h"
#include "c3_static.h"

#include "IntroMovieWindow.h"
#include "intromoviewin.h"



IntroMovieWindow::IntroMovieWindow(
									 AUI_ERRCODE *retval,
									 uint32 id,
									 MBCHAR *ldlBlock,
									 sint32 bpp,
									 AUI_WINDOW_TYPE type )
									 :
C3Window(retval, id, ldlBlock, bpp, type)
{
	*retval = InitCommonLdl(ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}



IntroMovieWindow::IntroMovieWindow(
									 AUI_ERRCODE *retval,
									 uint32 id,
									 sint32 x,
									 sint32 y,
									 sint32 width,
									 sint32 height,
									 sint32 bpp,
									 MBCHAR *pattern,
									 AUI_WINDOW_TYPE type)
									 :
C3Window( retval, id, x, y, width, height, bpp, pattern, type )
{
	*retval = InitCommon();
	Assert(AUI_SUCCESS(*retval));
	if (!AUI_SUCCESS(*retval)) return;
}



IntroMovieWindow::~IntroMovieWindow()
{
	if (m_movieButton)
		delete m_movieButton;

}




AUI_ERRCODE IntroMovieWindow::InitCommonLdl(MBCHAR *ldlBlock)
{
	MBCHAR			buttonBlock[k_AUI_LDL_MAXBLOCK+1];
	AUI_ERRCODE		errcode;

	sprintf(buttonBlock, "%s.%s", ldlBlock, "MovieButton");
	m_movieButton = new aui_MovieButton(&errcode, aui_UniqueId(), buttonBlock, intromoviewin_MovieButtonCallback);
	Assert(errcode == AUI_ERRCODE_OK);
	if (errcode != AUI_ERRCODE_OK) return AUI_ERRCODE_LOADFAILED;
	AddControl(m_movieButton);

	m_movieButton->SetFullScreen(true);

	return InitCommon();
}


AUI_ERRCODE IntroMovieWindow::InitCommon(void)
{		
	return C3Window::InitCommon();
}



void IntroMovieWindow::SetMovie(MBCHAR *filename)
{
	if (!m_movieButton) return;

	m_movieButton->SetMovie(filename);
}




AUI_ERRCODE IntroMovieWindow::Idle()
{
	
	
	return AUI_ERRCODE_OK;
}



AUI_ERRCODE IntroMovieWindow::DrawThis( aui_Surface *surface, sint32 x, sint32 y )
{
	
	if ( IsHidden() ) return AUI_ERRCODE_OK;
	

	

	
	

	
	return AUI_ERRCODE_OK;
}
