// These two are the default empty implementations for exception handlers
export fn blockingHandler() void {
    while (true) {}
}

export fn nullHandler() void {}

// This comes from the linker script and represents the initial stack pointer address.
// Not a function, but pretend it is to suppress type error
extern fn _stack() void;

// These are the exception handlers, which are weakly linked to the default handlers
// in the linker script
extern fn resetHandler() void;
extern fn nmiHandler() void;
extern fn hardFaultHandler() void;
extern fn memoryManagementFaultHandler() void;
extern fn busFaultHandler() void;
extern fn usageFaultHandler() void;
extern fn svCallHandler() void;
extern fn debugMonitorHandler() void;
extern fn pendSVHandler() void;
extern fn sysTickHandler() void;
