/**
 * @file NCDModule.h
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

#ifndef BADVPN_NCD_NCDMODULE_H
#define BADVPN_NCD_NCDMODULE_H

#include <stdarg.h>

#include <misc/debug.h>
#include <system/BReactor.h>
#include <base/BLog.h>
#include <ncd/NCDVal.h>
#include <ncd/NCDObject.h>
#include <ncd/NCDStringIndex.h>

#ifndef BADVPN_NO_PROCESS
#include <system/BProcess.h>
#endif
#ifndef BADVPN_NO_UDEV
#include <udevmonitor/NCDUdevManager.h>
#endif
#ifndef BADVPN_NO_RANDOM
#include <random/BRandom2.h>
#endif

#define NCDMODULE_EVENT_UP 1
#define NCDMODULE_EVENT_DOWN 2
#define NCDMODULE_EVENT_DOWNUP 3
#define NCDMODULE_EVENT_DEAD 4
#define NCDMODULE_EVENT_DEADERROR 5

struct NCDModuleInst_s;
struct NCDModuleProcess_s;
struct NCDModuleGroup;
struct NCDInterpModule;
struct NCDInterpModuleGroup;
struct NCDCall_interp_shared;
struct NCDInterpFunction;

/**
 * Function called to inform the interpeter of state changes of the
 * module instance.
 * Possible events are:
 * 
 * - NCDMODULE_EVENT_UP: the instance came up.
 *   The instance was in down state.
 *   The instance enters up state.
 * 
 * - NCDMODULE_EVENT_DOWN: the instance went down.
 *   The instance was in up state.
 *   The instance enters down state.
 * 
 *   After the instance goes down, the interpreter should eventually call
 *   {@link NCDModuleInst_Clean} or {@link NCDModuleInst_Die}, unless
 *   the module goes up again.
 * 
 * - NCDMODULE_EVENT_DEAD: the module died. To determine if the module
 *   died with error, read the is_error member of {@link NCDModuleInst}.
 *   The instance enters dead state.
 * 
 * This function is not being called in event context. The interpreter should
 * only update its internal state, and visibly react only via jobs that it pushes
 * from within this function. The only exception is that it may free the
 * instance from within the NCDMODULE_EVENT_DEAD event.
 * 
 * @param inst the module instance
 * @param event event number
 */
typedef void (*NCDModuleInst_func_event) (struct NCDModuleInst_s *inst, int event);

/**
 * Function called when the module instance wants the interpreter to
 * resolve an object from the point of view of its statement.
 * The instance will not be in dead state.
 * This function must not have any side effects.
 * 
 * @param inst the module instance
 * @param name name of the object as an {@link NCDStringIndex} identifier
 * @param out_object the object will be returned here
 * @return 1 on success, 0 on failure
 */
typedef int (*NCDModuleInst_func_getobj) (struct NCDModuleInst_s *inst, NCD_string_id_t name, NCDObject *out_object);

/**
 * Function called when the module instance wants the interpreter to
 * create a new process backend from a process template.
 * The instance will not be in dead state.
 * 
 * On success, the interpreter must have called {@link NCDModuleProcess_Interp_SetHandlers}
 * from within this function, to allow communication with the controller of the process.
 * On success, the new process backend enters down state.
 * 
 * This function is not being called in event context. The interpreter should
 * only update its internal state, and visibly react only via jobs that it pushes
 * from within this function.
 * 
 * @param user value of 'user' member of {@link NCDModuleInst_iparams}
 * @param p handle for the new process backend
 * @param template_name name of the template to create the process from,
 *                      as an {@link NCDStringIndex} identifier
 * @return 1 on success, 0 on failure
 */
typedef int (*NCDModuleInst_func_initprocess) (void *user, struct NCDModuleProcess_s *p, NCD_string_id_t template_name);

/**
 * Function called when the module instance wants the interpreter to
 * initiate termination, as if it received an external terminatio request (signal).
 * 
 * @param user value of 'user' member of {@link NCDModuleInst_iparams}
 * @param exit_code exit code to return the the operating system. This overrides any previously
 *                  set exit code, and will be overriden by a signal to the value 1.
 *   
 */
typedef void (*NCDModuleInst_func_interp_exit) (void *user, int exit_code);

/**
 * Function called when the module instance wants the interpreter to
 * provide its extra command line arguments.
 * 
 * @param user value of 'user' member of {@link NCDModuleInst_iparams}
 * @param mem value memory to use
 * @param out_value write value reference here on success
 * @return 1 if available, 0 if not available. If available, but out of memory, return 1
 *         and an invalid value.
 */
typedef int (*NCDModuleInst_func_interp_getargs) (void *user, NCDValMem *mem, NCDValRef *out_value);

/**
 * Function called when the module instance wants the interpreter to
 * provide its retry time.
 * 
 * @param user value of 'user' member of {@link NCDModuleInst_iparams}
 * @return retry time in milliseconds
 */
typedef btime_t (*NCDModuleInst_func_interp_getretrytime) (void *user);

/**
 * Function called when the module instance wants the interpreter to
 * load a new module group.
 * 
 * @param user value of 'user' member of {@link NCDModuleInst_iparams}
 * @param group module group to load
 * @return 1 on success, 0 on failure
 */
