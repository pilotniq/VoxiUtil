/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  External interface for binary tree (bt) functions
  
  This file was part of the Great Hemul Project, and has now been
  integrated into Voxi's ISI.
  
  */

#ifndef BT_H
#define BT_H

#include <voxi/types.h>

/* External types */

typedef struct bt_treestruct bt_Tree;

/* typedef int (*bt_compfunc)(void *value1, void *value2); */

#ifndef _COMPFUNCPTR
#define _COMPFUNCPTR

typedef int (*CompFuncPtr)(void *data1, void *data2);

#endif

#ifndef _DUPFUNCPTR
#define _DUPFUNCPTR

typedef void *(*DupFuncPtr)(void *data);

#endif

typedef bt_Tree *BinTree;

/* Internal types */

#ifdef BT_C

typedef struct node bt_node;
typedef enum { BT_LEFT, BT_RIGHT, BT_NONE } bt_direction;

struct node
{
  bt_node *right;
  bt_node *left;
  bt_node *father;
  bt_direction direction;

  void *value;
};

struct bt_treestruct
{
  bt_node *root;
  CompFuncPtr compare;
  bt_node *current_node;
	int count;
};

/* Internal Functions */
static void bt_destroynode(bt_node *node);
static void *bt_nodefind(bt_Tree *tree, bt_node *node, void *key);

#endif

/* External functions */

BinTree bt_create(CompFuncPtr compare);
void bt_destroy(bt_Tree *tree);

BinTree bt_dup(BinTree, DupFuncPtr);

int bt_add(BinTree tree, void *value);

/* Returns TRUE if the value was deleted, FALSE if it was not in the tree */
int bt_remove(bt_Tree *tree, void *value, Boolean ignoreErr);

/* Returns NULL if not found */
void *bt_find(bt_Tree *tree, void *key);

void bt_GoNext(bt_Tree *tree);

void bt_GoFirst(bt_Tree *tree);


#ifdef WIN32
int bt_getCount( BinTree tree );
void *bt_GetCur(bt_Tree *tree);
Boolean bt_EOT(bt_Tree *tree);
#else
__inline int bt_getCount( BinTree tree );
__inline void *bt_GetCur(bt_Tree *tree);
__inline Boolean bt_EOT(bt_Tree *tree);
#endif

#endif









