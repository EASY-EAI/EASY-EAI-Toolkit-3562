#include <stdio.h>
//======================  C  ======================
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
//=====================  SDK  =====================
#include "system_opt.h"
#include "ini_wrapper.h"
#include "log_manager.h"
#include "ipc.h"

void waittingTargetClientReady(int32_t cliId)
{
    while(1){
        IPC_client_query_registered_client(cliId);

        // IPC_client_dstClient_is_registered()内部会有500ms的延时等待查询结果
        if(IPC_client_dstClient_is_registered())
            break;

        PRINT_TRACE("-------------- client[%d] is not registered complete --------------", cliId);
    }

    return ;
}


