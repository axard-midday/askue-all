/*
 * sprintf_datetime.c
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
#include <stdlib.h>
#include <time.h>

static time_t offset_second ( time_t t, long int offset )
{
        return t + ( time_t ) offset;
}

static time_t offset_month ( time_t t, long int offset )
{
        struct tm *stm = localtime ( &t );
               
        if ( offset >= 12 )
        {
                div_t x = div ( ( int ) offset, 12 );
                
                stm -> tm_year += x.rem;
                
                if ( stm -> tm_mon > ( abs ( x.quot ) - 1 ) )
                {
                        stm -> tm_mon += x.quot;
                }
                else
                {
                       if ( x.quot > 0 )
                       {
                           stm -> tm_mon += x.quot;    
                       }
                       else
                       {
                           stm -> tm_mon = ( 11 + stm -> tm_mon + 1 ) + x.quot;
                           stm -> tm_year --;
                       }
                }
        }
        else
        {
                if ( stm -> tm_mon > ( abs ( offset ) - 1 ) )
                {
                        stm -> tm_mon += offset;
                }
                else
                {
                       if ( offset > 0 )
                       {
                           stm -> tm_mon += offset;    
                       }
                       else
                       {
                           stm -> tm_mon = ( 11 + stm -> tm_mon + 1 ) + offset;
                           stm -> tm_year --;
                       }
                }   
        }
        
        return mktime ( stm );
}

static time_t offset_day ( time_t t, long int offset )
{
          return offset_second ( t, offset * 86400 );
}

static time_t offset_year ( time_t t, long int offset )
{
        struct tm *stm = localtime ( &t );
        
        stm -> tm_year += offset;
        
        return mktime ( stm );
}

static time_t offset_hour ( time_t t, long int offset )
{
        return offset_second ( t, offset * 3600 );
}

static time_t offset_minute ( time_t t, long int offset )
{
        return offset_second ( t, offset * 60 );
}



static time_t start_of_minute ( time_t t )
{
        struct tm *stm = localtime ( &t );
        
        stm -> tm_sec = 0;
        
        return mktime ( stm );
}

static time_t start_of_hour ( time_t t )
{
        struct tm *stm = localtime ( &t );
        
        stm -> tm_sec = 0;
        stm -> tm_min = 0;
        
        return mktime ( stm );
}

static time_t start_of_day ( time_t t )
{
        struct tm *stm = localtime ( &t );
        
        stm -> tm_sec = 0;
        stm -> tm_min = 0;
        stm -> tm_hour = 0;
        
        return mktime ( stm );
}

static time_t start_of_month ( time_t t )
{
        struct tm *stm = localtime ( &t );
        
        stm -> tm_sec = 0;
        stm -> tm_min = 0;
        stm -> tm_hour = 0;
        stm -> tm_mday = 1;
        
        return mktime ( stm );
}

static time_t start_of_year ( time_t t )
{
        struct tm *stm = localtime ( &t );
        
        stm -> tm_sec = 0;
        stm -> tm_min = 0;
        stm -> tm_hour = 0;
        stm -> tm_mday = 1;
        stm -> tm_mon = 0;
        
        return mktime ( stm );
}


static time_t offset_mod ( time_t t, int opcode, long int offset )
{
        time_t result;
        
        switch ( opcode )
        {
                case 0x00:      result = offset_hour ( t, offset ); break;
                case 0x0f:      result = offset_year ( t, offset ); break;
                case 0x0e:      result = offset_minute ( t, offset ); break;
                case 0x10:      result = offset_second ( t, offset ); break;
                case 0x70:      result = offset_month ( t, offset ); break;
                case 0x7c:      result = offset_day ( t, offset ); break;
                default: break;
        }
        
        return result;
}

static time_t start_mod ( time_t t, int opcode )
{
        time_t result;
        
        switch ( opcode )
        {
                case 0x00:      result = start_of_hour ( t ); break;
                case 0x0f:      result = start_of_year ( t ); break;
                case 0x0e:      result = start_of_minute ( t ); break;
                case 0x10:      result = t; break;
                case 0x70:      result = start_of_month ( t ); break;
                case 0x7c:      result = start_of_day ( t ); break;
                default: break;
        }
        
        return result;
}

int get_opcode ( const char *mod )
{
        int opcode = 0;
	
	for ( int i = 0; i < strlen ( mod ); i++ )
		opcode ^= mod[ i ];
		
	return opcode;
}

static time_t prepare_time ( const char *argv[] )
{
        time_t t = time ( NULL );
        
        for ( int i = 0; argv[ i ] != NULL; i++ )
        {
                if ( argv[ i ][ 0 ] == '+' || argv[ i ][ 0 ] == '-' )
                {
                        long int offset;
                        
                        char mod[ 8 ];
                        
                        sscanf ( argv[ i ], "%ld %s", &offset, mod );
                        
                        t = offset_mod ( t, get_opcode ( mod ), offset );
                }
                else if ( !strncmp ( argv[ i ], "start", strlen ( "start" ) ) )
                {
                        char mod[ 8 ];
                        
                        sscanf ( argv[ i ], "start of %s", mod );
                        
                        t = start_mod ( t, get_opcode ( mod ) );
                }
                else
                {
                        continue;
                }
        }
        
        return t;
}

int sprintf_datetime ( char *str, size_t str_size, const char *format, const char *argv[] )
{
      if ( argv != NULL )
      {
           time_t t = prepare_time ( argv );
        
           return strftime ( str, str_size, format, localtime ( &t ) );
      }
      else
      {
               time_t t = time ( NULL );
        
               return strftime ( str, str_size, format, localtime ( &t ) );
      }
}
