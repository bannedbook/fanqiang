/**
 * @file NCDObject.h
 * @author Ambroz Bizjak <ambrop7@gmail.com>
 * 
 * @section LICENSE
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BADVPN_NCDOBJECT_H
#define BADVPN_NCDOBJECT_H

#include <stddef.h>

#include <misc/debug.h>
#include <structure/LinkedList0.h>
#include <ncd/NCDVal_types.h>
#include <ncd/NCDStringIndex.h>
#include <ncd/static_strings.h>

/**
 * Represents an NCD object.
 * Objects expose the following functionalities:
 * - resolving variables by name,
 * - resolving objects by name,
 * - provide information for calling method-like statements.
 * 
 * The NCDObject structure must not be stored persistently; it is only
 * valid at the time it was obtained, and any change of state in the
 * execution of the NCD program may render the object invalid.
 * However, the structure does not contain any resources, and can freely
 * be passed around by value.
 */
typedef struct NCDObject_s NCDObject;

/**
 * Serves as an anchor point for NCDObjRef weak references.
 * A callback produces the temporary NCDObject values on demand.
 */
typedef struct NCDPersistentObj_s NCDPersistentObj;

/**
 * A weak reference to an NCDPersistentObj.
 * This means that the existence of a reference does not prevent
 * the NCDPersistentObj from going away, rather when it goes away
 * the NCDObjRef becomes a broken reference - any further
 * dereference operations will fail "gracefully".
 */
typedef struct NCDObjRef_s NCDObjRef;

/**
 * Callback function for variable resolution requests.
 * 
 * @param obj const pointer to the NCDObject this is being called for.
 *            {@link NCDObject_DataPtr} and {@link NCDObject_DataInt} can be
 *            used to retrieve state information which was passed to
 *            {@link NCDObject_Build} or {@link NCDObject_BuildFull}.
 * @param name name of the variable being resolved, in form of an {@link NCDStringIndex}
 *             string identifier
 * @param mem pointer to the memory object where the resulting value should be
 *            constructed
 * @param out_value If the variable exists, *out_value should be set to the value
 *                  reference to the result value. If the variable exists but there
 *                  was an error constructing the value, should be set to an
 *                  invalid value reference. Can be modified even if the variable
 *                  does not exist.
 * @return 1 if the variable exists, 0 if not
 */
typedef int (*NCDObject_func_getvar) (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out_value);

/**
 * Callback function for object resolution requests.
 * 
 * @param obj const pointer to the NCDObject this is being called for.
 *            {@link NCDObject_DataPtr} and {@link NCDObject_DataInt} can be
 *            used to retrieve state information which was passed to
 *            {@link NCDObject_Build} or {@link NCDObject_BuildFull}.
 * @param name name of the object being resolved, in form of an {@link NCDStringIndex}
 *             string identifier
 * @param out_object If the object exists, *out_object should be set to the result
 *                   object. Can be modified even if the object does not exist.
 * @return 1 if the object exists, 0 if not
 */
typedef int (*NCDObject_func_getobj) (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object);

/**
 * A callback of NCDPersistentObj which produces the corresponding temporary
 * NCDObject value.
 */
typedef NCDObject (*NCDPersistentObj_func_retobj) (NCDPersistentObj const *pobj);

struct NCDObject_s {
    NCD_string_id_t type;
    int data_int;
    void *data_ptr;
    void *method_user;
    NCDObject_func_getvar func_getvar;
    NCDObject_func_getobj func_getobj;
    NCDPersistentObj *pobj;
};

struct NCDPersistentObj_s {
    NCDPersistentObj_func_retobj func_retobj;
    LinkedList0 refs_list;
};

struct NCDObjRef_s {
    NCDPersistentObj *pobj;
    LinkedList0Node refs_list_node;
};

/**
 * Basic object construction function.
 * This is equivalent to calling {@link NCDObject_BuildFull} with data_int=0,
 * method_user=data_ptr and pobj=NULL. See that function for detailed documentation.
 */
NCDObject NCDObject_Build (NCD_string_id_t type, void *data_ptr, NCDObject_func_getvar func_getvar, NCDObject_func_getobj func_getobj);

/**
 * Constructs an {@link NCDObject} structure.
 * This is the full version where all supported parameters have to be provided.
 * In most cases, {@link NCDObject_Build} will suffice.
 * 
 * @param type type of the object for the purpose of calling method-like statements
 *             on the object, in form of an {@link NCDStringIndex} string identifier.
 *             May be set to -1 if the object has no methods.
 * @param data_ptr state-keeping pointer which can be restored from callbacks using
 *                 {@link NCDObject_DataPtr}
 * @param data_int state-keeping integer which can be restored from callbacks using
 *                 {@link NCDObject_DataInt}
 * @param method_user state-keeping pointer to be passed to new method-like statements
 *                    created using this object. The value of this argument will be
 *                    available as params->method_user within the {@link NCDModule_func_new2}
 *                    module backend callback.
 * @param func_getvar callback for resolving variables within the object. This must not
 *                    be NULL; if the object exposes no variables, pass {@link NCDObject_no_getvar}.
 * @param func_getobj callback for resolving objects within the object. This must not
 *                    be NULL; if the object exposes no objects, pass {@link NCDObject_no_getobj}.
 * @param pobj pointer to an NCDPersistentObj, serving as a reference anchor for this
 *             object. NULL if references are not supported by the object.
 * @return an NCDObject structure encapsulating the information given
 */
