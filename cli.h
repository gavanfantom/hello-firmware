/* cli.h */

#ifndef CLI_H
#define CLI_H

void cli_init(void);
void cli_reinit(void);

/* This arguably shouldn't be here, but it is for now. */
const char *lfs_error(int error);

#endif /* CLI_H */
