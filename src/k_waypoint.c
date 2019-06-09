#include "k_waypoint.h"

#include "d_netcmd.h"
#include "p_local.h"
#include "p_tick.h"
#include "z_zone.h"
#include "k_bheap.h"

// The number of sparkles per waypoint connection in the waypoint visualisation
static const UINT32 SPARKLES_PER_CONNECTION = 16U;

// Some defaults for the size of the dynamically allocated sets for pathfinding. When the sets reach their max capacity
// they are reallocated to contain their old capacity plus these defines. Openset is smaller because most of the time
// the nodes will quickly be moved to closedset, closedset could contain an entire maps worth of waypoints.
// Additonally, in order to keep later calls to pathfinding quick and avoid reallocation, the highest size of the
// allocation is saved into a variable.
static const size_t OPENSET_BASE_SIZE    = 16U;
static const size_t CLOSEDSET_BASE_SIZE  = 256U;
static const size_t NODESARRAY_BASE_SIZE = 256U;

static waypoint_t **waypointheap = NULL;
static waypoint_t *firstwaypoint = NULL;
static size_t numwaypoints       = 0U;
static size_t numwaypointmobjs   = 0U;
static size_t baseopensetsize    = OPENSET_BASE_SIZE;
static size_t baseclosedsetsize  = CLOSEDSET_BASE_SIZE;
static size_t basenodesarraysize = NODESARRAY_BASE_SIZE;

/*--------------------------------------------------
	boolean K_GetWaypointIsShortcut(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
boolean K_GetWaypointIsShortcut(waypoint_t *waypoint)
{
	boolean waypointisshortcut = false;

	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_GetWaypointIsShortcut.\n");
	}
	else if (waypoint->mobj == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint mobj in K_GetWaypointIsShortcut.\n");
	}
	else
	{
		// TODO
	}

	return waypointisshortcut;
}

/*--------------------------------------------------
	boolean K_GetWaypointIsEnabled(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
boolean K_GetWaypointIsEnabled(waypoint_t *waypoint)
{
	boolean waypointisshortcut = true;

	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_GetWaypointIsEnabled.\n");
	}
	else if (waypoint->mobj == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint mobj in K_GetWaypointIsEnabled.\n");
	}
	else
	{
		// TODO
	}

	return waypointisshortcut;
}

/*--------------------------------------------------
	INT32 K_GetWaypointNextID(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
INT32 K_GetWaypointNextID(waypoint_t *waypoint)
{
	INT32 nextwaypointid = -1;

	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_GetWaypointNextID.\n");
	}
	else if (waypoint->mobj == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint mobj in K_GetWaypointNextID.\n");
	}
	else
	{
		nextwaypointid = waypoint->mobj->threshold;
	}

	return nextwaypointid;
}

/*--------------------------------------------------
	INT32 K_GetWaypointID(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
INT32 K_GetWaypointID(waypoint_t *waypoint)
{
	INT32 waypointid = -1;

	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_GetWaypointID.\n");
	}
	else if (waypoint->mobj == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint mobj in K_GetWaypointID.\n");
	}
	else
	{
		waypointid = waypoint->mobj->movecount;
	}

	return waypointid;
}

/*--------------------------------------------------
	void K_DebugWaypointsSpawnLine(waypoint_t *const waypoint1, waypoint_t *const waypoint2)

		Draw a debugging line between 2 waypoints

	Input Arguments:-
		waypoint1 - A waypoint to draw the line between
		waypoint2 - The other waypoint to draw the line between
--------------------------------------------------*/
static void K_DebugWaypointsSpawnLine(waypoint_t *const waypoint1, waypoint_t *const waypoint2)
{
	mobj_t *waypointmobj1, *waypointmobj2;
	mobj_t *spawnedmobj;
	fixed_t stepx, stepy, stepz;
	fixed_t x, y, z;
	INT32 n;
	UINT32 numofframes = 1; // If this was 0 it could divide by 0

	// Error conditions
	if (waypoint1 == NULL || waypoint2 == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_DebugWaypointsSpawnLine.\n");
		return;
	}
	if (waypoint1->mobj == NULL || waypoint2->mobj == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL mobj on waypoint in K_DebugWaypointsSpawnLine.\n");
		return;
	}
	if (cv_kartdebugwaypoints.value == 0)
	{
		CONS_Debug(DBG_GAMELOGIC, "In K_DebugWaypointsSpawnLine when kartdebugwaypoints is off.\n");
		return;
	}

	waypointmobj1 = waypoint1->mobj;
	waypointmobj2 = waypoint2->mobj;

	n = SPARKLES_PER_CONNECTION;
	numofframes = S_SPRK16 - S_SPRK1;

	// Draw the sparkles
	stepx = (waypointmobj2->x - waypointmobj1->x) / n;
	stepy = (waypointmobj2->y - waypointmobj1->y) / n;
	stepz = (waypointmobj2->z - waypointmobj1->z) / n;
	x = waypointmobj1->x;
	y = waypointmobj1->y;
	z = waypointmobj1->z;
	do
	{
		spawnedmobj = P_SpawnMobj(x, y, z, MT_SPARK);
		P_SetMobjState(spawnedmobj, S_SPRK1 + ((leveltime + n) % (numofframes + 1)));
		spawnedmobj->state->nextstate = S_NULL;
		spawnedmobj->state->tics = 1;
		x += stepx;
		y += stepy;
		z += stepz;
	} while (n--);
}

/*--------------------------------------------------
	void K_DebugWaypointsVisualise(void)

		See header file for description.
--------------------------------------------------*/
void K_DebugWaypointsVisualise(void)
{
	mobj_t *waypointmobj;
	mobj_t *debugmobj;
	waypoint_t *waypoint;
	waypoint_t *otherwaypoint;
	UINT32 i;

	if (waypointcap == NULL)
	{
		// No point putting a debug message here when it could easily happen when turning on the cvar in battle
		return;
	}
	if (cv_kartdebugwaypoints.value == 0)
	{
		// Going to nip this in the bud and say no drawing all this without the cvar, it's not particularly optimised
		return;
	}

	// Hunt through the waypointcap so we can show all waypoint mobjs and not just ones that were able to be graphed
	for (waypointmobj = waypointcap; waypointmobj != NULL; waypointmobj = waypointmobj->tracer)
	{
		waypoint = K_SearchWaypointHeapForMobj(waypointmobj);

		debugmobj = P_SpawnMobj(waypointmobj->x, waypointmobj->y, waypointmobj->z, MT_SPARK);
		P_SetMobjState(debugmobj, S_THOK);

		// There's a waypoint setup for this mobj! So draw that it's a valid waypoint and draw lines to its connections
		if (waypoint != NULL)
		{
			if (waypoint->numnextwaypoints == 0 && waypoint->numprevwaypoints == 0)
			{
				debugmobj->color = SKINCOLOR_RED;
			}
			else if (waypoint->numnextwaypoints == 0 || waypoint->numprevwaypoints == 0)
			{
				debugmobj->color = SKINCOLOR_ORANGE;
			}
			else
			{
				debugmobj->color = SKINCOLOR_BLUE;
			}

			// Valid waypoint, so draw lines of SPARKLES to its next or previous waypoints
			if (cv_kartdebugwaypoints.value == 1)
			{
				for (i = 0; i < waypoint->numnextwaypoints; i++)
				{
					if (waypoint->nextwaypoints[i] != NULL)
					{
						otherwaypoint = waypoint->nextwaypoints[i];
						K_DebugWaypointsSpawnLine(waypoint, otherwaypoint);
					}
				}
			}
			else if (cv_kartdebugwaypoints.value == 2)
			{
				for (i = 0; i < waypoint->numprevwaypoints; i++)
				{
					if (waypoint->prevwaypoints[i] != NULL)
					{
						otherwaypoint = waypoint->prevwaypoints[i];
						K_DebugWaypointsSpawnLine(waypoint, otherwaypoint);
					}
				}
			}
		}
		else
		{
			debugmobj->color = SKINCOLOR_RED;
		}
		debugmobj->state->tics = 1;
		debugmobj->state->nextstate = S_NULL;
	}
}

