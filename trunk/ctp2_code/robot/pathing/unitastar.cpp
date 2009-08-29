//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : A* algorithm for units
// Id           : $Id$
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
//
// PRINT_COSTS
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Corrected movement rate of ships above tunnels.
// - m_queue_index used.
// - Straight line corrected for worlds that do not have X-wrapping.
// - Standardised min/max usage.
// - Added method to check if there is a danger along the path (for civilian 
//   units). so units don't go near enemy cities (and can't be bombarded).
//   If no alternate path found, go on the first founded path. The method 
//   considers a danger if the owner is less than neutral - Calvitix
// - Disabled Calvitix check for danger. If an army encounter something on
//   its way the goal should be reconsidered. - Feb. 21st 2005 Martin G�hmann
// - Updated for wrap correction.
// - Made Government modified for units work here. (July 29th 2006 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "c3.h"
#include "UnitAstar.h"

#include "c3math.h"
#include "c3errors.h"
#include "globals.h"

#include "dynarr.h"
#include "Path.h"

// Added by Calvitix
// Had to include those files to determine if the army encounters a possible 
// danger on its way. Of course the is only danger if an ennemy army is 
// encountered and not an ally.
#include "Diplomat.h"
#include "AgreementMatrix.h"
#include "ArmyData.h"


#include "Army.h"
#include "Unit.h"
#include "cellunitlist.h"
#include "XY_Coordinates.h"
#include "World.h"              // g_theWorld, GROUND_Z, SPACE_Z
#include "Cell.h"

#include "dynarr.h"
#include "player.h"
#include "RandGen.h"
#include "UnitRecord.h"
#include "WonderRecord.h"
#include "TerrainRecord.h"
#include "UnitData.h"


#include "UnseenCell.h"
#include "MoveFlags.h"
#include "wonderutil.h"

#include "A_Star_Heuristic_Cost.h"
#include "terrainutil.h"

#define MARK_UNUSED     0xcdcdcdcd

namespace
{
#pragma warning(disable:4309)
    // Will generate a truncation warning, but only once.
    sint16 const    MARK_UNUSED16   = static_cast<sint16>(MARK_UNUSED);
#pragma warning(default:4309)
}

UnitAstar::UnitAstar()
{
    ClearMem();
}

//----------------------------------------------------------------------------
//
// Name       : UnitAstar::StraightLine
//
// Description: Compute a straight path from start to destination.
//
// Parameters : start           : start
//              dest            : destination
//
// Globals    : -
//
// Returns    : bool            : a_path has meaning (i.e. we are not at the
//                                destination already).
//              a_path          : the computed straight line path
//
// Remark(s)  : Does not handle vertical (UP/DOWN) movements.
//
//----------------------------------------------------------------------------
bool UnitAstar::StraightLine
(
	const MapPoint &		start, 
	const MapPoint &		dest, 
    Path &					a_path
) const
{
	if(start == dest)
	{
		a_path.Clear();
		return false;
	}

	a_path.SetStart(start);

	// Shortest distance vector from start to dest, using XY coordinates, and
	// taking world wrap properties into account.
	MapPointData const		diff        = start.NormalizedSubtract(dest);
	WORLD_DIRECTION	const	dirX		= (diff.x > 0) ? EAST  : WEST;
	WORLD_DIRECTION const	dirY		= (diff.y > 0) ? SOUTH : NORTH;
	WORLD_DIRECTION	const	dirDiagonal	= 
		(EAST == dirX) ? ((SOUTH == dirY) ? SOUTHEAST : NORTHEAST)
					   : ((SOUTH == dirY) ? SOUTHWEST : NORTHWEST);

	sint32					absdx		= ABS(diff.x);
	sint32					absdy		= ABS(diff.y);

	// Start with diagonal moves to make the path straight.
	for ( ;	(absdx > 0) && (absdy > 0); --absdx, --absdy)
	{
		a_path.AddDir(dirDiagonal);
	}

	// Pure X leftover - if any.
	for ( ; absdx > 0 ; absdx -=2)
	{
		a_path.AddDir(dirX);
	}

	// Pure Y leftover - if any.
	for ( ; absdy > 0 ; absdy -=2)
	{
		a_path.AddDir(dirY);
	}

	return true;
}

float UnitAstar::ComputeValidMovCost(const MapPoint &pos, Cell *the_pos_cell)
{
	bool const	is_tunnel_and_boat	= 
		g_theWorld->IsTunnel(pos) &&
		((m_move_intersection & k_Unit_MovementType_Sea_Bit) ||
		 (m_move_intersection & k_Unit_MovementType_ShallowWater_Bit)
		);

	if (is_tunnel_and_boat)
	{
		sint32 icost_without_tunnel;
		(void) g_theWorld->GetTerrain(pos)->GetEnvBase()->
			GetMovement(icost_without_tunnel);
		return std::min(m_army_minmax_move, 
						static_cast<float>(icost_without_tunnel)
					   );
	}
	else
	{
		return std::min(m_army_minmax_move, 
						static_cast<float>(the_pos_cell->GetMoveCost())
					   );
	}
}

bool UnitAstar::CanMoveIntoTransports(const MapPoint &pos) 
{
	
	static CellUnitList tmp_transports;
    if (m_army.m_id == (0)) { 
        return false;
    } else { 
        return m_army.CanMoveIntoTransport(m_dest, tmp_transports);
    }
}

bool UnitAstar::CanSpaceLaunch() 
{
	return false;
}

bool UnitAstar::CanSpaceLand()
{
	return false;
}


bool UnitAstar::CheckUnexplored(const MapPoint &prev, const MapPoint &pos,   
     Cell *the_prev_cell, Cell *the_pos_cell,                                 
    float &cost, bool &is_zoc, ASTAR_ENTRY_TYPE &entry, bool &can_enter)
{
     if (m_no_bad_path)
        return false;

     if (!m_is_robot) { 
        if(!g_player[m_owner]->IsExplored(prev)
        && !g_player[m_owner]->IsExplored(pos)
        ){
            cost = 100;
            can_enter = true;
            return true;
        }
     }

    return false;
}


