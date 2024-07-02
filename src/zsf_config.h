
#ifndef ZSF_CONFIG_H_
#  define ZSF_CONFIG_H_

#  ifdef __cplusplus
extern "C" {
#  endif

#  include "sealock.h"

#  define ZSF_MAX_LOCKS 50

typedef struct zsf_config_struct {
  sealock_state_t locks[ZSF_MAX_LOCKS];
  unsigned int num_locks;
  time_t start_time;
  time_t current_time;
  time_t end_time;
} zsf_config_t;

int zsf_config_load(zsf_config_t *config_ptr, const char *filepath);
void zsf_config_unload(zsf_config_t *config_ptr);

#  ifdef __cplusplus
}
#  endif

#endif // ZSF_CONFIG_H_