/*--------------------------------------------------
	static size_t K_GetOpensetBaseSize(void)

		Gets the base size the Openset hinary heap should have

	Input Arguments:-
		None

	Return:-
		The base size the Openset binary heap should have
--------------------------------------------------*/
static size_t K_GetOpensetBaseSize(void)
{
	size_t returnsize = 0;

	returnsize = baseopensetsize;

	return returnsize;
}

/*--------------------------------------------------
	static size_t K_GetClosedsetBaseSize(void)

		Gets the base size the Closedset heap should have

	Input Arguments:-
		None

	Return:-
		The base size the Closedset heap should have
--------------------------------------------------*/
static size_t K_GetClosedsetBaseSize(void)
{
	size_t returnsize = 0;

	returnsize = baseclosedsetsize;

	return returnsize;
}

/*--------------------------------------------------
	static size_t K_GetNodesArrayBaseSize(void)

		Gets the base size the Nodes array should have

	Input Arguments:-
		None

	Return:-
		The base size the Nodes array should have
--------------------------------------------------*/
static size_t K_GetNodesArrayBaseSize(void)
{
	size_t returnsize = 0;

	returnsize = basenodesarraysize;

	return returnsize;
}

/*--------------------------------------------------
	static void K_UpdateOpensetBaseSize(size_t newbaseopensetsize)

		Sets the new base size of the openset binary heap, if it is bigger than before.

	Input Arguments:-
		newbaseopensetsize - The size to try and set the base Openset size to

	Return:-
		None
--------------------------------------------------*/
static void K_UpdateOpensetBaseSize(size_t newbaseopensetsize)
{
	if (newbaseopensetsize > baseopensetsize)
	{
		baseopensetsize = newbaseopensetsize;
	}
}

/*--------------------------------------------------
	static void K_UpdateClosedsetBaseSize(size_t newbaseclosedsetsize)

		Sets the new base size of the closedset heap, if it is bigger than before.

	Input Arguments:-
		newbaseclosedsetsize - The size to try and set the base Closedset size to

	Return:-
		None
--------------------------------------------------*/
static void K_UpdateClosedsetBaseSize(size_t newbaseclosedsetsize)
{
	if (newbaseclosedsetsize > baseopensetsize)
	{
		baseclosedsetsize = newbaseclosedsetsize;
	}
}

/*--------------------------------------------------
	static void K_UpdateNodesArrayBaseSize(size_t newnodesarraysize)

		Sets the new base size of the nodes array, if it is bigger than before.

	Input Arguments:-
		newnodesarraysize - The size to try and set the base nodes array size to

	Return:-
		None
--------------------------------------------------*/
static void K_UpdateNodesArrayBaseSize(size_t newnodesarraysize)
{
	if (newnodesarraysize > basenodesarraysize)
	{
		basenodesarraysize = newnodesarraysize;
	}
}

/*--------------------------------------------------
	static UINT32 K_DistanceBetweenWaypoints(waypoint_t *const waypoint1, waypoint_t *const waypoint2)

		Gets the Euclidean distance between 2 waypoints by using their mobjs. Used for the heuristic.

	Input Arguments:-
		waypoint1 - The first waypoint
		waypoint2 - The second waypoint

	Return:-
		Euclidean distance between the 2 waypoints
--------------------------------------------------*/
static UINT32 K_DistanceBetweenWaypoints(waypoint_t *const waypoint1, waypoint_t *const waypoint2)
{
	UINT32 finaldist = UINT32_MAX;
	if (waypoint1 == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint1 in K_DistanceBetweenWaypoints.\n");
	}
	else if (waypoint2 == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint2 in K_DistanceBetweenWaypoints.\n");
	}
	else
	{
		const fixed_t xydist =
			P_AproxDistance(waypoint1->mobj->x - waypoint2->mobj->x, waypoint1->mobj->y - waypoint2->mobj->y);
		const fixed_t xyzdist = P_AproxDistance(xydist, waypoint1->mobj->z - waypoint2->mobj->z);
		finaldist = ((UINT32)xyzdist >> FRACBITS);
	}

	return finaldist;
}

/*--------------------------------------------------
	static UINT32 K_GetNodeFScore(pathfindnode_t *node)

		Gets the FScore of a node. The FScore is the GScore plus the HScore.

	Input Arguments:-
		node - The node to get the FScore of

	Return:-
		The FScore of the node.
--------------------------------------------------*/
static UINT32 K_GetNodeFScore(pathfindnode_t *node)
{
	UINT32 nodefscore = UINT32_MAX;

	if (node == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL node in K_GetNodeFScore.\n");
	}
	else
	{
		nodefscore = node->gscore + node->hscore;
	}

	return nodefscore;
}

/*--------------------------------------------------
	static boolean K_ClosedsetContainsNode(pathfindnode_t **closedset, pathfindnode_t *node, size_t closedsetcount)

		Checks whether the Closedset contains a node. Searches from the end to the start for speed reasons.

	Input Arguments:-
		closedset      - The closed set within the A* algorithm
		node           - The node to check is within the closed set
		closedsetcount - The current size of the closedset

	Return:-
		True if the node is in the closed set, false if it isn't
--------------------------------------------------*/
static boolean K_ClosedsetContainsNode(pathfindnode_t **closedset, pathfindnode_t *node, size_t closedsetcount)
{
	boolean nodeisinclosedset = false;

	if (closedset == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL closedset in K_SetContainsWaypoint.\n");
	}
	else if (node == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL node in K_SetContainsWaypoint.\n");
	}
	else
	{
		size_t i;
		// It is more likely that we'll find the node we are looking for from the end of the array
		// Yes, the for loop looks weird, remember that size_t is unsigned and we want to check 0, after it hits 0 it
		// will loop back up to SIZE_MAX
		for (i = closedsetcount - 1U; i < closedsetcount; i--)
		{
			if (closedset[i] == node)
			{
				nodeisinclosedset = true;
				break;
			}
		}
	}
	return nodeisinclosedset;
}

