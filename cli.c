// Copyright (c) Dmitrii Fedotov 2024
#include <string.h>
#include "cli.h"

static bool isAbortChar(pCLICtx_t pcictx, char ch)
{
    bool retvalue = false;

    for(int i = 0; i < CLI_ABORTCHARSMAX; i++)
    {
        if(ch == pcictx->abortchars[i])
        {
            retvalue = true;
            break;
        }
    }

    return retvalue;
}

CLIret_e cli(pCLICtx_t pcictx)
{
    CLIret_e result = CLIRET_CONTINUE;
    int num = 0;

    if(false != pcictx->printcmdprompt)
    {
        if(false != pcictx->printcmdpromptnewline)
        {
            pcictx->cb_cliwrite(pcictx->newline, strlen(pcictx->newline));
            pcictx->printcmdpromptnewline = false;
        }

        pcictx->cb_cliwrite(pcictx->promptline, strlen(pcictx->promptline));
        pcictx->printcmdprompt = false;
    }

    if(false != pcictx->printcmdline)
    {
        pcictx->cb_cliwrite(pcictx->cmdline, strlen(pcictx->cmdline));
        pcictx->printcmdline = false;
    }

    num = pcictx->cb_cliread(&pcictx->ch); // Blocking function

    if (num > 0)
    {
        if(pcictx->xon == pcictx->ch)
        {
            // Ignore standard Xon/Xoff characters
            result = CLIRET_XON;
        }
        else if(pcictx->xoff == pcictx->ch)
        {
            // Ignore standard Xon/Xoff characters
            result = CLIRET_XOFF;
        }
        else if((false == pcictx->rx_abort) && (false != isAbortChar(pcictx, pcictx->ch)))
        {
            // RX_ABORT means stop unit running,
            // in case user press <ESC> <SPACE> <ETX> or <CR> key from the command line
            pcictx->rx_abort = true; // flag for external code meaning stop send data to UART

            pcictx->bytesread = 0;
            pcictx->printcmdprompt = true;
            pcictx->printcmdpromptnewline = true;
            result = CLIRET_RXABORT;
        }
        else if(false != pcictx->esc_begin)
        {
            // Esc-sequence in progress
            if(CR == pcictx->ch)
            {
                pcictx->esc_begin = false;
            }
            else if((1U == pcictx->esc_idx) && ((pcictx->ch > 63) && (pcictx->ch < 96) && ((char)'[' != pcictx->ch)))
            {
                // 2 character sequence
                pcictx->escsentence[pcictx->esc_idx++] = pcictx->ch;

                pcictx->esc_begin = false;
                result = CLIRET_CONTINUE;
            }
            else if((1U == pcictx->esc_idx) && ((char)'[' == pcictx->ch))
            {
                // multi character sequence
                pcictx->escsentence[pcictx->esc_idx++] = pcictx->ch;
                result = CLIRET_CONTINUE;
            }
            else if((1U == pcictx->esc_idx) && ((pcictx->ch < 64) || (pcictx->ch > 95)))
            {
                //Wrong esc sequence?
                pcictx->esc_begin = false;
            }
            else if((pcictx->esc_idx > 1U) && ((pcictx->ch > 63) && (pcictx->ch < 127)))
            {
                // End of multi character escape sequence
                // Here we can handle esc sequence
                // Now ignore all escape sequences
                pcictx->escsentence[pcictx->esc_idx++] = pcictx->ch;

                pcictx->esc_begin = false;
                result = CLIRET_CONTINUE;
            }
            else if(pcictx->esc_idx > CLI_ESCSENTMAX)
            {
                // Sentence too long
                pcictx->esc_begin = false;
                result = CLIRET_CMDESCTOOLONG;
            }
            else
            {
                pcictx->escsentence[pcictx->esc_idx++] = pcictx->ch;
                result = CLIRET_CONTINUE;
            }
        }
        else if(ESC == pcictx->ch)
        {
            // Esc character
            pcictx->esc_begin = true;
            pcictx->esc_idx = 0;

            pcictx->escsentence[pcictx->esc_idx++] = pcictx->ch;
            result = CLIRET_CONTINUE;
        }
        else if(BACKSPACE == pcictx->ch) // User enter backspace
        {
            char outch;

            if(pcictx->bytesread > 0U)
            {
                pcictx->cmdline[--pcictx->bytesread] = 0U;

                if(false != pcictx->echo_on)
                {
                    outch = BACKSPACE;
                    pcictx->cb_cliwrite(&outch, 1);
                }

                outch = SPACE;
                pcictx->cb_cliwrite(&outch, 1);
                outch = BACKSPACE;
                pcictx->cb_cliwrite(&outch, 1);
            }
            else
            {
                if(false == pcictx->echo_on)
                {
                    pcictx->cb_cliwrite(CMDPROMPT, strlen(CMDPROMPT));
                }

                pcictx->cb_cliwrite(&pcictx->bel, 1);
            }

            result = CLIRET_CONTINUE;
        }
        else if(TAB == pcictx->ch) // Tab character
        {
#ifdef CLI_TIP

            if(NULL != pcictx->cmdlinecommands)
            {
                result = cli_tip(pcictx);

                if(CLIRET_CONTINUE_TIPOK == result)
                {
                    int slen = strlen(pcictx->autocompletion);

                    if((pcictx->bytesread + slen) < (CLI_CMDLINEMAX - 1))
                    {
                        strcat(pcictx->cmdline, pcictx->autocompletion);
                        pcictx->bytesread += slen;
                        pcictx->cb_cliwrite(pcictx->autocompletion, slen);
                    }
                    else
                    {
                        pcictx->cb_cliwrite(&pcictx->bel, 1);
                        result = CLIRET_CONTINUE;
                    }
                }
                else if(CLIRET_CONTINUE_TIPMANY == result)
                {
                    int slen = strlen(pcictx->autocompletion);

                    if((pcictx->bytesread + slen) < (CLI_CMDLINEMAX - 1))
                    {
                        strcat(pcictx->cmdline, pcictx->autocompletion);
                        pcictx->bytesread += slen;
                    }
                    else
                    {
                        pcictx->cb_cliwrite(&pcictx->bel, 1);
                    }

                    pcictx->printcmdpromptnewline = true;
                    pcictx->printcmdprompt = true;
                    pcictx->printcmdline = true;
                    result = CLIRET_CONTINUE;
                }
                else
                {
                    pcictx->cb_cliwrite(&pcictx->bel, 1);
                    result = CLIRET_CONTINUE;
                }
            }
            else
            {
                pcictx->cb_cliwrite(&pcictx->bel, 1);
                result = CLIRET_CONTINUE;
            }

#else
            pcictx->cb_cliwrite(&pcictx->bel, 1);
            result = CLIRET_CONTINUE;
#endif
        }
        else
        {
            // Ordinary character
            if(false != pcictx->echo_on)
            {
                pcictx->cb_cliwrite(&pcictx->ch, 1);
            }

            pcictx->cmdline[pcictx->bytesread++] = pcictx->ch;
//        }

            // Character processed, now do some additional checks and run command if <CR> is detected
            if(pcictx->bytesread > (CLI_CMDLINEMAX - 1)) // Input command line buffer overflow
            {
                pcictx->bytesread = 0;

                pcictx->printcmdprompt = true;
                pcictx->printcmdpromptnewline = true;
                result = CLIRET_CMDTOOLONG;
            }

            else if(CR == pcictx->ch) // User enter CR character
            {
                CLIret_e cmdresult = CLIRET_CMDOK;

                pcictx->cmdline[pcictx->bytesread - 1] = 0;

                // Parse the command
                // Remove initial spaces and tabs if any
                pcictx->command = pcictx->cmdline;

                while((SPACE == (*(pcictx->command))) || (LF == (*(pcictx->command))) || (TAB == (*(pcictx->command))))
                {
                    pcictx->command++;
                }

                if(strlen(pcictx->command) > 0U)
                {
                    int cmdidx = 0;
                    bool cmdfound = false;

                    pcictx->arguments = strchr(pcictx->command, SPACE);

                    if(NULL != pcictx->arguments)
                    {
                        *(pcictx->arguments++) = 0;

                        while((SPACE == (*(pcictx->arguments))) || (LF == (*(pcictx->arguments))) || (TAB == (*(pcictx->arguments))))
                        {
                            pcictx->arguments++;
                        }
                    }
                    else
                    {
                        pcictx->arguments = pcictx->command + strlen(pcictx->command);
                    }

#ifdef CLI_DEBUG
                    {
                        char cmd[CLI_MAXSPRINTSTACK];
                        int slen;

                        slen = sprintf(cmd, "Command: %s\r\nArguments: %s\r\n", command, arguments);
                        pcictx->cb_cliwrite(cmd, slen);
                    }
#endif

                    for(cmdidx = 0; NULL != pcictx->usercmds[cmdidx].cmd_exec; cmdidx++)
                    {
                        if(0 == strcmp(pcictx->command, pcictx->usercmds[cmdidx].command))
                        {
                            pcictx->cb_cliwrite(pcictx->newline, strlen(pcictx->newline));
                            cmdresult = pcictx->usercmds[cmdidx].cmd_exec(pcictx);
                            pcictx->cb_cliwrite(pcictx->newline, strlen(pcictx->newline));

                            for(int i = 0;; i++)
                            {
                                // it must breaks on CLIRET_MAX command finally
                                if(cmdresult == pcictx->responses[i].response)
                                {
                                    pcictx->responses[i].cmd_response(pcictx);
                                    break;
                                }
                            }

                            //result = (CLIRET_CMDOK == cmdresult) ? CLIRET_CMDOK : CLIRET_CMDFAIL;
                            if(CLIRET_CMDEXE == cmdresult)
                            {
                                pcictx->rx_abort = false;
                            }

                            cmdfound = true;
                            break;
                        }
                    }

                    if(false == cmdfound)
                    {
                        for(int i = 0;; i++)
                        {
                            // it must breaks on CLIRET_MAX command finally
                            if(CLIRET_MAX == pcictx->responses[i].response)
                            {
                                pcictx->responses[i].cmd_response(pcictx);
                                break;
                            }
                        }

                        cmdresult = CLIRET_CMDNOTFOUND;
                    }

#ifdef CLI_DEBUG
                    {
                        char cmd[CLI_MAXSPRINTSTACK];
                        int slen;

                        slen = sprintf(cmd, "\r\nCommand result: %d\r\n", (false != cmdfound) ? cmdresult : -1);
                        pcictx->cb_cliwrite(cmd, slen);
                    }
#endif
                    pcictx->bytesread = 0;
                    pcictx->printcmdpromptnewline = true; // false
                    pcictx->printcmdprompt = true;

                    result = cmdresult;//(false != cmdfound) ? cmdresult : CLIRET_CMDNOTFOUND;
                }
                else
                {
                    pcictx->printcmdpromptnewline = true;
                    pcictx->printcmdprompt = true;
                    result = CLIRET_CMDTOOSHORT;
                }

                pcictx->bytesread = 0;
            }
            else
            {
                /* MISRA else */
            }
        }
    }

    return result;
}

void cliinit(pCLICtx_t clictx, pcCommands_t usercmds, pcResponses_t responses, pCmdNode_t cmdlinecommands, pCBCli_read cbread, pCBCli_write cbwrite)
{
    (void)memset(clictx, 0, sizeof(CLICtx_t));
    clictx->cb_cliread  = cbread;
    clictx->cb_cliwrite = cbwrite;
    clictx->rx_abort = true;
    clictx->echo_on = true;
    clictx->printcmdprompt = true;
    clictx->printcmdpromptnewline = true;
    clictx->usercmds = usercmds;
    clictx->cmdlinecommands = cmdlinecommands;
    clictx->responses = responses;

    (void)strcpy(clictx->promptline, CMDPROMPT);
    (void)strcpy(clictx->newline, NEWLINE);
    clictx->xon = CLIXON;
    clictx->xoff = CLIXOFF;
    clictx->bel = BEL;

    clictx->abortchars[0] = ESC;
    clictx->abortchars[1] = ETX;
    clictx->abortchars[2] = SPACE;
    clictx->abortchars[3] = CR; // All other members set to 0 by first memset()
}
