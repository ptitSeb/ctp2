//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Loads Scenarios
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
// - Fixed memory leak in LoadScenarioPackData, by Martin G�hmann.
//
//----------------------------------------------------------------------------


#include "c3.h"

#include "PointerList.h"

#include "CivPaths.h"
#include "CivScenarios.h"
#include "GameFile.h"

extern CivPaths		*g_civPaths;

CivScenarios		*g_civScenarios = NULL;



void CivScenarios::Initialize(void)
{
	g_civScenarios = new CivScenarios();
}



void CivScenarios::Cleanup(void)
{
	if (g_civScenarios) {
		delete g_civScenarios;
		g_civScenarios = NULL;
	}	
}



CivScenarios::CivScenarios()
{
	m_numScenarioPacks = 0;
	m_scenarioPacks = NULL;

	LoadData();
}



CivScenarios::~CivScenarios()
{
	if (m_scenarioPacks)
		ClearData();
}



void CivScenarios::LoadScenarioData(Scenario *scenario, MBCHAR *scenPath)
{
	FILE				*scenFile;
	MBCHAR				scenFileName[_MAX_PATH];

	scenario->m_name[0] = '\0';
	scenario->m_description[0] = '\0';

	sprintf(scenFileName, "%s\\%s", scenPath, k_SCENARIO_INFO_FILENAME);
	
	
	scenFile = fopen(scenFileName, "r");

	if (scenFile) {
		if (!fgets(scenario->m_name, k_SCENARIO_NAME_MAX-1, scenFile)) return;
		scenario->m_name[k_SCENARIO_NAME_MAX-1]='\0';
		scenario->m_name[strlen(scenario->m_name)-1] = '\0';
		if (!fgets(scenario->m_description, k_SCENARIO_DESC_MAX-1, scenFile)) return;
		scenario->m_description[k_SCENARIO_DESC_MAX-1] = '\0';
		if(scenario->m_description[strlen(scenario->m_description)-1]!=0x0A)
		{
			MBCHAR tempbuf[_MAX_PATH];
			do
			{
				memset(tempbuf,0,sizeof(tempbuf));
				if(!fgets(tempbuf, _MAX_PATH, scenFile))
					break;
			} while(tempbuf[strlen(tempbuf)-1]!=0x0A);
		}
		else
		{
			scenario->m_description[strlen(scenario->m_description)-1] = '\0';
		}
		fclose(scenFile);
	}

}



void CivScenarios::LoadScenarioPackData(ScenarioPack *pack, MBCHAR *packPath)
{
	FILE				*listFile;
	MBCHAR				listFileName[_MAX_PATH];
	sint32				i;
    struct _stat		tmpstat;

	pack->m_numScenarios = 0;
	pack->m_scenarios = NULL;
	pack->m_name[0] = '\0';
	pack->m_description[0] = '\0';

	sprintf(listFileName, "%s\\%s", packPath, k_SCENARIO_PACK_LIST_FILENAME);

	
	listFile = fopen(listFileName, "r");
	if (!listFile) return;

	
	if (!fgets(pack->m_name, k_SCENARIO_PACK_NAME_MAX-1, listFile)) return;
	pack->m_name[k_SCENARIO_PACK_NAME_MAX-1] = '\0';
	pack->m_name[strlen(pack->m_name)-1] = '\0';

	
	if (!fgets(pack->m_description, k_SCENARIO_PACK_DESC_MAX-1, listFile)) return;
	pack->m_description[k_SCENARIO_PACK_DESC_MAX-1] = '\0';
	if(pack->m_description[strlen(pack->m_description)-1]!=0x0A)
	{
		MBCHAR tempbuf[_MAX_PATH];
		do
		{
			memset(tempbuf,0,sizeof(tempbuf));
			if(!fgets(tempbuf, _MAX_PATH, listFile))
				break;
		} while(tempbuf[strlen(tempbuf)-1]!=0x0A);
	}
	else
	{
		pack->m_description[strlen(pack->m_description)-1] = '\0';
	}

	
	sint32		numScenarios;
	fscanf(listFile, "%ld", &numScenarios);

	
	fclose(listFile);

	if (numScenarios <= 0) return;

	
	PointerList<MBCHAR>	*scenList = new PointerList<MBCHAR>;
	for (i=0; i<numScenarios; i++) {
		MBCHAR		scenPath[_MAX_PATH];
		MBCHAR		scenListName[_MAX_PATH];
		sint32		r;

		
		sprintf(scenPath, "%s\\%s%#.4d", packPath, k_SCENARIO_FOLDER_PREFIX, i, k_SCENARIO_INFO_FILENAME);
		sprintf(scenListName, "%s\\%s", scenPath, k_SCENARIO_INFO_FILENAME);

		r = _stat(scenListName, &tmpstat);
		if (!r) {
			MBCHAR *scenarioPath = new MBCHAR[strlen(scenPath)+1];
			strcpy(scenarioPath, scenPath);
			scenList->AddTail(scenarioPath);
		}
	}

	if (scenList->GetCount() > 0)
	{
		numScenarios = scenList->GetCount();

		
		pack->m_numScenarios = numScenarios;
		pack->m_scenarios = new Scenario[pack->m_numScenarios];

		PointerList<MBCHAR>::Walker walker(scenList);

		i=0;
		while (walker.IsValid()) {
			strcpy(pack->m_scenarios[i].m_path, walker.GetObj());
			LoadScenarioData(&pack->m_scenarios[i], walker.GetObj());

			walker.Next();
			i++;
		}
#if defined(ACTIVISION_ORIGINAL)
//Removed by Martin G�hmann
		scenList->DeleteAll();	
		delete scenList;
	}
#else
//Added by Martin G�hmann
	}

	//This must be deleted always
	scenList->DeleteAll();	
	delete scenList;