/*--------------------------------------------------
	static pathfindnode_t *K_NodesArrayContainsWaypoint(
		pathfindnode_t *nodesarray,
		waypoint_t* waypoint,
		size_t nodesarraycount)

		Checks whether the Nodes Array contains a node with a waypoint. Searches from the end to the start for speed
			reasons.

	Input Arguments:-
		nodesarray      - The nodes array within the A* algorithm
		waypoint        - The waypoint to check is within the nodes array
		nodesarraycount - The current size of the nodes array

	Return:-
		The pathfind node that has the waypoint if there is one. NULL if the waypoint is not in the nodes array.
--------------------------------------------------*/
static pathfindnode_t *K_NodesArrayContainsWaypoint(
	pathfindnode_t *nodesarray,
	waypoint_t* waypoint,
	size_t nodesarraycount)
{
	pathfindnode_t *foundnode = NULL;

	if (nodesarray == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL nodesarray in K_NodesArrayContainsWaypoint.\n");
	}
	else if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_NodesArrayContainsWaypoint.\n");
	}
	else
	{
		size_t i;
		// It is more likely that we'll find the node we are looking for from the end of the array
		// Yes, the for loop looks weird, remember that size_t is unsigned and we want to check 0, after it hits 0 it
		// will loop back up to SIZE_MAX
		for (i = nodesarraycount - 1U; i < nodesarraycount; i--)
		{
			if (nodesarray[i].waypoint == waypoint)
			{
				foundnode = &nodesarray[i];
				break;
			}
		}
	}
	return foundnode;
}

/*--------------------------------------------------
	static void K_NodeUpdateHeapIndex(void *const node, const size_t newheapindex)

		A callback for the Openset Binary Heap to be able to update the heapindex of the pathfindnodes when they are
			moved.

	Input Arguments:-
		node         - The node that has been updated, should be a pointer to a pathfindnode_t
		newheapindex - The new heapindex of the node.

	Return:-
		None
--------------------------------------------------*/
static void K_NodeUpdateHeapIndex(void *const node, const size_t newheapindex)
{
	if (node == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL node in K_NodeUpdateHeapIndex.\n");
	}
	else
	{
		pathfindnode_t *truenode = (pathfindnode_t*)node;
		truenode->heapindex = newheapindex;
	}
}

/*--------------------------------------------------
	static boolean K_ReconstructPath(path_t *const returnpath, pathfindnode_t *const destinationnode)

		From a pathfindnode that should be the destination, reconstruct a path from start to finish.

	Input Arguments:-
		returnpath      - The location of the path that is being created
		destinationnode - The node that is the destination from the pathfinding

	Return:-
		True if the path reconstruction was successful, false if it wasn't.
--------------------------------------------------*/
static boolean K_ReconstructPath(path_t *const returnpath, pathfindnode_t *const destinationnode)
{
	boolean reconstructsuccess = false;

	if (returnpath == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL returnpath in K_ReconstructPath.\n");
	}
	else if (destinationnode == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL destinationnode in K_ReconstructPath.\n");
	}
	else
	{
		size_t numnodes = 0U;
		pathfindnode_t *thisnode = destinationnode;

		// If the path we're placing our new path into already has data, free it
		if (returnpath->array != NULL)
		{
			Z_Free(returnpath->array);
			returnpath->numnodes = 0U;
			returnpath->totaldist = 0U;
		}

		// Do a fast check of how many nodes there are so we know how much space to allocate
		for (thisnode = destinationnode; thisnode; thisnode = thisnode->camefrom)
		{
			numnodes++;
		}

		if (numnodes > 0U)
		{
			// Allocate memory for the path
			returnpath->numnodes  = numnodes;
			returnpath->array     = Z_Calloc(numnodes * sizeof(pathfindnode_t), PU_STATIC, NULL);
			returnpath->totaldist = destinationnode->gscore;
			if (returnpath->array == NULL)
			{
				I_Error("K_ReconstructPath: Out of memory.");
			}

			// Put the nodes into the return array
			for (thisnode = destinationnode; thisnode; thisnode = thisnode->camefrom)
			{
				returnpath->array[numnodes - 1U] = *thisnode;
				// Correct the camefrom element to point to the previous element in the array instead
				if ((returnpath->array[numnodes - 1U].camefrom != NULL) && (numnodes > 1U))
				{
					returnpath->array[numnodes - 1U].camefrom = &returnpath->array[numnodes - 2U];
				}
				else
				{
					returnpath->array[numnodes - 1U].camefrom = NULL;
				}

				numnodes--;
			}

			reconstructsuccess = true;
		}
	}

	return reconstructsuccess;
}

