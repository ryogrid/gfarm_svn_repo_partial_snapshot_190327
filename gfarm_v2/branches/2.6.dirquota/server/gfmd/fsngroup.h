/*
 * $Id: fsngroup.h 10193 2016-08-14 14:09:12Z n-soda $
 */

struct host;
gfarm_error_t fsngroup_get_hosts(const char *, int *, struct host ***);

/*
 * Replication scheduler:
 */
struct inode;
struct dirset;
struct file_copy;
gfarm_error_t fsngroup_schedule_replication(
	struct inode *, struct dirset *tdirset,
	const char *, int, struct host **,
	int *, struct host **, gfarm_time_t, int *, struct host **,
	const char *, int *);

/*
 * Server side RPC stubs:
 */
struct peer;
gfarm_error_t gfm_server_fsngroup_get_all(
	struct peer *, int, int);
gfarm_error_t gfm_server_fsngroup_get_by_hostname(
	struct peer *, int, int);
gfarm_error_t gfm_server_fsngroup_modify(
	struct peer *, int, int);
