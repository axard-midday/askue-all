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


#include <stdio.h>
#include "config.h"
#include "journal.h"
#include "write_msg.h"
#include "main_loop.h"

int main(int argc, char **argv)
{
	write_msg ( stderr, "Test", "OK", "Hello, World!" );
    
    askue_cfg_t Cfg;
    askue_config_init ( &Cfg );
    askue_config_read ( &Cfg );
    if ( askue_journal_init ( &Cfg ) )
        write_msg ( stderr, "Test", "FAIL", "Journal Init!" );
    else
        write_msg ( stderr, "Test", "OK", "Journal Init!" );
    askue_config_destroy ( &Cfg );
    
	return 0;
}

