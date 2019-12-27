/* $Id: wiiosc.c 54 2007-03-10 14:54:20Z nescivi $ 
 *
 * Copyright (C) 2007, Marije Baalman <nescivi@gmail.com>
 *
 * based on examples for
 * libcwiid by L. Donnie Smith <cwiid@abstrakraft.org>
 * (libwiimote by Joel Andersson <bja@kth.se>)
 * and liblo by Steve Harris, Uwe Koloska
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 TODO: add speaker support
 TODO: create a connection scheme, so that multiple clients can connect; something similar to the cwonder stream mechanism
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <lo/lo.h>

#include <cwiid.h>

/// feel free to set to a higher number of WII's
#define MAX_NMOTES 4

#define toggle_bit(bf,b)	\
	(bf) = ((bf) & b)		\
	       ? ((bf) & ~(b))	\
	       : ((bf) | (b))

// HEADER

int done;
cwiid_wiimote_t *wiimote[MAX_NMOTES];
int ids[MAX_NMOTES];
struct cwiid_state *state[MAX_NMOTES];	/* wiimote state */
unsigned char rpt_mode[MAX_NMOTES];
unsigned char led_state[MAX_NMOTES];

// unsigned char rpt_mode = 0;


int nmotes;
int updtime;
lo_address t;
lo_server s;
lo_server_thread st;

cwiid_mesg_callback_t cwiid_callback;

int discover();
void enable(int id);
void enable_report(int id);
void enable_button(int id);
void enable_ir(int id);
void enable_motion(int id);
void enable_extension(int id);

void set_led_state(cwiid_wiimote_t *wiimote, unsigned char led_state);
void set_rpt_mode(cwiid_wiimote_t *wiimote, unsigned char rpt_mode);

void error(int num, const char *m, const char *path);

int generic_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data);

int enable_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);
int info_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);
int leds_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);
int rumble_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);
int discover_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);
int report_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);
int acc_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);
int button_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);
int extension_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);
int ir_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);
int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);

// int updatetime_handler(const char *path, const char *types, lo_arg **argv, int argc,
// 		 void *data, void *user_data);
// int get_extension_handler(const char *path, const char *types, lo_arg **argv, int argc,
// 		 void *data, void *user_data);


// HEADER end

cwiid_err_t err;
void err(cwiid_wiimote_t *wiimote, const char *s, va_list ap)
{
	if (wiimote) printf("%d:", cwiid_get_id(wiimote));
	else printf("-1:");
	vprintf(s, ap);
	printf("\n");
}

void add_bundle_message_2int( lo_bundle * b, const char *path, int i, int data )
{
	lo_message m = lo_message_new();
	lo_message_add_int32( m, i );
	lo_message_add_int32( m, data );
	lo_bundle_add_message( *b, path, m );
//  	lo_message_free( m );
}

void add_bundle_message_intfloat( lo_bundle * b, const char *path, int i, float data )
{
	lo_message m = lo_message_new();
	lo_message_add_int32( m, i );
	lo_message_add_float( m, data );
	lo_bundle_add_message( *b, path, m );
//  	lo_message_free( m );
}

void add_bundle_message_3int( lo_bundle * b, const char *path, int i, int j, int data )
{
	lo_message m = lo_message_new();
	lo_message_add_int32( m, i );
	lo_message_add_int32( m, j );
	lo_message_add_int32( m, data );
	lo_bundle_add_message( *b, path, m );
//  	lo_message_free( m );
}

void add_bundle_message_2intfloat( lo_bundle * b, const char *path, int i, int j, float data )
{
	lo_message m = lo_message_new();
	lo_message_add_int32( m, i );
	lo_message_add_int32( m, j );
	lo_message_add_float( m, data );
	lo_bundle_add_message( *b, path, m );
// 	lo_message_free( m );
}

void set_bit( unsigned char * bf, unsigned char b )
{
// 	printf( "set bit %i %i\n", *bf, b );
	if (!(*bf & b) )
		toggle_bit( *bf, b );
// 	printf( "set bit %i %i\n", *bf, b );
}

