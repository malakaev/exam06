#include <unistd.h>
#include <string.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <stdio.h>

int		count = 0, max_fd = 0;
int		ids[65536];
char	*msgs[65536];

fd_set	actual_set, read_set, write_set; // тип данных набора fd-шников
char	buf_read[1001], buf_write[42];

int		extract_message(char **buf, char **msg)	// copy function from main
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

char	*str_join(char *buf, char *add)	// copy function from main
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
	exit(1);
}

void	notify_other(int author, char *str) // отправляет сообщение всем текущим (актуальным) клиентам, кроме отправителя или того о ком сообщается
{
	for (int fd = 0; fd <= max_fd; fd++)
		if (FD_ISSET(fd, &write_set) && fd != author)
			send(fd, str, strlen(str), 0); // отправлет сообщение в сокет, Если значение flags = 0 эквивалентен write
}

void	register_client(int fd)
{
	max_fd = fd > max_fd ? fd : max_fd;
	ids[fd] = count++;
	msgs[fd] = NULL;
	FD_SET(fd, &actual_set);
	sprintf(buf_write, "server: client %d just arrived\n", ids[fd]);
	notify_other(fd, buf_write);
}

void	remove_client(int fd)
{
	sprintf(buf_write, "server: client %d just left\n", ids[fd]);
	notify_other(fd, buf_write);
	free(msgs[fd]);
	FD_CLR(fd, &actual_set); // удаляет файловый дискриптор из набора
	close(fd);
}

void	send_msg(int fd)
{
	char *msg;
	while (extract_message(&(msgs[fd]), &msg))
	{
		sprintf(buf_write, "client %d: ", ids[fd]);
		notify_other(fd, buf_write);
		notify_other(fd, msg);
		free(msg);
	}
}

int		create_socket()
{
	max_fd = socket(AF_INET, SOCK_STREAM, 0); // copy line from main 61  Создает сокет и возвращает дескриптор сокета, AF_INET - для IPv4, SOCK_STREAM - для TCP
	if (max_fd < 0)
		fatal_error();
	FD_SET(max_fd, &actual_set);
	return max_fd;
}

int		main(int ac, char **av)
{
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	FD_ZERO(&actual_set);  // сначала очищаем набор fd-шников
	int sockfd = create_socket(); // моя функ создаем слушающий сокет

	struct sockaddr_in servaddr;		// copy line 58 from main (without cli) структура для сокетов
	bzero(&servaddr, sizeof(servaddr));	// copy line 68 from main
	// заполняем структуру - назначаем ip адрес и порт
	servaddr.sin_family = AF_INET;					// copy line 71 from main
	servaddr.sin_addr.s_addr = htonl(2130706433);	// copy line 72 from main
	servaddr.sin_port = htons(atoi(av[1])); 		// copy line 73 from main and edit port 8081->atoi(av[1])

	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)))	// copy line 76 from main (Осуществляет "привязку" IP-адреса и порта к серверному сокету)
		fatal_error();
	if (listen(sockfd, SOMAXCONN))	// copy line 82 from main and replace 10 with SOMAXCONN (Переводит сокет в режим приема новых соединений (TCP))
		fatal_error();

	while (1)
	{
		read_set = write_set = actual_set;

		if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0) // мониторинг сетевых событий. ждет изменения статуса
			fatal_error();

		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (!FD_ISSET(fd, &read_set)) // FD_ISSET проверяет существует ли еще fd в наборе read_set. если нет то вернет 0
				continue;
			if (fd == sockfd)
			{
				socklen_t addr_len = sizeof(servaddr); // длина структуры servaddr
				int client_fd = accept(sockfd, (struct sockaddr *)&servaddr, &addr_len); // copy line 87 from main and edit reference. Принимает новое соединение и возвращает дескриптор на него
				if (client_fd >= 0)
				{
					register_client(client_fd); // моя функ
					break ;
				}
			}
			else
			{
				int read_bytes = recv(fd, buf_read, 1000, 0); // чтение из сокета, если flags=0 то эквивалентен read
				if (read_bytes <= 0)
				{
					remove_client(fd); // моя функ - удаляет fd и очищает память
					break ;
				}
				buf_read[read_bytes] = '\0';
				msgs[fd] = str_join(msgs[fd], buf_read);
				send_msg(fd); // моя функ
			}
		}
	}
	return 0;
}
