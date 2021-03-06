/*
 * pio operations for remote fragment
 *
 * $Id: gfs_pio_remote.c 11042 2019-02-21 10:16:34Z n-soda $
 */

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <sys/socket.h> /* struct sockaddr */
#include <sys/time.h>

#include <openssl/evp.h>

#include <gfarm/gfarm.h>

#include "queue.h"
#include "timer.h"

#include "host.h"
#include "config.h"
#include "gfm_proto.h"	/* GFM_PROTO_CKSUM_MAXLEN in gfs_io.h */
#include "gfs_proto.h"	/* GFS_PROTO_FSYNC_* */
#define GFARM_USE_OPENSSL
#include "gfs_client.h"
#define GFARM_USE_GFS_PIO_INTERNAL_CKSUM_INFO
#include "gfs_io.h"
#include "gfs_pio.h"
#include "gfs_pio_impl.h"
#include "schedule.h"
#include "gfs_profile.h"
#include "context.h"

#ifdef HAVE_INFINIBAND
#ifdef __KERNEL__
#undef HAVE_INFINIBAND
#else /* __KERNEL__ */
#include "gfs_rdma.h"
#endif /* __KERNEL__ */
#endif

#define staticp	(gfarm_ctxp->gfs_pio_remote_static)

#define rdma_min_size (gfarm_ctxp->rdma_min_size)
#define rdma_static_max_size (gfarm_ctxp->rdma_mr_reg_static_max_size)
#define rdma_reg_mr_dynamic \
	(gfarm_ctxp->rdma_mr_reg_mode & GFARM_RDMA_REG_MR_DYNAMIC)
#define rdma_reg_mr_static \
	(gfarm_ctxp->rdma_mr_reg_mode & GFARM_RDMA_REG_MR_STATIC)

struct gfarm_gfs_pio_remote_static {
	double write_time, read_time;
	unsigned long long write_size, read_size;
	unsigned long long write_count, read_count;
#ifdef HAVE_INFINIBAND
	double rdma_write_time, rdma_read_time;
	unsigned long long rdma_write_size, rdma_read_size;
	unsigned long long rdma_write_count, rdma_read_count;
#endif
};

gfarm_error_t
gfarm_gfs_pio_remote_static_init(struct gfarm_context *ctxp)
{
	struct gfarm_gfs_pio_remote_static *s;

	GFARM_MALLOC(s);
	if (s == NULL)
		return (GFARM_ERR_NO_MEMORY);

	s->write_time =
	s->read_time = 0;
	s->write_size =
	s->read_size =
	s->write_count =
	s->read_count = 0;

#ifdef HAVE_INFINIBAND
	s->rdma_write_time =
	s->rdma_read_time = 0;
	s->rdma_write_size =
	s->rdma_read_size =
	s->rdma_write_count =
	s->rdma_read_count = 0;
#endif

	ctxp->gfs_pio_remote_static = s;
	return (GFARM_ERR_NO_ERROR);
}

void
gfarm_gfs_pio_remote_static_term(struct gfarm_context *ctxp)
{
	free(ctxp->gfs_pio_remote_static);
}

