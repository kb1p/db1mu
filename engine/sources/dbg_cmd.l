%{
#include "DebugCommand.h"
#include "dbg_cmd.y.hpp"
%}
%%

pcpu {return CMD_PCPU;}
pmem {return CMD_PMEM;}
con[tinue]* {return CMD_CONTINUE;}
br[eak]* {return CMD_BREAK;}
rst {return CMD_RST;}
st[ep]? { return CMD_STEP; }

[0-9a-fA-F]+ {yylval=strtol(yytext, NULL, 16); return NUMBER;}
\" ;
\' ;
\. ;
; ;
[ \t\n] ;
%%