#endif








}



void CivScenarios::LoadData(void)
{
	MBCHAR				*lpFileName = NULL;
	MBCHAR				path[_MAX_PATH],
						rootPath[_MAX_PATH];
	WIN32_FIND_DATA		fileData;
	HANDLE				lpFileList;
	sint32				i;
    struct _stat		tmpstat;

	

	
	g_civPaths->GetScenarioRootPath(rootPath);

	
	sprintf(path, "%s\\*.*", rootPath);

	
	lpFileList = FindFirstFile(path,&fileData);

	
	if (lpFileList == INVALID_HANDLE_VALUE) return;

	MBCHAR				*fileListFileName;

	PointerList<MBCHAR>	*packList = new PointerList<MBCHAR>;
	do {
		if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			MBCHAR		packListName[_MAX_PATH];
			sint32		r;

			
			sprintf(packListName, "%s\\%s\\%s", rootPath, fileData.cFileName, k_SCENARIO_PACK_LIST_FILENAME);
			r = _stat(packListName, &tmpstat);
			if (!r) {
				fileListFileName = new MBCHAR[strlen(fileData.cFileName)+1];
				
				strcpy(fileListFileName, fileData.cFileName);

				packList->AddTail(fileListFileName);
			}
		}
	} while(FindNextFile(lpFileList,&fileData));

	FindClose(lpFileList);

	if (packList->GetCount() <= 0) {
		delete packList;

		return;
	}

	
	m_numScenarioPacks = packList->GetCount();
	m_scenarioPacks = new ScenarioPack[m_numScenarioPacks];

	PointerList<MBCHAR>::Walker *walker = new PointerList<MBCHAR>::Walker(packList);

	
	for (i=0; i<m_numScenarioPacks; i++) {
		MBCHAR		packPath[_MAX_PATH];

		
		fileListFileName = walker->GetObj();

		sprintf(packPath, "%s\\%s", rootPath, walker->GetObj());

		strcpy(m_scenarioPacks[i].m_path, packPath);
		m_scenarioPacks[i].m_index = i;
		LoadScenarioPackData(&m_scenarioPacks[i], packPath);

		
		delete fileListFileName;

		walker->Next();
	}

	delete walker;
	delete packList;

	return;

}



void CivScenarios::ClearData(void)
{
	sint32		i;

	for (i=0; i<m_numScenarioPacks; i++) {
		delete[] m_scenarioPacks[i].m_scenarios;
	}

	delete[] m_scenarioPacks;

	m_numScenarioPacks = 0;
	m_scenarioPacks = NULL;
}



void CivScenarios::ReloadData(void)
{
	if (m_scenarioPacks) {
		ClearData();
	}

	LoadData();
}


ScenarioPack *CivScenarios::GetScenarioPack(sint32 which)
{
	Assert(which >= 0 && which < m_numScenarioPacks);
	if (which < 0 || which >= m_numScenarioPacks) return NULL;

	return &m_scenarioPacks[which];
}

ScenarioPack *CivScenarios::GetScenarioPackByPath(const MBCHAR *path)
{
	sint32 p;
	for(p = 0; p < m_numScenarioPacks; p++) {
		if(!stricmp(path, m_scenarioPacks[p].m_path)) {
			return &m_scenarioPacks[p];
		}
	}
	return NULL;
}



BOOL CivScenarios::FindScenario(MBCHAR *scenarioName, ScenarioPack **pack, Scenario **scen)
{
	
	for (sint32 i=0; i<m_numScenarioPacks; i++) {
		ScenarioPack *scenarioPack = &m_scenarioPacks[i];
		
		if (!scenarioPack) continue;

		for (sint32 j=0; j<scenarioPack->m_numScenarios; j++) {
			Scenario *scenario = &scenarioPack->m_scenarios[j];

			if (!scenario) continue;


			if (!_stricoll(scenarioName, scenario->m_name)) {

			

				*pack = scenarioPack;
				*scen = scenario;

				return TRUE;
			}
		}
	}

	*pack = NULL;
	*scen = NULL;

	return FALSE;
}