typedef int (*NCDModuleInst_func_interp_loadgroup) (void *user, const struct NCDModuleGroup *group);

#define NCDMODULEPROCESS_EVENT_UP 1
#define NCDMODULEPROCESS_EVENT_DOWN 2
#define NCDMODULEPROCESS_EVENT_TERMINATED 3

/**
 * Handler which reports process state changes from the interpreter.
 * Possible events are:
 * 
 * - NCDMODULEPROCESS_EVENT_UP: the process went up.
 *   The process was in down state.
 *   The process enters up state.
 * 
 * - NCDMODULEPROCESS_EVENT_DOWN: the process went down.
 *   The process was in up state.
 *   The process enters waiting state.
 * 
 *   NOTE: the process enters waiting state, NOT down state, and is paused.
 *   To allow the process to continue, call {@link NCDModuleProcess_Continue}.
 * 
 * - NCDMODULEPROCESS_EVENT_TERMINATED: the process terminated.
 *   The process was in terminating state.
 *   The process enters terminated state.
 * 
 * @param user pointer to the process. Use {@link UPPER_OBJECT} to retrieve the pointer
 *             to the containing struct.
 * @param event event number
 */
typedef void (*NCDModuleProcess_handler_event) (struct NCDModuleProcess_s *process, int event);

/**
 * Function called when the interpreter wants to resolve a special
 * object in the process.
 * This function must have no side effects.
 * 
 * @param user pointer to the process. Use {@link UPPER_OBJECT} to retrieve the pointer
 *             to the containing struct.
 * @param name name of the object as an {@link NCDStringIndex} identifier
 * @param out_object the object will be returned here
 * @return 1 on success, 0 on failure
 */
typedef int (*NCDModuleProcess_func_getspecialobj) (struct NCDModuleProcess_s *process, NCD_string_id_t name, NCDObject *out_object);

#define NCDMODULEPROCESS_INTERP_EVENT_CONTINUE 1
#define NCDMODULEPROCESS_INTERP_EVENT_TERMINATE 2

/**
 * Function called to report process backend requests to the interpreter.
 * Possible events are:
 * 
 * - NCDMODULEPROCESS_INTERP_EVENT_CONTINUE: the process can continue.
 *   The process backend was in waiting state.
 *   The process backend enters down state.
 * 
 * - NCDMODULEPROCESS_INTERP_EVENT_TERMINATE: the process should terminate.
 *   The process backend was in down, up or waiting state.
 *   The process backend enters terminating state.
 * 
 *   The interpreter should call {@link NCDModuleProcess_Interp_Terminated}
 *   when the process terminates.
 * 
 * This function is not being called in event context. The interpreter should
 * only update its internal state, and visibly react only via jobs that it pushes
 * from within this function.
 * 
 * @param user as in {@link NCDModuleProcess_Interp_SetHandlers}
 * @param event event number
 */
typedef void (*NCDModuleProcess_interp_func_event) (void *user, int event);

/**
 * Function called to have the interpreter resolve an object within the process
 * of a process backend.
 * This function must not have any side effects.
 * 
 * @param user as in {@link NCDModuleProcess_Interp_SetHandlers}
 * @param name name of the object as an {@link NCDStringIndex} identifier
 * @param out_object the object will be returned here
 * @return 1 on success, 0 in failure
 */
typedef int (*NCDModuleProcess_interp_func_getobj) (void *user, NCD_string_id_t name, NCDObject *out_object);

struct NCDModule;

/**
 * Contains parameters to the module initialization function
 * ({@link NCDModule_func_new2}) that are passed indirectly.
 */
struct NCDModuleInst_new_params {
    /**
     * A reference to the argument list for the module instance.
     * The reference remains valid as long as the backend instance
     * exists.
     */
    NCDValRef args;
    
    /**
     * If the module instance corresponds to a method-like statement,
     * this pointer identifies the object it is being invoked with.
     * If the object is a statement (i.e. a {@link NCDModuleInst}), then this
     * will be the NCDModuleInst pointer, and {@link NCDModuleInst_Backend_GetUser}
     * can be called on this to retrieve the pointer to preallocated memory for
     * the backend instance; *unless* {@link NCDModuleInst_Backend_PassMemToMethods}
     * was called for the object on which the method is being called, in which case
     * this will directly point to the preallocated memory.
     * On the other hand, if this is a method on an internal object built using
     * only {@link NCDObject_Build} or {@link NCDObject_BuildFull},
     * this pointer will be whatever was passed as the "data_ptr" argument, for the
     * first function, and as "method_user", for the latter function.
     */
    void *method_user;
};

/**
 * Contains parameters to {@link NCDModuleInst_Init} that are passed indirectly.
 * This itself only contains parameters related to communication between the
 * backend and the creator of the module instance; other parameters are passed
 * via the iparams member;
 */
struct NCDModuleInst_params {
    /**
     * Callback to report state changes.
     */
    NCDModuleInst_func_event func_event;
    /**
     * Callback to resolve objects from the viewpoint of the instance.
     */
    NCDModuleInst_func_getobj func_getobj;
    /**
     * Log function which appends a log prefix with {@link BLog_Append}.
     */
    BLog_logfunc logfunc;
    /**
     * Pointer to an {@link NCDModuleInst_iparams} structure, which exposes
     * services provided by the interpreter.
     */
    const struct NCDModuleInst_iparams *iparams;
};