static gfarm_error_t
gfs_pio_remote_storage_close(GFS_File gf)
{
	gfarm_error_t e;
	struct gfs_file_section_context *vc = gf->view_context;
	struct gfs_connection *gfs_server = vc->storage_context;

	/*
	 * Do not close remote file from a child process because its
	 * open file count is not incremented.
	 * XXX - This behavior is not the same as expected, but better
	 * than closing the remote file.
	 */
	if (vc->pid != getpid())
		return (GFARM_ERR_NO_ERROR);

	e = gfs_client_close(gfs_server, gf->fd);
	gfarm_schedule_host_unused(
	    gfs_client_hostname(gfs_server),
	    gfs_client_port(gfs_server),
	    gfs_client_username(gfs_server),
	    gf->scheduled_age);

	vc->storage_context = NULL;
	gfs_client_connection_free(gfs_server);
	if (e != GFARM_ERR_NO_ERROR) {
		gflog_debug(GFARM_MSG_1001373,
			"gfs_client_close() failed: %s",
			gfarm_error_string(e));
	}
	return (e);
}
static gfarm_error_t
gfs_pio_remote_storage_pwrite(GFS_File gf,
	const char *buffer, size_t size, gfarm_off_t offset, size_t *lengthp)
{

	struct gfs_file_section_context *vc = gf->view_context;
	struct gfs_connection *gfs_server = vc->storage_context;
#ifdef HAVE_INFINIBAND
	struct rdma_context *rdma_context = gfs_ib_rdma_context(gfs_server);
#endif
	gfarm_error_t e;
	gfarm_timerval_t t1, t2;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	GFARM_KERNEL_UNUSE2(t1, t2);
	gfs_profile(gfarm_gettimerval(&t1));
#ifdef HAVE_INFINIBAND
	if (gfs_rdma_check(rdma_context) && size >= rdma_min_size) {
		gfarm_uint32_t rkey = 0;
		int bsize;
		void *buf;
		int reg_fail;

		gfs_client_connection_lock(gfs_server);

		reg_fail = gfs_rdma_get_bufinfo(rdma_context, &buf,
					&bsize, &rkey);
		if (!reg_fail && (!rdma_reg_mr_static	/* dynamic only */
		|| (rdma_reg_mr_dynamic && size > rdma_static_max_size)
			/* static|dynamic && big size */
		)) {
			void *mr;
			gfarm_uint32_t mrkey;
			long msize = (long) gfs_rdma_get_mlock_limit();

			if (size < msize)
				msize = size;
			if ((e = gfs_rdma_reg_mr_remote_read_write(rdma_context,
			 (char *)buffer, msize, &mr)) != GFARM_ERR_NO_ERROR) {
				gflog_debug(GFARM_MSG_1004618,
				"gfs_rdma_reg_mr_remote_read_write(%ld) failed",
					msize);
				reg_fail |= GFARM_RDMA_REG_MR_FAIL;
			} else {
				mrkey = gfs_rdma_get_rkey(mr);
				e = gfs_ib_rdma_pwrite(gfs_server, gf->fd,
					buffer, msize, offset, lengthp, mrkey);
				gfs_rdma_dereg_mr(rdma_context, &mr);
				goto rdma_done;
			}
		}
		if (rdma_reg_mr_static) {
			if (bsize < size && bsize < rdma_static_max_size
				/* static buffer is small && expandable */
				&& !reg_fail) {
				int nsize = gfs_rdma_resize_buffer(rdma_context,
						 size);
				if (nsize != bsize) /* success */
					gfs_rdma_get_bufinfo(rdma_context, &buf,
							&bsize, &rkey);
			}
			if (size < bsize)
				bsize = size;
			memcpy(buf, buffer, bsize);
			e = gfs_ib_rdma_pwrite(gfs_server, gf->fd, buf, bsize,
						offset, lengthp, rkey);
		} else {
			goto notrdma;
		}
rdma_done:
		gfs_client_connection_unlock(gfs_server);

		if (e != GFARM_ERR_NO_ERROR)
			return (e);
		gfs_profile(gfarm_gettimerval(&t2);
			staticp->rdma_write_time +=
					gfarm_timerval_sub(&t2, &t1);
			staticp->rdma_write_size += *lengthp;
			staticp->rdma_write_count++);
		return (e);
notrdma:
		gfs_client_connection_unlock(gfs_server);
	}
#endif
	/*
	 * buffer beyond GFS_PROTO_MAX_IOSIZE are just ignored by gfsd,
	 * we don't perform such GFS_PROTO_WRITE request, because it's
	 * inefficient.
	 * Note that upper gfs_pio layer should care this partial write.
	 */
	if (size > GFS_PROTO_MAX_IOSIZE)
		size = GFS_PROTO_MAX_IOSIZE;

	e = gfs_client_pwrite(gfs_server, gf->fd, buffer, size, offset,
					lengthp);

	if (e != GFARM_ERR_NO_ERROR)
		return (e);
	gfs_profile(gfarm_gettimerval(&t2));
	gfs_profile(staticp->write_time += gfarm_timerval_sub(&t2, &t1));
	gfs_profile(staticp->write_size += *lengthp);
	gfs_profile(staticp->write_count++);
	return (e);
}

static gfarm_error_t
gfs_pio_remote_storage_write(GFS_File gf,
	const char *buffer, size_t size, size_t *lengthp,
	gfarm_off_t *offsetp, gfarm_off_t *total_sizep)
{
	struct gfs_file_section_context *vc = gf->view_context;
	struct gfs_connection *gfs_server = vc->storage_context;

	/*
	 * buffer beyond GFS_PROTO_MAX_IOSIZE are just ignored by gfsd,
	 * we don't perform such GFS_PROTO_WRITE request, because it's
	 * inefficient.
	 * Note that upper gfs_pio layer should care this partial write.
	 */
	if (size > GFS_PROTO_MAX_IOSIZE)
		size = GFS_PROTO_MAX_IOSIZE;
	return (gfs_client_write(gfs_server, gf->fd, buffer, size,
	    lengthp, offsetp, total_sizep));
}