bool UnitAstar::CheckUnits
(
    const MapPoint   &prev,
    const MapPoint   &pos,
    Cell*             the_prev_cell,
    Cell*             the_pos_cell,
    float            &cost,
    bool             &is_zoc,
    bool             &can_be_zoc,
    ASTAR_ENTRY_TYPE &entry,
    bool             &can_enter
)
{
	if (the_pos_cell->GetCity().m_id != 0)
	{
		if (the_pos_cell->GetCity().GetOwner() != m_owner)
		{
			return false;
		}
	}

	CellUnitList* dest_army = the_pos_cell->UnitArmy();

	if (dest_army && (0 < dest_army->Num()))
	{
		if(m_is_robot || dest_army->IsVisible(m_owner))
		{
			PLAYER_INDEX dest_owner = dest_army->GetOwner();
			if (dest_owner != m_owner)
			{
				if (m_is_zero_attack)
				{
					cost = k_ASTAR_BIG;
					can_enter = false;
					entry = ASTAR_BLOCKED;
					
					return true;
				}
				else if (pos != m_dest)
				{
					if(m_is_robot && m_army_can_expel_stealth && dest_army->CanBeExpelled())
					{
						return false;
					}
					else if(m_is_robot && g_player[m_owner]->HasWarWith(dest_owner) && m_army_strength.HasEnough(Squad_Strength(pos)))
					{
						return false;
					}
					else
					{
						cost = k_ASTAR_BIG;
						can_enter = false;
						entry = ASTAR_BLOCKED;

						return true;
					}
				}
				else
				{
					can_be_zoc = false;
				}
			}
			else
			{
				can_be_zoc = false;
				
				if (pos == m_dest)
				{
					if (CanMoveIntoTransports(pos))
					{
						cost = k_MOVE_ENTER_TRANSPORT_COST;
						entry = ASTAR_CAN_ENTER;
						can_enter = true;
						return true;
					}
					else if(m_isTransporter)
					{
						return false;
					}
				}

				if(
				   (     m_check_dest
				    || 
				       (!m_check_dest
				     &&  pos != m_dest
				       )
				   )
				&&  k_MAX_ARMY_SIZE - dest_army->Num() < m_nUnits
				  )
				{
					cost = k_ASTAR_BIG;
					can_enter = false;
					entry = ASTAR_BLOCKED;
					return true;
				}
			}
		}
	}

#if 0
// Removed by Martin G�hmann should be reconsidered
// Maybe a special avoid danger astar
// Added by Calvitix
	MapPoint start;
	m_army->GetPos(start);
	if (pos != start && pos != m_dest)
	{
		if (CheckIsDangerForPos(pos, m_army->IsCivilian()))
		{
			if (cost < 1) cost = 1;
			cost *= k_MOVE_ISDANGER_COST; 
			can_enter = true;
			entry = ASTAR_CAN_ENTER; 
			return true; 
		}
	}
#endif

	return false;
}




bool UnitAstar::CheckHisCity(const MapPoint &prev, const MapPoint &pos,
    Cell *the_prev_cell, Cell *the_pos_cell, CityData *the_pos_city, 
    float &cost, bool &is_zoc, ASTAR_ENTRY_TYPE &entry, bool &can_enter)
{
	if (the_pos_city) {
		if (the_pos_city->GetOwner() != m_owner) {
			if (m_is_robot || the_pos_cell->GetCity().GetVisibility() & (0x01 << m_owner)) {
				if (pos == m_dest) {
					if ((m_move_intersection & k_Unit_MovementType_Air_Bit) ||
						(m_move_intersection & k_Unit_MovementType_Sea_Bit) ||
						(m_move_intersection & k_Unit_MovementType_ShallowWater_Bit)) {
						cost = ComputeValidMovCost(pos, the_pos_cell);
						can_enter = true;
						return true;
					} else if (m_is_zero_attack) {
						cost = k_ASTAR_BIG;
						can_enter = false;
						entry = ASTAR_BLOCKED;
						return true;
					} else {
						cost = ComputeValidMovCost(pos, the_pos_cell);
						can_enter = true;
						return true;
					}
				} else {
					cost = k_ASTAR_BIG;
					can_enter = false;
					entry = ASTAR_BLOCKED;
					return true;
				}
			}
		}
	}

	return false;
}


bool UnitAstar::CheckHeight(const MapPoint &prev, const MapPoint &pos, Cell *the_prev_cell, 
    Cell *the_pos_cell, float &cost, bool &is_zoc, ASTAR_ENTRY_TYPE &entry, bool &can_enter)
{


	return false;
}


bool UnitAstar::CheckMyCity(const MapPoint &prev, const MapPoint &pos,
    Cell *the_prev_cell, Cell *the_pos_cell, CityData *the_pos_city,
    float &cost, bool &is_zoc, ASTAR_ENTRY_TYPE &entry, bool &can_enter)
{
	if (!the_pos_city)
		return false;

	if (the_pos_city->GetOwner() == m_owner) {
		cost = ComputeValidMovCost(pos, the_pos_cell);
		can_enter = true;
		return true;
	}
	return false;
}





