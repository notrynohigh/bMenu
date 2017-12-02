# bMenu
a common frame for  menu, just for fun!

using process:
1. add bMenu.c and bMenu.h to your project .
2. calling bM_Init() function. add malloc and free interface
3. building your menu structure

for example:

/**************************************************************/

    item1----item2-----item3-----item4
               |
               item5----item6----item7
                          |
                          item8--item9
                          |
                          item10
  it's a four levels menu. then, divide it:
  create four objects by bM_CreateObject();
  add items and creating UI function to the object by bM_AddItemToObject();
  
  object1 {item1----item2----item3----item4}
  
  object2 {item5----item6----item7}
  
  object3 {item8----item9}
  
  object4 {item10}
  
 
 user_id of every object and item must be different.
 maybe you can do that through enum:
   
   enum
   {
     USER_ID_OBJECT_1,
     USER_ID_ITEM1,
     USER_ID_ITEM2,
     USER_ID_ITME3,
     USER_ID_ITEM4,
     USER_ID_OBJECT_2,
     USER_ID_ITEM5,
     USER_ID_ITEM6,
     USER_ID_ITEM7,
     USER_ID_OBJECT_3,
     USER_ID_ITEM8,
     USER_ID_ITEM9,
     USER_ID_OBJECT_4,
     USER_ID_ITEM10
   };
  example_func   
  {
      bM_OBJ_Handle  hobj, hobj_tmp;
      bM_ITEM_Handle hItem;
      
      hobj = bM_CreateObject(bM_HANDLE_INVALID, USER_ID_OBJECT_1);
      bM_AddItemToObject(hobj, USER_ID_ITEM1, func1);
      hItem = bM_AddItemToObject(hobj, USER_ID_ITEM2, func2);
      bM_AddItemToObject(hobj, USER_ID_ITME3, func3);
      bM_AddItemToObject(hobj, USER_ID_ITEM4, func4);
      
      hobj_tmp = bM_CreateObject(hItem, USER_ID_OBJECT_2);
      bM_AddItemToObject(hobj_tmp, USER_ID_ITEM5, func5);
      hItem = bM_AddItemToObject(hobj_tmp, USER_ID_ITEM6, func6);
      bM_AddItemToObject(hobj_tmp, USER_ID_ITEM7, func7);  
  
      hobj_tmp = bM_CreateObject(hItem, USER_ID_OBJECT_3);
      hItem = bM_AddItemToObject(hobj_tmp, USER_ID_ITEM8, func8);
      bM_AddItemToObject(hobj_tmp, USER_ID_ITEM9, func9);
 
      hobj_tmp = bM_CreateObject(hItem, USER_ID_OBJECT_4);
      bM_AddItemToObject(hobj_tmp, USER_ID_ITEM10, func10); 
      
      //then set the entry point:
      bM_SetMenuEntryPoint(hobj);      
  }
  
  then the menu structure has been created.
  
  add bM_BMenuModuleTask()  to while(1)
  
  then:
  you can call bM_SendMessage() to control switching.
  
  for example:
  
  bM_SendMessage(BM_OPERATE_NEXT, 0);   // to show the next item 
  
  bM_SendMessage(BM_OPERATE_JUMP_TO, USER_ID_ITEM5);   //to show the item5
  
  
  
  Ok! you can try it by yourself now!
  
  if you have some suggestion, please contact me.
  email: notrynohigh@outlook.com
  
  
  Thanks! 

/***************************************************************/






