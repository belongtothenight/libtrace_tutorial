///////////////////////////////////////////////////////////////////////////////
// CNSRL
// Department of Electrical Engineering
// Chung-Yuan Christian University
//
// File Name:yk_demo_hash.c
// Project:
// Author:
// Date:2020.10.22
// Dependencies:
//
// Description: misc demo for printing payload_length and hashing
//
//
// Copyright notice:  Copyright (C)2020 CNSRL CYCU
// Change history:
//
///////////////////////////////////////////////////////////////////////////////

/* Libtrace program designed to demonstrate the use of the trace_get_source_*
 * shortcut functions. 
 *
 * This code also contains examples of sockaddr manipulation.
 */
#include "libtrace.h"
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include "prng.h"
#include "../lib/massdal/prng.h"

/* This is not the nicest way to print a 6 byte MAC address, but it is 
 * effective. Libtrace will have provided us a pointer to the start of the
 * MAC address within the packet, so we can just use array indices to grab
 * each byte of the MAC address in turn */
static inline void print_mac(uint8_t *mac) {

	printf("%02x:%02x:%02x:%02x:%02x:%02x \n", mac[0], mac[1], mac[2], mac[3],
		mac[4], mac[5]);

}

/* Given a sockaddr containing an IP address, prints the IP address to stdout
 * using the common string representation for that address type */
static inline void print_ip(struct sockaddr *ip) {

	char str[20];
	struct in_addr source_ip_addr;//yk
	
	/* Check the sockaddr family so we can cast it to the appropriate
	 * address type, IPv4 or IPv6 */
	if (ip->sa_family == AF_INET) {
		/* IPv4 - cast the generic sockaddr to a sockaddr_in */
		struct sockaddr_in *v4 = (struct sockaddr_in *)ip;
		//see http://stackoverflow.com/questions/21099041/why-do-we-cast-sockaddr-in-to-sockaddr-when-calling-bind 
		// for more detail explainations

		/* Use inet_ntop to convert the address into a string using
		 * dotted decimal notation */
		printf("%s ", inet_ntop(AF_INET, &(v4->sin_addr), str, 20));
		source_ip_addr=v4->sin_addr;
		printf("\n Source IP address in hex (32-bit unsigned long) %x\n",ntohl(source_ip_addr.s_addr));
		printf("Source IP address hashed in hex (31-bit long long) %ld\n",
			hash31(3721,917,ntohl(source_ip_addr.s_addr)));
		//
		//this is a 2-Universal hash function h=(ax+b)mod p, where a=3, b=5
		//
	}

	if (ip->sa_family == AF_INET6) {
		/* IPv6 - cast the generic sockaddr to a sockaddr_in6 */
		struct sockaddr_in6 *v6 = (struct sockaddr_in6 *)ip;
		/* Use inet_ntop to convert the address into a string using
		 * IPv6 address notation */
		printf("%s ", inet_ntop(AF_INET6, &(v6->sin6_addr), str, 20));
	}


}

void per_packet(libtrace_packet_t *packet)
{
	
	struct sockaddr_storage addr;
	struct sockaddr *addr_ptr;
	uint16_t port;
	uint8_t *mac;
	size_t payload_length, frame_length;

	/* Get the source mac */
	mac = trace_get_source_mac(packet);
	
	/* If this packet does not contain a MAC address, print "NULL" */
	if (mac == NULL) 
		printf("NULL ");
	else
		print_mac(mac);
	//******************
	//get payload length //yk
	// https://research.wand.net.nz/software/libtrace-docs/html/libtrace_8h.html#a6648c75100b8defdad3b7d4632c32eba
	//******************
	payload_length=trace_get_payload_length(packet);
	frame_length=trace_get_framing_length(packet);
	printf("frame_Length=%ld\n",frame_length);
	if (payload_length == 0) 
		printf("can not get payload length");
	else
		printf("payload_Length=%ld\n",payload_length);	

	/* Get the source IP address */
	/* Note that we pass a casted sockaddr_storage into this function. This
	 * is because we do not know if the IP address we get back will be a
	 * v4 or v6 address. v6 addresses are much larger than v4 addresses and
	 * will not fit within a sockaddr_in structure used for storing IPv4
	 * addresses, leading to memory corruption and segmentation faults.
	 *
	 * The safest way to avoid this problem is to use a sockaddr_storage
	 * which is guaranteed to be large enough to contain any known address
	 * format. 
	 */
	addr_ptr = trace_get_source_address(packet, (struct sockaddr *)&addr);

	/* No IP address? Print "NULL" instead */
	if (addr_ptr == NULL)
		printf("NULL ");
	else
		print_ip(addr_ptr);
	
	/* Get the source port */
	port = trace_get_source_port(packet);

	/* If the port is zero, libtrace has told us that there is no
	 * legitimate port number present in the packet */
	if (port == 0)
		printf("NULL\n");
	else
		/* Port numbers are simply 16 bit values so we don't need to
		 * do anything special to print them. trace_get_source_port()
		 * even converts it into host byte order for us */
		printf("prot number: %u\n\n", port);

}

void libtrace_cleanup(libtrace_t *trace, libtrace_packet_t *packet) {

        /* It's very important to ensure that we aren't trying to destroy
         * a NULL structure, so each of the destroy calls will only occur
         * if the structure exists */
        if (trace)
                trace_destroy(trace);

        if (packet)
                trace_destroy_packet(packet);

}

int main(int argc, char *argv[])
{
        /* This is essentially the same main function from readdemo.c */

        libtrace_t *trace = NULL;
        libtrace_packet_t *packet = NULL;

	/* Ensure we have at least one argument after the program name */
        if (argc < 2) {
                fprintf(stderr, "Usage: %s inputURI\n", argv[0]);
                return 1;
        }

        packet = trace_create_packet();

        if (packet == NULL) {
                perror("Creating libtrace packet");
                libtrace_cleanup(trace, packet);
                return 1;
        }

        trace = trace_create(argv[1]);

        if (trace_is_err(trace)) {
                trace_perror(trace,"Opening trace file");
                libtrace_cleanup(trace, packet);
                return 1;
        }

        if (trace_start(trace) == -1) {
                trace_perror(trace,"Starting trace");
                libtrace_cleanup(trace, packet);
                return 1;
        }


        while (trace_read_packet(trace,packet)>0) {
                per_packet(packet);
        }


        if (trace_is_err(trace)) {
                trace_perror(trace,"Reading packets");
                libtrace_cleanup(trace, packet);
                return 1;
        }

        libtrace_cleanup(trace, packet);
        return 0;
}