/**
 * Contains parameters to {@link NCDModuleInst_Init} that are passed indirectly.
 * This only contains parameters related to services provided by the interpreter.
 */
struct NCDModuleInst_iparams {
    /**
     * Reactor we live in.
     */
    BReactor *reactor;
#ifndef BADVPN_NO_PROCESS
    /**
     * Process manager.
     */
    BProcessManager *manager;
#endif
#ifndef BADVPN_NO_UDEV
    /**
     * Udev manager.
     */
    NCDUdevManager *umanager;
#endif
#ifndef BADVPN_NO_RANDOM
    /**
     * Random number generator.
     */
    BRandom2 *random2;
#endif
    /**
     * String index which keeps a mapping between strings and string identifiers.
     */
    NCDStringIndex *string_index;
    /**
     * Pointer passed to the interpreter callbacks below, for state keeping.
     */
    void *user;
    /**
     * Callback to create a new template process.
     */
    NCDModuleInst_func_initprocess func_initprocess;
    /**
     * Callback to request interpreter termination.
     */
    NCDModuleInst_func_interp_exit func_interp_exit;
    /**
     * Callback to get extra command line arguments.
     */
    NCDModuleInst_func_interp_getargs func_interp_getargs;
    /**
     * Callback to get retry time.
     */
    NCDModuleInst_func_interp_getretrytime func_interp_getretrytime;
    /**
     * Callback to load a module group.
     */
    NCDModuleInst_func_interp_loadgroup func_loadgroup;
};

/**
 * Module instance.
 * The module instance is initialized by the interpreter by calling
 * {@link NCDModuleInst_Init}. It is implemented by a module backend
 * specified in a {@link NCDModule}.
 */
typedef struct NCDModuleInst_s {
    const struct NCDInterpModule *m;
    const struct NCDModuleInst_params *params;
    void *mem; // not modified by NCDModuleInst (but passed to module)
    unsigned int state:3;
    unsigned int pass_mem_to_methods:1;
    unsigned int istate:3; // untouched by NCDModuleInst
    NCDPersistentObj pobj;
    DebugObject d_obj;
} NCDModuleInst;

/**
 * Weak NCDModuleInst reference.
 */
typedef struct {
    NCDObjRef objref;
    DebugObject d_obj;
} NCDModuleRef;

/**
 * Process created from a process template on behalf of a module backend
 * instance, implemented by the interpreter.
 */
typedef struct NCDModuleProcess_s {
    NCDValRef args;
    NCDModuleProcess_handler_event handler_event;
    NCDModuleProcess_func_getspecialobj func_getspecialobj;
    void *interp_user;
    NCDModuleProcess_interp_func_event interp_func_event;
    NCDModuleProcess_interp_func_getobj interp_func_getobj;
#ifndef NDEBUG
    int state;
#endif
    DebugObject d_obj;
} NCDModuleProcess;

/**
 * Initializes an instance of an NCD module.
 * The instance is initialized in down state.
 * WARNING: this directly calls the module backend; expect to be called back
 * 
 * This and other non-Backend methods are the interpreter interface.
 * The Backend methods are the module backend interface and are documented
 * independently with their own logical states.
 * 
 * NOTE: the instance structure \a n should have the member 'mem' initialized
 * to point to preallocated memory for the statement. This memory must be
 * at least m->prealloc_size big and must be properly aligned for any object.
 * The 'mem' pointer is never modified by NCDModuleInst, so that the interpreter
 * can use it as outside the lifetime of NCDModuleInst.
 * 
 * @param n the instance
 * @param m pointer to the {@link NCDInterpModule} structure representing the module
 *          to be instantiated
 * @param method_context a context pointer passed to the module backend, applicable to method-like
 *                       statements only. This should be set to the 'user' member of the
 *                       {@link NCDObject} which represents the base object for the method.
 *                       The caller must ensure that the NCDObject that was used is of the type
 *                       expected by the module being instanciated.
 * @param args arguments to the module. Must be a list value. Must be available and unchanged
 *             as long as the instance exists.
 * @param user argument to callback functions
 * @param params more parameters, see {@link NCDModuleInst_params}
 */
void NCDModuleInst_Init (NCDModuleInst *n, const struct NCDInterpModule *m, void *method_context, NCDValRef args, const struct NCDModuleInst_params *params);

/**
 * Frees the instance.
 * The instance must be in dead state.
 * 
 * @param n the instance
 */
void NCDModuleInst_Free (NCDModuleInst *n);

/**
 * Requests the instance to die.
 * The instance must be in down or up state.
 * The instance enters dying state.
 * WARNING: this directly calls the module backend; expect to be called back
 * 
 * @param n the instance
 */
void NCDModuleInst_Die (NCDModuleInst *n);

/**
 * Attempts to destroy the instance immediately.
 * This function can be used to optimize destroying instances of modules which
 * don't specify any {@link NCDModule_func_die} handler. If immediate destruction
 * is not possible, this does nothing and returns 0; {@link NCDModuleInst_Die}
 * should be used to destroy the instance instead. If however immediate destruction
 * is possible, this destroys the module instance and returns 1; {@link NCDModuleInst_Free}
 * must not be called after that.
 * The instance must be in down or up state, as for {@link NCDModuleInst_Die}.
 * 
 * @param n the instance
 * @return 1 if destruction was performed, 0 if not
 */