static gfarm_error_t
gfs_pio_remote_storage_pread(GFS_File gf,
	char *buffer, size_t size, gfarm_off_t offset, size_t *lengthp)
{

	struct gfs_file_section_context *vc = gf->view_context;
	struct gfs_connection *gfs_server = vc->storage_context;
#ifdef HAVE_INFINIBAND
	struct rdma_context *rdma_context = gfs_ib_rdma_context(gfs_server);
#endif
	gfarm_error_t e;
	gfarm_timerval_t t1, t2;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	GFARM_KERNEL_UNUSE2(t1, t2);
	gfs_profile(gfarm_gettimerval(&t1));
	/*
	 * Unlike gfs_pio_remote_storage_write(), we don't care
	 * buffer size here, because automatic i/o size truncation
	 * performed by gfsd isn't inefficient for read case.
	 * Note that upper gfs_pio layer should care the partial read.
	 */

#ifdef HAVE_INFINIBAND
	if (gfs_rdma_check(rdma_context) && size >= rdma_min_size) {
		gfarm_uint32_t rkey = 0;
		int bsize;
		void *buf;
		int reg_fail;

		gfs_client_connection_lock(gfs_server);

		reg_fail = gfs_rdma_get_bufinfo(rdma_context, &buf,
					&bsize, &rkey);
		if (!reg_fail && (!rdma_reg_mr_static	/* dynamic only */
		|| (rdma_reg_mr_dynamic && size > rdma_static_max_size)
			/* static|dynamic && big size */
		)) {
			void *mr;
			gfarm_uint32_t mrkey;
			long msize = (long) gfs_rdma_get_mlock_limit();

			if (size < msize)
				msize = size;
			if ((e = gfs_rdma_reg_mr_remote_read_write(rdma_context,
			 (char *)buffer, msize, &mr)) != GFARM_ERR_NO_ERROR) {
				gflog_debug(GFARM_MSG_1004619,
				"gfs_rdma_reg_mr_remote_read_write(%ld) failed",
					msize);
				reg_fail |= GFARM_RDMA_REG_MR_FAIL;
			} else {
				mrkey = gfs_rdma_get_rkey(mr);
				e = gfs_ib_rdma_pread(gfs_server, gf->fd,
					buffer, msize, offset, lengthp, mrkey);
				gfs_rdma_dereg_mr(rdma_context, &mr);
				goto rdma_done;
			}
		}
		if (rdma_reg_mr_static) {
			if (bsize < size && bsize < rdma_static_max_size
				/* static buffer is small && expandable */
				&& !reg_fail) {
				int nsize = gfs_rdma_resize_buffer(rdma_context,
						 size);
				if (nsize != bsize) /* success */
					gfs_rdma_get_bufinfo(rdma_context, &buf,
							&bsize, &rkey);
			}
			if (size < bsize)
				bsize = size;
			e = gfs_ib_rdma_pread(gfs_server, gf->fd, buf, bsize,
						offset, lengthp, rkey);
			if (e == GFARM_ERR_NO_ERROR)
				memcpy(buffer, buf, *lengthp);
		} else {
			goto notrdma;
		}
rdma_done:
		gfs_client_connection_unlock(gfs_server);

		if (e != GFARM_ERR_NO_ERROR)
			return (e);
		gfs_profile(gfarm_gettimerval(&t2);
			staticp->rdma_read_time +=
					gfarm_timerval_sub(&t2, &t1);
			staticp->rdma_read_size += *lengthp;
			staticp->rdma_read_count++);
		return (e);
notrdma:
		gfs_client_connection_unlock(gfs_server);
	}
#endif
	e = gfs_client_pread(gfs_server, gf->fd, buffer, size, offset,
					lengthp);

	if (e != GFARM_ERR_NO_ERROR)
		return (e);
	gfs_profile(gfarm_gettimerval(&t2));
	gfs_profile(staticp->read_time += gfarm_timerval_sub(&t2, &t1));
	gfs_profile(staticp->read_size += *lengthp);
	gfs_profile(staticp->read_count++);
	return (e);
}

