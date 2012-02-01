#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "conf.h"

void
conf_defaults(struct conf *pc)
{
	assert(pc != NULL);

	bzero(pc, sizeof *pc);
	pc->window_x = 0;
	pc->window_y = 0;
	pc->window_w = 400;
	pc->window_h = 300;
}

int
load_conf(struct conf *pc)
{
	assert(pc != NULL);

	return 0;
}

int
save_conf(struct conf *pc)
{
	assert(pc != NULL);

	return 0;
}
