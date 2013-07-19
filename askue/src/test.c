/*
 * test.c
 * 
 * Copyright 2013 axard <axard@axard-desktop>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */
#define _GNU_SOURCE

#include <stdio.h>
#include "config.h"
#include "journal.h"
#include "log.h"
#include "write_msg.h"
#include "script_option.h"
#include "device_loop.h"

#define TESTBIT(bitfield, bitindex) \
	( !!( ( bitfield ) & ( 1 << ( bitindex ) ) ) )

int main(int argc, char **argv)
{
	write_msg ( stderr, "Test", "OK", "Hello, World!" );
    /*
    script_option_t SO;
    
    script_option_init ( &SO );
    script_option_set ( &SO, SA_PORT_PARITY, "no" );
    
    if ( TESTBIT ( SO.Flag, SA_PORT_PARITY ) )
        printf ( "%s\n", SO.Value[ SA_PORT_PARITY ] );
    
    askue_cfg_t Cfg;
    askue_config_init ( &Cfg );
    if ( !askue_config_read ( &Cfg ) )
    {
        write_msg ( stderr, "Test", "OK", "Config read!" );
        
        if ( askue_journal_init ( &Cfg ) )
            write_msg ( stderr, "Test", "FAIL", "Journal not Init!" );
        else
        {
            write_msg ( stderr, "Test", "OK", "Journal Init!" );
            
            FILE *Log = NULL;
            
            if ( !askue_log_open ( &Log, &Cfg ) )
            {
                write_msg ( stderr, "Test", "OK", "Log open!" );
                for ( int i = 1; i < 15; i++ )
                {
                    char Buffer[ 256 ];
                    snprintf ( Buffer, 256, "%.2d", i );
                    write_msg ( Log, "Test", "OK", Buffer );
                }
                
                askue_log_cut ( &Log, &Cfg );
                
                write_msg ( Log, "Test", "OK", "После обрезания" );
            }
            
            askue_log_close ( &Log );
            write_msg ( stderr, "Test", "OK", "Log close!" );
            
        }
        
    }
    askue_config_destroy ( &Cfg );
    */
	return 0;
}

