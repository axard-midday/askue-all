

#include <stdio.h>

// Встроенные скрипты
// поставляются вместе с Askue
// connect

static
const char *AskueBuiltinScript[] = 
{
    "connect",
    "test",
    NULL
};

// запуск встроенного скрипта
loop_state_t run_builtin_script ( FILE *Log, 
                                    const char *Script, 
                                    const char *TypeName, 
                                    const script_option_t *ScriptOptions, 
                                    const sigset_t *SignalSet )
{
    //char TypeName_lowercase[  ] 
    char Buffer[ 256 ];
    memset ( Buffer, '\0', 256 );
    size_t j = 0;
    // перевод типа в нижний регистр
    size_t len = strlen ( Script );
    for ( size_t i = 0; i < len; i++ )
    {
        if ( isalpha ( TypeName[ i ] ) )
        {
            Buffer[ j ] = tolower ( TypeName[ i ] );  
        }
        else
        {
            Buffer[ j ] = TypeName[ i ];
        }
        
        if ( j < 256 ) 
        {
            j++;
        }
    }
    
    strcat ( Buffer, Script );
    
    
    
    
}

int is_builtin_script ( const char *Script )
{
    int Result = 0;
    for ( size_t i = 0; AskueBuiltinScript[ i ] != NULL && !Result; i++ )
    {
        Result = !strcmp ( AskueBuiltinScript[ i ], Script ) )
    }
    
    return Result;
}

int main(int argc, char **argv)
{
	
	return 0;
}