bool UnitAstar::CheckMoveUnion(const MapPoint &prev, const MapPoint &pos, Cell *the_prev_cell, 
    Cell *the_pos_cell, float &cost, bool &is_zoc, const bool can_be_zoc,  
    ASTAR_ENTRY_TYPE &entry, bool &can_enter)
{
	if (m_army.m_id != 0)
	{
		if (m_army.CanEnter(pos))
		{
			cost = ComputeValidMovCost(pos, the_pos_cell);
			can_enter = true;
		}
		else
		{
			cost = k_ASTAR_BIG;
			can_enter = false;
			entry = ASTAR_BLOCKED;
		}
	}
	else
	{
		if( (m_move_union & k_Unit_MovementType_Land_Bit)
		&&  (m_move_union & k_Unit_MovementType_Mountain_Bit)
		){
			if ((!the_pos_cell->CanEnter(k_Unit_MovementType_Land_Bit)) &&
				(!the_pos_cell->CanEnter(k_Unit_MovementType_Mountain_Bit)))
			{ 
				cost = k_ASTAR_BIG;
				can_enter = false;
				entry = ASTAR_BLOCKED;
				return true;
			}
		}
		else if (m_move_union & k_Unit_MovementType_Land_Bit)
		{
			if (!the_pos_cell->CanEnter(k_Unit_MovementType_Land_Bit))
			{
				cost = k_ASTAR_BIG; 
				can_enter = false; 
				entry = ASTAR_BLOCKED; 
				return true;
			}
		}
		else if (m_move_union & k_Unit_MovementType_Mountain_Bit)
		{
			if (!the_pos_cell->CanEnter(k_Unit_MovementType_Mountain_Bit))
			{
				cost = k_ASTAR_BIG;
				can_enter = false;
				entry = ASTAR_BLOCKED;
				return true;
			}
		}

	
		if ((m_move_union & k_Unit_MovementType_Sea_Bit) ||
			(m_move_union & k_Unit_MovementType_ShallowWater_Bit))
		{
			if ((!the_pos_cell->CanEnter(k_Unit_MovementType_Sea_Bit)) &&
				(!the_pos_cell->CanEnter(k_Unit_MovementType_ShallowWater_Bit)))
			{ 
				cost = k_ASTAR_BIG;
				can_enter = false;
				entry = ASTAR_BLOCKED;
				return true;
			}
		}
		else if (m_move_union & k_Unit_MovementType_Sea_Bit)
		{
			if (!the_pos_cell->CanEnter(k_Unit_MovementType_Sea_Bit))
			{
				cost = k_ASTAR_BIG;
				can_enter = false;
				entry = ASTAR_BLOCKED;
				return true;
			}
		}
		else if (m_move_union & k_Unit_MovementType_ShallowWater_Bit)
		{
			if (!the_pos_cell->CanEnter(k_Unit_MovementType_ShallowWater_Bit))
			{
				cost = k_ASTAR_BIG;
				can_enter = false;
				entry = ASTAR_BLOCKED;
				return true;
			}
		}

	
		if (m_move_union & k_Unit_MovementType_Space_Bit)
		{
			if (!the_pos_cell->CanEnter(k_Unit_MovementType_Space_Bit))
			{
				cost = k_ASTAR_BIG;
				can_enter = false;
				entry = ASTAR_BLOCKED;
				return true;
			}
		}
	
	
		if (m_move_union & k_Unit_MovementType_Trade_Bit)
		{
			if (!the_pos_cell->CanEnter(k_Unit_MovementType_Trade_Bit))
			{
				cost = k_ASTAR_BIG;
				can_enter = false;
				entry = ASTAR_BLOCKED;
				return true;
			}
		}

		if (can_be_zoc && g_theWorld->IsMoveZOC (m_owner, prev, pos, true) &&
		    !IsBeachLanding(prev,pos,m_move_intersection)
		   )
		{
			is_zoc = true;
			cost = k_ASTAR_BIG;
			can_enter = false;
			entry = ASTAR_RETRY_DIRECTION;
		}
		else
		{
			is_zoc = false;
			cost = ComputeValidMovCost(pos, the_pos_cell);
			can_enter = true;
		}
	}

	return true;
}


bool UnitAstar::CheckMoveIntersection(const MapPoint &prev, const MapPoint &pos,
    Cell *the_prev_cell, Cell *the_pos_cell, float &cost, bool &is_zoc, const bool can_be_zoc, 
    ASTAR_ENTRY_TYPE &entry, bool &can_enter)
{
	if (m_move_intersection & k_Unit_MovementType_Air_Bit)
	{
		cost = k_MOVE_AIR_COST;
		can_enter = true;
	}
	else if (the_pos_cell->CanEnter(m_move_intersection))
	{
		if (can_be_zoc && g_theWorld->IsMoveZOC (m_owner, prev, pos, TRUE) &&
			!IsBeachLanding(prev,pos,m_move_intersection))
		{
			is_zoc = true;
			cost = k_ASTAR_BIG;
			can_enter = false;
			entry = ASTAR_RETRY_DIRECTION;
		}
		else
		{
			is_zoc = false;
			cost = ComputeValidMovCost(pos, the_pos_cell);
			can_enter = true; 
		}
	}
	else
	{
		cost = k_ASTAR_BIG;
		can_enter = false;
		entry = ASTAR_BLOCKED;
	}

	return true;
}

bool UnitAstar::EntryCost(const MapPoint &prev, const MapPoint &pos,
                            float &cost, bool &is_zoc, ASTAR_ENTRY_TYPE &entry) 
{
	entry = ASTAR_CAN_ENTER;

	bool can_be_zoc = !m_ignore_zoc;

	if(!g_theWorld->IsXwrap()) {
		sint16 w = (sint16)g_theWorld->GetXWidth();

		sint32 adjX1 = (prev.x + (prev.y / 2)) % w;
		sint32 adjX2 = (pos.x  + (pos.y  / 2)) % w;

		if((adjX1 == 0 && adjX2 == (w - 1)) ||
		   (adjX2 == 0 && adjX1 == (w - 1))) {
			cost = k_ASTAR_BIG;
			entry = ASTAR_BLOCKED;
			return false;
		}
	}
	
	if (!m_check_dest  &&
		pos == m_dest)
	{
		cost = 10;
		entry = ASTAR_CAN_ENTER;
		return true;
	}

	is_zoc = false;
	bool can_enter = false;
	Cell *the_prev_cell = g_theWorld->AccessCell(prev);
	Assert(the_prev_cell);
	Cell *the_pos_cell = g_theWorld->AccessCell(pos);
	Assert(the_pos_cell);

	uint32 origEnv = the_pos_cell->GetEnv();
	sint16 origMove = sint16(the_pos_cell->GetMoveCost());

	UnseenCellCarton ucell;
	bool restore = false;
	if(!m_is_robot)
	{
		if(g_player[m_owner]->GetLastSeen(pos, ucell))
		{
			// Ugly
			the_pos_cell->SetEnvFast(ucell.m_unseenCell->m_env);
			the_pos_cell->SetMoveCost(double(ucell.m_unseenCell->m_move_cost));
			restore = true;
		}
	}

#define RESTORE if(restore) {the_pos_cell->SetEnvFast(origEnv); the_pos_cell->SetMoveCost(double(origMove)); }

    
	if (CheckUnexplored(prev, pos, the_prev_cell, the_pos_cell, cost, is_zoc, entry, can_enter)){ RESTORE; return can_enter; }
    
    
    if (m_check_units_in_cell) { 
        if (CheckUnits(prev, pos, the_prev_cell, the_pos_cell, cost, is_zoc, can_be_zoc, entry, 
            can_enter)) { 
            RESTORE; 
            return can_enter;
        }
    }

    
    CityData *the_pos_city = NULL;
    if (the_pos_cell->GetCity().m_id !=  0) { 
        the_pos_city = the_pos_cell->GetCity().GetData()->GetCityData();
    }
    if (CheckHisCity(prev, pos, the_prev_cell, the_pos_cell, the_pos_city, 
					 cost, is_zoc, entry, can_enter)) {RESTORE; return can_enter; }

    
    if (CheckHeight(prev, pos, the_prev_cell, the_pos_cell, 
					cost, is_zoc, entry, can_enter)) {RESTORE; return can_enter; }

	
    if (CheckMyCity(prev, pos, the_prev_cell, the_pos_cell, the_pos_city, 
					cost, is_zoc, entry, can_enter)) {RESTORE; return can_enter; }

    if (m_move_union != 0) {
        
        
        
        
        if (CheckMoveUnion(prev, pos, the_prev_cell, the_pos_cell, cost, is_zoc, can_be_zoc, entry,
						   can_enter)) {RESTORE; return can_enter; }

    } else { 

        
        if (CheckMoveIntersection(prev, pos, the_prev_cell, the_pos_cell, cost, is_zoc, can_be_zoc, entry,
								  can_enter)) {RESTORE; return can_enter; }

    }

	RESTORE;

    
    cost = k_ASTAR_BIG;
	entry = ASTAR_BLOCKED;
    return false;
}



