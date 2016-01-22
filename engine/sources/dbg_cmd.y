%{
#include <stdio.h>
#include "DebugCommand.h"
int yylex(void);
void yyerror(DebugCommand* cmdParsed, const char *);
%}

%parse-param {DebugCommand* cmdParsed}

%token CMD_PCPU CMD_PMEM CMD_CONTINUE CMD_BREAK CMD_RST NUMBER

%%

commands:
	| commands command
	;

command:
	    cmd_print_mem_byte
	|	cmd_print_mem_range
	|	cmd_print_cpu
	|	cmd_continue
	|	cmd_break
	|	cmd_rst
	;

cmd_print_mem_byte:
	CMD_PMEM NUMBER {
//			printf("\nprint memory at %x\n",$2);
			cmdParsed->cmd = DebugCommand::CMD_PrintMemoryByte;
			cmdParsed->args.mem_byte = $2;
		}

cmd_print_mem_range:
		CMD_PMEM NUMBER NUMBER {
//            printf("\nprint memory at %x %x\n",$2, $3);
            cmdParsed->cmd = DebugCommand::CMD_PrintMemoryArray;
            cmdParsed->args.mem_array.mem_ptr = $2;
            cmdParsed->args.mem_array.array_len = $3;

}

cmd_print_cpu:
		CMD_PCPU {
//            printf("\nprint CPU state\n");
            cmdParsed->cmd = DebugCommand::CMD_PrintCPUState;
		}

cmd_continue:
		CMD_CONTINUE {
//            printf("\ncontinue\n");
            cmdParsed->cmd = DebugCommand::CMD_Continue;
		}

cmd_break:
		CMD_BREAK NUMBER {
//            printf("\nbreak at %x\n",$2);
            cmdParsed->cmd = DebugCommand::CMD_Break;
            cmdParsed->args.break_addr = $2;
        }

cmd_rst:
		CMD_RST {
//            printf("\nprint CPU state\n");
			cmdParsed->cmd = DebugCommand::CMD_RST;
		}
;



%%

void yyerror(DebugCommand* cmdParsed, char const* s)
{
//  fprintf(stderr, "error: %s\n", s);
}