/*--------------------------------------------------
	static boolean K_WaypointAStar(
		waypoint_t *const sourcewaypoint,
		waypoint_t *const destinationwaypoint,
		path_t *const     returnpath,
		const boolean     useshortcuts,
		const boolean     huntbackwards)

		From a source waypoint and destination waypoint, find the best path between them using the A* algorithm.

	Input Arguments:-
		sourcewaypoint      - The source waypoint to pathfind from
		destinationwaypoint - The destination waypoint to pathfind to
		returnpath          - The path to return to if the pathfinding was successful.
		useshortcuts        - Whether the pathfinding can use shortcut waypoints.
		huntbackwards       - Whether the pathfinding should hunt through previous or next waypoints

	Return:-
		True if a path was found between source and destination, false otherwise.
--------------------------------------------------*/
static boolean K_WaypointAStar(
	waypoint_t *const sourcewaypoint,
	waypoint_t *const destinationwaypoint,
	path_t *const     returnpath,
	const boolean     useshortcuts,
	const boolean     huntbackwards)
{
	boolean pathfindsuccess = false;

	if (sourcewaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL sourcewaypoint in K_WaypointAStar.\n");
	}
	else if (destinationwaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL destinationwaypoint in K_WaypointAStar.\n");
	}
	else if (returnpath == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL returnpath in K_WaypointAStar.\n");
	}
	else if (sourcewaypoint == destinationwaypoint)
	{
		// Source and destination waypoint are the same, we're already there
		// Just for simplicity's sake, create a single node on the destination and reconstruct path
		pathfindnode_t singlenode;
		singlenode.camefrom  = NULL;
		singlenode.waypoint  = destinationwaypoint;
		singlenode.heapindex = SIZE_MAX;
		singlenode.hscore    = 0U;
		singlenode.gscore    = 0U;

		pathfindsuccess = K_ReconstructPath(returnpath, &singlenode);
	}
	else if (((huntbackwards == false) && (sourcewaypoint->numnextwaypoints == 0))
		|| ((huntbackwards == true) && (sourcewaypoint->numprevwaypoints == 0)))
	{
		CONS_Debug(DBG_GAMELOGIC,
			"K_WaypointAStar: sourcewaypoint with ID %d has no next waypoint\n",
			K_GetWaypointID(sourcewaypoint));
	}
	else if (((huntbackwards == false) && (destinationwaypoint->numprevwaypoints == 0))
		|| ((huntbackwards == true) && (destinationwaypoint->numnextwaypoints == 0)))
	{
		CONS_Debug(DBG_GAMELOGIC,
			"K_WaypointAStar: destinationwaypoint with ID %d has no previous waypoint\n",
			K_GetWaypointID(destinationwaypoint));
	}
	else if ((K_GetWaypointIsEnabled(destinationwaypoint) == false) ||
			 (!useshortcuts &&(K_GetWaypointIsShortcut(destinationwaypoint) == true)))
	{
		// No path to the destination is possible
	}
	else
	{
		size_t         opensetcapacity      = K_GetOpensetBaseSize();
		size_t         nodesarraycapacity   = K_GetNodesArrayBaseSize();
		size_t         closedsetcapacity    = K_GetClosedsetBaseSize();
		bheapitem_t    poppeditem           = {};
		pathfindnode_t *currentnode         = NULL;
		pathfindnode_t *neighbournode       = NULL;
		bheap_t        openset              = {};
		pathfindnode_t **closedset          = Z_Calloc(closedsetcapacity * sizeof(pathfindnode_t*), PU_STATIC, NULL);
		pathfindnode_t *nodesarray          = Z_Calloc(nodesarraycapacity * sizeof(pathfindnode_t), PU_STATIC, NULL);
		pathfindnode_t *newnode             = NULL;
		size_t         closedsetcount       = 0U;
		size_t         nodesarraycount      = 0U;
		size_t         numcheckwaypoints    = 0U;
		waypoint_t     **checkwaypoints     = NULL;
		UINT32         *checkwaypointsdists = NULL;
		UINT32         tentativegscore      = UINT32_MAX;
		size_t         i                    = 0U;
		size_t         findopensetindex     = 0U;

		if (closedset == NULL || nodesarray == NULL)
		{
			I_Error("K_WaypointAStar: Out of memory.");
		}

		K_BHeapInit(&openset, opensetcapacity);

		newnode           = &nodesarray[0];
		newnode->waypoint = sourcewaypoint;
		newnode->hscore   = K_DistanceBetweenWaypoints(sourcewaypoint, destinationwaypoint);
		newnode->gscore   = 0U;
		newnode->camefrom = NULL;
		nodesarraycount++;

		K_BHeapPush(&openset, &nodesarray[0], K_GetNodeFScore(&nodesarray[0]), K_NodeUpdateHeapIndex);

		if (opensetcapacity != openset.capacity)
		{
			opensetcapacity = openset.capacity;
			K_UpdateOpensetBaseSize(opensetcapacity);
		}

		while (openset.count > 0)
		{
			K_BHeapPop(&openset, &poppeditem);
			currentnode = ((pathfindnode_t*)poppeditem.data);

			if (currentnode->waypoint == destinationwaypoint)
			{
				pathfindsuccess = K_ReconstructPath(returnpath, currentnode);
				break;
			}

			// The node is now placed into the closed set because it is evaluated
			if (closedsetcount >= closedsetcapacity)
			{
				K_UpdateClosedsetBaseSize(closedsetcapacity * 2);

				closedsetcapacity = K_GetClosedsetBaseSize();
				closedset         = Z_Realloc(closedset, closedsetcapacity * sizeof (pathfindnode_t*), PU_STATIC, NULL);

				if (closedset == NULL)
				{
					I_Error("K_WaypointAStar: Out of memory");
				}
			}
			closedset[closedsetcount] = currentnode;
			closedsetcount++;

			if (huntbackwards)
			{
				numcheckwaypoints   = currentnode->waypoint->numprevwaypoints;
				checkwaypoints      = currentnode->waypoint->prevwaypoints;
				checkwaypointsdists = currentnode->waypoint->prevwaypointdistances;
			}
			else
			{
				numcheckwaypoints   = currentnode->waypoint->numnextwaypoints;
				checkwaypoints      = currentnode->waypoint->nextwaypoints;
				checkwaypointsdists = currentnode->waypoint->nextwaypointdistances;
			}

			for (i = 0; i < numcheckwaypoints; i++)
			{
				tentativegscore = currentnode->gscore + checkwaypointsdists[i];

				// Can this double search be sped up at all? I feel like allocating and deallocating memory for nodes
				// constantly would be slower
				// Find if the neighbournode is already created first, if it is then check if it's in the closedset
				// If it's in the closedset, then skip as we don't need to check it again, if it isn't then see if the
				// new route from currentnode is faster to it and update accordingly
				neighbournode = K_NodesArrayContainsWaypoint(nodesarray, checkwaypoints[i], nodesarraycount);

				if (neighbournode != NULL)
				{
					// If the closedset contains the node, then it is already evaluated and doesn't need to be checked
					if (K_ClosedsetContainsNode(closedset, neighbournode, closedsetcount) != false)
					{
						continue;
					}

					if (tentativegscore < neighbournode->gscore)
					{
						neighbournode->camefrom = currentnode;
						neighbournode->gscore = tentativegscore;

						findopensetindex = K_BHeapContains(&openset, neighbournode, neighbournode->heapindex);
						if (findopensetindex != SIZE_MAX)
						{
							K_UpdateBHeapItemValue(&openset.array[findopensetindex], K_GetNodeFScore(neighbournode));
						}
						else
						{
							// What??? How is this node NOT in the openset???
							// A node should always be in either the openset or closedset
							CONS_Debug(DBG_GAMELOGIC, "Node unexpectedly not in openset in K_WaypointAStar.\n");
							K_BHeapPush(&openset, neighbournode, K_GetNodeFScore(neighbournode), K_NodeUpdateHeapIndex);
						}
					}
				}
				else
				{
					// Don't process this waypoint if it's not traversable
					if ((K_GetWaypointIsEnabled(checkwaypoints[i]) == false)
						|| (!useshortcuts && K_GetWaypointIsShortcut(checkwaypoints[i]) == true))
					{
						continue;
					}

					// reallocate the nodesarray if needed
					if (nodesarraycount >= nodesarraycapacity)
					{
						K_UpdateNodesArrayBaseSize(nodesarraycapacity * 2);
						nodesarraycapacity = K_GetNodesArrayBaseSize();
						nodesarray = Z_Realloc(nodesarray, nodesarraycapacity * sizeof(pathfindnode_t), PU_STATIC, NULL);

						if (nodesarray == NULL)
						{
							I_Error("K_WaypointAStar: Out of memory");
						}
					}

					// There is currently no node created for this waypoint, so make one
					newnode            = &nodesarray[nodesarraycount];
					newnode->camefrom  = currentnode;
					newnode->heapindex = SIZE_MAX;
					newnode->gscore    = tentativegscore;
					newnode->hscore    = K_DistanceBetweenWaypoints(checkwaypoints[i], destinationwaypoint);
					newnode->waypoint  = checkwaypoints[i];
					nodesarraycount++;

					// because there was no node for the waypoint, it's also not in the openset, add it
					K_BHeapPush(&openset, newnode, K_GetNodeFScore(newnode), K_NodeUpdateHeapIndex);
				}
			}
		}

		// Clean up the memory
		K_BHeapFree(&openset);
		Z_Free(closedset);
		Z_Free(nodesarray);
	}

	return pathfindsuccess;
}