bool UnitAstar::InitPoint(AstarPoint *parent, AstarPoint *point, 
                          const MapPoint &pos, 
                          const float pc, const MapPoint &dest)
{ 
    AstarPoint *d = point; 
    bool is_zoc = false;
    ASTAR_ENTRY_TYPE entry = ASTAR_CAN_ENTER; 

    d->m_flags = 0; 
	d->SetEntry(ASTAR_CAN_ENTER);
    d->SetZoc(false);
    d->SetExpanded(false);
    d->m_pos = pos;
    d->m_parent = parent;
    d->m_queue_idx = -1;
    
	d->m_past_cost = pc;
    if (parent == NULL) { 
       d->m_entry_cost = 0.0;
       d->m_future_cost = EstimateFutureCost(d->m_pos, dest);
       d->m_total_cost = d->m_past_cost + d->m_entry_cost 
            + d->m_future_cost;
#ifdef PRINT_COSTS
			g_theWorld->SetColor(pos,  d->m_total_cost); 
#endif 
       return true;

    } else if (EntryCost(parent->m_pos, d->m_pos, d->m_entry_cost, is_zoc, entry)){

        Assert(entry != ASTAR_RETRY_DIRECTION);

#ifdef _DEBUG
if (entry == ASTAR_RETRY_DIRECTION) { 
    is_zoc = false;
    entry=ASTAR_CAN_ENTER;

    d->m_flags = 0;
    d->SetEntry(ASTAR_CAN_ENTER);
    d->SetZoc(false);
    d->SetExpanded(false);
    d->m_pos = pos;
    d->m_parent = parent;
    d->m_queue_idx = -1;

    d->m_past_cost = pc;
    d->m_entry_cost = 0.0;
    d->m_future_cost = EstimateFutureCost(d->m_pos, dest);
    d->m_total_cost = d->m_past_cost + d->m_entry_cost 
            + d->m_future_cost;
    EntryCost(parent->m_pos, d->m_pos, d->m_entry_cost, is_zoc, entry);

}
#endif

        d->SetZoc(is_zoc); 
        d->SetEntry(entry); 

        if (m_pretty_path)
            Astar::DecayOrtho(parent, point, d->m_entry_cost);

        d->m_future_cost = EstimateFutureCost(d->m_pos, dest);
        d->m_total_cost = d->m_past_cost + d->m_entry_cost 
            + d->m_future_cost;

#ifdef PRINT_COSTS
		g_theWorld->SetColor(pos,  d->m_total_cost); 
#endif 






        return true;
    } else {
            



        if (entry == ASTAR_RETRY_DIRECTION) { 
            d->SetEntry(ASTAR_RETRY_DIRECTION);
        } else { 
    		d->SetEntry(ASTAR_BLOCKED);
        }

        d->m_future_cost = k_ASTAR_BIG;
 
        d->m_total_cost = d->m_past_cost + d->m_entry_cost 
            + d->m_future_cost;
#ifdef PRINT_COSTS
			g_theWorld->SetColor(pos,  d->m_total_cost); 
#endif 
        return false;
    }
}

float UnitAstar::EstimateFutureCost(const MapPoint &pos, const MapPoint &dest)
{

	if (m_move_intersection & k_Unit_MovementType_Air_Bit) { 
															
		float air_dist = 90.0f * pos.NormalizedDistance(dest); 

#ifdef SUPER_DEBUG_HEURISTIC
			g_theWorld->SetColor(pos, sint32(air_dist)); 
#endif

		return air_dist;

	} else { 
		return Astar::EstimateFutureCost(pos, dest); 
	}
}
  



void UnitAstar::RecalcEntryCost(AstarPoint *parent, AstarPoint *node, float &new_entry_cost, 
    bool &new_is_zoc, ASTAR_ENTRY_TYPE &new_entry)
{

    
    

   
    bool can_be_zoc = true;
    bool can_enter;
    Cell *the_prev_cell = g_theWorld->AccessCell(parent->m_pos); 
    Cell *the_pos_cell = g_theWorld->AccessCell(node->m_pos);
    if (CheckMoveIntersection(parent->m_pos, node->m_pos, 
        the_prev_cell, the_pos_cell, new_entry_cost, new_is_zoc, 
          can_be_zoc, new_entry, can_enter)) return; 

    if (m_pretty_path)
        Astar::DecayOrtho(parent, node, new_entry_cost);
}


