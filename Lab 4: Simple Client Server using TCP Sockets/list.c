// list/list.c
// Complete linked list implementation (1-based indexing) - FIXED VERSION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "list.h"

list_t *list_alloc() {
  list_t *mylist = (list_t *) malloc(sizeof(list_t));
  if (mylist == NULL) return NULL;
  mylist->head = NULL;
  return mylist;
}

void list_free(list_t *l) {
  if (l == NULL) return;
  node_t *cur = l->head;
  while (cur != NULL) {
    node_t *next = cur->next;
    free(cur);
    cur = next;
  }
  free(l);
}

void list_print(list_t *l) {
  if (l == NULL) {
    printf("NULL\n");
    return;
  }
  node_t *cur = l->head;
  if (cur == NULL) {
    printf("NULL\n");
    return;
  }
  while (cur != NULL) {
    printf("%d->", cur->value);
    cur = cur->next;
  }
  printf("NULL\n");
}

char * listToString(list_t *l) {
  if (l == NULL || l->head == NULL) {
    char *buf = (char *) malloc(5); // "NULL" + '\0'
    if (!buf) return NULL;
    strcpy(buf, "NULL");
    return buf;
  }

  // compute length to allocate enough space: assume up to 11 chars per element (sign + up to 10 digits) + "->"
  int len = 0;
  node_t *cur = l->head;
  while (cur) { len++; cur = cur->next; }

  size_t bufsize = (size_t)len * 15 + 6; // safe upper bound
  char *buf = (char *) malloc(bufsize);
  if (!buf) return NULL;

  char *p = buf;
  cur = l->head;
  while (cur) {
    p += sprintf(p, "%d->", cur->value);
    cur = cur->next;
  }
  sprintf(p, "NULL");
  return buf;
}

int list_length(list_t *l) {
  if (l == NULL) return 0;
  int count = 0;
  node_t *cur = l->head;
  while (cur) {
    count++;
    cur = cur->next;
  }
  return count;
}

void list_add_to_back(list_t *l, elem value) {
  if (l == NULL) return;
  node_t *n = getNode(value);
  if (n == NULL) return; // Check if node allocation failed
  
  if (l->head == NULL) {
    l->head = n;
    return;
  }
  node_t *cur = l->head;
  while (cur->next) cur = cur->next;
  cur->next = n;
}

void list_add_to_front(list_t *l, elem value) {
  if (l == NULL) return;
  node_t *n = getNode(value);
  if (n == NULL) return; // Check if node allocation failed
  n->next = l->head;
  l->head = n;
}

node_t * getNode(elem value) {
  node_t *mynode = (node_t *) malloc(sizeof(node_t));
  if (!mynode) return NULL;
  mynode->value = value;
  mynode->next = NULL;
  return mynode;
}

/* Insert at 1-based index. index == 1 => insert at front.
 * If index is greater than length+1, do nothing. */
void list_add_at_index(list_t *l, elem value, int index) {
  if (l == NULL || index < 1) return;
  
  if (index == 1) {
    list_add_to_front(l, value);
    return;
  }
  
  // Handle empty list - can only insert at index 1
  if (l->head == NULL) {
    if (index == 1) {
      list_add_to_front(l, value);
    }
    return;
  }
  
  int pos = 1;
  node_t *cur = l->head;
  // move to node at position index-1
  while (cur != NULL && pos < index - 1) {
    cur = cur->next;
    pos++;
  }
  
  // If we couldn't reach position index-1, then index is too large
  if (cur == NULL) {
    return;
  }
  
  node_t *n = getNode(value);
  if (n == NULL) return; // Check if node allocation failed
  n->next = cur->next;
  cur->next = n;
}

elem list_remove_from_back(list_t *l) {
  if (l == NULL || l->head == NULL) return -1;
  
  // Only one element
  if (l->head->next == NULL) {
    elem v = l->head->value;
    free(l->head);
    l->head = NULL;
    return v;
  }
  
  // Find second-to-last node
  node_t *cur = l->head;
  while (cur->next != NULL && cur->next->next != NULL) {
    cur = cur->next;
  }
  
  elem v = cur->next->value;
  free(cur->next);
  cur->next = NULL;
  return v;
}

elem list_remove_from_front(list_t *l) {
  if (l == NULL || l->head == NULL) return -1;
  node_t *tmp = l->head;
  elem v = tmp->value;
  l->head = tmp->next;
  free(tmp);
  return v;
}

/* Remove at 1-based index. index==1 => remove front.
 * Returns removed element or -1 on error/out-of-range. */
elem list_remove_at_index(list_t *l, int index) {
  if (l == NULL || l->head == NULL || index < 1) return -1;
  
  if (index == 1) {
    return list_remove_from_front(l);
  }
  
  int pos = 1;
  node_t *cur = l->head;
  // Find node at position index-1
  while (cur != NULL && pos < index - 1) {
    cur = cur->next;
    pos++;
  }
  
  // Check if we found the node and it has a next node to remove
  if (cur == NULL || cur->next == NULL) {
    return -1; // out of range
  }
  
  node_t *target = cur->next;
  elem v = target->value;
  cur->next = target->next;
  free(target);
  return v;
}

bool list_is_in(list_t *l, elem value) {
  if (l == NULL) return false;
  node_t *cur = l->head;
  while (cur) {
    if (cur->value == value) return true;
    cur = cur->next;
  }
  return false;
}

/* 1-based index get. index==1 => head. return -1 if invalid/out of range */
elem list_get_elem_at(list_t *l, int index) {
  if (l == NULL || index < 1) return -1;
  int pos = 1;
  node_t *cur = l->head;
  while (cur && pos < index) {
    cur = cur->next;
    pos++;
  }
  if (cur == NULL) return -1;
  return cur->value;
}

/* find first occurrence; returns 1-based index or -1 if not found */
int list_get_index_of(list_t *l, elem value) {
  if (l == NULL) return -1;
  int pos = 1;
  node_t *cur = l->head;
  while (cur) {
    if (cur->value == value) return pos;
    cur = cur->next;
    pos++;
  }
  return -1;
}