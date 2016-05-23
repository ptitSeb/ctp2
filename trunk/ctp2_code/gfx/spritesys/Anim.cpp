//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Animation handling
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
// __SPRITETEST__
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Fixed Destructor to fix some memory leaks, by Martin G�hmann.
//
//----------------------------------------------------------------------------

#include "c3.h"
#include "Anim.h"

#include <algorithm>    // std::copy
#include "director.h"   // g_director

Anim::Anim()
:
	m_type              (ANIMTYPE_SEQUENTIAL),
	m_numFrames         (0),
	m_frames            (NULL),
	m_moveDeltas        (NULL),
	m_transparencies    (NULL),
	m_playbackTime      (0),
	m_delay             (0),
	m_delayEnd          (0),
	m_lastFrameTime     (0),
	m_elapsed           (0),
	m_loopFinished      (FALSE),
	m_finished          (FALSE),
	m_weAreInDelay      (FALSE),
	m_noIdleJustDelay   (FALSE)
{ ; }

Anim::Anim(Anim const & copy)
:
	m_type              (copy.m_type),
	m_numFrames         (copy.m_numFrames),
	m_frames            (NULL),
	m_moveDeltas        (NULL),
	m_transparencies    (NULL),
	m_playbackTime      (copy.m_playbackTime),
	m_delay             (copy.m_delay),
	m_delayEnd          (copy.m_delayEnd),
	m_lastFrameTime     (copy.m_lastFrameTime),
	m_elapsed           (copy.m_elapsed),
	m_loopFinished      (copy.m_loopFinished),
	m_finished          (copy.m_finished),
	m_weAreInDelay      (copy.m_weAreInDelay),
	m_noIdleJustDelay   (copy.m_noIdleJustDelay)
{
    if (m_numFrames > 0)
    {
        m_frames            = new uint16[m_numFrames];
        m_transparencies    = new uint16[m_numFrames];
        m_moveDeltas        = new POINT [m_numFrames];

        std::copy(copy.m_frames, copy.m_frames + m_numFrames, m_frames);
        std::copy(copy.m_transparencies, copy.m_transparencies + m_numFrames, m_transparencies);
        std::copy(copy.m_moveDeltas, copy.m_moveDeltas + m_numFrames, m_moveDeltas);
    }
}

Anim const & Anim::operator = (Anim const & copy)
{
    if (this != &copy)
    {
        if (m_numFrames != copy.m_numFrames)
        {
            delete [] m_frames;
            delete [] m_transparencies;
            delete [] m_moveDeltas;
        }

        m_type              = copy.m_type;
        m_numFrames         = copy.m_numFrames;
    	m_playbackTime      = copy.m_playbackTime;
	    m_delay             = copy.m_delay;
	    m_delayEnd          = copy.m_delayEnd;
	    m_lastFrameTime     = copy.m_lastFrameTime;
	    m_elapsed           = copy.m_elapsed;
	    m_loopFinished      = copy.m_loopFinished;
	    m_finished          = copy.m_finished;
	    m_weAreInDelay      = copy.m_weAreInDelay;
	    m_noIdleJustDelay   = copy.m_noIdleJustDelay;

        if (m_numFrames > 0)
        {
            m_frames            = new uint16[m_numFrames];
            m_transparencies    = new uint16[m_numFrames];
            m_moveDeltas        = new POINT [m_numFrames];

            std::copy(copy.m_frames, copy.m_frames + m_numFrames, m_frames);
            std::copy(copy.m_transparencies, copy.m_transparencies + m_numFrames, m_transparencies);
            std::copy(copy.m_moveDeltas, copy.m_moveDeltas + m_numFrames, m_moveDeltas);
        }
        else
        {
            m_frames            = NULL;
            m_transparencies    = NULL;
            m_moveDeltas        = NULL;
        }
    }

    return *this;
}

Anim::~Anim()
{
	delete [] m_frames;
	delete [] m_moveDeltas;
	delete [] m_transparencies;
}

uint16 Anim::GetFrame(sint32 animPos)
{
	if (m_numFrames == 0) return 0;

	return m_frames[(animPos % m_numFrames)];
}

uint16 Anim::GetPlaybackTime(void)
{


	return m_playbackTime;
}

sint32 Anim::GetNextPosition(sint32 animPos)
{
	uint32		curTime = g_director->GetMasterCurTime();

	if (m_finished) {
		m_loopFinished = TRUE;
		return animPos;
	}

	if (m_lastFrameTime == 0) {
		m_lastFrameTime = curTime;
		animPos++;
		return animPos;
	}

	switch (m_type) {
	case ANIMTYPE_SEQUENTIAL:
		animPos++;

		if (animPos >= m_numFrames) {
			m_finished = TRUE;
		}
		break;
	case ANIMTYPE_LOOPED:
		animPos++;

		if(m_noIdleJustDelay)
			animPos = m_numFrames-1;

		if (animPos >= m_numFrames)
		{
			animPos = 0;
			if (m_delay != 0)
			{

				m_weAreInDelay = TRUE;
				m_delayEnd = curTime + (sint32)m_delay;
			}

			else if(m_delay == 0 && m_numFrames <= 1)
			{

				m_weAreInDelay = TRUE;
				m_delayEnd = curTime + (sint32)60000;
			}
			else
			{
				m_loopFinished = TRUE;
			}
		}
		break;
	}

	return animPos;
}

























































































































