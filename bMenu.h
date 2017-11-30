/******************************************************************************
 * Reserve ! 
 ******************************************************************************/

#ifndef __BMENU_H__
#define __BMENU_H__


/******************************************************************************
 * According to your CPU to change these.
 ******************************************************************************/
typedef unsigned char  bM_U8;
typedef signed char    bM_S8;
typedef unsigned short bM_U16;
typedef signed short   bM_S16;
typedef unsigned int   bM_U32;
typedef signed int     bM_S32;

/* Debug interface */
#define BM_DEBUG_ENABLE        1     

#if BM_DEBUG_ENABLE
    #define bM_Debug(...)  printf(__VA_ARGS__)
#else
    #define bM_Debug(...)
#endif
/******************************************************************************
 * define
 ******************************************************************************/
#define  bM_ID           bM_U8
#define  bM_Handle       bM_U32
#define  bM_OBJ_Handle   bM_Handle
#define  bM_ITEM_Handle  bM_Handle

#define  bM_NULL       ((void *)0)
#define  bM_TRUE       1
#define  bM_FALSE      0

#define  bM_HANDLE_INVALID  0X0

/******************************************************************************
 * typedef 
 ******************************************************************************/
typedef enum
{
    BM_FALSE = bM_FALSE,
	BM_TRUE = bM_TRUE,
}bM_bool_t;


typedef void (*bM_CreateUI_t)(void *);

/**
 * bM object Manage List
 */
typedef struct bM_Object
{
    struct bM_Object *prev;
	struct bM_Object *next;
	bM_OBJ_Handle    handle;
	bM_ITEM_Handle   hParent;
	bM_Item_t        *pFirstItem;
	bM_U8            item_number;
}bM_Object_t;

typedef struct bM_Item
{
	struct bM_Item  *prev;
	struct bM_Item  *next;
	bM_Object_t     *child;
	bM_CreateUI_t   create_ui;
	bM_ITEM_Handle  handle;
}bM_Item_t;




/**
 * bM_Result: error number
 */
typedef enum
{
   BM_ERROR,
   BM_SUCCESS,
   BM_MEMORY_ERR,
   BM_OTHER_FAULT
}bM_Result_t;


/**
 * dynamic memory control interface
 */
typedef void* (*bM_Malloc)(int);
typedef void (*bM_Free)(void *);

typedef struct
{
   bM_Malloc pMalloc;
   bM_Free pFree;
}bM_DMC_Interface_t;






/******************************************************************************
 * public functions
 ******************************************************************************/
/**
 * initianize bM module
 * param: dynamic memory control interface
 */
bM_Result_t bM_Init(bM_DMC_Interface_t bM_DMC_Interface);




/**
 * create a basic menu object. if it's independent, parent = bM_HANDLE_INVALID
 * param: the handle of parent or bM_HANDLE_INVALID
 *        attach an identification : all identifications must be different;
 * return: handle (0: error) (> 0)success
 */
bM_OBJ_Handle bM_CreateObject(bM_ITEM_Handle hParent, bM_ID id);



/**
 * add item to bM module. the items which at the same level can be added.
 * hobj: the bM object handle
 * id  : the item identification.
 * func: the UI create function
 */
bM_ITEM_Handle bM_AddItemToObject(bM_OBJ_Handle hobj, bM_ID id, bM_CreateUI_t func);