void clear_bit( unsigned char * bf, unsigned char b )
{
// 	printf( "clear bit %i %i\n", *bf, b );
	if ((*bf & b) )
		toggle_bit( *bf, b );
// 	printf( "set bit %i %i\n", *bf, b );
}

int main(int argc, char **argv)
{
// 	int astate = 0;
//     int i = 0;
// 	char *cvar, *carg;
    char *port = "57150";
    char *outport = "57120";
	char *ip = "127.0.0.1";
	int autoconnect = 0;
	int disc;

	cwiid_set_err(err);

 	updtime = 100000;
// 	updtime = 2000;

    nmotes = 0;
    done = 0;
	
    /* Print help information. */

//     printf("argv: %s %s %s %s %s %i\n", argv[0], argv[1], argv[2], argv[3], argv[4], argc );
	if ( argc >= 4 )
		{
		autoconnect = 1;
		ip = argv[3];
		port = argv[2];
		outport = argv[1];
		}
	else if ( argc == 4 )
		{
		ip = argv[3];
		port = argv[2];
		outport = argv[1];
		}
	else if ( argc == 3 )
		{
		port = argv[2];
		outport = argv[1];
		}
	else if ( argc == 2 )
		{
		outport = argv[1];
		}

	
    printf("============================================================================\n");
    printf("wiiosc - v0.3 - sends out osc data on incoming wii data\n");
    printf("(c) 2007, Marije Baalman\n");
    printf("start with \"wiiosc <target_port> <recv_port> <target_ip>\" \n");
	printf("use \"wiiosc <target_port> <recv_port> <target_ip> auto\" to start discovery automatically\n");
    printf("This is free software released under the GNU/General Public License\n");
    printf("============================================================================\n\n");
    printf("Listening to port: %s\n", port );
    printf("Sending to ip and port: %s %s\n", ip, outport );
	if ( !autoconnect )
    	printf("Send message /wii/discover to discover a WII and\n");
    printf("press buttons 1 and 2 on the wiimote to connect.\n");
    fflush(stdout);

    /* create liblo addres */
    t = lo_address_new(ip, outport); // change later to use other host
// 	printf("OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t));

	// me
//     lo_address me = lo_address_new(NULL, "57150"); // change later to use other host
    
	lo_server_thread st = lo_server_thread_new(port, error);
	lo_server_thread_add_method(st, "/wii/discover", "", discover_handler, NULL);
	lo_server_thread_add_method(st, "/wii/enable", "ii", enable_handler, NULL);

    lo_server_thread_add_method(st, "/wii/info", "", info_handler, NULL);
    lo_server_thread_add_method(st, "/wii/enable/motion", "ii", acc_handler, NULL);
    lo_server_thread_add_method(st, "/wii/enable/button", "ii", button_handler, NULL);
	lo_server_thread_add_method(st, "/wii/enable/report", "ii", report_handler, NULL);
    lo_server_thread_add_method(st, "/wii/enable/extension", "ii", extension_handler, NULL);
    lo_server_thread_add_method(st, "/wii/enable/ir", "ii", ir_handler, NULL);
    lo_server_thread_add_method(st, "/wii/leds", "iiiii", leds_handler, NULL);
    lo_server_thread_add_method(st, "/wii/rumble", "ii", rumble_handler, NULL);
    lo_server_thread_add_method(st, "/wii/quit", "", quit_handler, NULL);

//     lo_server_thread_add_method(st, "/wii/extension", "i", get_extension_handler, NULL);
//     lo_server_thread_add_method(st, "/wii/updatetime", "i", updatetime_handler, NULL);

    lo_server_thread_add_method(st, NULL, NULL, generic_handler, NULL);

    lo_server_thread_start(st);

    lo_server s = lo_server_thread_get_server( st );

	if ( autoconnect )
		{
		disc = discover();
		if ( disc != -1 ){
			enable( disc );
			enable_report(disc);
			enable_button(disc);
			enable_motion(disc);
			enable_ir(disc);
			enable_extension(disc);
			}
		}

    while (!done) {
		// keep waiting for callbacks
		usleep(updtime); // every 5 milliseconds (longer makes reaction sluggish)
    }

    lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/quit", "s", "nothing more to do, quitting" );
    lo_server_thread_free( st );
    lo_address_free( t );

    return 0;
}

