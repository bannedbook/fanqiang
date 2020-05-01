/**
 * @defgroup lwip lwIP
 *
 * @defgroup infrastructure Infrastructure
 *
 * @defgroup callbackstyle_api Callback-style APIs
 * Non thread-safe APIs, callback style for maximum performance and minimum
 * memory footprint.
 * 
 * @defgroup sequential_api Sequential-style APIs
 * Sequential-style APIs, blocking functions. More overhead, but can be called
 * from any thread except TCPIP thread.
 * 
 * @defgroup netifs NETIFs
 * 
 * @defgroup apps Applications
 */

/**
 * @mainpage Overview
 * @verbinclude "README"
 */

/**
 * @page upgrading Upgrading
 * @verbinclude "UPGRADING"
 */

/**
 * @page changelog Changelog
 * @verbinclude "CHANGELOG"
 */

/**
 * @page contrib How to contribute to lwIP
 * @verbinclude "contrib.txt"
 */

/**
 * @page pitfalls Common pitfalls
 *
 * Multiple Execution Contexts in lwIP code
 * ========================================
 *
 * The most common source of lwIP problems is to have multiple execution contexts
 * inside the lwIP code.
 * 
 * lwIP can be used in two basic modes: @ref lwip_nosys (no OS/RTOS 
 * running on target system) or @ref lwip_os (there is an OS running
 * on the target system).
 *
 * Mainloop Mode
 * -------------
 * In mainloop mode, only @ref callbackstyle_api can be used.
 * The user has two possibilities to ensure there is only one 
 * exection context at a time in lwIP:
 *
 * 1) Deliver RX ethernet packets directly in interrupt context to lwIP
 *    by calling netif->input directly in interrupt. This implies all lwIP 
 *    callback functions are called in IRQ context, which may cause further
 *    problems in application code: IRQ is blocked for a long time, multiple
 *    execution contexts in application code etc. When the application wants
 *    to call lwIP, it only needs to disable interrupts during the call.
 *    If timers are involved, even more locking code is needed to lock out
 *    timer IRQ and ethernet IRQ from each other, assuming these may be nested.
 *
 * 2) Run lwIP in a mainloop. There is example code here: @ref lwip_nosys.
 *    lwIP is _ONLY_ called from mainloop callstacks here. The ethernet IRQ
 *    has to put received telegrams into a queue which is polled in the
 *    mainloop. Ensure lwIP is _NEVER_ called from an interrupt, e.g.
 *    some SPI IRQ wants to forward data to udp_send() or tcp_write()!
 *
 * OS Mode
 * -------
 * In OS mode, @ref callbackstyle_api AND @ref sequential_api can be used.
 * @ref sequential_api are designed to be called from threads other than
 * the TCPIP thread, so there is nothing to consider here.
 * But @ref callbackstyle_api functions must _ONLY_ be called from
 * TCPIP thread. It is a common error to call these from other threads
 * or from IRQ contexts. ​Ethernet RX needs to deliver incoming packets
 * in the correct way by sending a message to TCPIP thread, this is
 * implemented in tcpip_input().​​
 * Again, ensure lwIP is _NEVER_ called from an interrupt, e.g.
 * some SPI IRQ wants to forward data to udp_send() or tcp_write()!
 * 
 * 1) tcpip_callback() can be used get called back from TCPIP thread,
 *    it is safe to call any @ref callbackstyle_api from there.
 *
 * 2) Use @ref LWIP_TCPIP_CORE_LOCKING. All @ref callbackstyle_api
 *    functions can be called when lwIP core lock is aquired, see
 *    @ref LOCK_TCPIP_CORE() and @ref UNLOCK_TCPIP_CORE().
 *    These macros cannot be used in an interrupt context!
 *    Note the OS must correctly handle priority inversion for this.
 */

/**
 * @page bugs Reporting bugs
 * Please report bugs in the lwIP bug tracker at savannah.\n
 * BEFORE submitting, please check if the bug has already been reported!\n
 * https://savannah.nongnu.org/bugs/?group=lwip
 */

/**
 * @page zerocopyrx Zero-copy RX
 * The following code is an example for zero-copy RX ethernet driver:
 * @include ZeroCopyRx.c
 */

/**
 * @defgroup lwip_nosys Mainloop mode ("NO_SYS")
 * @ingroup lwip
 * Use this mode if you do not run an OS on your system. \#define NO_SYS to 1.
 * Feed incoming packets to netif->input(pbuf, netif) function from mainloop,
 * *not* *from* *interrupt* *context*. You can allocate a @ref pbuf in interrupt
 * context and put them into a queue which is processed from mainloop.\n
 * Call sys_check_timeouts() periodically in the mainloop.\n
 * Porting: implement all functions in @ref sys_time, @ref sys_prot and 
 * @ref compiler_abstraction.\n
 * You can only use @ref callbackstyle_api in this mode.\n
 * Sample code:\n
 * @include NO_SYS_SampleCode.c
 */

/**
 * @defgroup lwip_os OS mode (TCPIP thread)
 * @ingroup lwip
 * Use this mode if you run an OS on your system. It is recommended to
 * use an RTOS that correctly handles priority inversion and
 * to use @ref LWIP_TCPIP_CORE_LOCKING.\n
 * Porting: implement all functions in @ref sys_layer.\n
 * You can use @ref callbackstyle_api together with @ref tcpip_callback,
 * and all @ref sequential_api.
 */

/**
 * @page raw_api lwIP API
 * @verbinclude "rawapi.txt"
 */