NCDObject NCDObject_BuildFull (NCD_string_id_t type, void *data_ptr, int data_int, void *method_user, NCDObject_func_getvar func_getvar, NCDObject_func_getobj func_getobj, NCDPersistentObj *pobj);

/**
 * Returns the 'type' attribute; see {@link NCDObject_BuildFull}.
 */
NCD_string_id_t NCDObject_Type (const NCDObject *o);

/**
 * Returns the 'data_ptr' attribute; see {@link NCDObject_BuildFull}.
 */
void * NCDObject_DataPtr (const NCDObject *o);

/**
 * Returns the 'data_int' attribute; see {@link NCDObject_BuildFull}.
 */
int NCDObject_DataInt (const NCDObject *o);

/**
 * Returns the 'method_user' attribute; see {@link NCDObject_BuildFull}.
 */
void * NCDObject_MethodUser (const NCDObject *o);

/**
 * Attempts to resolve a variable within the object.
 * This just calls {@link NCDObject_func_getvar}, but also has some assertions to detect
 * incorrect behavior of the callback.
 */
int NCDObject_GetVar (const NCDObject *o, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out_value) WARN_UNUSED;

/**
 * Attempts to resolve an object within the object.
 * This just calls {@link NCDObject_func_getobj}, but also has some assertions to detect
 * incorrect behavior of the callback.
 */
int NCDObject_GetObj (const NCDObject *o, NCD_string_id_t name, NCDObject *out_object) WARN_UNUSED;

/**
 * Returns a pointer to the NCDPersistentObj pointer for this
 * object (if any).
 */
NCDPersistentObj * NCDObject_Pobj (const NCDObject *o);

/**
 * Resolves a variable expression starting with this object.
 * A variable expression is usually represented in dotted form,
 * e.g. object1.object2.variable (for a named variable) or object1.object2.object3
 * (for an empty string variable). This function however receives the expression
 * as an array of string identifiers.
 * 
 * Consult the implementation for exact semantics of variable expression resolution.
 * 
 * @param o object to start the resolution with
 * @param names pointer to an array of names for the resolution. May be NULL if num_names is 0.
 * @param num_names number in names in the array
 * @param mem pointer to the memory object where the resulting value
 *            should be constructed
 * @param out_value If the variable exists, *out_value will be set to the value
 *                  reference to the result value. If the variable exists but there
 *                  was an error constructing the value, will be set to an
 *                  invalid value reference. May be modified even if the variable
 *                  does not exist.
 * @return 1 if the variable exists, 0 if not
 */
int NCDObject_ResolveVarExprCompact (const NCDObject *o, const NCD_string_id_t *names, size_t num_names, NCDValMem *mem, NCDValRef *out_value) WARN_UNUSED;

/**
 * Resolves an object expression starting with this object.
 * An object expression is usually represented in dotted form,
 * e.g. object1.object2.object3. This function however receives the expression
 * as an array of string identifiers.
 * 
 * Consult the implementation for exact semantics of object expression resolution.
 * 
 * @param o object to start the resolution with
 * @param names pointer to an array of names for the resolution. May be NULL if num_names is 0.
 * @param num_names number in names in the array
 * @param out_object If the object exists, *out_object will be set to the result
 *                   object. May be modified even if the object does not exist.
 * @return 1 if the object exists, 0 if not
 */
int NCDObject_ResolveObjExprCompact (const NCDObject *o, const NCD_string_id_t *names, size_t num_names, NCDObject *out_object) WARN_UNUSED;

/**
 * Returns 0. This can be used as a dummy variable resolution callback.
 */
int NCDObject_no_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out_value);

/**
 * Returns 0. This can be used as a dummy object resolution callback.
 */
int NCDObject_no_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object);

/**
 * Initializes an NCDPersistentObj, an anchor point for NCDObjRef
 * refrences.
 * 
 * @param func_retobj callback for producing NCDObject temporary objects
 */
void NCDPersistentObj_Init (NCDPersistentObj *o, NCDPersistentObj_func_retobj func_retobj);

/**
 * Frees the NCDPersistentObj.
 * This breaks any NCDObjRef references referencing this object.
 * This means that any further NCDObjRef_Deref calls on those
 * references will fail.
 */
void NCDPersistentObj_Free (NCDPersistentObj *o);

/**
 * Initializes an object reference.
 * 
 * @param pobj the NCDPersistentObj for the reference, or NULL to make
 *             a broken reference
 */
void NCDObjRef_Init (NCDObjRef *o, NCDPersistentObj *pobj);

/**
 * Frees the object reference.
 */
void NCDObjRef_Free (NCDObjRef *o);

/**
 * Dereferences the object reference.
 * Note that the refernce will be broken if the originally
 * reference NCDPersistentObj was freed.
 * 
 * @param res on success, *res will be set to the resulting NCDObject
 * @return 1 on success, 0 on failure (broken reference)
 */
int NCDObjRef_Deref (NCDObjRef *o, NCDObject *res) WARN_UNUSED;

#endif