sint32 Anim::ParseFromTokens(Token *theToken)
{
	sint32		tmp;
	sint32		i;

	if (!token_ParseValNext(theToken, TOKEN_ANIM, tmp)) return FALSE;
	if (tmp == 0) return FALSE;

	if (!token_ParseAnOpenBraceNext(theToken)) return FALSE;

	if (!token_ParseValNext(theToken, TOKEN_ANIM_TYPE, tmp)) return FALSE;
	m_type = (uint16)tmp;

	if (!token_ParseValNext(theToken, TOKEN_ANIM_NUM_FRAMES, tmp)) return FALSE;
	m_numFrames = (uint16)tmp;

	if (!token_ParseValNext(theToken, TOKEN_ANIM_PLAYBACK_TIME, tmp)) return FALSE;
	m_playbackTime = (uint16)tmp;

	if (!token_ParseValNext(theToken, TOKEN_ANIM_DELAY, tmp)) return FALSE;
	m_delay = (uint16)tmp;

	m_frames = new uint16[m_numFrames];
	if (!token_ParseKeywordNext(theToken, TOKEN_ANIM_FRAME_DATA)) return FALSE;
	for (i=0; i<m_numFrames; i++)
	{
		if (theToken->Next() == TOKEN_NUMBER) theToken->GetNumber(tmp);
		else return FALSE;
		m_frames[i] = (uint16)tmp;
	}

	m_moveDeltas = new POINT[m_numFrames];
	for (i=0; i<m_numFrames; i++)
	{
		POINT p = {0,0};
		m_moveDeltas[i] = p;
	}
	if (!token_ParseValNext(theToken, TOKEN_ANIM_MOVE_DELTAS, tmp)) return FALSE;
	if (tmp)
	{
		if (!token_ParseAnOpenBraceNext(theToken)) return FALSE;
		for (i=0; i<m_numFrames; i++)
		{
			POINT		p;

			if (theToken->Next() == TOKEN_NUMBER) theToken->GetNumber(tmp);
			else return FALSE;

			p.x = tmp;

			if (theToken->Next() == TOKEN_NUMBER) theToken->GetNumber(tmp);
			else return FALSE;

			p.y = tmp;

			m_moveDeltas[i] = p;
		}
		if (!token_ParseAnCloseBraceNext(theToken)) return FALSE;
	}

	m_transparencies = new uint16[m_numFrames];
	for (i=0; i<m_numFrames; i++)
	{
		m_transparencies[i] = 15;
	}
	if (!token_ParseValNext(theToken, TOKEN_ANIM_TRANSPARENCIES, tmp)) return FALSE;
	if (tmp)
	{
		if (!token_ParseAnOpenBraceNext(theToken)) return FALSE;
		for (i=0; i<m_numFrames; i++)
		{
			if (theToken->Next() == TOKEN_NUMBER) theToken->GetNumber(tmp);
			else return FALSE;
			m_transparencies[i] = (uint16)tmp;
		}
		if (!token_ParseAnCloseBraceNext(theToken)) return FALSE;
	}

	if (!token_ParseAnCloseBraceNext(theToken)) return FALSE;

	return TRUE;
}

void Anim::Export(FILE *file)
{
	extern TokenData	g_allTokens[];
	sint32				i;

	fprintf(file, "\t%s\t1\n", g_allTokens[TOKEN_ANIM].keyword);

	fprintf(file, "\t{\n");

	fprintf(file, "\t\t%s\t%d\n", g_allTokens[TOKEN_ANIM_TYPE].keyword, m_type);

	fprintf(file, "\t\t%s\t%d\n", g_allTokens[TOKEN_ANIM_NUM_FRAMES].keyword, m_numFrames);

	fprintf(file, "\t\t%s\t%ld\n", g_allTokens[TOKEN_ANIM_PLAYBACK_TIME].keyword, m_playbackTime);

	fprintf(file, "\t\t%s\t%ld\n", g_allTokens[TOKEN_ANIM_DELAY].keyword, m_delay);

	fprintf(file, "\t\t%s", g_allTokens[TOKEN_ANIM_FRAME_DATA].keyword);
	for (i=0; i<m_numFrames; i++)
	{
		if (i%5 == 0) fprintf(file, "\n\t\t\t");
		fprintf(file, "%d\t", m_frames[i]);
	}
	fprintf(file, "\n\n");

	fprintf(file, "\t\t%s\t1\n\t\t{\n", g_allTokens[TOKEN_ANIM_MOVE_DELTAS].keyword);
	for (i=0; i<m_numFrames; i++)
	{
		fprintf(file, "\t\t\t%d %d\n", m_moveDeltas[i].x, m_moveDeltas[i].y);
	}
	fprintf(file, "\n\t\t}\n");

	fprintf(file, "\t\t%s\t1\n\t\t{", g_allTokens[TOKEN_ANIM_TRANSPARENCIES].keyword);
	for (i=0; i<m_numFrames; i++)
	{
		if (i%5 == 0) fprintf(file, "\n\t\t\t");
		fprintf(file, "%d\t", m_transparencies[i]);
	}
	fprintf(file, "\n\t\t}\n\n");

	fprintf(file, "\t}\n\n");

	return;
}