int NCDModuleInst_TryFree (NCDModuleInst *n);

/**
 * Informs the module that it is in a clean state to proceed.
 * The instance must be in down state.
 * WARNING: this directly calls the module backend; expect to be called back
 * 
 * @param n the instance
 */
void NCDModuleInst_Clean (NCDModuleInst *n);

/**
 * Returns an {@link NCDObject} which can be used to resolve variables and objects
 * within this instance, as well as call its methods. The resulting object may only
 * be used immediately, and becomes invalid when the instance is freed.
 * 
 * @param n the instance
 * @return an NCDObject for this instance
 */
NCDObject NCDModuleInst_Object (NCDModuleInst *n);

/**
 * If this is called, any methods called on this object will receive the preallocated
 * memory pointer as the object state pointer. This means that in the
 * {@link NCDModule_func_getvar2} function which is called when a method is created,
 * the preallocated memory should be accessed as params->method_user.
 * By default, however, params->method_user points to the NCDModuleInst of the base
 * object, and {@link NCDModuleInst_Backend_GetUser} is needed to retrieve the
 * preallocated memory pointer.
 */
void NCDModuleInst_Backend_PassMemToMethods (NCDModuleInst *n);

/**
 * Retuns the state pointer passed to handlers of a module backend instance;
 * see {@link NCDModule_func_new2}.
 * 
 * @param n backend instance handle
 * @return argument passed to handlers
 */
void * NCDModuleInst_Backend_GetUser (NCDModuleInst *n);

/**
 * Puts the backend instance into up state.
 * The instance must be in down state.
 * The instance enters up state.
 * 
 * @param n backend instance handle
 */
void NCDModuleInst_Backend_Up (NCDModuleInst *n);

/**
 * Puts the backend instance into down state.
 * The instance must be in up state.
 * The instance enters down state.
 * 
 * @param n backend instance handle
 */
void NCDModuleInst_Backend_Down (NCDModuleInst *n);

/**
 * Puts the backend instance into down state, then immediatly back into the up state.
 * This effectively causes the interpreter to start backtracking to this statement.
 * The instance must be in up state, and remains in up state.
 * 
 * @param n backend instance handle
 */
void NCDModuleInst_Backend_DownUp (NCDModuleInst *n);

/**
 * Destroys the backend instance.
 * The backend instance handle becomes invalid and must not be used from
 * the backend any longer.
 * 
 * @param n backend instance handle
 */
void NCDModuleInst_Backend_Dead (NCDModuleInst *n);

/**
 * Like {@link NCDModuleInst_Backend_Dead}, but also reports an error condition
 * to the interpreter.
 */
void NCDModuleInst_Backend_DeadError (NCDModuleInst *n);

/**
 * Resolves an object for a backend instance, from the point of the instance's
 * statement in the containing process.
 * 
 * @param n backend instance handle
 * @param name name of the object to resolve as an {@link NCDStringIndex} identifier
 * @param out_object the object will be returned here
 * @return 1 on success, 0 on failure
 */
int NCDModuleInst_Backend_GetObj (NCDModuleInst *n, NCD_string_id_t name, NCDObject *out_object) WARN_UNUSED;

/**
 * Logs a backend instance message.
 * 
 * @param n backend instance handle
 * @param channel log channel
 * @param level loglevel
 * @param fmt format string as in printf, arguments follow
 */
void NCDModuleInst_Backend_Log (NCDModuleInst *n, int channel, int level, const char *fmt, ...);

/**
 * Like {@link NCDModuleInst_Backend_Log}, but the extra arguments are passed
 * as a va_list. This allows creation of logging wrappers.
 */
void NCDModuleInst_Backend_LogVarArg (NCDModuleInst *n, int channel, int level, const char *fmt, va_list vl);

/**
 * Returns a logging context. The context is valid until the backend dies.
 */
BLogContext NCDModuleInst_Backend_LogContext (NCDModuleInst *n);

/**
 * Initiates interpreter termination.
 * 
 * @param n backend instance handle
 * @param exit_code exit code to return to the operating system. This overrides
 *                  any previously set exit code, and will be overriden by a
 *                  termination signal to the value 1.
 */
void NCDModuleInst_Backend_InterpExit (NCDModuleInst *n, int exit_code);

/**
 * Retrieves extra command line arguments passed to the interpreter.
 * 
 * @param n backend instance handle
 * @param mem value memory to use
 * @param out_value the arguments will be written here on success as a list value
 * @return 1 if available, 0 if not available. If available, but out of memory, returns 1
 *         and an invalid value.
 */
int NCDModuleInst_Backend_InterpGetArgs (NCDModuleInst *n, NCDValMem *mem, NCDValRef *out_value);

/**
 * Returns the retry time of the intepreter.
 * 
 * @param n backend instance handle
 * @return retry time in milliseconds
 */
btime_t NCDModuleInst_Backend_InterpGetRetryTime (NCDModuleInst *n);

/**
 * Loads a module group into the interpreter.
 * 
 * @param n backend instance handle
 * @param group module group to load
 * @return 1 on success, 0 on failure
 */
