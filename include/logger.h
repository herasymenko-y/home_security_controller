#ifndef LOGGER_H
#define LOGGER_H

int logger_init(const char *filepath);
int log_event(const char *state_str, const char *message);
int logger_close(void);

#endif // LOGGER_H