BOOL CivScenarios::FindScenarioFromSaveFile(MBCHAR *saveName, ScenarioPack **pack, Scenario **scen)
{
	MBCHAR path[_MAX_PATH];
	strcpy(path, saveName);
	MBCHAR *lastBackslash = strrchr(path, '\\');
	if(!lastBackslash)
		return FALSE;

	*lastBackslash = '\0';

	Scenario tmpscen;
	tmpscen.m_name[0] = 0;
	LoadScenarioData(&tmpscen, path);
	if(tmpscen.m_name[0] != 0) {
		return FindScenario(tmpscen.m_name, pack, scen);
	}

	return FALSE;
}




BOOL CivScenarios::ScenarioHasSavedGame(Scenario *scen)
{
	MBCHAR	tempPath[_MAX_PATH];

	if (!scen) return FALSE;

	sprintf(tempPath, "%s\\%s", 
						scen->m_path, 
						k_SCENARIO_DEFAULT_SAVED_GAME_NAME);

	if (c3files_PathIsValid(tempPath)) {
		return TRUE;
	} 

	return FALSE;
}

SaveInfo *CivScenarios::LoadSaveInfo(Scenario *scen)
{
	MBCHAR	tempPath[_MAX_PATH];

	if (!scen) return NULL;

	sprintf(tempPath, "%s\\%s", 
						scen->m_path, 
						k_SCENARIO_DEFAULT_SAVED_GAME_NAME);

	if (c3files_PathIsValid(tempPath)) {
		SaveInfo *info = new SaveInfo;
		if(!GameFile::FetchExtendedSaveInfo(tempPath, info)) {
			delete info;
			return NULL;
		} else {
			return info;
		}
	} 

	return FALSE;
}
	

CIV_SCEN_ERR CivScenarios::MakeNewPack(MBCHAR *dirName, MBCHAR *packName, MBCHAR *packDesc)
{
	MBCHAR				path[_MAX_PATH],
						rootPath[_MAX_PATH];
    struct _stat		tmpstat;

	
	g_civPaths->GetScenarioRootPath(rootPath);

	sprintf(path, "%s\\%s", rootPath, dirName);
	if(!_stat(path, &tmpstat)) {
		
		return CIV_SCEN_DIR_EXISTS;
	}

	c3files_CreateDirectory(path);

	strcat(path, "\\packlist.txt");
	FILE *packlist = fopen(path, "w");
	Assert(packlist);
	if(!packlist)
		return CIV_SCEN_CANT_CREATE_FILE;
	
	fprintf(packlist, "%s\n", packName);
	fprintf(packlist, "%s\n", packDesc);
	fprintf(packlist, "0\n");

	fclose(packlist);

	ReloadData();

	return CIV_SCEN_OK;
}

CIV_SCEN_ERR CivScenarios::UpdatePacklist(ScenarioPack *pack)
{
	MBCHAR path[_MAX_PATH];
	sprintf(path, "%s\\packlist.txt", pack->m_path);

	FILE *packList = fopen(path, "w");

	Assert(packList);
	if(!packList) {
		return CIV_SCEN_CANT_CREATE_FILE;
	}

	fprintf(packList, "%s\n", pack->m_name);
	fprintf(packList, "%s\n", pack->m_description);
	fprintf(packList, "%d\n", pack->m_numScenarios);

	fclose(packList);

	return CIV_SCEN_OK;	
}

CIV_SCEN_ERR CivScenarios::MakeNewScenario(ScenarioPack *pack, MBCHAR *scenName, MBCHAR *scenDesc)
{
    struct _stat		tmpstat;

	MBCHAR scenPath[_MAX_PATH];
	sprintf(scenPath, "%s\\scen%04d", pack->m_path, pack->m_numScenarios);
	if(!_stat(scenPath, &tmpstat)) {
		
		return CIV_SCEN_DIR_EXISTS;
	}

	c3files_CreateDirectory(scenPath);

	MBCHAR descPath[_MAX_PATH];
	sprintf(descPath, "%s\\scenario.txt", scenPath);
	FILE *scenFile = fopen(descPath, "w");
	Assert(scenFile);
	if(!scenFile)
		return CIV_SCEN_CANT_CREATE_FILE;

	fprintf(scenFile, "%s\n", scenName);
	fprintf(scenFile, "%s\n", scenDesc);
	fclose(scenFile);

	MBCHAR dataPath[_MAX_PATH];
	sprintf(dataPath, "%s\\default", scenPath);
	c3files_CreateDirectory(dataPath);

	strcat(dataPath, "\\gamedata");
	c3files_CreateDirectory(dataPath);

	MBCHAR filePath[_MAX_PATH];
	sprintf(filePath, "%s\\scenario.slc", dataPath);
	
	FILE *script = fopen(filePath, "w");
	Assert(script);
	if(!script)
		return CIV_SCEN_CANT_CREATE_FILE;
	
	fprintf(script, "// Scenario script for %s\n", scenName);
	fclose(script);

	pack->m_numScenarios++;
	UpdatePacklist(pack);
	

	ReloadData();

	return CIV_SCEN_OK;
}

