/*
 * rs232_port.c
 *
 *  Created on: 05.12.2012
 *      Author: andrey
 */



//#include <glib.h>

#include "rs232_port.h"

#include <errno.h>
#include <string.h>

const char* speed_str_array[] =
{
	"0",
	"50",
	"75",
	"110",
	"134",
	"150",
	"200",
	"300",
	"600",
	"1200",
	"1800",
	"2400",
	"4800",
	"9600",
	"19200",
	"38400",
	"57600",
	"115200",
	"230400",
	NULL
};

speed_t speed_array[] =
{
	B0,
	B50,
	B75,
	B110,
	B134,
	B150,
	B200,
	B300,
	B600,
	B1200,
	B1800,
	B2400,
	B4800,
	B9600,
	B19200,
	B38400,
	B57600,
	B115200,
	B230400,
};

static void alarm_handler(int signo)
{

}

static int start_reading_timer(struct timeval TVal)
{
	int __out;
	//назначить обработчик прерывания таймера
	if( signal(SIGALRM, alarm_handler) == SIG_ERR )
	{
		__out = -1;
	}
	else
	{
		struct itimerval iTVal;

		iTVal.it_interval.tv_sec = 0; //нет периода повторения
		iTVal.it_interval.tv_usec = 0;

		iTVal.it_value = TVal; //значение для отсчёта

		if( setitimer(ITIMER_REAL, &iTVal, NULL) < 0 ) //запуск таймера
		{
			__out = -1;
		}
		else
		{
			__out = 0;
		}
	}

	return __out;
}

static int fire_reading_timer()
{
	struct itimerval TVal;
	int __out;

	if( !getitimer(ITIMER_REAL, &TVal) ) //получим состояние таймера
	{
		//таймер выгорел или нет ещё
		__out = (TVal.it_value.tv_sec > 0) || (TVal.it_value.tv_usec > 0);
	}
	else
	{
        __out = 0;
	}

	return __out;
}

int rs232_open(const char* rs232_file_name)
{
	int rs232_fd = open ( rs232_file_name, O_RDWR | O_NOCTTY | O_NONBLOCK );

	if ( rs232_fd >= 0 )
	{
		//Если в текущий момент нет символов доступных для чтения,
		//то вызов не будет блокироваться
       		if( fcntl(rs232_fd, F_SETFL, 0) == -1 )
		{
			rs232_close ( rs232_fd );
			rs232_fd = -1;
		}
	}

	return rs232_fd;
}

int rs232_write( int rs232_fd, const uint8_array_t *out )
{
    int R, written_bytes = 0;

    for( size_t i = 0; i < out -> len; i++ )
	{
        if( write ( rs232_fd, &(out -> data[i]), 1 ) < 0 )
		{
			written_bytes = -1;
            R = EE_WRITE;
			break;
		}
		else
        {
            
			written_bytes++;
        }
	}

    R = ( written_bytes == out->len ) ? EE_SUCCESS : EE_WRITE;

	return written_bytes;
}

static struct timeval msec_to_timeval ( uint32_t _msec_timeout )
{
	div_t raw = div( (int)_msec_timeout, 1000 );

	struct timeval cooked = { .tv_sec = raw.quot, .tv_usec = raw.rem * 1000 };

	return cooked;
}

/*
uint8_array_t* rs232_read ( int fd, uint32_t msec_timeout )
{
	int readen_bytes = 0;
	uint8_array_t *in = uint8_array_new ( 0 );
	struct timeval read_TVal = msec_to_timeval ( msec_timeout );
	//стартануть таймер с первыми значениями
	if( !start_reading_timer(read_TVal) && fd >= 0 )
	{
		for(;;)
		{
			int bytes = 0;
			ioctl(fd, FIONREAD, &bytes);
			if( bytes > 0 ) //есть доступные символы
			{
				for ( int i = 0; i < bytes; i++ )
				{
					char buf; //буфер чтения

					int rstatus = read(fd, &buf, 1);
					if( rstatus > 0 ) //символ считан
					{
						in = uint8_array_append_data ( in, (uint8_t*)&buf, 1 );

						readen_bytes++;

						if( start_reading_timer(read_TVal) ) //стартануть таймер для байта
						{
							break;
						}
					}
					else if ( rstatus < 0 )
					{
						in = uint8_array_delete ( in );
						readen_bytes = -1;
					}
				}
			}

			if( !fire_reading_timer(read_TVal) && !readen_bytes ) //время на чтение вышло и ничего не считано вообще
			{
				break;
			}
			else if( !fire_reading_timer(read_TVal) && readen_bytes ) //время на чтение вышло, но есть считанные символы
			{
				break;
			}
		}
	}

	return in;
}
*/

void rs232_close( int rs232_fd )
{
	close ( rs232_fd );
}