void UnitAstar::InitArmy(const Army &army, sint32 &nUnits, 
    uint32 &move_intersection,  uint32 &move_union, float &army_minmax_move)
{
    sint32 i; 

    move_intersection = 0xffffffff;     
    nUnits = army.Num(); 

    
    m_can_space_launch = TRUE;
    m_can_space_land = TRUE;
    m_is_zero_attack = true;
    m_army_can_expel_stealth = false;
    const UnitRecord *rec=NULL; 
    for (i=0; i<nUnits; i++) {         
        move_intersection &= army[i].GetMovementType();

        rec = army[i].GetDBRec();
        if (!rec->HasSpaceLaunch()) {
            m_can_space_launch = FALSE;
        }

        if (!rec->GetSpaceLand()) {
            m_can_space_land = FALSE;
        }

        if (rec->GetCanExpel()) { 
            m_army_can_expel_stealth = true;
        }

        if (0 < army[i].GetAttack()) {
            m_is_zero_attack = false;
        }


    }

    
    UnitDynamicArray* cargo_list;
    m_is_cargo_pod = FALSE;
    m_cargo_pod_intersection = 0xffffffff;
    sint32 j;
    sint32 num_carried;
    if (m_can_space_land) { 
        for (i=0; i<nUnits; i++) {  
            
            if (army[i].GetDBRec()->GetCargoPod()) { 

                
                
                m_is_cargo_pod = true;

                num_carried = army[i].GetNumCarried();
                cargo_list = army[i].GetData()->GetCargoList(); 

                for (j=0; j<num_carried; j++) { 
                    m_cargo_pod_intersection &= cargo_list->Get(j).GetMovementType();                    
                } 
            }
        } 
    }
    
    
    move_union = 0; 
    if (move_intersection == 0) {
        for (i=0; i<nUnits; i++) { 
            move_union |= army[i].GetMovementType();
        }
		if((move_union & k_Unit_MovementType_ShallowWater_Bit) &&
		   wonderutil_GetAllBoatsDeepWater(g_player[army.GetOwner()]->m_builtWonders)) {
			move_union |= k_Unit_MovementType_Sea_Bit;
		}
    } else {
		if((move_intersection & k_Unit_MovementType_ShallowWater_Bit) &&
		   wonderutil_GetAllBoatsDeepWater(g_player[army.GetOwner()]->m_builtWonders)) {
			move_intersection |= k_Unit_MovementType_Sea_Bit;
		}
	}
   
    army_minmax_move  = 1000000000; 

    for (i = 0; i < nUnits; i++) 
    { 
        army_minmax_move = std::min<float>
                            (army_minmax_move, static_cast<float>(army[i].GetMaxMovePoints())); 
    } 

    Assert (move_intersection || move_union); 
}

void UnitAstar::InitSearch(const MapPoint &start, const PLAYER_INDEX owner, 
    const MapPoint &dest, Path &good_path, bool &is_broken_path, Path &bad_path)
{

    bad_path.Clear();
    good_path.Clear();

    is_broken_path = false;

    
    m_owner = owner;
    m_is_robot = Player::IsThisPlayerARobot(owner);
    m_dest = dest;
    m_start = start;

    m_mask_alliance = g_player[owner]->GetMaskAlliance(); 

    
    m_max_dir = NOWHERE;
    
}


bool UnitAstar::EnterPathPoints(Path &a_path, MapPoint &old)
{
    static MapPoint pos; 
    float cost; 
    bool is_zoc;
	ASTAR_ENTRY_TYPE entry;

    a_path.Start(old); 
    a_path.Next(pos);  
    for ( ; !a_path.IsEnd(); a_path.Next(pos)) { 
        if (EntryCost(old, pos, cost, is_zoc, entry)) {
            if (entry!= ASTAR_CAN_ENTER) { 
                return false;
            } 
           old = pos; 
        } else {
           return false;
        }                 
    }      
    if (old != pos) { 
        if (EntryCost(old, pos, cost, is_zoc, entry)) {
            if (entry != ASTAR_CAN_ENTER) { 
                return false;
            } 
           old = pos;
        } else {
            return false;
        }
    }
    return true;
}

bool UnitAstar::FindVisionEdge(Path &a_path, MapPoint &old)
{
    static MapPoint pos; 

    a_path.Start(old); 
    a_path.Next(pos);  

    sint32 ao;
    CellUnitList *a = NULL;
    for ( ; !a_path.IsEnd(); a_path.Next(pos)) { 
        if (g_player[m_owner]->IsExplored(pos)) {
            a = g_theWorld->GetArmyPtr(pos); 
    
            if (a && (0 < a->Num())) {  
      			if(a->IsVisible(m_owner)) {
	    			ao = a->GetOwner(); 
		    		if (ao != m_owner) { 
                        continue; 
                    }
                }
            }

            old = pos; 
        } else {
           return false;
        }                 
    }      
    if (old != pos) { 
        if (g_player[m_owner]->IsExplored(pos)) {
            a = g_theWorld->GetArmyPtr(pos); 
    
            if (a && (0 < a->Num())) {  
      			if(a->IsVisible(m_owner)) {
	    			ao = a->GetOwner(); 
		    		if (ao != m_owner) { 
                        return true; 
                    }
                }
            }

            old = pos; 
        } else {
            return false;
        }
    }
    return true;
}

bool UnitAstar::FindBrokenPath(const MapPoint &start, const MapPoint &dest, 
    Path &good_path, Path &bad_path, float &total_cost)

{           
    StraightLine(start, dest, bad_path);

    Assert(0<bad_path.Num()); 
    if (bad_path.Num() < 1) { 
       return false;
    } 

    static MapPoint no_enter_pos;
    if (EnterPathPoints(bad_path, no_enter_pos)) 
    { 
        good_path = bad_path; 
        bad_path.Clear(); 
    } 
    else 
    {        
        sint32 nodes_opened = 0; 
        sint32 cutoff       = 2000000000;

        if (Astar::FindPath(start, no_enter_pos, good_path, total_cost, FALSE, 
                            cutoff, nodes_opened
                           )
           )
        {
            bad_path.ClipStartToCurrent();
        } 
        else 
        { 
            good_path.Clear(); 
        }
    } 

    return true;
}










sint32 UnitAstar::GetMaxDir(MapPoint &pos) const
{   
    return SOUTH; 
} 



