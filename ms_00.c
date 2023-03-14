#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

fd_set afds, wfds, rfds;

int main(int ac, char **av)
{
	if (ac != 2)
	{
		write (2, "Wrong number of arguments\n", 26);
		exit 1;
	};
	
	FD_ZERO(&afds)
	int sockfd = create_socket();

	struct sockaddr_in servaddr; 
	bzero(&servaddr, sizeof(servaddr)); 
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))))
		fatal_error();
	if (listen(sockfd, SOMAXCONN))
		fatal_error();

	while (1)
	{
		wfds = rfds = afds;
		if (select(max_fd, &rfds, &wfds, NULL, NULL) < 0)
			fatal_error();

		for (int fd = 0; fd <= maxfd; fd++)
		{
			if (!FD_ISSET(fd, &rfds))
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
				read_buf[read_bytes] = '\0';
				msgs[fd] = str_join(msgs[fd], read_buf);
				send_msg(fd);
			}

		}
	}
	return 0;
}