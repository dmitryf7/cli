// Copyright (c) Dmitrii Fedotov 2024
#ifndef CLI_H
#define CLI_H

#include <stdint.h>
#include <stdbool.h>

//#define CLI_DEBUG

#define CLI_MAXNODECOUNT        64
#define CLI_MAXNODELENGTH       32

#define CLI_DESCRIPTIONTABCOUNT 16

#define CLI_CMDLINEMAX          64 // 256
#define CLI_ESCSENTMAX          16
#define CLI_PROMPTMAX            4
#define CLI_MAXSPRINTSTACK     128
#define CLI_ABORTCHARSMAX        5

#define NEWLINE             "\r\n"
#define CMDPROMPT              ">"
#define BACKSPACE           '\x08'
#define CR                  '\x0d'
#define LF                  '\x0a'
#define ETX                 '\x03'
#define TAB                 '\x09'
#define ESC                 '\x1b'
#define BEL                 '\x07'
#define SPACE               '\x20'
#define CLIXON              '\x11'
#define CLIXOFF             '\x13'

#ifdef __cplusplus
extern "C" {
#endif

enum cliretcodes
{
    CLIRET_CMDOK,           // Successfully execute the command
    CLIRET_CMDEXE,          // Command execution (#go)
    CLIRET_CMDRESET,        // Reset (#reset)
    CLIRET_CMDFAIL,         // Command execution failed
    CLIRET_INVARG,          // Command argument(s) is invalid
    CLIRET_CMDNOTFOUND,     // Command not found
    CLIRET_CMDNOTAVAIL,     // Command not available
    CLIRET_XON,             // Xon character entered
    CLIRET_XOFF,            // Xoff character entered
    CLIRET_CMDTOOLONG,      // Command entered is too long (>CLI_CMDLINEMAX)
    CLIRET_CMDTOOSHORT,     // Command entered is too short (<1)
    CLIRET_CMDESCTOOLONG,   // Esc sequence entered is too long (>CLI_ESCSENTMAX)
    CLIRET_RXABORT,         // Receive RX_ABORT command
    CLIRET_CONTINUE,        // Intermediate state, command not recognized yet
    CLIRET_CONTINUE_NOTIP,  // No tip found
    CLIRET_CONTINUE_TIPOK,  // Tip found
    CLIRET_CONTINUE_TIPMANY,// Multiply autocompletion

    CLIRET_USER1,           // User-defined retcode
    CLIRET_USER2,           // User-defined retcode
    CLIRET_USER3,           // User-defined retcode
    CLIRET_USER4,           // User-defined retcode
    CLIRET_USER5,           // User-defined retcode
    CLIRET_USER6,           // User-defined retcode
    CLIRET_USER7,           // User-defined retcode
    CLIRET_USER8,           // User-defined retcode

    CLIRET_CONTINUE_TIPNODEERROR,  // No nodes with this value
    CLIRET_MAX = 255
};

typedef enum cliretcodes CLIret_e;

enum nodetypes
{
    NODE_NONE,
    NODE_NONLEAF_FIXEDCHILD,
    NODE_NONLEAF_NONFIXEDCHILD, // Tag node
    NODE_LEAF_SINGLEVALUE,
    NODE_LEAF_MULTIPLYVALUES,
    NODE_LEAF_NONEVALUE,        // Valueless leaf
    NODE_VALUE
};

typedef enum nodetypes NodeTypes_e;

typedef int (*pCBCli_read)  (char *data);
typedef int (*pCBCli_write) (const char *data, int num);
typedef CLIret_e (*pCBCli_exec)  (void *);
typedef void (*pCBCli_response)  (void *);

struct commands
{
    const char *command;
    pCBCli_exec cmd_exec;
};

typedef struct commands   Commands_t;
typedef struct commands *pCommands_t;
typedef const struct commands *pcCommands_t;

struct responses
{
    CLIret_e response;
    pCBCli_response cmd_response;
};

typedef struct responses   Responses_t;
typedef struct responses *pResponses_t;
typedef const struct responses *pcResponses_t;

struct cmdnode
{
    NodeTypes_e type;
    void *next;
    char *nodestr;
    char *description;
};

typedef struct cmdnode   CmdNode_t;
typedef struct cmdnode *pCmdNode_t;

struct clictx
{
    char cmdline[CLI_CMDLINEMAX];
    uint16_t bytesread;
    char ch;

    char escsentence[CLI_ESCSENTMAX];
    uint8_t esc_idx;
    bool esc_begin;

    char promptline[CLI_PROMPTMAX];
    char newline[CLI_PROMPTMAX];
    char abortchars[CLI_ABORTCHARSMAX];

    char bel;
    char xon;
    char xoff;

    bool printcmdprompt;
    bool printcmdpromptnewline;
    bool printcmdline;

    bool rx_abort;
    bool echo_on;

    pcCommands_t usercmds;
    pcResponses_t responses;

    pCmdNode_t cmdlinecommands;
    char autocompletion[CLI_MAXNODELENGTH];

    pCBCli_read cb_cliread;
    pCBCli_write cb_cliwrite;

    char *command;
    char *arguments;
};

typedef struct clictx   CLICtx_t;
typedef struct clictx *pCLICtx_t;

typedef int (*pCBCli_checknode) (const uint8_t *param, uint8_t idx, uint8_t **result);
typedef int (*pCBCli_checkvalue)(const uint8_t *param, uint8_t idx, uint8_t **result);

struct varcmdnode
{
    NodeTypes_e type;
    void *next;
    char *nodestr;
    char *description;
    pCBCli_checknode cb_checknode;
};

typedef struct varcmdnode   VarCmdNode_t;
typedef struct varcmdnode *pVarCmdNode_t;

typedef struct varcmdnode   ValueNode_t;
typedef struct varcmdnode *pValueNode_t;

extern void cliinit(pCLICtx_t clictx, pcCommands_t usercmds, pcResponses_t responses, pCmdNode_t cmdlinecommands, pCBCli_read cbread, pCBCli_write cbwrite);
extern CLIret_e cli(pCLICtx_t pcictx);

#ifdef __cplusplus
}
#endif

#endif