int NCDModuleInst_Backend_InterpLoadGroup (NCDModuleInst *n, const struct NCDModuleGroup *group);

/**
 * Initializes a weak reference to a module instance.
 * The instane must no have had NCDModuleInst_Backend_PassMemToMethods
 * called.
 */
void NCDModuleRef_Init (NCDModuleRef *o, NCDModuleInst *inst);

/**
 * Frees the reference.
 */
void NCDModuleRef_Free (NCDModuleRef *o);

/**
 * Dereferences the reference.
 * If the reference was broken, returns NULL.
 */
NCDModuleInst * NCDModuleRef_Deref (NCDModuleRef *o);

/**
 * Initializes a process in the interpreter from a process template.
 * This must be called on behalf of a module backend instance.
 * The process is initializes in down state.
 * 
 * @param o the process
 * @param n backend instance whose interpreter will be providing the process
 * @param template_name name of the process template as an {@link NCDStringIndex} identifier
 * @param args arguments to the process. Must be an invalid value or a list value.
 *             The value must be available and unchanged while the process exists.
 * @param handler_event handler which reports events about the process from the
 *                      interpreter
 * @return 1 on success, 0 on failure
 */
int NCDModuleProcess_InitId (NCDModuleProcess *o, NCDModuleInst *n, NCD_string_id_t template_name, NCDValRef args, NCDModuleProcess_handler_event handler_event) WARN_UNUSED;

/**
 * Wrapper around {@link NCDModuleProcess_InitId} which takes the template name as an
 * {@link NCDValRef}, which must point to a string value.
 */
int NCDModuleProcess_InitValue (NCDModuleProcess *o, NCDModuleInst *n, NCDValRef template_name, NCDValRef args, NCDModuleProcess_handler_event handler_event) WARN_UNUSED;

/**
 * Frees the process.
 * The process must be in terminated state.
 * 
 * @param o the process
 */
void NCDModuleProcess_Free (NCDModuleProcess *o);

/**
 * Does nothing.
 * The process must be in terminated state.
 * 
 * @param o the process
 */
void NCDModuleProcess_AssertFree (NCDModuleProcess *o);

/**
 * Sets callback functions for providing special objects within the process.
 * 
 * @param o the process
 * @param func_getspecialobj function for resolving special objects, or NULL
 */
void NCDModuleProcess_SetSpecialFuncs (NCDModuleProcess *o, NCDModuleProcess_func_getspecialobj func_getspecialobj);

/**
 * Continues the process after the process went down.
 * The process must be in waiting state.
 * The process enters down state.
 * 
 * @param o the process
 */
void NCDModuleProcess_Continue (NCDModuleProcess *o);

/**
 * Requests the process to terminate.
 * The process must be in down, up or waiting state.
 * The process enters terminating state.
 * 
 * @param o the process
 */
void NCDModuleProcess_Terminate (NCDModuleProcess *o);

/**
 * Resolves an object within the process from the point
 * at the end of the process.
 * This function has no side effects.
 * 
 * @param o the process
 * @param name name of the object to resolve as an {@link NCDStringIndex} identifier
 * @param out_object the object will be returned here
 * @return 1 on success, 0 on failure
 */
int NCDModuleProcess_GetObj (NCDModuleProcess *o, NCD_string_id_t name, NCDObject *out_object) WARN_UNUSED;

/**
 * Sets callback functions for the interpreter to implement the
 * process backend.
 * Must be called from within {@link NCDModuleInst_func_initprocess}
 * if success is to be reported there.
 * 
 * @param o process backend handle, as in {@link NCDModuleInst_func_initprocess}
 * @param interp_user argument to callback functions
 * @param interp_func_event function for reporting continue/terminate requests
 * @param interp_func_getobj function for resolving objects within the process
 */
void NCDModuleProcess_Interp_SetHandlers (NCDModuleProcess *o, void *interp_user,
                                          NCDModuleProcess_interp_func_event interp_func_event,
                                          NCDModuleProcess_interp_func_getobj interp_func_getobj);

/**
 * Reports the process backend as up.
 * The process backend must be in down state.
 * The process backend enters up state.
 * WARNING: this directly calls the process creator; expect to be called back
 * 
 * @param o process backend handle
 */
void NCDModuleProcess_Interp_Up (NCDModuleProcess *o);

/**
 * Reports the process backend as down.
 * The process backend must be in up state.
 * The process backend enters waiting state.
 * WARNING: this directly calls the process creator; expect to be called back
 * 
 * NOTE: the backend enters waiting state, NOT down state. The interpreter should
 * pause the process until {@link NCDModuleProcess_interp_func_event} reports
 * NCDMODULEPROCESS_INTERP_EVENT_CONTINUE, unless termination is requested via
 * NCDMODULEPROCESS_INTERP_EVENT_TERMINATE.
 * 
 * @param o process backend handle
 */
void NCDModuleProcess_Interp_Down (NCDModuleProcess *o);

/**
 * Reports termination of the process backend.
 * The process backend must be in terminating state.
 * The process backend handle becomes invalid and must not be used
 * by the interpreter any longer.
 * WARNING: this directly calls the process creator; expect to be called back
 * 
 * @param o process backend handle
 */
