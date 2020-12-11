//garbage collector in C
//Point one: we must make sure that we don't get rid of objects still in use
//Hence any object that's being referenced by a variable that's still in scope or
//Any object that's referenced by another object that's in use, is still in use
//and hence should not be collected by the GC.

//We're using a mark-sweep algorithm, invented by John McCarthy (inventor of Lisp)
//we start by the roots and traverse the entire object graph. Every time we reahc an object
//we mark it to be "true". Once we're done, we delete all the objects that aren't set to true.

//To test the algorithm we'll pretend to be writing an interpreter for a little language.
//It's dynamically typed, and has two types of objects: ints and pairs. 

//That will be all wrapped up in a little virtual machine structure, so that we have a stack.

#include <stdio.h>
#include <stdlib.h>


#define STACK_MAX 256
#define INT_OBJ_NUM_MAX 8

typedef enum{
    OBJ_INT,
    OBJ_PAIR
}ObjectType;

//I now use a tagged union to create pairs

typedef struct sObject{
    ObjectType type;
    unsigned char marked;

    struct sObject* next;

    union {
       /*OBJ INT*/
       int value;

       /*OBJ PAIR*/
       struct{
           struct sObject* head;
           struct sObject* tail;
       };
    };
}Object;

//Virtual Machine:

typedef struct {
    Object* stack[STACK_MAX];
    int stackSize;

    Object* firstObject;

    int numObjects;

    int maxObjects;
} VM;

void assert(int condition, const char* message){
    if(!condition){
        printf("%s\n", message);
        exit(1);
    }
}

//Now I need a function to create and initialize the VM

VM* newVM(){
    VM* vm = malloc(sizeof(VM));
    vm->stackSize = 0;
    vm->firstObject = NULL;
    vm->numObjects = 0;
    vm->maxObjects = INT_OBJ_NUM_MAX;
    return vm;
}

//Now we need to be able to manipulate its stack:

void push(VM* vm, Object* value){
    assert(vm->stackSize < STACK_MAX, "Stack overflow!");
    vm->stack[vm->stackSize++] = value;
}

Object* pop(VM* vm){
    assert(vm->stackSize > 0, "Stack underflow!");
    return vm->stack[--vm->stackSize];
}

//We're now able to create objects. For that we use this function:

Object* newObject(VM* vm, ObjectType type){
    
    if(vm->numObjects == vm->maxObjects) gc(vm);
    
    Object* object = malloc(sizeof(Object));
    object->type = type;
    object->next = vm->firstObject;
    vm->firstObject = object;
    object->marked = 0;

    vm->numObjects++;


    return object;
}

//The following objects are for pushing objects onto the VM's stack

void pushInt(VM* vm, int intValue) {
  Object* object = newObject(vm, OBJ_INT);
  object->value = intValue;
  push(vm, object);
}

Object* pushPair(VM* vm) {
  Object* object = newObject(vm, OBJ_PAIR);
  object->tail = pop(vm);
  object->head = pop(vm);

  push(vm, object);
  return object;
}

//functions used for marking. We modify newObject() to initialize marked to zero.

void mark(Object* object){
    
   if (object->marked) return;

    object->marked = 1;

    //We want to mark all elements of Pairs

    if(object->type == OBJ_PAIR){
        mark(object->head);
        mark(object->tail);
    }
}

void markAll(VM* vm){
    for(int i = 0; i < vm->stackSize; i++){
        mark(vm->stack[i]);
    }
}

//To sweep through and delete unmarked objects, we traverse our list:

void sweep(VM* vm){
    Object** object = &vm->firstObject;
    while(*object){
        if (!(*object)->marked){
            /*Object unreached, we remove it*/
            
            Object* unreached = *object;
            *object = unreached->next;
            free(unreached);

            vm->numObjects--;
        }else{
            /*Object reached, unmark it for next GC and move on to the next*/
            (*object)->marked = 0;
            object = &(*object)->next;
        }
    }
}

//We now just have to call the GC

void gc(VM* vm){
    int numObjects = vm->numObjects;
   
    markAll(vm);
    sweep(vm);

    vm->maxObjects = vm->numObjects == 0 ? INIT_OBJ_NUM_MAX : vm->numObjects * 2;

    printf("Collected %d objects, %d remaining.\n", numObjects - vm->numObjects,
         vm->numObjects);
}
