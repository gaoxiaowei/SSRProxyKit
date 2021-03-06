#ifndef __LOCAL_API_H__
#define __LOCAL_API_H__

#include "ssr_executive.h"

struct ssr_local_state;

int ssr_local_main_loop(const struct server_config *config, void(*feedback_state)(struct ssr_local_state *state, void *p), void *p);
int ssr_Local_listen_socket_fd(struct ssr_local_state *state);

void ssr_local_update_token(const char* ssr_token);
void ssr_token_safe_destroy();
void ssr_quit();
#endif // __LOCAL_API_H__