bool UnitAstar::FindStraightPath(const MapPoint &start, const MapPoint &dest, 
                           Path &good_path, 
                           bool &is_broken_path, Path &bad_path, 
                           float &total_cost, bool no_bad_path, 
                           const sint32 cutoff, sint32 &nodes_opened)
{            
    StraightLine(start, dest, good_path);

    static MapPoint no_enter_pos;
    if (EnterPathPoints(good_path, no_enter_pos)) {   
       return true; 
    } 

    bool r; 
    static MapPoint  tmp_point;
   
    if (g_player[m_owner]->IsExplored(dest) && 
        g_theWorld->CanEnter(dest, m_move_intersection)) 
    {
       if ((start.x == no_enter_pos.x) && (no_enter_pos.y == start.y)) {
            
          r = Astar::FindPath(start, dest, good_path, total_cost, false,
              cutoff, nodes_opened);
          if (r) { 
             return true;
          } else if (no_bad_path) {
              return false;
          } else { 
               good_path.Clear();
               is_broken_path = true;
               StraightLine(start, dest, bad_path);
               return true;
          } 
        } else {
    
           StraightLine(start, no_enter_pos, good_path);
           if (EnterPathPoints(good_path, tmp_point)) { 
               r = Astar::FindPath(no_enter_pos, dest, bad_path, total_cost,
                   false, cutoff, nodes_opened); 
               if (r) { 
                   good_path.Concat(bad_path); 
                   bad_path.Clear();
                   return true;
               } else if (no_bad_path) {
                   return false;
               } else { 
                   is_broken_path = true;
                   good_path.Clear();
                   StraightLine(start, dest, bad_path);
                   return true;
               }
           } else { 
               r = Astar::FindPath(start, dest, good_path, total_cost, false,
                   cutoff, nodes_opened);
               if (r) {
                   bad_path.Clear();
                   return true;
               } else if (no_bad_path) {
                   return false;
               } else {
                   is_broken_path = true;
                   good_path.Clear();
                   StraightLine(start, dest, bad_path);
                   return true;
               }
           } 
        }
    } else if (no_bad_path) {
            return false;
    } else {
        
  
       is_broken_path = true;
       StraightLine(start, dest, bad_path);

       static MapPoint  vision_edge;
       FindVisionEdge(bad_path, vision_edge); 
       EnterPathPoints(bad_path, no_enter_pos);

       if ((no_enter_pos.x == vision_edge.x) && (no_enter_pos.y == vision_edge.y)) {
           if ((no_enter_pos.x == start.x) && (no_enter_pos.y == start.y)) {
               if (no_bad_path) {
                   return false;
               } else { 
                    good_path.Clear(); 
                    StraightLine(start, dest, bad_path); 
                    return true;
               }
           } else { 
                StraightLine(start, vision_edge, good_path);
                StraightLine(vision_edge, dest, bad_path); 
                if (EnterPathPoints(good_path, no_enter_pos)) { 
                    return true;
                } else { 
                    return Astar::FindPath(start, vision_edge, good_path, total_cost,  FALSE, 
                        cutoff, nodes_opened);
                }
           }
       } else {
           StraightLine(start, no_enter_pos, good_path); 
           Path tmp_path;
           if (((no_enter_pos.x == start.x) && (no_enter_pos.y == start.y)) ||
               !EnterPathPoints(good_path, tmp_point)) { 
               r = Astar::FindPath(start, vision_edge, good_path, total_cost,  false, 
                   cutoff, nodes_opened); 
               if (r) { 
                   StraightLine(vision_edge, dest, bad_path);
               } else if (no_bad_path) {
                   return false;
               } else { 
                   good_path.Clear(); 
                   StraightLine(start, dest, bad_path);
                   return true;
               }

               return r;
           } else {
               r = Astar::FindPath(no_enter_pos, vision_edge, tmp_path, 
                   total_cost,  false, cutoff, nodes_opened);
               if (r) {
                   good_path.Concat(tmp_path);
                   StraightLine(vision_edge, dest, bad_path);
               } else if (no_bad_path) {
                   return false;
               } else { 
                   good_path.Clear(); 
                   StraightLine(start, dest, bad_path);
                   return true;
               }
     
               return r;
           } 
       }
    }
}

bool UnitAstar::PretestDest_Enterable(const MapPoint &start, const MapPoint &dest)
{
	if (m_move_intersection) { 
         if(m_check_dest && !g_theWorld->HasCity(dest)) { 
            if(!g_theWorld->CanEnter(dest, m_move_intersection)) { 
                if (m_army.m_id == (0)) { 
                    return false;
                } else { 
                    CellUnitList tmp_transports;
                    return m_army.CanMoveIntoTransport(dest, tmp_transports);
                }
            } 
		}
    }

    return true;
}

bool UnitAstar::PretestDest_HasRoom(const MapPoint &start, const MapPoint &dest)
{
    CellUnitList *dest_army = g_theWorld->GetArmyPtr(dest);

    if (dest_army && m_check_dest) {
		if (m_owner ==  dest_army ->GetOwner()) { 
			if (k_MAX_ARMY_SIZE < (m_nUnits + g_theWorld->GetArmyPtr(dest)->Num())) { 
                if (!CanMoveIntoTransports(dest)) { 
                    return false;
			    }
			}
        } else if (m_is_zero_attack &&
            !g_theWorld->HasCity(dest) &&  
            dest_army->IsVisible(m_owner)) { 
            return false;
        } 
	}

    return true;
}

bool UnitAstar::PretestDest_SameLandContinent(const MapPoint &start, const MapPoint &dest)
{
    if (((m_move_intersection & k_Unit_MovementType_Land_Bit) ||
        (m_move_intersection & k_Unit_MovementType_Mountain_Bit)) 
        
        &&

        (FALSE == ((m_move_intersection & k_Unit_MovementType_Air_Bit) ||
            (m_move_intersection & k_Unit_MovementType_Space_Bit) ||
            (m_move_intersection & k_Unit_MovementType_Sea_Bit) ||
            (m_move_intersection & k_Unit_MovementType_ShallowWater_Bit))
         )
    )    
    { 
        sint16  start_cont_number;
        bool    start_is_land;
        g_theWorld->GetContinent(start, start_cont_number, start_is_land);

        sint16  dest_cont_number;
        bool    dest_is_land;
        g_theWorld->GetContinent(dest, dest_cont_number, dest_is_land);

        /// @todo More logical to return FALSE when any of the 2 is not land?
        ///       How about cities?
        if(start_is_land && dest_is_land) { 
            if(start_cont_number != dest_cont_number) {
                return false;
            }
        }
    }
    return true;
}


bool UnitAstar::PretestDest_SameWaterContinent(const MapPoint &start, const MapPoint &dest)
{
	if(
	   (
	    (m_move_intersection & k_Unit_MovementType_Sea_Bit) ||
	    (m_move_intersection & k_Unit_MovementType_Sea_Bit)
	   )
	   &&
	   (FALSE == 
	    (
	     (m_move_intersection & k_Unit_MovementType_Air_Bit) ||
	     (m_move_intersection & k_Unit_MovementType_Space_Bit) ||
	     (m_move_intersection & k_Unit_MovementType_Land_Bit) ||
	     (m_move_intersection & k_Unit_MovementType_Mountain_Bit)
	    )
	   )
	  )
	{
		bool   start_is_land;
		bool    dest_is_land;
		sint16 start_cont_number;
		sint16  dest_cont_number;

		g_theWorld->GetContinent(start, start_cont_number, start_is_land);
		g_theWorld->GetContinent(dest, dest_cont_number, dest_is_land);

		/// @todo More logical to return FALSE when any of the 2 is not water?
		///       How about cities?
		if(!start_is_land
		&& !dest_is_land
		&& (start_cont_number != dest_cont_number)
		){
			return false;
		}
	}
	return true; 
}

