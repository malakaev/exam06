#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>

int count = 0, max_fd = 0;
int ids[65000];
char *msgs[65000];

fd_set actual_set, read_set, write_set;
char	read_buf[1001];
char	write_buf[42];

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void	fatal_error()
{
	write(2, "Fatal error\n", 12);
	exit (1);
}

void	notify_other(int author, char *str)
{
	for (int fd = 0; fd <= max_fd; fd++)
		if (FD_ISSET(fd, &write_set) && fd != author)
			send(fd, str, strlen(str), 0);
}

void	register_client(int fd)
{
	max_fd = fd > max_fd ? fd : max_fd;
	ids[fd] = count++;
	msgs[fd] = NULL;
	FD_SET(fd, &actual_set);
	sprintf(write_buf, "server: client %d just arrived\n", ids[fd]);
	notify_other(fd, write_buf);
}

void	remove_client(int fd)
{
	sprintf(write_buf, "server: client %d just left\n", ids[fd]);
	notify_other(fd, write_buf);
	free(msgs[fd]);
	FD_CLR(fd, &actual_set);
	close(fd);
}

void	send_msg(int fd)
{
	char *msg;
	while(extract_message(&(msgs[fd]), &msg))
	{
		sprintf(write_buf, "client %d: ", ids[fd]);
		notify_other(fd, write_buf);
		notify_other(fd, msg);
		free (msg);
	}
}

int		create_socket()
{
	max_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (max_fd < 0)
		fatal_error();
	FD_SET(max_fd, &actual_set);
	return max_fd;
}

int	main(int ac, char **av)
{
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	FD_ZERO (&actual_set);
	int sockfd = create_socket();

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal_error();
	if (listen(sockfd, SOMAXCONN) != 0) 
		fatal_error();

	while (1)
	{
		read_set = write_set = actual_set;

		if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0)
			fatal_error();
		
		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (!FD_ISSET(fd, &read_set))
				continue;
			if (fd == sockfd)
			{
				socklen_t addr_len = sizeof(servaddr);
				int client_fd = accept(sockfd, (struct sockaddr *)&servaddr, &addr_len);
				if (client_fd >= 0)
				{
					register_client(client_fd);
					break ;
				}
			}
			else
			{
				int read_bytes = recv(fd, read_buf, 1000, 0);
				if (read_bytes <= 0)
				{
					remove_client(fd);
					break ;
				}
				read_buf[read_bytes] = "\0";
				msgs[fd] = str_join(msgs[fd], read_buf);
				send_msg(fd);
			}
		}
	}
	return 0;
}