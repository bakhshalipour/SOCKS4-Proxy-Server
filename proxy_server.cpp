#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <pthread.h>
#include <stdio.h>

#define 	BACKLOG				10
#define		COMMAND_PACKET_SIZE		500
#define		SUCCESSFUL_CONNECTION		90
#define		UNSUCCESSFUL_CONECTION		91
#define		DATA_PACKET_SIZE		32 * 1024

using namespace std;

struct __attribute__ ((__packed__)) arguments
{
	int file_descriptor;
	struct sockaddr_in socket_address;
};

struct __attribute__ ((__packed__)) new_thread_arguments
{
	int source_file_descriptor;
	int dest_file_descriptor;
	pthread_mutex_t *mutex_exit;
	bool *already_done;
};

pthread_mutex_t mutex_print;

void* worker(void *args);
void* new_worker(void *args);

int main(int argc, char *argv[])
{
	pthread_mutex_init(&mutex_print, 0);
	
	int port = atoi(argv[1]);
	int sd = socket(PF_INET, SOCK_STREAM, 0);

	if (sd == -1)	
	{
		cerr << "Error in creating socket." << endl;
		return -1;
	}

	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_port = htons(port);
	addr.sin_family = PF_INET;
	
	if (bind(sd, (sockaddr *)&addr, sizeof(addr)))
	{
		cerr << "Error in bind." << endl;
		close(sd);
		return -1;
	}

	if (listen(sd, BACKLOG))	
	{
		cerr << "Error in listen." << endl;
		close(sd);
		return -1;
	}

	while (true)
	{
		struct sockaddr_in client_addr;
		int client_addr_size = sizeof(client_addr);
		int cd = accept(sd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
		if (cd == -1)
			continue;
		struct arguments args;
		args.file_descriptor = cd;
		args.socket_address = client_addr;
		
		pthread_t tid;
		pthread_create(&tid, 0, worker, new arguments(args));
	}

	close(sd);
	
	pthread_mutex_destroy(&mutex_print);
	return 0;
}

void *worker(void *args)
{
	arguments *my_args = (arguments *)args;
	int cd = my_args->file_descriptor;
	struct sockaddr_in client_addr = my_args->socket_address;
	char *client_ip;
	uint16_t client_port;
	client_ip = inet_ntoa(client_addr.sin_addr);
	client_port = ntohs(client_addr.sin_port);

	pthread_mutex_lock(&mutex_print);
	cout << "A client from " << client_ip << ":" << client_port << " was connected" << endl;
	pthread_mutex_unlock(&mutex_print);

	int satisfier_size = 1;
	char *command_packet = (char *)malloc(sizeof(char) * COMMAND_PACKET_SIZE * satisfier_size);
	memset(command_packet, 0, sizeof(char) * COMMAND_PACKET_SIZE * satisfier_size);	
	int received_bytes = 0;
	bool requested_by_name = false;

	while (true)
	{
		received_bytes += recv(cd, command_packet + received_bytes, satisfier_size * COMMAND_PACKET_SIZE - received_bytes, 0);

		if (received_bytes < COMMAND_PACKET_SIZE * satisfier_size && command_packet[received_bytes - 1] != 0)
			continue;
		else if (received_bytes == satisfier_size * COMMAND_PACKET_SIZE) 
		{
			satisfier_size++;
			command_packet = (char *)realloc(command_packet, sizeof(char) * COMMAND_PACKET_SIZE * satisfier_size);
			continue;
		}
		else
		{
			requested_by_name = (command_packet[4] == 0) && (command_packet[5] == 0) && (command_packet[6] == 0) && (command_packet[7] != 0);
			if (requested_by_name && received_bytes == 9 + strlen(command_packet + 8))
				continue;
			break;
		}
	}

	if ((unsigned int)command_packet[0] != 4 || (unsigned int)command_packet[1] != 1)
	{
		cerr << "Invalid command." << endl;
		close (cd);
		pthread_exit(0);
	}

	uint16_t server_port = (command_packet[2] << 8) + command_packet[3];
	
	stringstream server_ip_stream;
	server_ip_stream << (int)(unsigned char)command_packet[4] << "." << (int)(unsigned char)command_packet[5] << "." 
		<< (int)(unsigned char)command_packet[6] << "." << (int)(unsigned char)command_packet[7];
	stringstream server_port_stream;
	server_port_stream << server_port;
	
	//const char *server_ip = &(server_ip_stream.str()[0]);
	const char *server_name = command_packet + 9 + strlen(command_packet + 8);
	//const char *server_port_string = &(server_port_stream.str()[0]);
	const char *final_server_ip;

	char server_ip [30], server_port_string [30];
	sprintf(server_ip, "%d.%d.%d.%d", (int)(unsigned char)command_packet[4], (int)(unsigned char)command_packet[5], (int)(unsigned char)command_packet[6], (int)(unsigned char)command_packet[7]);
	sprintf(server_port_string, "%d", server_port);

	pthread_mutex_lock(&mutex_print);
	cout << "The client from " << client_ip << ":" << client_port << " requested for " << (requested_by_name ? server_name : server_ip) << ":" << server_port << endl;
	pthread_mutex_unlock(&mutex_print);

	bool successful_connect = true;
	int sfd;
	char response_to_client[8] = {0};

	if (requested_by_name)	//Acknowledgment: Many codes of this block (till beginning of respective 'else' block) are from Linux man page. See "man getaddrinfo".
	{
		struct addrinfo hints;
		struct addrinfo *result, *rp;
		
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = PF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = 0;
		hints.ai_protocol = 0;

		if (getaddrinfo(server_name, server_port_string, &hints, &result))
			successful_connect = false;
		
		if (successful_connect)
		{
			for (rp = result; rp != NULL; rp = rp->ai_next) 
			{
		       		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		       		if (sfd == -1)
		           		continue;

		       		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
		           		break;                

		       		close(sfd);
          		}
			
			if (rp == NULL) 
				successful_connect = false;
		}

		if (successful_connect)	
		{
			char *connected_ip;
			struct sockaddr_in *connected_addr;
			connected_addr = (sockaddr_in *)rp->ai_addr;
			connected_ip = inet_ntoa(connected_addr->sin_addr);
			final_server_ip = connected_ip;
			
			pthread_mutex_lock(&mutex_print);
			cout << "Connection established with " << connected_ip << ":" << server_port << endl;
			pthread_mutex_unlock(&mutex_print);

			response_to_client[1] = SUCCESSFUL_CONNECTION;
			int response_to_client_bytes = 0;
			while (response_to_client_bytes < 8)
				response_to_client_bytes += send(cd, response_to_client + response_to_client_bytes, 8 - response_to_client_bytes, 0);
		}
		else
		{
			pthread_mutex_lock(&mutex_print);
			cout << "Could not connect to " << server_name << ":" << server_port << endl;
			pthread_mutex_unlock(&mutex_print);

			response_to_client[1] = UNSUCCESSFUL_CONECTION;
			int response_to_client_bytes = 0;
			while (response_to_client_bytes < 8)
				response_to_client_bytes += send(cd, response_to_client + response_to_client_bytes, 8 - response_to_client_bytes, 0);
			close(cd);
			close(sfd);
			pthread_exit(0);
		}
	}

	else
	{
		sfd = socket(PF_INET, SOCK_STREAM, 0);

		if (sfd == -1)
			successful_connect = false;

		sockaddr_in connection_via_server;
		memset(&connection_via_server, 0, sizeof(sockaddr_in));
		connection_via_server.sin_family = AF_INET;
		connection_via_server.sin_port = htons(server_port);
		connection_via_server.sin_addr.s_addr = inet_addr(server_ip);
		int connection_via_server_size = sizeof(connection_via_server);	

		if (connect(sfd, (struct sockaddr *)&connection_via_server, (socklen_t)connection_via_server_size) == -1)
			successful_connect = false;
		
		if (successful_connect)
		{
			final_server_ip = server_ip;
			pthread_mutex_lock(&mutex_print);	
			cout << "Connection established with " << server_ip << ":" << server_port << endl;
			pthread_mutex_unlock(&mutex_print);

			response_to_client[1] = SUCCESSFUL_CONNECTION;
			int response_to_client_bytes = 0;
			while (response_to_client_bytes < 8)
				response_to_client_bytes += send(cd, response_to_client + response_to_client_bytes, 8 - response_to_client_bytes, 0);
		}
		else
		{
			pthread_mutex_lock(&mutex_print);
			cout << "Could not connect to " << server_ip << ":" << server_port << endl;
			pthread_mutex_unlock(&mutex_print);
			
			pthread_mutex_unlock(&mutex_print);
			response_to_client[1] = UNSUCCESSFUL_CONECTION;
			int response_to_client_bytes = 0;
			while (response_to_client_bytes < 8)
				response_to_client_bytes += send(cd, response_to_client + response_to_client_bytes, 8 - response_to_client_bytes, 0);
			close(cd);
			close(sfd);
			pthread_exit(0);
		}
	}

	bool already_done = false;
	
	pthread_mutex_t mutex_exit;
	pthread_mutex_init(&mutex_exit, 0);
	struct new_thread_arguments *new_thread_args;
	new_thread_args = (struct new_thread_arguments *)calloc(1, sizeof(struct new_thread_arguments));
	
	new_thread_args->source_file_descriptor = sfd;
	new_thread_args->dest_file_descriptor = cd;
	new_thread_args->mutex_exit = &mutex_exit;
	new_thread_args->already_done = &already_done;
	
	pthread_t new_thread_id;	
	pthread_create(&new_thread_id, 0, new_worker, (void *)new_thread_args);

	int data_received_bytes;
	int data_sent_bytes;
	char data_buffer [DATA_PACKET_SIZE];

	while (true)
	{
		data_received_bytes = recv(cd, data_buffer, DATA_PACKET_SIZE, 0); 
		if (data_received_bytes <= 0)
		{
			pthread_mutex_lock(&mutex_exit);
			if (!already_done)
			{
				close(cd);
				close(sfd);
				already_done = true;
			}
			pthread_mutex_unlock(&mutex_exit);
			
			pthread_mutex_lock(&mutex_print);
			cout << "The client from " << client_ip << ":" << client_port << " was disconnected" << endl;
			cout << "The connection with " << final_server_ip << ":" << server_port << " was lost" << endl;
			pthread_mutex_unlock(&mutex_print);
			break;
		}
		data_sent_bytes = 0;
		while (data_sent_bytes < data_received_bytes)
			data_sent_bytes += send(sfd, data_buffer + data_sent_bytes, data_received_bytes - data_sent_bytes, 0);
	}

	pthread_mutex_destroy(&mutex_exit);
	pthread_exit(0);
	return NULL;
}

void *new_worker(void *args)
{
	struct new_thread_arguments *my_args;
	my_args = (struct new_thread_arguments *)args;

	int src_descriptor = my_args->source_file_descriptor;
	int dst_descriptor = my_args->dest_file_descriptor;
	pthread_mutex_t *mutex_exit; 
	mutex_exit = my_args->mutex_exit;
	bool *already_done;
	already_done = my_args->already_done;

	int received_bytes;
	int sent_bytes;
	char buffer [DATA_PACKET_SIZE];

	while (true)
	{
		received_bytes = recv(src_descriptor, buffer, DATA_PACKET_SIZE, 0); 
		if (received_bytes <= 0)
		{
			pthread_mutex_lock(mutex_exit);
			if (!*already_done)
			{
				close(src_descriptor);
				close(dst_descriptor);
				*already_done = true;		
			}
			pthread_mutex_unlock(mutex_exit);
			break;
		}
		sent_bytes = 0;
		while (sent_bytes < received_bytes)
			sent_bytes += send(dst_descriptor, buffer + sent_bytes, received_bytes - sent_bytes, 0);
	}

	pthread_exit(0);
	return NULL;
}


