bool UnitAstar::PretestDest_ZocEnterable(const MapPoint &start, const MapPoint &dest)
{
    if (m_ignore_zoc) return true; 

    MapPoint neighbor;

    for (int i = 0; i <= SOUTH; i++) 
    { 
        if (!dest.GetNeighborPosition(WORLD_DIRECTION(i), neighbor)) continue;

       
        if ((start.x == neighbor.x) && (start.y == neighbor.y)) { 
            return true;
        } 

       
       

        if (!PretestDest_Enterable(start, neighbor)) continue; 

       
       CellUnitList * the_army = g_theWorld->GetArmyPtr(neighbor);
       if (the_army) {
    		if (m_owner !=  the_army->GetOwner()) continue; 

            if ((m_nUnits + the_army->Num()) <= k_MAX_ARMY_SIZE)  { 
                return true; 
            } else { 
                continue; 
            }
 	    }

        
       if(g_theWorld->HasCity(neighbor)) { 
           if (g_theWorld->GetCity(neighbor).GetOwner() == m_owner) { 
                return true; 
           } else { 
               continue; 
           } 

       }

       
       if (!g_theWorld->IsMoveZOC (m_owner, neighbor, dest, true)) { 
           return true; 
       } 

    }

    return false; 
}

bool UnitAstar::PretestDest(const MapPoint &start, const MapPoint &dest)
{
    if (!m_is_robot)
    {
        if (!g_player[m_owner]->IsExplored(dest))
        {
            return false;
        }
    }

    if (!PretestDest_Enterable         (start, dest)) return false;
    if (!PretestDest_HasRoom           (start, dest)) return false;
    if (!PretestDest_SameLandContinent (start, dest)) return false;
    if (!PretestDest_SameWaterContinent(start, dest)) return false;
    if (!PretestDest_ZocEnterable      (start, dest)) return false;

    return true;
}


bool UnitAstar::FindPath(Army &army,  MapPoint const & start,
    const PLAYER_INDEX owner, MapPoint const & dest, Path &good_path, 
    bool &is_broken_path, Path &bad_path, float &total_cost)
                           
{
    sint32 nUnits; 
    uint32 move_intersection; 
    uint32 move_union; 

    InitArmy (army, nUnits, move_intersection, move_union, m_army_minmax_move); 

	if (!Player::IsThisPlayerARobot(owner))
		m_pretty_path = true;

    sint32 cutoff       = 2000000000;
    sint32 nodes_opened = 0;
    bool result = FindPath(army, nUnits, move_intersection, move_union,  
       start, owner, dest, good_path, is_broken_path, bad_path, 
       total_cost, FALSE, FALSE, m_pretty_path, cutoff, nodes_opened, 
       TRUE, FALSE, TRUE);

	m_pretty_path = false;

	return result;

}

bool UnitAstar::FindPath(Army army,
                         sint32 nUnits,
                         uint32 move_intersection,
                         uint32 move_union,
                         const MapPoint & start,
                         const PLAYER_INDEX owner,
                         const MapPoint & dest,
                         Path &good_path, 
                         bool &is_broken_path,
                         Path &bad_path, 
                         float &total_cost,
                         const bool no_bad_path,
                         const bool check_rail_launcher,
                         const bool pretty_path,
                         const sint32 cutoff,
                         sint32 &nodes_opened,
                         const bool &check_dest,
                         const bool no_straight_lines, 
                         const bool check_units_in_cell)
{
    if (start == dest) {
        return false;
    }

	m_army_strength.Set_Army_Strength(army);
    m_army = army; 
    m_nUnits = nUnits; 
    m_move_intersection = move_intersection;
    m_move_union = move_union;
    m_pretty_path = pretty_path; 
	m_check_dest = check_dest;
    m_check_units_in_cell = check_units_in_cell; 

	m_can_be_cargo_podded = FALSE;
    
    m_check_rail_launchers = check_rail_launcher; 
	m_ignore_zoc = (m_army.m_id != (0) && m_army.IsIgnoresZOC());
    if (!check_units_in_cell)
        m_ignore_zoc = true;

    m_no_bad_path = no_bad_path; 

    InitSearch(start, owner, dest, good_path, is_broken_path, bad_path);

    if (!PretestDest(start, dest)) { 

        if (no_bad_path) { 
            ClearMem(); 
            return false; 
        } else { 
            bool find_ok =  FindBrokenPath(start, dest, good_path, bad_path, total_cost);
            is_broken_path = true;
            ClearMem();
            return find_ok;
        }
    }


    static MapPoint pos, old; 
    static MapPoint tmpa, tmpb;
    static MapPoint no_enter_pos; 

    bool r;

    Assert(VerifyMem());

    if (m_move_union != 0) { 
       
        if (Astar::FindPath(start, dest, good_path, total_cost, false, cutoff, nodes_opened)) { 
            ClearMem(); 
            return true; 
        } else if (no_bad_path) { 
            ClearMem(); 
            return false;
        }

    } else {



        if ((m_move_intersection & k_Unit_MovementType_Air_Bit)) { 
            if (!no_straight_lines) { 
                r = FindStraightPath(start, dest, good_path, is_broken_path, 
                                     bad_path, total_cost, no_bad_path, cutoff, nodes_opened);
                ClearMem(); 
                return r; 
            }
        }


        if (((m_move_intersection & k_Unit_MovementType_Sea_Bit)||
			 (m_move_intersection & k_Unit_MovementType_ShallowWater_Bit)) &&
			!(m_move_intersection & k_Unit_MovementType_Land_Bit)) {
			
			
			

            if (!m_check_dest || 
				(g_theWorld->CanEnter(dest, m_move_intersection) ||
				 (g_theWorld->HasCity(dest)
                 
				  
                  
                 ) )) { 
                
                if (!no_straight_lines) { 
                    StraightLine(start, dest, good_path);
                    if (EnterPathPoints(good_path, no_enter_pos)) {   
                        ClearMem(); 
                        return true;
                    } 
                }

                if (Astar::FindPath(start, dest, good_path, total_cost, FALSE, cutoff, nodes_opened)) { 
                    ClearMem();
                    return true;
                } else if (no_bad_path) { 
                    ClearMem();
                    return false;
                }
            } else if (no_bad_path) { 
                ClearMem(); 
                return false;
            }

            is_broken_path = true;
            r = FindBrokenPath(start, dest, good_path, bad_path, total_cost);
            ClearMem(); 
            return r; 
        }
    }

    if (!m_check_dest ||
		(
            (g_theWorld->CanEnter(dest, m_move_intersection)) ||
		    CanMoveIntoTransports(pos)
        )
       ) {
    
        if (Astar::FindPath(start, dest, good_path, total_cost, FALSE, cutoff, nodes_opened)) { 
            ClearMem();
            return true;
        } else if (no_bad_path) {
            ClearMem();
            return false;
        }
    } else if (no_bad_path) {
        ClearMem();
        return false;
    }

    is_broken_path = true;
 

    r = FindBrokenPath(start, dest, good_path, bad_path, total_cost);
    is_broken_path = true; // That should depend on r

    ClearMem(); 
    return r; 
}


