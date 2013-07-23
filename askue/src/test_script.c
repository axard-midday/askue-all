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


#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

static void hdl (int sig, siginfo_t *siginfo, void *context)
//static void hdl (int sig)
{
    //if ( sig == SIGTERM )
    //{
        fprintf ( stderr, "[ %s | %s ]: %s\n", "Сигнал скрипта", "ОК", "Скрипт завершён по сигналу" );
        exit ( EXIT_SUCCESS );
    //}
    //else
    //{
    //    write_log ( stderr, "Сигнал скрипта", "ERROR", "Ошибка" );
    //}
}


int main(int argc, char **argv)
{
    //signal ( SIGTERM, hdl );
    
    setpgrp();
    struct sigaction act;
 
	memset (&act, '\0', sizeof(act));
 
	act.sa_sigaction = &hdl;
 
	act.sa_flags = SA_SIGINFO;
 
	if (sigaction(SIGUSR2, &act, NULL) < 0) {
		fprintf ( stderr, "[ %s | %s ]: %s\n", "Скрипт", "FAIL", "sigaction()" );
		exit ( EXIT_FAILURE );
	}
    
    fprintf ( stderr, "[ %s | %s ]: %s\n", "Скрипт", "ОК", "Скрипт начал работу" );
    sleep ( 15 );
    fprintf ( stderr, "[ %s | %s ]: %s\n", "Скрипт", "ОК", "Скрипт окончил работу" );
	exit ( EXIT_SUCCESS );
}