void NCDModuleProcess_Interp_Terminated (NCDModuleProcess *o);

/**
 * Resolves a special process object for the process backend.
 * 
 * @param o process backend handle
 * @param name name of the object as an {@link NCDStringIndex} identifier
 * @param out_object the object will be returned here
 * @return 1 on success, 0 on failure
 */
int NCDModuleProcess_Interp_GetSpecialObj (NCDModuleProcess *o, NCD_string_id_t name, NCDObject *out_object) WARN_UNUSED;

/**
 * Function called before any instance of any backend in a module
 * group is created;
 * 
 * @param params structure containing global resources, such as
 *               {@link BReactor}, {@link BProcessManager} and {@link NCDUdevManager}.
 * @return 1 on success, 0 on failure
 */
typedef int (*NCDModule_func_globalinit) (struct NCDInterpModuleGroup *group, const struct NCDModuleInst_iparams *params);

/**
 * Function called to clean up after {@link NCDModule_func_globalinit} and modules
 * in a module group.
 * There are no backend instances alive from this module group.
 */ 
typedef void (*NCDModule_func_globalfree) (struct NCDInterpModuleGroup *group);

/**
 * Handler called to create an new module backend instance.
 * The backend is initialized in down state.
 * 
 * If the backend fails initialization, this function should report the backend
 * instance to have died with error by calling {@link NCDModuleInst_Backend_DeadError}.
 * 
 * @param o if the module specifies a positive alloc_size value in the {@link NCDModule}
 *          structure, this will point to the allocated memory that can be used by the
 *          module instance while it exists. If the alloc_size is 0 (default), this may or
 *          may not be NULL.
 * @param i module backend instance handler. The backend may only use this handle via
 *          the Backend functions of {@link NCDModuleInst}.
 */
typedef void (*NCDModule_func_new2) (void *o, NCDModuleInst *i, const struct NCDModuleInst_new_params *params);

/**
 * Handler called to request termination of a backend instance.
 * The backend instance was in down or up state.
 * The backend instance enters dying state.
 * 
 * @param o state pointer, as in {@link NCDModule_func_new2}
 */
typedef void (*NCDModule_func_die) (void *o);

/**
 * Function called to resolve a variable within a backend instance.
 * The backend instance is in up state, or in up or down state if can_resolve_when_down=1.
 * This function must not have any side effects.
 * 
 * @param o state pointer, as in {@link NCDModule_func_new2}
 * @param name name of the variable to resolve
 * @param mem value memory to use
 * @param out on success, the backend should initialize the value here
 * @return 1 if exists, 0 if not exists. If exists, but out of memory, return 1
 *         and an invalid value.
 */
typedef int (*NCDModule_func_getvar) (void *o, const char *name, NCDValMem *mem, NCDValRef *out);

/**
 * Function called to resolve a variable within a backend instance.
 * The backend instance is in up state, or in up or down state if can_resolve_when_down=1.
 * This function must not have any side effects.
 * 
 * @param o state pointer, as in {@link NCDModule_func_new2}
 * @param name name of the variable to resolve as an {@link NCDStringIndex} identifier
 * @param mem value memory to use
 * @param out on success, the backend should initialize the value here
 * @return 1 if exists, 0 if not exists. If exists, but out of memory, return 1
 *         and an invalid value.
 */
typedef int (*NCDModule_func_getvar2) (void *o, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out);

/**
 * Function called to resolve an object within a backend instance.
 * The backend instance is in up state, or in up or down state if can_resolve_when_down=1.
 * This function must not have any side effects.
 * 
 * @param o state pointer, as in {@link NCDModule_func_new2}
 * @param name name of the object to resolve as an {@link NCDStringIndex} identifier
 * @param out_object the object will be returned here
 * @return 1 on success, 0 on failure
 */
typedef int (*NCDModule_func_getobj) (void *o, NCD_string_id_t name, NCDObject *out_object);

/**
 * Handler called when the module instance is in a clean state.
 * This means that all statements preceding it in the process are
 * up, this statement is down, and all following statements are
 * uninitialized. When a backend instance goes down, it is guaranteed,
 * as long as it stays down, that either this will be called or
 * termination will be requested with {@link NCDModule_func_die}.
 * The backend instance was in down state.
 * 
 * @param o state pointer, as in {@link NCDModule_func_new2}
 */
typedef void (*NCDModule_func_clean) (void *o);

#define NCDMODULE_FLAG_CAN_RESOLVE_WHEN_DOWN (1 << 0)

/**
 * Structure encapsulating the implementation of a module backend.
 */
struct NCDModule {
    /**
     * If this implements a plain statement, the name of the statement.
     * If this implements a method, then "base_type::method_name".
     */
    const char *type;
    
    /**
     * The base type for methods operating on instances of this backend.
     * Any module with type of form "base_type::method_name" is considered
     * a method of instances of this backend.
     * If this is NULL, the base type will default to type.
     */
    const char *base_type;
    
    /**
     * Function called to create an new backend instance.
     */
    NCDModule_func_new2 func_new2;
    
    /**
     * Function called to request termination of a backend instance.
     * May be NULL, in which case the default is to call NCDModuleInst_Backend_Dead().
     */
    NCDModule_func_die func_die;
    
