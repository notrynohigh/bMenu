

/******************************************************************************
 * include
 ******************************************************************************/
#include "bMenu.h"


/**
 * global variable
 */
static bM_DMC_Interface_t g_bM_DMC_Interface = {bM_NULL, bM_NULL};
static bM_Object_t        g_bM_ManageRoot = {bM_NULL, bM_NULL, 0, 0, 0, bM_NULL,0};
static bM_Object_t        *gp_bM_ManageRoot = &gp_bM_ManageRoot;

static bM_U32             g_bM_OBJ_Number = 0;
static bM_U32             g_bM_ItemNumber = 0;

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

typedef enum
{
	BM_HANDLE_OBJ,
	BM_HANDLE_ITEM,
}bM_HANDLE_Types_t;
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
	gp_bM_ManageRoot->prev->next = pbM_OBJ;
	pbM_OBJ->next = gp_bM_ManageRoot;
	pbM_OBJ->prev = gp_bM_ManageRoot->prev;
	gp_bM_ManageRoot->prev = pbM_OBJ;
	return BM_SUCCESS;
}

static bM_Result_t _bM_AddItemToObject(bM_Object_t *pobj, bM_Item_t *pitem)
{
	if(pobj == bM_NULL || pitem == bM_NULL)
	{
		return BM_FALSE;
	}
	if(pobj->pFirstItem == NULL)
	{
		pobj->pFirstItem = pitem;
		pitem->next = pitem;
		pitem->prev = pitem;
	}
	else
	{
		pobj->pFirstItem->prev->next = pitem;
		pitem->prev = pobj->pFirstItem->prev;
		pitem->next = pobj->pFirstItem;
		pobj->pFirstItem->prev = pitem;
	}
	pobj->item_number++;
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


static bM_Object_t *_bM_GetObjectFromManage(bM_OBJ_Handle hobj)
{
	bM_Object_t *pobj = gp_bM_ManageRoot->next;
	bM_ID id = _bM_GetIdFromHandle(hobj, BM_ID_OBJ);
	
	if(hobj == bM_HANDLE_INVALID || pobj == gp_bM_ManageRoot)
	{
		return bM_NULL;
	}
	do
	{
		if(_bM_GetIdFromHandle(pobj->handle, BM_ID_OBJ) == id)
		{
			return pobj;
		}
		pobj = pobj->next;
	}while(pobj != gp_bM_ManageRoot);
	return bM_NULL;
}

static bM_Item_t *_bM_GetItemtFromManage(bM_ITEM_Handle hitem)
{
	bM_Object_t *pobj = _bM_GetObjectFromManage(hitem);
	bM_Item_t *pitem = bM_NULL;
	if(pobj == bM_NULL)
	{
		return bM_NULL;
	}
	if(pobj->pFirstItem == bM_NULL)
	{
		return bM_NULL;
	}
	pitem = pobj->pFirstItem;
	do
	{
		if(pitem->handle == hitem)
		{
			return pitem;
		}
		pitem = pitem->next;
	}while(pitem != pobj->pFirstItem);
	return bM_NULL;
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

	gp_bM_ManageRoot.next = gp_bM_ManageRoot;
	gp_bM_ManageRoot.prev = gp_bM_ManageRoot;
    
	return BM_SUCCESS;
}

/**
 * create a new object and appoint its parent handle
 */
bM_OBJ_Handle bM_CreateObject(bM_ITEM_Handle hParent, bM_ID id)
{
    bM_Object_t *pbM_ObjTmp = bM_NULL;
	bM_Item_t   *pitem = bM_NULL;
	pitem = _bM_GetItemtFromManage(hParent);
	pbM_ObjTmp = (bM_Object_t*)g_bM_DMC_Interface.pMalloc(sizeof(bM_Object_t));
	if(pbM_ObjTmp == bM_NULL)
	{
		return bM_HANDLE_INVALID;
	}
    pbM_ObjTmp->handle = _bM_CreateHandle(id, g_bM_OBJ_Number + 1, 0);
	pbM_ObjTmp->hParent = (pitem == bM_NULL) ? bM_HANDLE_INVALID : hParent;
    pbM_ObjTmp->item_number = 0;
	pbM_ObjTmp->pFirstItem = bM_NULL;
	if(_bM_AddObjectToManage(pbM_ObjTmp) != BM_SUCCESS)
	{
		g_bM_DMC_Interface.pFree(pbM_ObjTmp);
		return bM_HANDLE_INVALID;
	}

	if(pitem != bM_NULL)
	{
		pitem->child = pbM_ObjTmp->handle;
	}
	
	return pbM_ObjTmp->handle;
}


/**
 * Add items to bM object
 */
bM_ITEM_Handle bM_AddItemToObject(bM_OBJ_Handle hobj, bM_ID id, bM_CreateUI_t func)
{
	bM_Item_t *pitem = bM_NULL;
	bM_Object_t *pobj = bM_NULL;

	pobj = _bM_GetObjectFromManage(hobj);
	if(pobj == bM_NULL)
	{
		return bM_HANDLE_INVALID;
	}
	pitem = (bM_Item_t *)g_bM_DMC_Interface.pMalloc(sizeof(bM_Item_t));
	if(pitem == bM_NULL)
	{
		bM_Debug("memory error: %s\n\r", __func__);
		return bM_HANDLE_INVALID;
	}

	pitem->child = bM_NULL;
	pitem->create_ui = func;
	pitem->handle = _bM_CreateHandle(id, _bM_GetIdFromHandle(hobj, BM_ID_OBJ), (g_bM_ItemNumber + 1));
	if(_bM_AddItemToObject(pobj, pitem) != BM_SUCCESS)
	{
		g_bM_DMC_Interface.pFree(pitem);
		return bM_HANDLE_INVALID;
	}
	return pitem->handle;
}










