/*****************************************************************************
 *      Copyright (C) 2017-2018  Bean  email: notrynohigh@outlook.com
 *      Author: Bean 
 *      File: bMenu.c  Version: v0.0.1
 *      Everyone can modify and use this file
 *****************************************************************************/
/******************************************************************************
 * include
 ******************************************************************************/
#include "stdio.h"
#include "string.h"
#include "bMenu.h"

 /******************************************************************************
 * global variable
 ******************************************************************************/

static bM_DMC_Interface_t g_bM_DMC_Interface = {bM_NULL, bM_NULL};
static bM_Object_t        g_bM_ManageRoot = {bM_NULL, bM_NULL, 0, 0, bM_NULL,0};
static bM_Object_t* const gp_bM_ManageRoot = &g_bM_ManageRoot;
static bM_Object_t        *gp_bM_MenuEntryPoint = bM_NULL;

static volatile bM_TaskManage_t    g_bM_TaskManage;
/** be used to create handles     */
static bM_U32             g_bM_OBJ_Number = 0;
static bM_U32             g_bM_ItemNumber = 0;

#if BM_OS_ENABLE == 0
static bM_bool_t g_bM_Semaphor = BM_FALSE;
#endif

 /******************************************************************************
 * private defined
 ******************************************************************************/
 /* bM module version number */
#define bM_VERSION    "V0.0.1"

 /******************************************************************************
 * private typedef
 ******************************************************************************/
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

static bM_Result_t bM_GiveSemaphore()
{
#if BM_OS_ENABLE
	//add your code ....
#else
	if (g_bM_Semaphor == BM_FALSE)
	{
		g_bM_Semaphor = BM_TRUE;
		return BM_SUCCESS;
	}
	else
	{
		return BM_ERROR;
	}
#endif
}

static bM_Result_t bM_TakeSemaphore()
{
#if BM_OS_ENABLE
	//add your code ....
#else
	if (g_bM_Semaphor == BM_TRUE)
	{
		g_bM_Semaphor = BM_FALSE;
		return BM_SUCCESS;
	}
	else
	{
		return BM_ERROR;
	}
#endif
}

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
		bM_Debug("[%s]param error\n\r", __func__);
		return BM_ERROR;
	}
	if(pobj->pFirstItem == bM_NULL)
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

/**
 * find out object or item by user_id
 * id: user_id
 * result: output the result.
 *         error: result->handle == bM_HANDLE_INVALID 
 */
static void _bM_GetOBJorItemById(bM_ID id, bM_UserIdResult_t *result)
{
	bM_Object_t *pobj = gp_bM_ManageRoot->next;
	bM_Item_t* pitem;
	bM_U32 i = 0;
	result->handle = bM_HANDLE_INVALID;
	if(pobj == gp_bM_ManageRoot || pobj == bM_NULL)
	{
		return;
	}
	while(pobj != gp_bM_ManageRoot)
	{
		if(_bM_GetIdFromHandle(pobj->handle, BM_ID_USER) == id)
		{
			result->handle = pobj->handle;
			result->result.pobj = pobj;
			return;
		}
		else if(pobj->item_number > 0)
		{
			pitem = pobj->pFirstItem;
			if(pitem != bM_NULL)
			{
				for(i = 0;i < pobj->item_number;i++)
				{
					if(_bM_GetIdFromHandle(pitem->handle, BM_ID_USER) == id)
					{
						result->handle = pitem->handle;
						result->result.pitem = pitem;
						return;
					}
					pitem = pitem->next;
				}
			}
		}
		pobj = pobj->next;
	}
}

static void _bM_FreeAllResource()
{
	bM_Object_t *pobj = gp_bM_ManageRoot->next;
	bM_Item_t* pitem, *pitem_temp;

	while(pobj != gp_bM_ManageRoot && pobj != bM_NULL)
	{
		pitem = pobj->pFirstItem;
		while(pitem != bM_NULL)
		{
			pitem_temp = pitem->next;
			if(pitem_temp == pitem)
			{
				g_bM_DMC_Interface.pFree(pitem);
				pitem = bM_NULL;
			}
			else
			{
				pitem->next = pitem_temp->next;
				pitem_temp->next->prev = pitem;
				g_bM_DMC_Interface.pFree(pitem_temp);
				pitem_temp = bM_NULL;
			}
		}
		pobj->pFirstItem = bM_NULL;
		gp_bM_ManageRoot->next =  pobj->next;
		pobj->next->prev = gp_bM_ManageRoot;
		g_bM_DMC_Interface.pFree(pobj);
		pobj = gp_bM_ManageRoot->next;
	}
}

/**
 * direction : BM_TRUE(next)  BM_FALSE(prev)
 */