    /**
     * Function called to resolve a variable within the backend instance.
     * May be NULL.
     */
    NCDModule_func_getvar func_getvar;
    
    /**
     * Function called to resolve a variable within the backend instance.
     * May be NULL.
     */
    NCDModule_func_getvar2 func_getvar2;
    
    /**
     * Function called to resolve an object within the backend instance.
     * May be NULL.
     */
    NCDModule_func_getobj func_getobj;
    
    /**
     * Function called when the backend instance is in a clean state.
     * May be NULL.
     */
    NCDModule_func_clean func_clean;
    
    /**
     * Various flags.
     * 
     * - NCDMODULE_FLAG_CAN_RESOLVE_WHEN_DOWN
     *   Whether the interpreter is allowed to call func_getvar and func_getobj
     *   even when the backend instance is in down state (as opposed to just
     *   in up state.
     */
    int flags;
    
    /**
     * The amount of memory to preallocate for each module instance.
     * Preallocation can be used to avoid having to allocate memory from
     * module initialization. The memory can be accessed via the first
     * argument to {@link NCDModule_func_new2} and other calls.
     */
    int alloc_size;
};

/**
 * Structure encapsulating a group of module backend implementations,
 * with global init and free functions.
 */
struct NCDModuleGroup {
    /**
     * Function called before any instance of any module backend in this
     * group is crated. May be NULL.
     */
    NCDModule_func_globalinit func_globalinit;
    
    /**
     * Function called to clean up after {@link NCDModule_func_globalinit}.
     * May be NULL.
     */
    NCDModule_func_globalfree func_globalfree;
    
    /**
     * Array of module backends. The array must be terminated with a
     * structure that has a NULL type member.
     */
    const struct NCDModule *modules;
    
    /**
     * A pointer to an array of strings which will be mapped to
     * {@link NCDStringIndex} string identifiers for the module to use.
     * The array must be terminated by NULL. The resulting string
     * identifiers will be available in the 'strings' member in
     * {@link NCDInterpModuleGroup}.
     */
    const char *const*strings;
    
    /**
     * A pointer to an array of global functions implemented by this module
     * group. The array must be terminated with a structure that has a NULL
     * func_name member. May be NULL.
     */
    struct NCDModuleFunction const *functions;
};

/**
 * Represents an {@link NCDModule} within an interpreter.
 * This structure is initialized by the interpreter when it loads a module group.
 */
struct NCDInterpModule {
    /**
     * A copy of the original NCDModule structure,
     */
    struct NCDModule module;
    
    /**
     * The string identifier of this module's base_type (or type if base_type is
     * not specified) according to {@link NCDStringIndex}.
     */
    NCD_string_id_t base_type_id;
    
    /**
     * A pointer to the {@link NCDInterpModuleGroup} representing the group
     * this module belongs to.
     */
    struct NCDInterpModuleGroup *group;
};

/**
 * Represents an {@link NCDModuleGroup} within an interpreter.
 * This structure is initialized by the interpreter when it loads a module group.
 */
struct NCDInterpModuleGroup {
    /**
     * A copy of the original NCDModuleGroup structure.
     */
    struct NCDModuleGroup group;
    
    /**
     * An array of string identifiers corresponding to the strings
     * in the 'strings' member of NCDModuleGroup. May be NULL if there
     * are no strings in the NCDModuleGroup.
     */
    NCD_string_id_t *strings;
    
    /**
     * Pointer which allows the module to keep private interpreter-wide state.
     * This can be freely modified by the module; the interpeter will not
     * read or write it.
     */
    void *group_state;
};

/**
 * An abstraction of function call evaluations, mutually decoupling the
 * interpreter and the function implementations.
 * 
 * The function implementation is given an instance of this structure
 * in the evaluation callback (func_eval), and uses it to request
 * information and submit results. The function implementation does these
 * things by calling the various NCDCall functions with the NCDCall
 * instance. Note that function arguments are evaluated on demand from
 * the function implementation. This enables behavior such as
 * short-circuit evaluation of logical operators.
 * 
 * The NCDCall struct has a value semantic - it can be copied
 * around freely by the function implementation during the
 * lifetime of the evaluation call.
 */
typedef struct {
    struct NCDCall_interp_shared const *interp_shared;
    void *interp_user;
    struct NCDInterpFunction const *interp_function;
    size_t arg_count;
    NCDValMem *res_mem;
    NCDValRef *out_ref;
} NCDCall;

/**
 * Used by the intepreter to call a function.
 * 
 * It calls the func_eval callback of the function implementation
 * with an appropriately initialized NCDCall value. This is the only
 * NCDCall related function used by the interpreter.
 * 
 * As part of the call, callbacks to the interpreter (given in interp_shared)
 * may occur. All of these callbacks are passed interp_user as the first
 * argument. The callbacks are:
 * - logfunc (to log a message),
 * - func_eval_arg (to evaluate a particular function argument).
 * 
 * @param interp_shared pointer to things expected to be the same for all
 *   function calls by the interpreter. This includes interpreter callbacks.
 * @param interp_user pointer to be passed through to interpreter callbacks
 * @param interp_function the function to call. The evaluation function of
 *   the function implementation that will be called is taken from here
 *   (interp_function->function.func_eval), but this is also exposed to the
 *   function implementation, so it should be initialized appropriately.
 * @param arg_count number of arguments passed to the function.
 *   The function implementation is only permitted to attempt evaluation
 *   of arguments with indices lesser than arg_count.
 * @param res_mem the (initialized) value memory object for the result to
 *   be stored into. However this may also be used as storage for temporary
 *   values computed as part of the call.
 * @param res_out on success, *res_out will be set to the result of the call
 * @return 1 on success, 0 on error
 */