static gfarm_error_t
gfs_pio_remote_storage_recvfile(GFS_File gf, gfarm_off_t r_off,
	int w_fd, gfarm_off_t w_off, gfarm_off_t len,
	EVP_MD_CTX *md_ctx, gfarm_off_t *recvp)
{
	gfarm_error_t e;
	struct gfs_file_section_context *vc = gf->view_context;
	struct gfs_connection *gfs_server = vc->storage_context;
	int md_aborted;
	gfarm_timerval_t t1, t2;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	GFARM_KERNEL_UNUSE2(t1, t2);
	gfs_profile(gfarm_gettimerval(&t1));

	e = gfs_client_recvfile(gfs_server, gf->fd, r_off,
	    w_fd, w_off, len, (gf->open_flags & GFARM_FILE_APPEND) != 0,
	    md_ctx, &md_aborted, recvp);
	if (e == GFARM_ERR_NO_ERROR) {
		gfs_profile(gfarm_gettimerval(&t2);
		staticp->read_time += gfarm_timerval_sub(&t2, &t1);
		staticp->read_size += *recvp;
		staticp->read_count++);
	}
	if (md_aborted)
		gf->mode &= ~GFS_FILE_MODE_DIGEST_CALC;
	return (e);
}

static gfarm_error_t
gfs_pio_remote_storage_sendfile(GFS_File gf, gfarm_off_t w_off,
	int r_fd, gfarm_off_t r_off, gfarm_off_t len,
	EVP_MD_CTX *md_ctx, gfarm_off_t *sentp)
{
	gfarm_error_t e;
	struct gfs_file_section_context *vc = gf->view_context;
	struct gfs_connection *gfs_server = vc->storage_context;
	gfarm_timerval_t t1, t2;

	GFARM_TIMEVAL_FIX_INITIALIZE_WARNING(t1);
	GFARM_KERNEL_UNUSE2(t1, t2);
	gfs_profile(gfarm_gettimerval(&t1));

	e = gfs_client_sendfile(gfs_server, gf->fd, w_off,
	    r_fd, r_off, len, md_ctx, sentp);
	if (e == GFARM_ERR_NO_ERROR) {
		gfs_profile(gfarm_gettimerval(&t2);
		staticp->write_time += gfarm_timerval_sub(&t2, &t1);
		staticp->write_size += *sentp;
		staticp->write_count++);
	}
	if (e != GFARM_ERR_NO_ERROR && md_ctx != NULL) {
		/*
		 * re: gfs_client_sendfile():
		 * when an error happens in gfsd side,
		 * the message digest in *md_ctx may not reflect
		 * the actually written contents.
		 * thus, *md_ctx has to be invalidated.
		 */
		gf->mode &= ~GFS_FILE_MODE_DIGEST_CALC;
	}
	return (e);
}

static gfarm_error_t
gfs_pio_remote_storage_ftruncate(GFS_File gf, gfarm_off_t length)
{
	struct gfs_file_section_context *vc = gf->view_context;
	struct gfs_connection *gfs_server = vc->storage_context;

	return (gfs_client_ftruncate(gfs_server, gf->fd, length));
}

static gfarm_error_t
gfs_pio_remote_storage_fsync(GFS_File gf, int operation)
{
	struct gfs_file_section_context *vc = gf->view_context;
	struct gfs_connection *gfs_server = vc->storage_context;

	return (gfs_client_fsync(gfs_server, gf->fd, operation));
}

static gfarm_error_t
gfs_pio_remote_storage_fstat(GFS_File gf, struct gfs_stat *st)
{
	struct gfs_file_section_context *vc = gf->view_context;
	struct gfs_connection *gfs_server = vc->storage_context;

	return (gfs_client_fstat(gfs_server, gf->fd,
	    &st->st_size, &st->st_atimespec.tv_sec, &st->st_atimespec.tv_nsec,
	    &st->st_mtimespec.tv_sec, &st->st_mtimespec.tv_nsec));
}

static gfarm_error_t
gfs_pio_remote_storage_cksum(GFS_File gf, const char *type,
	char *cksum, size_t size, size_t *lenp)
{
	struct gfs_file_section_context *vc = gf->view_context;
	struct gfs_connection *gfs_server = vc->storage_context;

	return (gfs_client_cksum(gfs_server, gf->fd, type, cksum, size, lenp));
}

static gfarm_error_t
gfs_pio_remote_storage_reopen(GFS_File gf)
{
	gfarm_error_t e;
	struct gfs_file_section_context *vc = gf->view_context;
	struct gfs_connection *gfs_server = vc->storage_context;

	if ((e = gfs_client_open(gfs_server, gf->fd)) != GFARM_ERR_NO_ERROR)
		gflog_debug(GFARM_MSG_1003379,
		    "gfs_client_open_local: %s", gfarm_error_string(e));
	return (e);
}

