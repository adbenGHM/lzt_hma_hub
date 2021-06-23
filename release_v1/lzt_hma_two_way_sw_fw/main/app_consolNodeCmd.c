#include "app_main.h"
static const char *TAG="app_consolNodeCmd";
static struct {
    struct arg_str *nodeId;
    struct arg_str *cmdStr;
    struct arg_end *end;
}consolNodeCmdArg;
static app_nodeData_t nodeCmd;
static int consolNodeCmdHandler(int argc, char **argv){
    int nerrors = arg_parse(argc, argv, (void **) &consolNodeCmdArg);
    if (nerrors != 0) {
        arg_print_errors(stderr, consolNodeCmdArg.end, argv[0]);
        return 1;
    }
    memset((char*)&nodeCmd,'\0',sizeof(nodeCmd));
    strncpy(nodeCmd.nodeId,consolNodeCmdArg.nodeId->sval[0],APP_CONFIG_NODE_ID_LEN);
    strncpy(nodeCmd.data,consolNodeCmdArg.cmdStr->sval[0],APP_CONFIG_NODE_DATA_MAX_LEN);
    xQueueSend(app_nodeCommandQueue, &nodeCmd, 0);
    ESP_LOGI(TAG,"Node ID= %s, Cmd= %s",consolNodeCmdArg.nodeId->sval[0],consolNodeCmdArg.cmdStr->sval[0]);
    return 0;
}
app_status_t app_consolRegisterNodeCmd(void){
    consolNodeCmdArg.nodeId = arg_str1(NULL, NULL, "<id>", "Id of the Node");
    consolNodeCmdArg.cmdStr = arg_str1(NULL, NULL, "<cmd>", "Command String");
    consolNodeCmdArg.end = arg_end(2);
    //Example command : node "24:0a:c4:af:4c:fc" "{sw1: 1, sw2 : 0}"
    const esp_console_cmd_t consolCmd = {
        .command = "node",
        .help = "Control Node state with command",
        .hint = NULL,
        .func = &consolNodeCmdHandler,
        .argtable=&consolNodeCmdArg,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&consolCmd) );
    return APP_STATUS_OK;

}