/*--------------------------------------------------
	boolean K_PathfindToWaypoint(
		waypoint_t *const sourcewaypoint,
		waypoint_t *const destinationwaypoint,
		path_t *const     returnpath,
		const boolean     useshortcuts,
		const boolean     huntbackwards)

		See header file for description.
--------------------------------------------------*/
boolean K_PathfindToWaypoint(
	waypoint_t *const sourcewaypoint,
	waypoint_t *const destinationwaypoint,
	path_t *const     returnpath,
	const boolean     useshortcuts,
	const boolean     huntbackwards)
{
	boolean pathfound    = false;

	if (sourcewaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL sourcewaypoint in K_PathfindToWaypoint.\n");
	}
	else if (destinationwaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL destinationwaypoint in K_PathfindToWaypoint.\n");
	}
	else if (((huntbackwards == false) && (sourcewaypoint->numnextwaypoints == 0))
		|| ((huntbackwards == true) && (sourcewaypoint->numprevwaypoints == 0)))
	{
		CONS_Debug(DBG_GAMELOGIC,
			"K_PathfindToWaypoint: sourcewaypoint with ID %d has no next waypoint\n",
			K_GetWaypointID(sourcewaypoint));
	}
	else if (((huntbackwards == false) && (destinationwaypoint->numprevwaypoints == 0))
		|| ((huntbackwards == true) && (destinationwaypoint->numnextwaypoints == 0)))
	{
		CONS_Debug(DBG_GAMELOGIC,
			"K_PathfindToWaypoint: destinationwaypoint with ID %d has no previous waypoint\n",
			K_GetWaypointID(destinationwaypoint));
	}
	else
	{
		pathfound = K_WaypointAStar(sourcewaypoint, destinationwaypoint, returnpath, useshortcuts, huntbackwards);
	}

	return pathfound;
}

/*--------------------------------------------------
	waypoint_t *K_GetNextWaypointToDestination(
		waypoint_t *const sourcewaypoint,
		waypoint_t *const destinationwaypoint,
		const boolean     useshortcuts,
		const boolean     huntbackwards)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_GetNextWaypointToDestination(
	waypoint_t *const sourcewaypoint,
	waypoint_t *const destinationwaypoint,
	const boolean     useshortcuts,
	const boolean     huntbackwards)
{
	waypoint_t *nextwaypoint = NULL;

	if (sourcewaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL sourcewaypoint in K_GetNextWaypointToDestination.\n");
	}
	else if (destinationwaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL destinationwaypoint in K_GetNextWaypointToDestination.\n");
	}
	else if (sourcewaypoint == destinationwaypoint)
	{
		// Source and destination waypoint are the same, we're already there
		nextwaypoint = destinationwaypoint;
	}
	else if (((huntbackwards == false) && (sourcewaypoint->numnextwaypoints == 0))
		|| ((huntbackwards == true) && (sourcewaypoint->numprevwaypoints == 0)))
	{
		CONS_Debug(DBG_GAMELOGIC,
			"K_GetNextWaypointToDestination: sourcewaypoint with ID %d has no next waypoint\n",
			K_GetWaypointID(sourcewaypoint));
	}
	else if (((huntbackwards == false) && (destinationwaypoint->numprevwaypoints == 0))
		|| ((huntbackwards == true) && (destinationwaypoint->numnextwaypoints == 0)))
	{
		CONS_Debug(DBG_GAMELOGIC,
			"K_GetNextWaypointToDestination: destinationwaypoint with ID %d has no previous waypoint\n",
			K_GetWaypointID(destinationwaypoint));
	}
	else
	{
		// If there is only 1 next waypoint it doesn't matter if it's a shortcut
		if ((huntbackwards == false) && sourcewaypoint->numnextwaypoints == 1)
		{
			nextwaypoint = sourcewaypoint->nextwaypoints[0];
		}
		else if ((huntbackwards == true) && sourcewaypoint->numprevwaypoints == 1)
		{
			nextwaypoint = sourcewaypoint->prevwaypoints[0];
		}
		else
		{
			path_t pathtowaypoint;
			boolean pathfindsuccess =
				K_WaypointAStar(sourcewaypoint, destinationwaypoint, &pathtowaypoint, useshortcuts, huntbackwards);

			if (pathfindsuccess)
			{
				// A direct path to the destination has been found.
				if (pathtowaypoint.numnodes > 1)
				{
					nextwaypoint = pathtowaypoint.array[1].waypoint;
				}
				else
				{
					// Shouldn't happen, as this is the source waypoint.
					CONS_Debug(DBG_GAMELOGIC, "Only one waypoint pathfound in K_GetNextWaypointToDestination.\n");
					nextwaypoint = pathtowaypoint.array[0].waypoint;
				}

				Z_Free(pathtowaypoint.array);
			}
			else
			{
				size_t     i                   = 0U;
				waypoint_t **nextwaypointlist  = NULL;
				size_t     numnextwaypoints    = 0U;
				boolean waypointisenabled      = true;
				boolean waypointisshortcut     = false;

				if (huntbackwards)
				{
					nextwaypointlist = sourcewaypoint->prevwaypoints;
					numnextwaypoints = sourcewaypoint->numprevwaypoints;
				}
				else
				{
					nextwaypointlist = sourcewaypoint->nextwaypoints;
					numnextwaypoints = sourcewaypoint->numnextwaypoints;
				}

				// No direct path to the destination has been found, choose a next waypoint from what is available
				// 1. If shortcuts are allowed, pick the first shortcut path that is enabled
				// 2. If shortcuts aren't allowed, or there are no shortcuts, pick the first enabled waypoint
				// 3. If there's no waypoints enabled, then nothing can be done and there is no next waypoint
				if (useshortcuts)
				{
					for (i = 0U; i < numnextwaypoints; i++)
					{
						waypointisenabled  = K_GetWaypointIsEnabled(nextwaypointlist[i]);
						waypointisshortcut = K_GetWaypointIsShortcut(nextwaypointlist[i]);

						if (waypointisenabled && waypointisshortcut)
						{
							nextwaypoint = nextwaypointlist[i];
							break;
						}
					}
				}

				if (nextwaypoint == NULL)
				{
					for (i = 0U; i < numnextwaypoints; i++)
					{
						waypointisenabled  = K_GetWaypointIsEnabled(nextwaypointlist[i]);

						if (waypointisenabled)
						{
							nextwaypoint = nextwaypointlist[i];
							break;
						}
					}
				}
			}
		}
	}

	return nextwaypoint;
}

/*--------------------------------------------------
	boolean K_CheckWaypointForMobj(waypoint_t *const waypoint, void *const mobjpointer)

		Compares a waypoint's mobj and a void pointer that *should* point to an mobj. Intended for use with the
		K_SearchWaypoint functions ONLY. No, it is not my responsibility to make sure the pointer you sent in is
		actually an mobj.

	Input Arguments:-
		waypoint    - The waypoint that is currently being compared against
		mobjpointer - A pointer that should be to an mobj to check with the waypoint for matching

  Return:-
		The waypoint that uses that mobj, NULL if it wasn't found, NULL if it isn't an MT_WAYPOINT
--------------------------------------------------*/
static boolean K_CheckWaypointForMobj(waypoint_t *const waypoint, void *const mobjpointer)
{
	boolean mobjsmatch = false;

	// Error Conditions
	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_CheckWaypointForMobj.\n");
	}
	else if (waypoint->mobj == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "Waypoint has NULL mobj in K_CheckWaypointForMobj.\n");
	}
	else if (mobjpointer == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL mobjpointer in K_CheckWaypointForMobj.\n");
	}
	else
	{
		mobj_t *mobj = (mobj_t *)mobjpointer;

		if (P_MobjWasRemoved(mobj))
		{
			CONS_Debug(DBG_GAMELOGIC, "Mobj Was Removed in K_CheckWaypointForMobj");
		}
		else if (mobj->type != MT_WAYPOINT)
		{
			CONS_Debug(DBG_GAMELOGIC, "Non MT_WAYPOINT mobj in K_CheckWaypointForMobj. Type=%d.\n", mobj->type);
		}
		else
		{
			// All that error checking for 3 lines :^)
			if (waypoint->mobj == mobj)
			{
				mobjsmatch = true;
			}
		}
	}

	return mobjsmatch;
}

