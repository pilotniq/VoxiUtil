/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  Binary tree routines for buckets lab.
  
  This is pre-historic code from an early ProgK lab. It has since migrated to 
  the Great Hemul Project, Erlib and has now become part of voxi/util.
  */

#define BT_C

#include <stdlib.h>
#include <voxi/alwaysInclude.h>
#include <voxi/util/err.h>
#include <voxi/util/bt.h>

CVSID("$Id$");

/* static functions */
static bt_node *bt_findnode(bt_Tree *tree, bt_node *node, void *key);

bt_Tree *bt_create(CompFuncPtr compare)
{
  bt_Tree *result = malloc(sizeof(bt_Tree));

  DEBUG("enter\n");

  result->root = NULL;
  result->compare = compare;
	result->count = 0;
	
  DEBUG("leave\n");

  return(result);
}

int bt_remove(bt_Tree *tree, void *value, Boolean ignoreErr)
{
  bt_node *deletenode, *leftnode, *rightnode;

  ErrPushFunc("bt_remove");

  deletenode = bt_findnode(tree, tree->root, value);

  if(deletenode == NULL)
  {
    if(!ignoreErr)
      ERR ERR_WARN, "Could not remove element %p - not found\n", value
	ENDERR;

    ErrPopFunc();
    return(FALSE);
  }

  /* Algorithm: place left node at the left-most leaf of rightnode */

  leftnode = deletenode->left;
  rightnode = deletenode->right;

  if(rightnode == NULL)
    deletenode->father = leftnode;
  else
  {
    while(rightnode->left != NULL)
      rightnode = rightnode->left;

    rightnode->left = leftnode;
  }
	
	tree->count--;
	
  ErrPopFunc();
  return(TRUE);
}

/* Returns 0 if success
   Returns -1 if value already existed in tree

   */

int bt_add(bt_Tree *tree, void *value)
{
  bt_node *cur_node = tree->root;
  bt_node *father = NULL;

  int tempint;
  bt_direction direction = BT_NONE;

/*   DisplayTrace(TRUE); */

  ErrPushFunc("bt_add(%p, %p)", tree, value);

  while(cur_node != NULL)
  {
    father = cur_node;

    tempint = (tree->compare)(value, cur_node->value);

    if(tempint < 0)
    {
      cur_node = cur_node->left;
      direction = BT_LEFT;
    }
    else if(tempint > 0)
    {
      cur_node = cur_node->right;
      direction = BT_RIGHT;
    }
    else
    {
      ERR ERR_WARN, "Value already in tree!" ENDERR;
      ErrPopFunc();
      return(-1);
    }
  }

  /* Create the new node */

  cur_node = malloc(sizeof(bt_node));
  cur_node->value = value;
  cur_node->left = cur_node->right = NULL;
  cur_node->father = father;
  cur_node->direction = direction;

  if(father==NULL)
    tree->root = cur_node;
  else
    if(direction==BT_LEFT)
      father->left = cur_node;
    else
      father->right = cur_node;
	
	tree->count++;
	
  ErrPopFunc();

/*   DisplayTrace(FALSE); */
  return(0);
}

void bt_destroy(bt_Tree *tree)
{
  if(tree->root != NULL) bt_destroynode(tree->root);
  free(tree);
}

void *bt_find(bt_Tree *tree, void *key)
{
  return(bt_nodefind(tree, tree->root, key));
}

#ifndef WIN32
__inline
#endif
Boolean bt_EOT(bt_Tree *tree)
{
  return(tree->current_node == NULL);
}

void bt_GoFirst(bt_Tree *tree)
{
  bt_node *node;

  node = tree->root;
  if(node != NULL)
    while(node->left != NULL)
      node = node->left;

  tree->current_node = node;
}

#ifndef WIN32
__inline
#endif
void *bt_GetCur(bt_Tree *tree)
{
  return(tree->current_node->value);
}

void bt_GoNext(bt_Tree *tree)
{
  bt_node *node = tree->current_node;
  bt_direction dir;

  if(node->right != NULL)
  {
    node = node->right;
    while(node->left != NULL)
      node = node->left;
    tree->current_node = node;
  }
  else
  {
    dir = node->direction;
    node = node->father;

    while((node != NULL) && (dir != BT_LEFT))
    {
      dir = node->direction;
      node = node->father;
    }
    tree->current_node = node;
  }
}

static bt_node *bt_findnode(bt_Tree *tree, bt_node *node, void *key)
{
  int tempint;

  if(node == NULL)  return(NULL);

  tempint = (tree->compare)(key, node->value);

  if(tempint == 0)
    return(node);
  else
    if(tempint < 0)
      return(bt_findnode(tree, node->left, key));
  else
    return(bt_findnode(tree, node->right, key));
}

static void *bt_nodefind(bt_Tree *tree, bt_node *node, void *key)
{
  int tempint;

  if(node == NULL)  return(NULL);

  tempint = (tree->compare)(key, node->value);

  if(tempint == 0)
    return(node->value);
  else
    if(tempint < 0)
      return(bt_nodefind(tree, node->left, key));
  else
    return(bt_nodefind(tree, node->right, key));
}

static void bt_destroynode(bt_node *node)
{
  if(node->left != NULL)
    bt_destroynode(node->left);

  if(node->right != NULL)
    bt_destroynode(node->right);

  free(node);
}

BinTree bt_dup(BinTree bt, DupFuncPtr dupfunc)
{
  BinTree result;
  bt_node *oldnode;

  ErrPushFunc("bt_dup");

  result = bt_create(bt->compare);

  /* Save previoulsy current */

  oldnode = bt->current_node;

  /* Duplicate nodes */

  bt_GoFirst(bt);
  while(!bt_EOT(bt))
  {
    bt_add(result, (dupfunc==NULL) ? bt_GetCur(bt) : dupfunc(bt_GetCur(bt)));
    bt_GoNext(bt);
  }

  /* restore previously current */

  bt->current_node = oldnode;
	
	result->count = bt->count;
	
  ErrPopFunc();

  return(result);
}

#ifndef WIN32
__inline
#endif
int bt_getCount( BinTree bt )
{
	return bt->count;
}