int rs232_init( int rs232_fd, struct termios *T )
{
	if ( isatty ( rs232_fd ) ) //проверка соответствия поданного дескриптора устройству tty
	{
		//получить старые настройки com-порта
		if( !tcgetattr ( rs232_fd, T ) )
		{
			T -> c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);

			T -> c_oflag = 0;

			T -> c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

			T -> c_cflag &= ~(CSIZE | CSTOPB );

			T -> c_cflag |= CS8 | CREAD;

			T -> c_cflag &= ~PARENB;

			for ( int i = 0; i < NCCS; i++ )
				T -> c_cc[ i ] = 0;

			T -> c_cc[ VMIN ]  = 1;
			T -> c_cc[ VTIME ] = 1;

			return 0;
		}
	}

	return -1;
}

int rs232_set_speed( struct termios *T, const char *speed )
{
	int index = 0;
	while ( speed_str_array[ index ] != NULL )
	{
		if ( !strcmp ( speed, speed_str_array[ index ] ) )
		{
			break;
		}
		else
			index++;
	}
	 // установка скорости ввода/вывода
	if ( ( cfsetispeed ( T, speed_array[ index ] ) < 0 ) ||
		 ( cfsetospeed ( T, speed_array[ index ] ) < 0 ) )
		return -1;
	else
		return 0;

}

void rs232_set_parity ( struct termios *T, const char *parity )
{
	if ( !strcmp ( "no", parity ) )
	{
		T -> c_iflag &= ~INPCK; //непроверять чётность на входе
		T -> c_cflag &= ~PARENB; //не генерировать бит чётности
	}
	else if ( !strcmp ( "odd", parity ) )
	{
		T -> c_iflag |= INPCK; //включить проверку чётности на входе
		T -> c_cflag |= PARENB; //генерить проверку чётности
		T -> c_cflag |= PARODD; //переключить на нечётность
	}
	else if ( !strcmp ( "even", parity ) )
	{
		T -> c_iflag |= INPCK; //включить проверку чётности на входе
		T -> c_cflag |= PARENB; //генерить проверку чётности
	}

}

void rs232_set_stopbits ( struct termios *T, const char *stopbits )
{
	if ( !strcmp ( "1", stopbits ) )
	{
		T -> c_cflag &= ~( CSTOPB ); //один стоп бит
	}
	else if ( !strcmp ( "2", stopbits ) )
	{
		T -> c_cflag |= CSTOPB; //два стоп бита
	}

}

void rs232_set_databits ( struct termios *T, const char *databits )
{
	switch ( databits[0] )
	{
		case '8':
			T -> c_cflag &= ~( CSIZE );
			T -> c_cflag |= CS8;
			break;
		case '7':
			T -> c_cflag &= ~( CSIZE );
			T -> c_cflag |= CS7;
			break;
		case '6':
			T -> c_cflag &= ~( CSIZE );
			T -> c_cflag |= CS6;
			break;
		case '5':
			T -> c_cflag &= ~( CSIZE );
			T -> c_cflag |= CS5;
			break;
		default:
			break;
	}
}

int rs232_apply( int rs232_fd, struct termios *T )
{
	return tcsetattr ( rs232_fd, TCSAFLUSH, T );
}

int rs232_read ( int fd, uint32_t start_timeout, uint32_t stop_timeout, uint8_array_t**in )
{
    int R;
    
	uint8_array_t *_in = uint8_array_new ( 0 );
	struct timeval start_TVal = msec_to_timeval ( start_timeout ); // таймер на ожидание начала приёма
	struct timeval stop_TVal = msec_to_timeval ( stop_timeout ); // таймер на ожидание конца приёма
	struct timeval tmp_TVal = start_TVal;

	//стартануть таймер с первыми значениями
	if ( !start_reading_timer ( start_TVal ) && fd >= 0 )
	{
		for ( ;; ) // событийный цикл
		{
			int bytes = 0;
			int rstatus = 0;

			ioctl(fd, FIONREAD, &bytes); // сколько символов считано
			
			if( bytes > 0 ) // есть доступные символы
			{
				uint8_t buf[ bytes ]; // буфер чтения
				rstatus = read ( fd, &buf, bytes ); // читать все символы
					
				if( rstatus > 0 ) //символ считан
				{
					_in = uint8_array_append_data ( _in, buf, rstatus ); // добавить символы

					if ( start_reading_timer ( stop_TVal ) ) // стартануть таймер ожидания конца передачи
					{
						// ошибка запуска таймера
						_in = uint8_array_delete ( _in );
						rstatus = -1;
                        R = EE_TIMER;
                        
						break;
					}
				}
				else if ( rstatus < 0 )
				{
					// ошибка чтения
					_in = uint8_array_delete ( _in );
                    R = EE_READ;
                    	
					break;	
				}
			}
			
			int tstatus = fire_reading_timer (); // проверка окончания работы таймера
			
            if ( tstatus < 0 )
            {
                _in = uint8_array_delete ( _in ); // ошибка таймера
                R = EE_TIMER;
                break;
            }
            else
            {
                R = EE_SUCCESS;
                break;
            }
		}
	}

    *in = _in

	return R;
}