/*--------------------------------------------------
	waypoint_t *K_TraverseWaypoints(
		waypoint_t *waypoint,
		boolean    (*conditionalfunc)(waypoint_t *const, void *const),
		void       *const condition,
		boolean    *const visitedarray)

		Searches through the waypoint list for a waypoint that matches a condition, just does a simple flood search
		of the graph with no pathfinding

	Input Arguments:-
		waypoint        - The waypoint that is currently being checked, goes through nextwaypoints after this one
		conditionalfunc - The function that will be used to check a waypoint against condition
		condition       - the condition being checked by conditionalfunc
		visitedarray    - An array of booleans that let us know if a waypoint has already been checked, marked to true
			when one is, so we don't repeat going down a path. Cannot be changed to a different pointer

  Return:-
		The waypoint that uses that mobj, NULL if it wasn't found, NULL if it isn't an MT_WAYPOINT
--------------------------------------------------*/
static waypoint_t *K_TraverseWaypoints(
	waypoint_t *waypoint,
	boolean    (*conditionalfunc)(waypoint_t *const, void *const),
	void       *const condition,
	boolean    *const visitedarray)
{
	waypoint_t *foundwaypoint = NULL;

	// Error conditions
	if (condition == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL condition in K_TraverseWaypoints.\n");
	}
	else if (conditionalfunc == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL conditionalfunc in K_TraverseWaypoints.\n");
	}
	else if (visitedarray == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL visitedarray in K_TraverseWaypoints.\n");
	}
	else
	{

searchwaypointstart:
		if (waypoint == NULL)
		{
			CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_TraverseWaypoints.\n");
		}
		else
		{
			// If we've already visited this waypoint, we've already checked the next waypoints, no point continuing
			if (visitedarray[waypoint->id] != true)
			{
				// Mark this waypoint as being visited
				visitedarray[waypoint->id] = true;

				if (conditionalfunc(waypoint, condition) == true)
				{
					foundwaypoint = waypoint;
				}
				else
				{
					// If this waypoint only has one next waypoint, set the waypoint to be the next one and jump back
					// to the start, this is to avoid going too deep into the stack where we can
					// Yes this is a horrible horrible goto, but the alternative is a do while loop with an extra
					// variable, which is slightly more confusing. This is probably the fastest and least confusing
					// option that keeps this functionality
					if (waypoint->numnextwaypoints == 1 && waypoint->nextwaypoints[0] != NULL)
					{
						waypoint = waypoint->nextwaypoints[0];
						goto searchwaypointstart;
					}
					else if (waypoint->numnextwaypoints != 0)
					{
						// The nesting here is a bit nasty, but it's better than potentially a lot of function calls on
						// the stack, and another function would be very small in this case
						UINT32 i;
						// For each next waypoint, Search through it's path continuation until we hopefully find the one
						// we're looking for
						for (i = 0; i < waypoint->numnextwaypoints; i++)
						{
							if (waypoint->nextwaypoints[i] != NULL)
							{
								foundwaypoint = K_TraverseWaypoints(waypoint->nextwaypoints[i], conditionalfunc,
									condition, visitedarray);

								if (foundwaypoint != NULL)
								{
									break;
								}
							}
						}
					}
					else
					{
						// No next waypoints, this function will be returned from
					}

				}
			}
		}
	}

	return foundwaypoint;
}

/*--------------------------------------------------
	waypoint_t *K_SearchWaypointGraph(
		boolean (*conditionalfunc)(waypoint_t *const, void *const),
		void    *const condition)

		Searches through the waypoint graph for a waypoint that matches the conditional

	Input Arguments:-
		conditionalfunc - The function that will be used to check a waypoint against condition
		condition       - the condition being checked by conditionalfunc

  Return:-
		The waypoint that uses that mobj, NULL if it wasn't found, NULL if it isn't an MT_WAYPOINT
--------------------------------------------------*/
static waypoint_t *K_SearchWaypointGraph(
	boolean (*conditionalfunc)(waypoint_t *const, void *const),
	void    *const condition)
{
	boolean *visitedarray = NULL;
	waypoint_t *foundwaypoint = NULL;

	// Error conditions
	if (condition == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL condition in K_SearchWaypointGraph.\n");
	}
	else if (conditionalfunc == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL conditionalfunc in K_SearchWaypointGraph.\n");
	}
	else if (firstwaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_SearchWaypointsForMobj called when no first waypoint.\n");
	}
	else
	{
		visitedarray = Z_Calloc(numwaypoints * sizeof(boolean), PU_STATIC, NULL);
		foundwaypoint = K_TraverseWaypoints(firstwaypoint, conditionalfunc, condition, visitedarray);
		Z_Free(visitedarray);
	}

	return foundwaypoint;
}

/*--------------------------------------------------
	waypoint_t *K_SearchWaypointGraphForMobj(mobj_t * const mobj)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_SearchWaypointGraphForMobj(mobj_t *const mobj)
{
	waypoint_t *foundwaypoint = NULL;

	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL mobj in K_SearchWaypointGraphForMobj.\n");
	}
	else if (mobj->type != MT_WAYPOINT)
	{
		CONS_Debug(DBG_GAMELOGIC, "Non MT_WAYPOINT mobj in K_SearchWaypointGraphForMobj. Type=%d.\n", mobj->type);
	}
	else
	{
		foundwaypoint = K_SearchWaypointGraph(K_CheckWaypointForMobj, (void *)mobj);
	}

	return foundwaypoint;
}

/*--------------------------------------------------
	waypoint_t *K_SearchWaypointHeap(
		boolean (*conditionalfunc)(waypoint_t *const, void *const),
		void    *const condition)

		Searches through the waypoint heap for a waypoint that matches the conditional

	Input Arguments:-
		conditionalfunc - The function that will be used to check a waypoint against condition
		condition       - the condition being checked by conditionalfunc

  Return:-
		The waypoint that uses that mobj, NULL if it wasn't found, NULL if it isn't an MT_WAYPOINT
--------------------------------------------------*/
static waypoint_t *K_SearchWaypointHeap(
	boolean (*conditionalfunc)(waypoint_t *const, void *const),
	void    *const condition)
{
	UINT32 i = 0;
	waypoint_t *foundwaypoint = NULL;

	// Error conditions
	if (condition == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL condition in K_SearchWaypointHeap.\n");
	}
	else if (conditionalfunc == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL conditionalfunc in K_SearchWaypointHeap.\n");
	}
	else if (waypointheap == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_SearchWaypointHeap called when no waypointheap.\n");
	}
	else
	{
		// Simply search through the waypointheap for the waypoint which matches the condition. Much simpler when no
		// pathfinding is needed. Search up to numwaypoints and NOT numwaypointmobjs as numwaypoints is the real number of
		// waypoints setup in the heap while numwaypointmobjs ends up being the capacity
		for (i = 0; i < numwaypoints; i++)
		{
			if (conditionalfunc(waypointheap[i], condition) == true)
			{
				foundwaypoint = waypointheap[i];
				break;
			}
		}
	}

	return foundwaypoint;
}