static int
gfs_pio_remote_storage_fd(GFS_File gf)
{
	/*
	 * Unlike Gfarm version 1, we tell the caller that
	 * gfs_client_connection_fd() isn't actually usable.
	 */
	return (-1);
}

struct gfs_storage_ops gfs_pio_remote_storage_ops = {
	gfs_pio_remote_storage_close,
	gfs_pio_remote_storage_fd,
	gfs_pio_remote_storage_pread,
	gfs_pio_remote_storage_pwrite,
	gfs_pio_remote_storage_ftruncate,
	gfs_pio_remote_storage_fsync,
	gfs_pio_remote_storage_fstat,
	gfs_pio_remote_storage_reopen,
	gfs_pio_remote_storage_write,
	gfs_pio_remote_storage_cksum,
	gfs_pio_remote_storage_recvfile,
	gfs_pio_remote_storage_sendfile,
};

gfarm_error_t
gfs_pio_open_remote_section(GFS_File gf, struct gfs_connection *gfs_server)
{
	gfarm_error_t e;
	struct gfs_file_section_context *vc = gf->view_context;

	e = gfs_client_open(gfs_server, gf->fd);
	if (e != GFARM_ERR_NO_ERROR) {
		gflog_debug(GFARM_MSG_1001374,
			"gfs_client_open() failed: %s",
			gfarm_error_string(e));
		return (e);
	}

	vc->ops = &gfs_pio_remote_storage_ops;
	vc->storage_context = gfs_server;
	vc->fd = -1; /* not used */
	vc->pid = getpid();
	return (GFARM_ERR_NO_ERROR);
}

struct gfs_profile_list remote_profile_items[] = {
	{ "remote_read_time", "remote read time   : %g sec", "%g", 'd',
	  offsetof(struct gfarm_gfs_pio_remote_static, read_time) },
	{ "remote_read_size", "remote read size   : %llu", "%llu", 'l',
	  offsetof(struct gfarm_gfs_pio_remote_static, read_size) },
	{ "remote_read_count", "remote read count  : %llu", "%llu", 'l',
	  offsetof(struct gfarm_gfs_pio_remote_static, read_count) },
	{ "remote_write_time", "remote write time  : %g sec", "%g", 'd',
	  offsetof(struct gfarm_gfs_pio_remote_static, write_time) },
	{ "remote_write_size",  "remote write size  : %llu", "%llu", 'l',
	  offsetof(struct gfarm_gfs_pio_remote_static, write_size) },
	{ "remote_write_count", "remote write count : %llu", "%llu", 'l',
	  offsetof(struct gfarm_gfs_pio_remote_static, write_count) },
#ifdef HAVE_INFINIBAND
	{ "rdma_read_time", "rdma read time   : %g sec", "%g", 'd',
	  offsetof(struct gfarm_gfs_pio_remote_static, rdma_read_time) },
	{ "rdma_read_size", "rdma read size   : %llu", "%llu", 'l',
	  offsetof(struct gfarm_gfs_pio_remote_static, rdma_read_size) },
	{ "rdma_read_count", "rdma read count  : %llu", "%llu", 'l',
	  offsetof(struct gfarm_gfs_pio_remote_static, rdma_read_count) },
	{ "rdma_write_time", "rdma write time  : %g sec", "%g", 'd',
	  offsetof(struct gfarm_gfs_pio_remote_static, rdma_write_time) },
	{ "rdma_write_size", "rdma write size  : %llu", "%llu", 'l',
	  offsetof(struct gfarm_gfs_pio_remote_static, rdma_write_size) },
	{ "rdma_write_count", "rdma write count : %llu", "%llu", 'l',
	  offsetof(struct gfarm_gfs_pio_remote_static, rdma_write_count) },
#endif
};

void
gfs_pio_remote_display_timers(void)
{
	int n = GFARM_ARRAY_LENGTH(remote_profile_items);

	gfs_profile_display_timers(n, remote_profile_items, staticp);
}

gfarm_error_t
gfs_pio_remote_profile_value(const char *name, char *value, size_t *sizep)
{
	int n = GFARM_ARRAY_LENGTH(remote_profile_items);

	return (gfs_profile_value(name, n, remote_profile_items,
		    staticp, value, sizep));
}
