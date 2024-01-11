#ifndef SYXSRV_H

#define SYXSRV_H
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()
#include <time.h>
#include <stdint.h>
#include "lib/printf.h"
#include "sysex.h"  // Assuming handle_incoming_sysex is defined here
#include "pack.h"
#include "definitions.h"


#ifdef __cplusplus
extern "C" {
#endif

// Function declarations for syxsrv.c
void startSyxServer();
void stopSyxServer();
void handleSyxMessage();

#ifdef __cplusplus
}
#endif

#endif // SYXSRV_H