/*--------------------------------------------------
	waypoint_t *K_SearchWaypointHeapForMobj(mobj_t *const mobj)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_SearchWaypointHeapForMobj(mobj_t *const mobj)
{
	waypoint_t *foundwaypoint = NULL;

	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL mobj in K_SearchWaypointHeapForMobj.\n");
	}
	else if (mobj->type != MT_WAYPOINT)
	{
		CONS_Debug(DBG_GAMELOGIC, "Non MT_WAYPOINT mobj in K_SearchWaypointHeapForMobj. Type=%d.\n", mobj->type);
	}
	else
	{
		foundwaypoint = K_SearchWaypointHeap(K_CheckWaypointForMobj, (void *)mobj);
	}

	return foundwaypoint;
}

/*--------------------------------------------------
	static void K_AddPrevToWaypoint(waypoint_t *const waypoint, waypoint_t *const prevwaypoint)

		Adds another waypoint to a waypoint's previous waypoint list, this needs to be done like this because there is no
		way to identify previous waypoints from just IDs, so we need to reallocate the memory for every previous waypoint

	Input Arguments:-
		waypoint     - The waypoint which is having its previous waypoint list added to
		prevwaypoint - The waypoint which is being added to the previous waypoint list

	Return:-
		Pointer to waypoint_t for the rest of the waypoint data to be placed into
--------------------------------------------------*/
static void K_AddPrevToWaypoint(waypoint_t *const waypoint, waypoint_t *const prevwaypoint)
{
	// Error conditions
	if (waypoint == NULL)
	{
		CONS_Debug(DBG_SETUP, "NULL waypoint in K_AddPrevToWaypoint.\n");
	}
	else if (prevwaypoint == NULL)
	{
		CONS_Debug(DBG_SETUP, "NULL prevwaypoint in K_AddPrevToWaypoint.\n");
	}
	else
	{
		waypoint->numprevwaypoints++;
		waypoint->prevwaypoints =
			Z_Realloc(waypoint->prevwaypoints, waypoint->numprevwaypoints * sizeof(waypoint_t *), PU_LEVEL, NULL);

		if (!waypoint->prevwaypoints)
		{
			I_Error("K_AddPrevToWaypoint: Failed to reallocate memory for previous waypoints.");
		}

		waypoint->prevwaypointdistances =
			Z_Realloc(waypoint->prevwaypointdistances, waypoint->numprevwaypoints * sizeof(fixed_t), PU_LEVEL, NULL);

		if (!waypoint->prevwaypointdistances)
		{
			I_Error("K_AddPrevToWaypoint: Failed to reallocate memory for previous waypoint distances.");
		}

		waypoint->prevwaypoints[waypoint->numprevwaypoints - 1] = prevwaypoint;
	}
}

/*--------------------------------------------------
	static waypoint_t *K_NewWaypoint(mobj_t *mobj)

		Creates memory for a new waypoint

	Input Arguments:-
		mobj - The map object that this waypoint is represented by

	Return:-
		Pointer to waypoint_t for the rest of the waypoint data to be placed into
--------------------------------------------------*/
static waypoint_t *K_NewWaypoint(mobj_t *const mobj)
{
	waypoint_t *waypoint = NULL;

	// Error conditions
	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_SETUP, "NULL mobj in K_NewWaypoint.\n");
	}
	else if (waypointheap == NULL)
	{
		CONS_Debug(DBG_SETUP, "NULL waypointheap in K_NewWaypoint.\n");
	}
	else
	{
		// Each made waypoint is placed directly into the waypoint heap to be able to search it during creation
		waypointheap[numwaypoints] = Z_Calloc(sizeof(waypoint_t), PU_LEVEL, NULL);
		waypoint = waypointheap[numwaypoints];
		// numwaypoints is incremented later when waypoint->id is set

		if (waypoint == NULL)
		{
			I_Error("K_NewWaypoint: Failed to allocate memory for waypoint.");
		}

		P_SetTarget(&waypoint->mobj, mobj);
		waypoint->id = numwaypoints++;
	}

	return waypoint;
}

/*--------------------------------------------------
	static waypoint_t *K_MakeWaypoint(mobj_t *const mobj)

		Make a new waypoint from a map object. Setups up most of the data for it, and allocates most memory
		Remaining creation is handled in K_SetupWaypoint

	Input Arguments:-
		mobj - The map object that this waypoint is represented by

	Return:-
		Pointer to the setup waypoint, NULL if one was not setup
--------------------------------------------------*/
static waypoint_t *K_MakeWaypoint(mobj_t *const mobj)
{
	waypoint_t *madewaypoint = NULL;
	mobj_t *otherwaypointmobj = NULL;

	// Error conditions
	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_SETUP, "NULL mobj in K_MakeWaypoint.\n");
	}
	else if (waypointcap == NULL)
	{
		CONS_Debug(DBG_SETUP, "K_MakeWaypoint called with NULL waypointcap.\n");
	}
	else
	{
		madewaypoint = K_NewWaypoint(mobj);

		// Go through the other waypoint mobjs in the map to find out how many waypoints are after this one
		for (otherwaypointmobj = waypointcap; otherwaypointmobj != NULL; otherwaypointmobj = otherwaypointmobj->tracer)
		{
			// threshold = next waypoint id, movecount = my id
			if (mobj->threshold == otherwaypointmobj->movecount)
			{
				madewaypoint->numnextwaypoints++;
			}
		}

		// No next waypoints
		if (madewaypoint->numnextwaypoints != 0)
		{
			// Allocate memory to hold enough pointers to all of the next waypoints
			madewaypoint->nextwaypoints =
				Z_Calloc(madewaypoint->numnextwaypoints * sizeof(waypoint_t *), PU_LEVEL, NULL);
			if (madewaypoint->nextwaypoints == NULL)
			{
				I_Error("K_MakeWaypoint: Out of Memory allocating next waypoints.");
			}
			madewaypoint->nextwaypointdistances =
				Z_Calloc(madewaypoint->numnextwaypoints * sizeof(fixed_t), PU_LEVEL, NULL);
			if (madewaypoint->nextwaypointdistances == NULL)
			{
				I_Error("K_MakeWaypoint: Out of Memory allocating next waypoint distances.");
			}
		}
	}

	return madewaypoint;
}