static bM_Item_t* _bM_FindOutNextItemToShow(bM_Item_t *pCurrentItem, bM_bool_t direction)
{
	bM_Item_t *ptemp = bM_NULL;
	if (pCurrentItem == bM_NULL)
	{
		return bM_NULL;
	}
	ptemp = (direction == BM_TRUE) ? pCurrentItem->next : pCurrentItem->prev;
	while (ptemp != pCurrentItem)
	{
		if (ptemp->visible == BM_TRUE)
		{
			return ptemp;
		}
		ptemp = (direction == BM_TRUE) ? ptemp->next : ptemp->prev;
	}
	return bM_NULL;
}

static bM_Item_t* _bM_OperationGotoChild(bM_Object_t *pobj)
{
	bM_Item_t *pitem = bM_NULL;
	if (pobj != bM_NULL)
	{
		/** child valid */
		pitem = pobj->pFirstItem;
		if (pitem != bM_NULL)
		{
			/** the first item valid */
			if (pitem->visible == BM_FALSE)
			{
				/** the first item can be shown, so find next */
				pitem = _bM_FindOutNextItemToShow(pitem, BM_TRUE);
			}
		}
	}
	return pitem;
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

	gp_bM_ManageRoot->next = gp_bM_ManageRoot;
	gp_bM_ManageRoot->prev = gp_bM_ManageRoot;
    
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
    pbM_ObjTmp->handle = _bM_CreateHandle(id, ++g_bM_OBJ_Number, 0);
	pbM_ObjTmp->pParent = pitem;
    pbM_ObjTmp->item_number = 0;
	pbM_ObjTmp->pFirstItem = bM_NULL;
	if(_bM_AddObjectToManage(pbM_ObjTmp) != BM_SUCCESS)
	{
		g_bM_DMC_Interface.pFree(pbM_ObjTmp);
		return bM_HANDLE_INVALID;
	}

	if(pitem != bM_NULL)
	{
		pitem->child = pbM_ObjTmp;
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
		bM_Debug("error: cant find obj\n\r");
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
	pitem->visible = BM_TRUE;
	pitem->handle = _bM_CreateHandle(id, _bM_GetIdFromHandle(hobj, BM_ID_OBJ), (++g_bM_ItemNumber));
	if(_bM_AddItemToObject(pobj, pitem) != BM_SUCCESS)
	{
		bM_Debug("error: add item\n\r");
		g_bM_DMC_Interface.pFree(pitem);
		return bM_HANDLE_INVALID;
	}
	return pitem->handle;
}


/**
 * appoint an object as the entry
 */
 bM_Result_t bM_SetMenuEntryPoint(bM_OBJ_Handle hobj)
{
	bM_Object_t *pobj_temp = bM_NULL;
	if(hobj == bM_HANDLE_INVALID)
	{
		return BM_ERROR;
	}
	pobj_temp = _bM_GetObjectFromManage(hobj);
	if(pobj_temp == bM_NULL)
	{
		return BM_ERROR;
	}
	gp_bM_MenuEntryPoint = pobj_temp;
	g_bM_TaskManage.NewMessage.opt = BM_OPERATE_INIT;
	g_bM_TaskManage.pCurrentObj = pobj_temp;
	g_bM_TaskManage.pCurrentItem = pobj_temp->pFirstItem;
	if (pobj_temp->pFirstItem != bM_NULL)
	{
		pobj_temp->pFirstItem->visible = BM_TRUE;
	}
	bM_GiveSemaphore();
	return BM_SUCCESS;
}

/**
 * send operation message to bM management
 */
 bM_Result_t bM_SendMessage(bM_Operation_t opt, bM_ID id)
 {
 	bM_Message_t msg;
	if(opt == BM_OPERATE_INIT || opt == BM_OPERATE_NULL)
	{
		return BM_ERROR;
	}
	if(opt == BM_OPERATE_JUMP_TO)
	{
		_bM_GetOBJorItemById(id, &msg.result);
		if(msg.result.handle == bM_HANDLE_INVALID)
		{
			return BM_ERROR;
		}
	}
	msg.opt = opt;
	memcpy((void *)(&g_bM_TaskManage.NewMessage), &msg, sizeof(bM_Message_t));
	return bM_GiveSemaphore();
 }

/**
 * free bM dynamic memory and deinit all global variable
 */
void bM_BMenuModuleEnd(void)
{
	_bM_FreeAllResource();
	g_bM_ItemNumber = 0;
	g_bM_OBJ_Number = 0;
}

/**
 * bM Main Task
 */
void bM_BMenuModuleTask(void)
{
	bM_Item_t *pitem = bM_NULL;
	bM_Handle hTemp = bM_HANDLE_INVALID;
	if (BM_SUCCESS != bM_TakeSemaphore())
	{
		return;
	}
	switch (g_bM_TaskManage.NewMessage.opt)
		{
		case BM_OPERATE_INIT: 
			{
			    pitem = g_bM_TaskManage.pCurrentItem;
				break;
		    }
		case BM_OPERATE_NEXT:
			{
				pitem = _bM_FindOutNextItemToShow(g_bM_TaskManage.pCurrentItem, BM_TRUE);
				if (pitem != bM_NULL)
				{
					g_bM_TaskManage.pCurrentItem = pitem;
				}
				break;
		    }
		case BM_OPERATE_PREV:
			{
				pitem = _bM_FindOutNextItemToShow(g_bM_TaskManage.pCurrentItem, BM_FALSE);
				if (pitem != bM_NULL)
				{
					g_bM_TaskManage.pCurrentItem = pitem;
				}
				break;
			}
		case BM_OPERATE_GOTO_CHILD:
			{
				pitem = _bM_OperationGotoChild(g_bM_TaskManage.pCurrentItem->child);
				if (pitem != bM_NULL)
				{
					g_bM_TaskManage.pCurrentObj = g_bM_TaskManage.pCurrentItem->child;
					g_bM_TaskManage.pCurrentItem = pitem;
				}
				break;
		    }
		case BM_OPERATE_BACK_PARENT:
			{
				if(g_bM_TaskManage.pCurrentObj->pParent != bM_NULL && g_bM_TaskManage.pCurrentObj != gp_bM_MenuEntryPoint)
				{
					pitem = g_bM_TaskManage.pCurrentObj->pParent;
					g_bM_TaskManage.pCurrentItem = pitem;
					g_bM_TaskManage.pCurrentObj = _bM_GetObjectFromManage(pitem->handle);
				}
				break;
			}
		case BM_OPERATE_JUMP_TO:
			{
				hTemp = g_bM_TaskManage.NewMessage.result.handle;
				if (hTemp != bM_HANDLE_INVALID)
				{
					if (_bM_GetIdFromHandle(hTemp, BM_ID_ITEM) == 0x00)
					{
						pitem = _bM_OperationGotoChild(g_bM_TaskManage.NewMessage.result.result.pobj);
						if (pitem == bM_NULL || pitem == g_bM_TaskManage.pCurrentItem)
						{
							pitem = bM_NULL;
						}
						else
						{
							g_bM_TaskManage.pCurrentObj = g_bM_TaskManage.NewMessage.result.result.pobj;
							g_bM_TaskManage.pCurrentItem = pitem;
						}
					}
					else
					{
						pitem = g_bM_TaskManage.NewMessage.result.result.pitem;
						if (pitem == g_bM_TaskManage.pCurrentItem || pitem->visible == BM_FALSE)
						{
							pitem = bM_NULL;
						}
						else
						{
							g_bM_TaskManage.pCurrentObj = _bM_GetObjectFromManage(pitem->handle);
							g_bM_TaskManage.pCurrentItem = pitem;
						}
					}
				}
			}
		default: break;
		}
	g_bM_TaskManage.NewMessage.opt = BM_OPERATE_NULL;
	if(pitem != bM_NULL)
	{
		if (pitem->create_ui != bM_NULL)
		{
			pitem->create_ui();
		}
	}
}

bM_Result_t bM_ChangeVisibleStatus(bM_ID id, bM_bool_t visible)
{
	bM_UserIdResult_t tmp_result;
	_bM_GetOBJorItemById(id, &tmp_result);
	if(tmp_result.handle == BM_HANDLE_ITEM)
	{
		bM_Debug("invalid id !\n\r");
		return BM_ERROR;
	}
	if (_bM_GetIdFromHandle(tmp_result.handle, BM_ID_ITEM) == 0x00)
	{
		bM_Debug("sorry, cant set for object !\n\r");
		return BM_ERROR;
	}
	if (gp_bM_MenuEntryPoint->pFirstItem != bM_NULL)
	{
		if (_bM_GetIdFromHandle(gp_bM_MenuEntryPoint->pFirstItem, BM_ID_USER) == id)
		{
			bM_Debug("sorry, cant set for entry first item !\n\r");
			return BM_ERROR;
		}
	}
	tmp_result.result.pitem->visible = visible;
	return BM_SUCCESS;
}


bM_ID bM_GetUseridOfCurrentItem()
{
	return _bM_GetIdFromHandle(g_bM_TaskManage.pCurrentItem->handle, BM_ID_USER);
}



/******************************************************************************
* End !
******************************************************************************/














