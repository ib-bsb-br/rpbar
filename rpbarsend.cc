//  Copyright (C) 2010 Daniel Maturana
//  This file is part of rpbar.
//
//  rpbar is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  rpbar is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with rpbar. If not, see <http://www.gnu.org/licenses/>.
//

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <pwd.h>

// TODO just use c stuff
#include <string>
#include <sstream>

#include <ini.h>

typedef struct {
  const char *path;
} configuration;

static int handler(void* user, const char *section, const char *name, const char *value) {
  configuration* pconfig = (configuration*)user;

  #define SMATCH(s) strcmp(section, s) == 0
  #define NMATCH(n) strcmp(name, n) == 0
  #define MATCH(s, n) SMATCH(s) && NMATCH(n)
  if (MATCH("program", "socket_path")) pconfig->path = strdup(value);

  return 1;
}

int main(int argc, const char *argv[]) {
  const char *default_message = "m";
  const char *message;
  if (argc == 2) {
    message = argv[1];
  } else {
    message = default_message;
  }

  int sockfd;
  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
      perror("socket");
      exit(2);
  }

  struct sockaddr_un servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sun_family = AF_UNIX;

  // hacky solution
  const char *homedir;
  const char *cache_path = "/.cache/.rpbarsend";
  const char *config_path = "/.rpbar.ini";
  configuration config;
  char result_path[256];
  if ((homedir = getenv("HOME")) == NULL) {
     homedir = getpwuid(getuid())->pw_dir;
  }
  strcpy(result_path, homedir);
  strcat(result_path, cache_path);
  FILE *fptr;
  if ((fptr = fopen(result_path, "r")) == NULL) {
    strcpy(result_path, homedir);
    strcat(result_path, config_path);
	if (ini_parse(result_path, handler, &config)) {
      perror("config");
	}
  } else {
    // todo: read cache
  }

  uid_t uid = geteuid();
  std::stringstream ss;
  ss << config.path << "-" << uid;
  std::string socket_path(ss.str());

  strcpy(servaddr.sun_path, socket_path.c_str());
  int numbytes;
  if ((numbytes = sendto(sockfd,
                         message,
                         strlen(message),
                         0,
                         (struct sockaddr *) &servaddr,
                         sizeof(servaddr))) == -1) {
    perror("sendto");
    exit(1);
  }

  close(sockfd);
  return 0;
}