/*--------------------------------------------------
	static waypoint_t *K_SetupWaypoint(mobj_t *const mobj)

		Either gets an already made waypoint, or sets up a new waypoint for an mobj,
		including next and previous waypoints

	Input Arguments:-
		mobj - The map object that this waypoint is represented by

	Return:-
		Pointer to the setup waypoint, NULL if one was not setup
--------------------------------------------------*/
static waypoint_t *K_SetupWaypoint(mobj_t *const mobj)
{
	waypoint_t *thiswaypoint = NULL;

	// Error conditions
	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_SETUP, "NULL mobj in K_SetupWaypoint.\n");
	}
	else if (mobj->type != MT_WAYPOINT)
	{
		CONS_Debug(DBG_SETUP, "Non MT_WAYPOINT mobj in K_SetupWaypoint. Type=%d.\n", mobj->type);
	}
	else if (waypointcap == NULL)
	{
		CONS_Debug(DBG_SETUP, "K_SetupWaypoint called with NULL waypointcap.\n");
	}
	else
	{
		// If we have waypoints already created, search through them first to see if this mobj is already added.
		if (firstwaypoint != NULL)
		{
			thiswaypoint = K_SearchWaypointHeapForMobj(mobj);
		}

		// The waypoint hasn't already been made, so make it
		if (thiswaypoint == NULL)
		{
			mobj_t *otherwaypointmobj = NULL;
			UINT32 nextwaypointindex = 0;

			thiswaypoint = K_MakeWaypoint(mobj);

			if (thiswaypoint != NULL)
			{
				// Temporarily set the first waypoint to be the first waypoint we setup, this is so that we can search
				// through them as they're made and added to the linked list
				if (firstwaypoint == NULL)
				{
					firstwaypoint = thiswaypoint;
				}

				if (thiswaypoint->numnextwaypoints > 0)
				{
					waypoint_t *nextwaypoint = NULL;
					fixed_t nextwaypointdistance = 0;
					// Go through the waypoint mobjs to setup the next waypoints and make this waypoint know they're its
					// next. I kept this out of K_MakeWaypoint so the stack isn't gone down as deep
					for (otherwaypointmobj = waypointcap;
						otherwaypointmobj != NULL;
						otherwaypointmobj = otherwaypointmobj->tracer)
					{
						// threshold = next waypoint id, movecount = my id
						if (mobj->threshold == otherwaypointmobj->movecount)
						{
							nextwaypoint = K_SetupWaypoint(otherwaypointmobj);
							nextwaypointdistance = K_DistanceBetweenWaypoints(thiswaypoint, nextwaypoint);
							thiswaypoint->nextwaypoints[nextwaypointindex] = nextwaypoint;
							thiswaypoint->nextwaypointdistances[nextwaypointindex] = nextwaypointdistance;
							K_AddPrevToWaypoint(nextwaypoint, thiswaypoint);
							nextwaypoint->prevwaypointdistances[nextwaypoint->numprevwaypoints - 1] = nextwaypointdistance;
							nextwaypointindex++;
						}
						if (nextwaypointindex >= thiswaypoint->numnextwaypoints)
						{
							break;
						}
					}
				}
				else
				{
					CONS_Alert(
						CONS_WARNING, "Waypoint with ID %d has no next waypoint.\n", K_GetWaypointNextID(thiswaypoint));
				}
			}
			else
			{
				CONS_Debug(DBG_SETUP, "K_SetupWaypoint failed to make new waypoint with ID %d.\n", mobj->movecount);
			}
		}
	}

	return thiswaypoint;
}

/*--------------------------------------------------
	static boolean K_AllocateWaypointHeap(void)

		Allocates the waypoint heap enough space for the number of waypoint mobjs on the map

	Return:-
		True if the allocation was successful, false if it wasn't. Will I_Error if out of memory still.
--------------------------------------------------*/
static boolean K_AllocateWaypointHeap(void)
{
	mobj_t *waypointmobj = NULL;
	boolean allocationsuccessful = false;

	// Error conditions
	if (waypointheap != NULL)
	{
		CONS_Debug(DBG_SETUP, "K_AllocateWaypointHeap called when waypointheap is already allocated.\n");
	}
	else if (waypointcap == NULL)
	{
		CONS_Debug(DBG_SETUP, "K_AllocateWaypointHeap called with NULL waypointcap.\n");
	}
	else
	{
		// This should be an allocation for the first time. Reset the number of mobjs back to 0 if it's not already
		numwaypointmobjs = 0;

		// Find how many waypoint mobjs there are in the map, this is the maximum number of waypoints there CAN be
		for (waypointmobj = waypointcap; waypointmobj != NULL; waypointmobj = waypointmobj->tracer)
		{
			if (waypointmobj->type != MT_WAYPOINT)
			{
				CONS_Debug(DBG_SETUP,
					"Non MT_WAYPOINT mobj in waypointcap in K_AllocateWaypointHeap. Type=%d\n.", waypointmobj->type);
				continue;
			}

			numwaypointmobjs++;
		}

		if (numwaypointmobjs > 0)
		{
			// Allocate space in the heap for every mobj, it's possible some mobjs aren't linked up and not all of the
			// heap allocated will be used, but it's a fairly reasonable assumption that this isn't going to be awful
			waypointheap = Z_Calloc(numwaypointmobjs * sizeof(waypoint_t **), PU_LEVEL, NULL);

			if (waypointheap == NULL)
			{
				// We could theoretically CONS_Debug here and continue without using waypoints, but I feel that will
				// require error checks that will end up spamming the console when we think waypoints SHOULD be working.
				// Safer to just exit if out of memory
				I_Error("K_AllocateWaypointHeap: Out of memory.");
			}
			allocationsuccessful = true;
		}
		else
		{
			CONS_Debug(DBG_SETUP, "No waypoint mobjs in waypointcap.\n");
		}
	}

	return allocationsuccessful;
}

/*--------------------------------------------------
	void K_FreeWaypoints(void)

		For safety, this will free the waypointheap and all the waypoints allocated if they aren't already before they
		are setup. If the PU_LEVEL tag is cleared, make sure to call K_ClearWaypoints or this will try to free already
		freed memory!
--------------------------------------------------*/
static void K_FreeWaypoints(void)
{
	if (waypointheap != NULL)
	{
		// Free each waypoint if it's not already
		UINT32 i;
		for (i = 0; i < numwaypoints; i++)
		{
			if (waypointheap[i] != NULL)
			{
				Z_Free(waypointheap[i]);
			}
			else
			{
				CONS_Debug(DBG_SETUP, "NULL waypoint %d attempted to be freed.\n", i);
			}
		}

		// Free the waypointheap
		Z_Free(waypointheap);
	}

	K_ClearWaypoints();
}

/*--------------------------------------------------
	boolean K_SetupWaypointList(void)

		See header file for description.
--------------------------------------------------*/
boolean K_SetupWaypointList(void)
{
	boolean setupsuccessful = false;

	K_FreeWaypoints();

	if (!waypointcap)
	{
		CONS_Alert(CONS_ERROR, "No waypoints in map.\n");
	}
	else
	{
		if (K_AllocateWaypointHeap() == true)
		{
			// The waypoint in the waypointcap is going to be considered our first waypoint
			K_SetupWaypoint(waypointcap);

			if (!firstwaypoint)
			{
				CONS_Alert(CONS_ERROR, "No waypoints in map.\n");
			}
			else
			{
				CONS_Debug(DBG_SETUP, "Successfully setup %zu waypoints.\n", numwaypoints);
				if (numwaypoints < numwaypointmobjs)
				{
					CONS_Alert(CONS_WARNING,
						"Not all waypoints in the map are connected! %zu waypoints setup but %zu waypoint mobjs.\n",
						numwaypoints, numwaypointmobjs);
				}
				setupsuccessful = true;
			}
		}
	}

	return setupsuccessful;
}

/*--------------------------------------------------
	void K_ClearWaypoints(void)

		See header file for description.
--------------------------------------------------*/
void K_ClearWaypoints(void)
{
	waypointheap = NULL;
	firstwaypoint = NULL;
	numwaypoints = 0;
	numwaypointmobjs = 0;
}