void error(int num, const char *msg, const char *path)
{
     printf("liblo server error %d in path %s: %s\n", num, path, msg);
     fflush(stdout);
}

int enable_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
	if ( argv[1]->i == 1 ) {
		if (cwiid_enable(wiimote[argv[0]->i], CWIID_FLAG_MESG_IFC)) {
			if ( lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/error", "s", "Unable to enable\n") == -1 )
				{ printf("wii unable to enable: OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t)); }
		}
	} else {
		if (cwiid_disable(wiimote[argv[0]->i], CWIID_FLAG_MESG_IFC)) {
			if ( lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/error", "s", "Unable to disable\n") == -1 )
				{ printf("wii unable to disable: OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t)); }
		}
	}
	return 0;
}

void enable( int id )
{
	if (cwiid_enable(wiimote[id], CWIID_FLAG_MESG_IFC)) {
		if ( lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/error", "s", "Unable to enable\n") == -1 )
			{ printf("wii unable to enable: OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t)); }
	}
}

int discover()
{
	int res = -1;
	bdaddr_t bdaddr;

	if ( nmotes < MAX_NMOTES ){
		bdaddr = *BDADDR_ANY;
		rpt_mode[nmotes] = 0;
		led_state[nmotes] = 0;
		if ( (wiimote[nmotes] = cwiid_open(&bdaddr, CWIID_FLAG_MESG_IFC)) == NULL ) {
				if ( lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/error", "s", "Unable to connect to wiimote\n") == -1 )
					{ printf("wii unable to connect: OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t)); }
        	} else {
				if (cwiid_set_mesg_callback(wiimote[nmotes], cwiid_callback)) {
					if ( lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/error", "s", "Unable to set message callback\n") == -1 )
						{ printf("wii unable to set callback: OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t)); }
					if ( cwiid_close(wiimote[nmotes]) ) {
						if ( lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/error", "s", "Unable to close wii\n") == -1 )
							{ printf("wii unable to close wii: OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t)); }
						}
					wiimote[nmotes] = NULL;
			    } else {
					ids[nmotes] = cwiid_get_id( wiimote[nmotes] );
					if ( lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/found", "i", nmotes ) == -1 )
						{ printf("wii found: OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t)); }
					if (cwiid_enable(wiimote[nmotes], CWIID_FLAG_MESG_IFC)) {
						if ( lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/error", "s", "Unable to enable messages\n") == -1 )
							{ printf("wii unable to enable messages: OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t)); }
						}
					rpt_mode[nmotes] = CWIID_RPT_STATUS;
 					cwiid_command( wiimote[nmotes], CWIID_CMD_RPT_MODE, CWIID_RPT_STATUS );
					printf("wii found\n");
					res = ids[nmotes];
					nmotes++;
				}
			}
		}
    fflush(stdout);
	return res;
}

int discover_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
	discover();
    return 0;
}

int info_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
	int i;

    for (i=0; i<nmotes; i++) {
		if ( lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/found", "i", i) == -1 )
			{ printf("wii found: OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t)); }
		if ( lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/connected", "i", i) == -1 )
			{ printf("wii connected: OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t)); }
    }

    fflush(stdout);

    return 0;
}

// int updatetime_handler(const char *path, const char *types, lo_arg **argv, int argc,
// 		 void *data, void *user_data)
// {
//     /* example showing pulling the argument values out of the argv array */
// //     printf("%s <- i:%d, i:%d, i:%d, i:%d, i:%d\n\n", path, argv[0]->i, argv[1]->i, argv[2]->i, argv[3]->i, argv[4]->i);
// //     fflush(stdout);
// 
//     updtime = argv[0]->i * 1000;
// 
//     return 0;
// }


int leds_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
    /* example showing pulling the argument values out of the argv array */
	if ( argv[1]->i )
		set_bit(&led_state[argv[0]->i], CWIID_LED1_ON);
	else
		clear_bit(&led_state[argv[0]->i], CWIID_LED1_ON);
	if ( argv[2]->i )
		set_bit(&led_state[argv[0]->i], CWIID_LED2_ON);
	else
		clear_bit(&led_state[argv[0]->i], CWIID_LED2_ON);
	if ( argv[3]->i )
		set_bit(&led_state[argv[0]->i], CWIID_LED3_ON);
	else
		clear_bit(&led_state[argv[0]->i], CWIID_LED3_ON);
	if ( argv[4]->i )
		set_bit(&led_state[argv[0]->i], CWIID_LED4_ON);
	else
		clear_bit(&led_state[argv[0]->i], CWIID_LED4_ON);
	set_led_state(wiimote[argv[0]->i], led_state[argv[0]->i]);

    return 0;
}

int rumble_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
    /* example showing pulling the argument values out of the argv array */
	unsigned char rumble = 0;
	rumble = argv[1]->i;
	if (cwiid_set_rumble(wiimote[argv[0]->i], rumble)) {
		if ( lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/error", "s", "Unable to set rumble\n") == -1 )
			{ printf("wii unable to set rumble: OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t)); }
	}

    return 0;
}

			

int report_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{

	if ( argv[1]->i )
		set_bit(&rpt_mode[argv[0]->i], CWIID_RPT_STATUS);
	else
		clear_bit(&rpt_mode[argv[0]->i], CWIID_RPT_STATUS);
	set_rpt_mode(wiimote[argv[0]->i], rpt_mode[argv[0]->i]);

    return 0;
}

void enable_report( int id )
{
	set_bit(&rpt_mode[id], CWIID_RPT_STATUS);
	set_rpt_mode(wiimote[id], rpt_mode[id]);
}

void enable_motion( int id )
{
	set_bit(&rpt_mode[id], CWIID_RPT_ACC);
	set_rpt_mode(wiimote[id], rpt_mode[id]);
}

void enable_button( int id )
{
	set_bit(&rpt_mode[id], CWIID_RPT_BTN);
	set_rpt_mode(wiimote[id], rpt_mode[id]);
}

void enable_extension( int id )
{
	set_bit(&rpt_mode[id], CWIID_RPT_EXT);
	set_rpt_mode(wiimote[id], rpt_mode[id]);
}

void enable_ir( int id )
{
	set_bit(&rpt_mode[id], CWIID_RPT_IR);
	set_rpt_mode(wiimote[id], rpt_mode[id]);
}

int acc_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
	if ( argv[1]->i )
		set_bit(&rpt_mode[argv[0]->i], CWIID_RPT_ACC);
	else
		clear_bit(&rpt_mode[argv[0]->i], CWIID_RPT_ACC);
	set_rpt_mode(wiimote[argv[0]->i], rpt_mode[argv[0]->i]);

    return 0;
}

int button_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
// 	printf( "enable buttons\n"); fflush(stdout);
	if ( argv[1]->i )
		set_bit(&rpt_mode[argv[0]->i], CWIID_RPT_BTN);
	else
		clear_bit(&rpt_mode[argv[0]->i], CWIID_RPT_BTN);
	set_rpt_mode(wiimote[argv[0]->i], rpt_mode[argv[0]->i]);

    return 0;
}

int extension_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{

 	if ( argv[1]->i )
		set_bit(&rpt_mode[argv[0]->i], CWIID_RPT_EXT);
	else
		clear_bit(&rpt_mode[argv[0]->i], CWIID_RPT_EXT);
	set_rpt_mode(wiimote[argv[0]->i], rpt_mode[argv[0]->i]);

    return 0;
}

int ir_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{

	if ( argv[1]->i )
		set_bit(&rpt_mode[argv[0]->i], CWIID_RPT_IR);
	else
		clear_bit(&rpt_mode[argv[0]->i], CWIID_RPT_IR);
	set_rpt_mode(wiimote[argv[0]->i], rpt_mode[argv[0]->i]);


    return 0;
}

// int get_extension_handler(const char *path, const char *types, lo_arg **argv, int argc,
// 		 void *data, void *user_data)
// {
//     /* example showing pulling the argument values out of the argv array */
// //      printf("%s <- i:%d\n\n", path, argv[0]->i);
// //      fflush(stdout);
// 
// //     lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/extension", "ii", argv[0]->i, wiimote[argv[0]->i].ext.id );
// 
//     return 0;
// }


int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
    done = 1;
    printf("wiiosc: allright, that's it, I quit\n");
    fflush(stdout);

    return 0;
}

/* catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods */
int generic_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data)
{
    int i;

    printf("path: <%s>\n", path);
    for (i=0; i<argc; i++) {
	printf("arg %d '%c' ", i, types[i]);
	lo_arg_pp(types[i], argv[i]);
	printf("\n");
    }
    printf("\n");
    fflush(stdout);

    return 1;
}


void set_led_state(cwiid_wiimote_t *wiimotet, unsigned char led_state)
{
	if (cwiid_set_led(wiimotet, led_state)) {
		if ( lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/error", "s", "Unable to set led state\n") == -1 )
			{ printf("wii unable to set led state: OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t)); }
	}
}
	
void set_rpt_mode(cwiid_wiimote_t *wiimotet, unsigned char rpt_mode)
{
	if (cwiid_set_rpt_mode(wiimotet, rpt_mode)) {
		if ( lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/error", "s", "Unable to set report mode\n") == -1 )
			{ printf("wii unable to set report mode: OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t)); }
	}
}

void cwiid_callback(cwiid_wiimote_t *wiimotet, int mesg_count,
                    union cwiid_mesg mesg[], struct timespec *timestamp)
{
	int i, j;
	int valid_source;
	int thisid, id;

	id = 0;

// 	printf( "callback \n" ); fflush( stdout );

	lo_bundle b = lo_bundle_new( LO_TT_IMMEDIATE );

	thisid = cwiid_get_id( wiimotet );
	for ( i=0; i < nmotes; i++ )
		{
		if ( thisid == ids[i] )
			{
			id = i;
			break;
			}
		}

	printf( "mesg_count %i", mesg_count );
	fflush( stdout );

	for (i=0; i < mesg_count; i++)
	{
		switch (mesg[i].type) {
		case CWIID_MESG_STATUS:
		    add_bundle_message_intfloat( &b, "/wii/battery", id, (float) mesg[i].status_mesg.battery / CWIID_BATTERY_MAX);

			add_bundle_message_2int( &b, "/wii/extension", id, mesg[i].status_mesg.ext_type );	

		// FIXME:
/*			switch (mesg[i].status_mesg.ext_type) {
			case CWIID_EXT_NONE:
				add_bundle_message_2int( &b, "/wii/extension", id, "None" );	
				break;
			case CWIID_EXT_NUNCHUK:
				add_bundle_message_2int( &b, "/wii/extension", id, "Nunchuk" );	
				break;
			case CWIID_EXT_CLASSIC:
			    add_bundle_message_2int( &b, "/wii/extension", id, "Classic controller" );	
				break;
			default:
				add_bundle_message_2int( &b, "/wii/extension", id, "Unknown" );	
				break;
			}*/
// 			printf("\n");
			break;
		case CWIID_MESG_BTN:
// 			printf("Button Report: %.4X\n", mesg[i].btn_mesg.buttons);
		    add_bundle_message_2int( &b, "/wii/keys/one", id, (CWIID_BTN_1 & mesg[i].btn_mesg.buttons) > 0 );	
		    add_bundle_message_2int( &b, "/wii/keys/two", id, (CWIID_BTN_2 & mesg[i].btn_mesg.buttons)  > 0 );
		   	add_bundle_message_2int( &b, "/wii/keys/a", id, (CWIID_BTN_A & mesg[i].btn_mesg.buttons)  > 0 );
			add_bundle_message_2int( &b, "/wii/keys/b", id, (CWIID_BTN_B & mesg[i].btn_mesg.buttons)   > 0);
			add_bundle_message_2int( &b, "/wii/keys/left", id, (CWIID_BTN_LEFT & mesg[i].btn_mesg.buttons)  > 0);
	    	add_bundle_message_2int( &b, "/wii/keys/right",  id, (CWIID_BTN_RIGHT & mesg[i].btn_mesg.buttons)  > 0);
	    	add_bundle_message_2int( &b, "/wii/keys/up",  id, (CWIID_BTN_UP & mesg[i].btn_mesg.buttons)  > 0);
	    	add_bundle_message_2int( &b, "/wii/keys/down",  id, (CWIID_BTN_DOWN & mesg[i].btn_mesg.buttons)  > 0);
	    	add_bundle_message_2int( &b, "/wii/keys/home",  id, (CWIID_BTN_HOME & mesg[i].btn_mesg.buttons)  > 0);
	    	add_bundle_message_2int( &b, "/wii/keys/plus",  id, (CWIID_BTN_PLUS & mesg[i].btn_mesg.buttons)  > 0);
	    	add_bundle_message_2int( &b, "/wii/keys/minus",  id, (CWIID_BTN_MINUS & mesg[i].btn_mesg.buttons)  > 0);
			break;
		case CWIID_MESG_ACC:
			add_bundle_message_intfloat( &b, "/wii/acc/x",  id, (float) mesg[i].acc_mesg.acc[CWIID_X] / CWIID_ACC_MAX);
			add_bundle_message_intfloat( &b, "/wii/acc/y",  id, (float) mesg[i].acc_mesg.acc[CWIID_Y] / CWIID_ACC_MAX);
			add_bundle_message_intfloat( &b, "/wii/acc/z",  id, (float) mesg[i].acc_mesg.acc[CWIID_Z] / CWIID_ACC_MAX);
			break;
		case CWIID_MESG_IR:
			for (j = 0; j < CWIID_IR_SRC_COUNT; j++) {
				valid_source = 0;
				if (mesg[i].ir_mesg.src[j].valid) {
					valid_source = 1;
					add_bundle_message_2intfloat( &b, "/wii/ir/x", id, j, (float) mesg[i].ir_mesg.src[j].pos[CWIID_X] / CWIID_IR_X_MAX);
					add_bundle_message_2intfloat( &b, "/wii/ir/y", id, j, (float) mesg[i].ir_mesg.src[j].pos[CWIID_Y] / CWIID_IR_Y_MAX);
// 				add_bundle_message_2int( &b, "/wii/ir/size", id, j, wiimote[i].ir1.size);
					}
				add_bundle_message_3int( &b, "/wii/ir/valid", id, j, valid_source );
			}
			break;
		case CWIID_MESG_NUNCHUK:
// 			printf("Button Report: %.4X\n", mesg[i].nunchuk_mesg.buttons);
			add_bundle_message_intfloat( &b, "/wii/nunchuk/acc/x",  id, (float) mesg[i].nunchuk_mesg.acc[CWIID_X] / CWIID_ACC_MAX);
			add_bundle_message_intfloat( &b, "/wii/nunchuk/acc/y",  id, (float) mesg[i].nunchuk_mesg.acc[CWIID_Y] / CWIID_ACC_MAX);
			add_bundle_message_intfloat( &b, "/wii/nunchuk/acc/z",  id, (float) mesg[i].nunchuk_mesg.acc[CWIID_Z] / CWIID_ACC_MAX);
			add_bundle_message_intfloat( &b, "/wii/nunchuk/joy/x",  id, (float) mesg[i].nunchuk_mesg.stick[CWIID_X] / 256);
			add_bundle_message_intfloat( &b, "/wii/nunchuk/joy/y",  id, (float) mesg[i].nunchuk_mesg.stick[CWIID_Y] / 256);
			add_bundle_message_2int( &b, "/wii/nunchuk/keys/z",  id, (CWIID_NUNCHUK_BTN_Z & mesg[i].nunchuk_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/nunchuk/keys/c",  id, (CWIID_NUNCHUK_BTN_C & mesg[i].nunchuk_mesg.buttons)>0);
			break;
		case CWIID_MESG_CLASSIC:
// 			printf("Button Report: %.4X\n", mesg[i].classic_mesg.buttons);
			add_bundle_message_intfloat( &b, "/wii/classic/joy1/x",  id, (float) mesg[i].classic_mesg.l_stick[CWIID_X] / CWIID_CLASSIC_L_STICK_MAX);
			add_bundle_message_intfloat( &b, "/wii/classic/joy1/y",  id, (float) mesg[i].classic_mesg.l_stick[CWIID_Y] / CWIID_CLASSIC_L_STICK_MAX);
			add_bundle_message_intfloat( &b, "/wii/classic/joy2/x",  id, (float) mesg[i].classic_mesg.r_stick[CWIID_X] / CWIID_CLASSIC_R_STICK_MAX);
			add_bundle_message_intfloat( &b, "/wii/classic/joy2/y",  id, (float) mesg[i].classic_mesg.r_stick[CWIID_Y] / CWIID_CLASSIC_R_STICK_MAX);
			add_bundle_message_intfloat( &b, "/wii/classic/analog/l",  id, (float) mesg[i].classic_mesg.l / CWIID_CLASSIC_LR_MAX);
			add_bundle_message_intfloat( &b, "/wii/classic/analog/r",  id, (float) mesg[i].classic_mesg.r / CWIID_CLASSIC_LR_MAX);
		
			add_bundle_message_2int( &b, "/wii/classic/keys/left",  id, (CWIID_CLASSIC_BTN_LEFT & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/right",  id, (CWIID_CLASSIC_BTN_RIGHT & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/up",  id, (CWIID_CLASSIC_BTN_UP & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/down",  id, (CWIID_CLASSIC_BTN_DOWN & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/l",  id, (CWIID_CLASSIC_BTN_L & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/r",  id, (CWIID_CLASSIC_BTN_R & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/zl",  id, (CWIID_CLASSIC_BTN_ZL & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/zr",  id, (CWIID_CLASSIC_BTN_ZR & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/minus",  id, (CWIID_CLASSIC_BTN_MINUS & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/plus",  id, (CWIID_CLASSIC_BTN_PLUS & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/home",  id, (CWIID_CLASSIC_BTN_HOME & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/x",  id, (CWIID_CLASSIC_BTN_X & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/y",  id, (CWIID_CLASSIC_BTN_Y & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/a",  id, (CWIID_CLASSIC_BTN_A & mesg[i].classic_mesg.buttons)>0);
			add_bundle_message_2int( &b, "/wii/classic/keys/b",  id, (CWIID_CLASSIC_BTN_B & mesg[i].classic_mesg.buttons)>0);

			break;
		case CWIID_MESG_ERROR:
			if (cwiid_close(wiimote[id])) {
				lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/disconnected", "i", id);
				lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/error", "s", "could not close device");
// 				exit(-1);
			} else {
				lo_send_from( t, s, LO_TT_IMMEDIATE, "/wii/disconnected", "i", id);
			}
// 			exit(0);
			break;
		default:
			printf("Unknown Report");
			break;
		}
	}

	lo_send_bundle_from( t, s, b );
// 	lo_bundle_free( b );

	// this should be:
 	lo_bundle_free_messages( b );
	// but this is only in liblo > 0.25, which is not shipped yet with Debian
}

