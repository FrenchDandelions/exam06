#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

int		socketfd = -1;
char	recv_buf[1000];
char	send_buf[1000];
int		ids[5000];
char	*message = NULL;
char	*fatal = "Fatal error";
char	*array_strings[5000];
int		maxfd;
fd_set curr_set, write_set, read_set;

int	extract_msg(char **buf, char **msg)
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
			newbuf = calloc(1, sizeof(*newbuf) * (strlen((*buf + i + 1))) + 1);
			if (!newbuf)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = '\0';
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char	*strjoin(char *buf, char *add)
{
	int		len;
	char	*newbuf;

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
	if (socketfd != -1)
		close(socketfd);
	exit(0);
}

void	send_msg(int fd)
{
	for (int i = 0; i <= maxfd; i++)
	{
		if (FD_ISSET(i, &write_set) && i != fd && i != socketfd)
		{
			send(i, send_buf, strlen(send_buf), 0);
			if (message)
				send(i, message, strlen(message), 0);
		}
	}
}

int	main(int ac, char **av)
{
	struct sockaddr_in	serveraddr;

	int connectfd, id, len;
	if (ac != 2)
		err("Wrong number of arguments");
	if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err(fatal);
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	serveraddr.sin_port = htons(atoi(av[1]));
	if ((bind(socketfd, (const struct sockaddr *)&serveraddr,
				sizeof(serveraddr))) != 0 || (listen(socketfd, 10)) != 0)
		err(fatal);
	maxfd = socketfd;
	FD_ZERO(&curr_set);
	FD_SET(socketfd, &curr_set);
	id = 0;
	while (1)
	{
		read_set = write_set = curr_set;
		if (select(maxfd + 1, &read_set, &write_set, NULL, NULL) <= 0)
			continue ;
		if (FD_ISSET(socketfd, &read_set))
		{
			connectfd = accept(socketfd, NULL, NULL);
			if (connectfd < 0)
				err(fatal);
			array_strings[connectfd] = NULL;
			ids[connectfd] = id++;
			FD_SET(connectfd, &curr_set);
			if (connectfd > maxfd)
				maxfd = connectfd;
			sprintf(send_buf, "server: client %d just arrived\n",
				ids[connectfd]);
			send_msg(connectfd);
		}
		for (int fd = 0; fd <= maxfd; fd++)
		{
			if (FD_ISSET(fd, &read_set) && fd != socketfd)
			{
				len = recv(fd, recv_buf, sizeof(recv_buf), 0);
				if (len <= 0)
				{
					FD_CLR(fd, &curr_set);
					sprintf(send_buf, "server: client %d just left\n", ids[fd]);
					send_msg(fd);
					close(fd);
					if (array_strings[fd])
						free(array_strings[fd]);
					break ;
				}
				else
				{
					recv_buf[len] = '\0';
					array_strings[fd] = strjoin(array_strings[fd], recv_buf);
					message = NULL;
					while (extract_msg(&array_strings[fd], &message))
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
}
