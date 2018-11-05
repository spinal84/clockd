#ifndef SERVER_H
#define SERVER_H

int server_init(void);
void server_quit(void);

struct server_callback
{
  const char *member;
  DBusMessage *(*callback)(DBusMessage *method_call);
};

#endif // SERVER_H
