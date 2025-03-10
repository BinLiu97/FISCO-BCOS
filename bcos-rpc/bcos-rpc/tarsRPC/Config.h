#pragma once
#include <string_view>

constexpr static std::string_view TARS_CONFIG_TEMPLATE =
    "<tars>\n"
    "  <application>\n"
    "    enableset=n\n"
    "    setdivision=NULL\n"
    "    <server>\n"
    "       app=fiscobcos\n"
    "       server=rpc\n"
    "       localip=127.0.0.1\n"
    "       basepath=./\n"
    "       datapath=./\n"
    "       logpath=./tars_logs\n"
    "       logsize=10M\n"
    "       deactivating-timeout=2000\n"
    "       logLevel=INFO\n"
    "       closecout=0\n"
    "       <fiscobcos.rpc.RPCObjAdapter>\n"
    "            allow\n"
    "            endpoint=tcp -h [[TARS_HOST]] -p [[TARS_PORT]] -t 60000\n"
    "            handlegroup=fiscobcos.rpc.RPCObjAdapter\n"
    "            maxconns=1000\n"
    "            protocol=tars\n"
    "            queuecap=1000000\n"
    "            queuetimeout=60000\n"
    "            servant=fiscobcos.rpc.RPCObj\n"
    "            threads=[[TARS_THREAD_COUNT]]\n"
    "       </fiscobcos.rpc.RPCObjAdapter>\n"
    "    </server>\n"
    "    <client>\n"
    "       modulename=fiscobcos\n"
    "    </client>\n"
    "  </application>\n"
    "</tars>\n";