int NCDCall_DoIt (
    struct NCDCall_interp_shared const *interp_shared, void *interp_user,
    struct NCDInterpFunction const *interp_function,
    size_t arg_count, NCDValMem *res_mem, NCDValRef *res_out
) WARN_UNUSED;

/**
 * Returns a pointer to the NCDInterpFunction object for the function,
 * initialized by the interpreter. This alows, among other things,
 * determining which function is being called, and getting the module
 * group's private data pointer.
 */
struct NCDInterpFunction const * NCDCall_InterpFunction (NCDCall const *o);

/**
 * Returns a pointer to an NCDModuleInst_iparams structure, exposing
 * services provided by the interpreter.
 */
struct NCDModuleInst_iparams const * NCDCall_Iparams (NCDCall const *o);

/**
 * Returns the number of arguments for the function call.
 */
size_t NCDCall_ArgCount (NCDCall const *o);

/**
 * Attempts to evaluate a function argument.
 * 
 * @param index the index of the argument to be evaluated. Must be
 *   in the range [0, ArgCount).
 * @param mem the value memory object for the value to be stored into.
 *   However temporary value data may also be stored here.
 * @return on success, the value reference to the argument value;
 *   on failure, an invalid value reference
 */
NCDValRef NCDCall_EvalArg (NCDCall const *o, size_t index, NCDValMem *mem);

/**
 * Returns a pointer to the value memory object that the result
 * of the call should be stored into. The memory object may also
 * be used to store temporary value data.
 */
NCDValMem * NCDCall_ResMem (NCDCall const *o);

/**
 * Provides result value of the evaluation to the interpreter.
 * Note that this only stores the result without any immediate
 * action.
 * 
 * Passing an invalid value reference indicates failure of the
 * call, though failure is also assumed if this function is
 * not called at all during the call. When a non-invalid
 * value reference is passed (indicating success), it must point
 * to a value stored within the memory object returned by
 * NCDCall_ResMem.
 * 
 * @param ref the result value for the call, or an invalid
 *   value reference to indicate failure of the call
 */
void NCDCall_SetResult (NCDCall const *o, NCDValRef ref);

/**
 * Returns a log context that can be used to log messages
 * associated with the call.
 */
BLogContext NCDCall_LogContext (NCDCall const *o);

/**
 * A structure initialized by the interpreter and passed
 * to NCDCall_DoIt for function evaluations.
 * It contains a pointer to things expected to be the same for all
 * function calls by the interpreter, so it can be initialized once
 * and not for every call.
 */
struct NCDCall_interp_shared {
    /**
     * A callack for log messages originating from the function call.
     * The first argument is the interp_user argument to NCDCall_DoIt.
     */
    BLog_logfunc logfunc;
    
    /**
     * A callback for evaluating function arguments.
     * 
     * @param user interpreter private data (the interp_user argument
     *   to NCDCall_DoIt)
     * @param index the index of the argument to be evaluated.
     *   This will be in the range [0, arg_count).
     * @param mem the value memory object where the result of the
     *   evaluation should be stored. Temporary value data may also
     *   be stored here.
     * @param out on success, *out should be set to the value reference
     *   to the result of the evaluation. An invalid value reference is
     *   permitted, in which case failure is assumed.
     * @return 1 on success (but see above), 0 on failure
     */
    int (*func_eval_arg) (void *user, size_t index, NCDValMem *mem, NCDValRef *out);
    
    /**
     * A pointer to an NCDModuleInst_iparams structure, exposing
     * services provided by the interpreter.
     */
    struct NCDModuleInst_iparams const *iparams;
};

/**
 * This structure is initialized statically by a function
 * implementation to describe the function and provide
 * the resources required for its evaluation by the interpreter.
 * 
 * These structures always appear in arrays, pointed to by
 * the functions pointer in the NCDModuleGroup structure.
 */
struct NCDModuleFunction {
    /**
     * The name of the function.
     * NULL to terminate the array of functions.
     */
    char const *func_name;
    
    /**
     * Callback for evaluating the function.
     */
    void (*func_eval) (NCDCall call);
};

/**
 * Represents an {@link NCDModuleFunction} within an interpreter.
 * This structure is initialized by the interpreter when it loads a module group.
 */
struct NCDInterpFunction {
    /**
     * A copy of the original NCDModuleFunction structure.
     * We could just use a pointer to the original statically defined structure,
     * but then we wouldn't be annoying the premature-optimization hipsters.
     */
    struct NCDModuleFunction function;
    
    /**
     * The string identifier of this functions's name. according to
     * {@link NCDStringIndex}.
     */
    NCD_string_id_t func_name_id;
    
    /**
     * A pointer to the {@link NCDInterpModuleGroup} representing the group
     * this function belongs to.
     */
    struct NCDInterpModuleGroup *group;
};

#endif
