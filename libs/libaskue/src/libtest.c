/*
 * libtest.c
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
#include <string.h>

#include "port.h"
#include "uint8_array.h"

int main(int argc, char **argv)
{
    
    
    
	askue_port_t Port;
    askue_port_cfg_t PortCfg;
    PortCfg.File = "/dev/ttyS0";
    PortCfg.Parity = "no";
    PortCfg.Speed = "38400";
    PortCfg.DBits = "8";
    PortCfg.SBits = "1";
    if ( port_init ( &Port, &PortCfg ) == -1 )
    {
        perror ( "port_init()." );
        return -1;
    }
    puts ( "Port init - done." );
    uint8_array_t u8aW;
    uint8_array_init ( &u8aW, 0 );
    uint8_array_update ( &u8aW, "ATZ\n\r", strlen ( "ATZ\n\r" ) );
    if ( port_write ( &Port, &u8aW ) <= 0 )
    {
        perror ( "port_write()." );
        port_destroy ( &Port );
        uint8_array_destroy ( &u8aW );
        return -1;
    }
    puts ( "Write to port - done." );
    uint8_array_destroy ( &u8aW );
    uint8_array_t u8aR;
    uint8_array_init ( &u8aR, 0 );
    if ( port_read ( &Port, &u8aR, 5000 ) <= 0 )
    {
        perror ( "port_read()." );
        port_destroy ( &Port );
        uint8_array_destroy ( &u8aR );
        return -1;
    }
    puts ( "Read from port - done." );
    fprintf ( stdout, "%.*s\n", u8aR.Size, u8aR.Item );
    uint8_array_destroy ( &u8aR );
    
	return 0;
}

