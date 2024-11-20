#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int		socketfd = -1;
int		max_fd;
int		ids[5000];
char	*array_string[5000];
char	*fatal = "Fatal error";
char	*message = NULL;
char	send_buf[1000];
char	recv_buf[1000];
fd_set current_set, write_set, read_set;

int	extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int		i;

	i = 0;
	*msg = 0;
	if (!(*buf))
		return (0);
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (!newbuf)
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

char	*strjoin(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	len = buf ? strlen(buf) : 0;
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (!newbuf)
		return (NULL);
	newbuf[0] = 0;
	if (buf)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void	err(char *s)
{
	write(2, s, strlen(s));
	write(2, "\n", 1);
	if (socketfd >= 0)
		close(socketfd);
	exit(0);
}

void	send_msg(int fd)
{
	for (int i = 0; i <= max_fd; i++)
	{
		if (FD_ISSET(i, &write_set) && i != fd & i != socketfd)
		{
			send(i, send_buf, strlen(send_buf), 0);
			if (message)
				send(i, message, strlen(message), 0);
		}
	}
}

int	main(int argc, char **argv)
{
	struct sockaddr_in	servaddr;

	int connect_fd, id, len;
	if (argc != 2)
	{
		err("Wrong number of arguments");
	}
	if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err(fatal);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	servaddr.sin_port = htons(atoi(argv[1]));
	if ((bind(socketfd, (const struct sockaddr *)&servaddr,
				sizeof(servaddr))) != 0 || listen(socketfd, 10) != 0)
		err(fatal);
	max_fd = socketfd;
	FD_ZERO(&current_set);
	FD_SET(socketfd, &current_set);
	id = 0;
	while (1)
	{
		read_set = write_set = current_set;
		if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) <= 0)
			continue ;
		if (FD_ISSET(socketfd, &read_set))
		{
			connect_fd = accept(socketfd, NULL, NULL);
			if (connect_fd <= 0)
				err(fatal);
			array_string[connect_fd] = NULL;
			ids[connect_fd] = id++;
			FD_SET(connect_fd, &current_set);
			if (connect_fd > max_fd)
				max_fd = connect_fd;
			sprintf(send_buf, "server: client %d just arrived\n",
				ids[connect_fd]);
			send_msg(connect_fd);
			continue ;
		}
		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (FD_ISSET(fd, &read_set) && fd != socketfd)
			{
				len = recv(fd, recv_buf, sizeof(recv_buf), 0);
				if (len <= 0)
				{
					FD_CLR(fd, &current_set);
					sprintf(send_buf, "server: client %d just left\n", ids[fd]);
					send_msg(fd);
					close(fd);
					if (array_string[fd])
						free(array_string[fd]);
					break ;
				}
				else
				{
					recv_buf[len] = '\0';
					array_string[fd] = strjoin(array_string[fd], recv_buf);
					message = NULL;
					while (extract_message(&array_string[fd], &message))
					{
						sprintf(send_buf, "client %d: ", ids[fd]);
						send_msg(fd);
						free(message);
						message = NULL;
					}
				}
			}
		}
	}
	return (0);
}
