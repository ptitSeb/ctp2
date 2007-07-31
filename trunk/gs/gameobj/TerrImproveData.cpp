









#include "c3.h"
#include "XY_Coordinates.h"
#include "World.h"
#include "Cell.h"
#include "TerrImproveData.h"
#include "CivArchive.h"
#include "TiledMap.h"
#include "TerrainRecord.h"
#include "Player.h"
#include "Installation.h"
#include "Vision.h"
#include "TerrImprove.h"
#include "DynArr.h"
#include "installationtree.h"

#include "network.h"
#include "terrainutil.h"
#include "TerrainImprovementRecord.h"
#include "MaterialPool.h"

#include "GameEventManager.h"
#include "directions.h"

#include "CityInfluenceIterator.h"

TerrainImprovementData::TerrainImprovementData(ID id,
											   sint32 owner,
											   MapPoint pnt,
											   sint32 type,
											   sint32 extraData)
	: GameObj(id.m_id)
{
	m_owner = owner;
	m_type = type;
	m_point = pnt;

	m_turnsToComplete = terrainutil_GetProductionTime(type, pnt, extraData);
	m_materialCost = terrainutil_GetProductionCost(type, pnt, extraData);
	m_isComplete = false;
	m_isBuilding = false;

	g_theWorld->GetCell(pnt)->SetColor(20);

	ENQUEUE();
}

TerrainImprovementData::TerrainImprovementData(CivArchive &archive) : GameObj(0)
{
	Serialize(archive);
}

BOOL TerrainImprovementData::Complete(void) 
{
	TerrainImprovement imp(m_id);

	Cell* theCell = g_theWorld->GetCell(m_point);

	for(sint32 p = 0; p < k_MAX_PLAYERS; p++) {
		if(!g_player[p]) continue;
		if(p == m_owner) continue; 
		g_player[p]->m_vision->AddUnseen(m_point);
	}
	DPRINTF(k_DBG_GAMESTATE, ("Completed improvement %d at (%d,%d)\n",
							  m_type, m_point.x, m_point.y));

	const TerrainImprovementRecord *rec = g_theTerrainImprovementDB->Get(m_type);
	if(!rec->GetClassTerraform()) {
		theCell->InsertDBImprovement(m_type);
	} else {
		sint32 terr;
		if(rec->GetTerraformTerrainIndex(terr)) {
			g_theWorld->ClearGoods(m_point.x, m_point.y);
			g_theWorld->SmartSetTerrain(m_point, terr, 0);
		}
	}
	theCell->RemoveImprovement(imp);

	
	
	if(rec->GetClassTerraform()) {
		g_theWorld->CutImprovements(m_point);
	}


	Assert(g_player[m_owner]);
	if(!g_player[m_owner])
		return TRUE;

	
	if (terrainutil_IsInstallation(m_type))
	{
		g_player[m_owner]->CreateInstallation( m_type, m_point );
	}

	
	
	
	
	theCell->CalcTerrainMoveCost();

	terrainutil_DoVision(m_point);

	if(rec->GetIntBorderRadius() && rec->GetSquaredBorderRadius()) {
		sint32 intRad, sqRad;
		rec->GetIntBorderRadius(intRad);
		rec->GetSquaredBorderRadius(sqRad);

		GenerateBorders(m_point, m_owner, intRad, sqRad);
	}

	g_tiledMap->PostProcessTile(m_point, g_theWorld->GetTileInfo(m_point));
	g_tiledMap->TileChanged(m_point);

	MapPoint pos;
	

	for(WORLD_DIRECTION d = NORTH; d < NOWHERE; d = (WORLD_DIRECTION)((sint32)d + 1)) {
		if(m_point.GetNeighborPosition(d, pos)) {
			g_tiledMap->PostProcessTile(pos, g_theWorld->GetTileInfo(pos));
			g_tiledMap->TileChanged(pos);
			g_tiledMap->RedrawTile(&pos);
		}
	}
	g_tiledMap->RedrawTile(&m_point);
	if(g_network.IsHost()) {
		g_network.Enqueue(theCell, m_point.x, m_point.y);
	}

	g_theWorld->GetCell(m_point)->SetColor(1000);

	imp.Kill();

	return TRUE;
}

BOOL TerrainImprovementData::AddTurn(sint32 turns)
{
	if(!m_isBuilding)
		
		return FALSE;

	m_turnsToComplete -= turns;
	g_network.Block(m_owner);
	ENQUEUE();
	g_network.Unblock(m_owner);

	if(m_turnsToComplete <= 0) {
		
		g_gevManager->AddEvent(GEV_INSERT_Tail,
							   GEV_ImprovementComplete,
							   GEA_Improvement, TerrainImprovement(m_id),
							   GEA_End);
		
	}
	return FALSE;
}

void TerrainImprovementData::Serialize(CivArchive &archive)
{
	uint8 hasChild;

	if(archive.IsStoring()) {
		GAMEOBJ::Serialize(archive);
		archive.StoreChunk((uint8 *)&m_owner, ((uint8 *)&m_isBuilding)+sizeof(m_isBuilding));

		hasChild = m_lesser != NULL;
		archive << hasChild;
		if (m_lesser)
			((TerrainImprovementData *)(m_lesser))->Serialize(archive) ;

		hasChild = m_greater != NULL;
		archive << hasChild;
		if (m_greater)
			((TerrainImprovementData *)(m_greater))->Serialize(archive) ;

	} else {
		GAMEOBJ::Serialize(archive);
		archive.LoadChunk((uint8 *)&m_owner, ((uint8 *)&m_isBuilding)+sizeof(m_isBuilding));

		archive >> hasChild;
		if(hasChild) m_lesser = new TerrainImprovementData(archive);
		else m_lesser = NULL;

		archive >> hasChild;
		if(hasChild) m_greater = new TerrainImprovementData(archive);
		else m_greater = NULL;
	}
}


sint32 TerrainImprovementData::PercentComplete() const
{
	sint32 totalTurns, turnsDone;

	MapPoint p = m_point;
	totalTurns = terrainutil_GetTimeToBuild(p, m_type, m_transformType);

	turnsDone = totalTurns - m_turnsToComplete;
	if (totalTurns == 0) return 100;
	return (turnsDone * 100) / totalTurns;
}

void TerrainImprovementData::StartBuilding()
{
	if(!g_player[m_owner])
		return;

	if(g_player[m_owner]->m_materialPool->GetMaterials() < m_materialCost)
		return;

	g_player[m_owner]->m_materialPool->SubtractMaterials(m_materialCost);
	m_isBuilding = true;
}