bool UnitAstar::IsBeachLanding(const MapPoint &prev,
							   const MapPoint &pos,
							   const uint32 &m_move_intersection)
{
	
	if (m_move_intersection & k_Unit_MovementType_Air_Bit)
		return false;

	if ((m_move_intersection & k_Unit_MovementType_Sea_Bit ||
		 m_move_intersection & k_Unit_MovementType_ShallowWater_Bit) &&
		(m_move_intersection & k_Unit_MovementType_Land_Bit ||
		 m_move_intersection & k_Unit_MovementType_Mountain_Bit))
	{
		uint32  prev_move   = g_theWorld->GetMovementType(prev); 
		uint32  pos_move    = g_theWorld->GetMovementType(pos);

		if ((prev_move & k_Unit_MovementType_Sea_Bit || 
			 prev_move & k_Unit_MovementType_ShallowWater_Bit) &&
			(pos_move & k_Unit_MovementType_Land_Bit ||
			 pos_move & k_Unit_MovementType_Mountain_Bit))
			return TRUE;
	}
	return false;
}


void UnitAstar::ClearMem()
{
    m_move_union          = MARK_UNUSED;
    m_move_intersection   = MARK_UNUSED;
    m_max_dir             = MARK_UNUSED;
    m_mask_alliance       = MARK_UNUSED;
    m_dest.x              = MARK_UNUSED16;
    m_dest.y              = MARK_UNUSED16;
	m_start.x             = MARK_UNUSED16;
	m_start.y             = MARK_UNUSED16;
    m_owner               = MARK_UNUSED;
    m_nUnits              = MARK_UNUSED;
    m_army.m_id           = MARK_UNUSED;
    m_army_minmax_move    = -9999999.0f;
    m_can_space_launch    = 0x0;
    m_can_space_land      = 0x0;
	m_can_be_cargo_podded = MARK_UNUSED;
	m_army_strength       = Squad_Strength();
}


bool UnitAstar::VerifyMem() const
{
    if (m_move_union          == MARK_UNUSED)   return false;
    if (m_move_intersection   == MARK_UNUSED)   return false;
    if (m_max_dir             == MARK_UNUSED)   return false;
    if (m_mask_alliance       == MARK_UNUSED)   return false;
    if (m_dest.x              == MARK_UNUSED16) return false;
    if (m_dest.y              == MARK_UNUSED16) return false;
    if (m_start.x             == MARK_UNUSED16) return false;
    if (m_start.y             == MARK_UNUSED16) return false;
    if (m_owner               == MARK_UNUSED)   return false;
    if (m_nUnits              == MARK_UNUSED)   return false;
    if (m_army.m_id           == MARK_UNUSED)   return false;
    if (m_army_minmax_move    == -9999999.0f)   return false;
    if (m_can_space_launch    == MARK_UNUSED)   return false;
    if (m_can_space_land      == MARK_UNUSED)   return false;
    if (m_can_be_cargo_podded == MARK_UNUSED)   return false;

    return true;
}

bool UnitAstar::CheckIsDangerForPos(const MapPoint & myPos, const bool IsCivilian)
{
	Diplomat & diplomat = Diplomat::GetDiplomat(m_owner);
	ai::Regard baseRegard = NEUTRAL_REGARD;

	MapPoint neighbor;
	MapPoint start;
	m_army->GetPos(start);

	for (int i = 0; i <= SOUTH; i++)
	{
		if (!myPos.GetNeighborPosition(WORLD_DIRECTION(i), neighbor)) continue;

		if (neighbor == start || neighbor == m_dest)
		{
			continue;
		}
		
		//Check for hostile army
		CellUnitList *  the_army = g_theWorld->GetArmyPtr(neighbor);
		Unit            the_city = g_theWorld->GetCity(neighbor);

		if (the_army || the_city.IsValid()) 
		{
			PLAYER_INDEX owner = 
			    (the_army) ? the_army->GetOwner() : the_city.GetOwner();

			if (m_owner != owner)
			{
				baseRegard = diplomat.GetBaseRegard(owner);
				sint32 turnsatwar = AgreementMatrix::s_agreements.TurnsAtWar(m_owner, owner);
				if (baseRegard <= NEUTRAL_REGARD || turnsatwar >= 0)
				{
					if (the_city.IsValid()) //TO DO : Add conditions (in danger only if the_army not civilian
					{
					/*	DPRINTF(k_DBG_MAPANALYSIS, 
						("\t Danger for Pos (%3d,%3d) : City (%3d,%3d)\n",
						myPos.x,
						myPos.y,
						neighbor.x,
						neighbor.y));*/
						return true;
					}
				
					if (the_army->Num() > g_theWorld->GetArmyPtr(start)->Num() || IsCivilian)
					{
					/*	DPRINTF(k_DBG_MAPANALYSIS, 
						("\t Danger for Pos (%3d,%3d) : Bigger Army at (%3d,%3d)\n",
						myPos.x,
						myPos.y,
						neighbor.x,
						neighbor.y));
						return true;*/
					}
				}
			}
		}
	}
	/*DPRINTF(k_DBG_MAPANALYSIS, 
	("\t No Danger for Pos (%3d,%3d)\n",
	myPos.x,
	myPos.y));*/
	return false;
}


