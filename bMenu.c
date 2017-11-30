

/******************************************************************************
 * include
 ******************************************************************************/
#include "bMenu.h"


/**
 * global variable
 */
static bM_DMC_Interface_t g_bM_DMC_Interface = {bM_NULL, bM_NULL};
static bM_Object_t        g_bM_Manage_root = {bM_NULL, bM_NULL, 0, 0, 0, bM_NULL,0};
static bM_U32             g_bM_OBJ_Number = 0;


/**
 *  private defined
 */
 /* bM module version number */
#define bM_VERSION    "V0.0.1"

/**
 * private typedef
 */
typedef enum
{
	BM_ID_USER,
	BM_ID_OBJ,
	BM_ID_ITEM,
}bM_ID_Types_t;


/******************************************************************************
 *  private functions
 ******************************************************************************/
/**
 * assemble bM_Handle [8bits reserve][8bits id][8bits obj_index][8bits item_index]
 */
static bM_Handle _bM_CreateHandle(bM_ID id, bM_ID obj_index, bM_ID item_index)
{
	bM_Handle handle = 0;
	handle = (id << 16) | (obj_index << 8) | item_index;
	return handle;
}

/**
 * add new object to manage list
 */
static bM_Result_t _bM_AddObjectToManage(bM_Object_t *pbM_OBJ)
{
	if(pbM_OBJ == bM_NULL)
	{
		return BM_ERROR;
	}
	g_bM_Manage_root.prev->next = pbM_OBJ;
	pbM_OBJ->next = &g_bM_Manage_root;
	pbM_OBJ->prev = g_bM_Manage_root.prev;
	g_bM_Manage_root.prev = pbM_OBJ;
	return BM_SUCCESS;
}

static bM_ID _bM_GetIdFromHandle(bM_Handle handle, bM_ID_Types_t t)
{
	bM_ID id = 0;
	switch (t)
		{
		case BM_ID_USER: 
			{
				id = (handle >> 16) & 0xff;
				break;
		    }
		case BM_ID_OBJ: 
			{
				id = (handle >> 8) & 0xff;
				break;
		    }
		case BM_ID_ITEM: 
			{
				id = (handle >> 0) & 0xff;
				break;
		    }
		default: break;

		}
	return id;
}


static bM_bool_t _bM_IsHandleExist(bM_Handle handle)
{
	bM_Object_t *pobj = g_bM_Manage_root.next;
	bM_Item_t *pitem = g_bM_Manage_root.next->pFirstItem;
    bM_U32 i = 0;
	bM_ID obj_id = _bM_GetIdFromHandle(handle, BM_ID_OBJ);
	if(g_bM_Manage_root.next == &g_bM_Manage_root)
	{
		return BM_FALSE;
	}
	do
	{
		if(_bM_GetIdFromHandle(pobj->handle, BM_ID_OBJ) == obj_id)
		{
			pitem = pobj->pFirstItem;
			for(i = 0;i < pobj->item_number;i++)
			{
				if(handle == pitem->handle)
				{
					return BM_TRUE;
				}
				pitem = pitem->next;
			}
		}
		pobj = pobj->next;
	}
	while (pobj != &g_bM_Manage_root);
	return BM_FALSE;
}


static bM_Result_t _bM_SetChildObjForItem(bM_Handle hParent, bM_Handle hObj)
{
	bM_Object_t *pobj = g_bM_Manage_root.next;
	bM_Item_t *pitem = g_bM_Manage_root.next->pFirstItem;
    bM_U32 i = 0;
	bM_ID obj_id = _bM_GetIdFromHandle(hParent, BM_ID_OBJ);
	if(g_bM_Manage_root.next == &g_bM_Manage_root || hParent == bM_HANDLE_INVALID)
	{
		return BM_ERROR;
	}
	do
	{
		if(_bM_GetIdFromHandle(pobj->handle, BM_ID_OBJ) == obj_id)
		{
			pitem = pobj->pFirstItem;
			for(i = 0;i < pobj->item_number;i++)
			{
				if(hParent == pitem->handle)
				{
					pitem->child = hObj;
					return BM_SUCCESS;
				}
				pitem = pitem->next;
			}
		}
		pobj = pobj->next;
	}
	while (pobj != &g_bM_Manage_root);
	return BM_ERROR;
}

/******************************************************************************
 *  public functions
 ******************************************************************************/

/**
 * initianize bM module. this function should be called first
 */
bM_Result_t bM_Init(bM_DMC_Interface_t bM_DMC_Interface)
{
    bM_Debug("bM version: %s\n\r", bM_VERSION);  
	if(bM_DMC_Interface.pMalloc == bM_NULL || bM_DMC_Interface.pFree == bM_NULL)
	{
		return BM_ERROR;
	}
	g_bM_DMC_Interface.pFree = bM_DMC_Interface.pFree;
	g_bM_DMC_Interface.pMalloc = bM_DMC_Interface.pMalloc;

	g_bM_Manage_root.next = &g_bM_Manage_root;
	g_bM_Manage_root.prev = &g_bM_Manage_root;
    
	return BM_SUCCESS;
}

/**
 * create a new object and appoint its parent handle
 */
bM_Handle bM_CreateObject(bM_Handle hParent, bM_ID id)
{
    bM_Object_t *pbM_ObjTmp = bM_NULL;

	if(hParent != bM_HANDLE_INVALID)
	{
		if(BM_FALSE == _bM_IsHandleExist(hParent))
		{
			return bM_HANDLE_INVALID;
		}
	}
	pbM_ObjTmp = (bM_Info_t *)g_bM_DMC_Interface.pMalloc(sizeof(bM_Object_t));
	if(pbM_ObjTmp == bM_NULL)
	{
		return BM_MEMORY_ERR;
	}
    pbM_ObjTmp->handle = _bM_CreateHandle(id, g_bM_OBJ_Number + 1, 0);
	pbM_ObjTmp->hChild = bM_HANDLE_INVALID;
	pbM_ObjTmp->hParent = hParent;
    pbM_ObjTmp->item_number = 0;
	pbM_ObjTmp->pFirstItem = bM_NULL;
	if(_bM_AddObjectToManage(pbM_ObjTmp) != BM_SUCCESS)
	{
		g_bM_DMC_Interface.pFree(pbM_ObjTmp);
		return bM_HANDLE_INVALID;
	}

	_bM_SetChildObjForItem(hParent, pbM_ObjTmp->handle);
	
	return pbM_ObjTmp->handle;
}














