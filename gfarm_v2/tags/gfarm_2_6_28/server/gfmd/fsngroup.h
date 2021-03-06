/*
 * $Id: fsngroup.h 10959 2019-01-15 14:02:28Z n-soda $
 */

struct host;
gfarm_error_t fsngroup_get_hosts(const char *, int *, struct host ***);

/*
 * Replication scheduler:
 */
struct inode;
struct file_copy;
struct hostset;
gfarm_error_t fsngroup_schedule_replication(
	struct inode *, const char *, int, struct host **,
	int *, struct hostset *, gfarm_time_t, int *, struct hostset *,
	const char *, int *, int *);
gfarm_error_t fsngroup_has_spare_for_repattr(struct inode *, int,
	const char *, const char *, int);

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
