#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "vrt.h"
#include "cache/cache.h"
#include "cache/cache_director.h"
#include "cache/cache_backend.h"

#include "vcc_if.h"

struct trouble {
	unsigned		magic;
#define TROUBLE_MAGIC		0x4211ab21
	uint8_t			digest[DIGEST_LEN];
	double			timeout;
	VTAILQ_ENTRY(trouble)	list;
};

struct vmod_saintmode_saintmode {
	unsigned		magic;
#define VMOD_SAINTMODE_MAGIC	0xa03756e4
	struct director		sdir[1];
	const struct director	*be;
	pthread_mutex_t		mtx;
	unsigned		threshold;
	unsigned		n_trouble;
	VTAILQ_HEAD(, trouble)	troublelist;
};

VCL_BACKEND
vmod_saintmode_backend(VRT_CTX, struct vmod_saintmode_saintmode *sm) {

}

VCL_BOOL
vmod_blacklist(VRT_CTX, VCL_DURATION expires) {
	/* since 9ba9a8bb1b40 we can use ctx->bo->director_resp to
	 * find the offending backend */
}

static const struct director *
resolve(const struct director *sdir, struct worker *wrk, struct busyobj *bo) {
	/* if bo->digest is on trouble list: sick */

	return (NULL);
}

/* All adapted from PHK's saintmode implementation in Varnish 3.0 */
unsigned
healthy(const struct director *dir, struct busyobj *bo, double *changed) {
	struct trouble *tr;
	struct trouble *tr2;
	unsigned retval;
	unsigned int threshold;
	struct vmod_saintmode_saintmode *sm;
	VTAILQ_HEAD(, trouble)  troublelist;
	double now;

	CHECK_OBJ_NOTNULL(dir, DIRECTOR_MAGIC);
	CAST_OBJ_NOTNULL(sm, dir->priv, VMOD_SAINTMODE_MAGIC);
	CHECK_OBJ_NOTNULL(sm->be, DIRECTOR_MAGIC);

	/* If we don't have a bo with a digest to look at, we can't
	 * know if we are on the trouble list or not. Fall back to the
	 * backend's healthy() function. */
	if (!bo)
		return (sm->be->healthy(sm->be, bo, changed));

	/* Saintmode is disabled, or list is empty */
	if (sm->threshold == 0 || sm->n_trouble == 0)
		return (sm->be->healthy(sm->be, bo, changed));

	now = bo->t_first;

	retval = 1;
	VTAILQ_INIT(&troublelist);
	pthread_mutex_lock(&sm->mtx);
	VTAILQ_FOREACH_SAFE(tr, &sm->troublelist, list, tr2) {
		CHECK_OBJ_NOTNULL(tr, TROUBLE_MAGIC);

		if (tr->timeout < now) {
			VTAILQ_REMOVE(&sm->troublelist, tr, list);
			VTAILQ_INSERT_HEAD(&troublelist, tr, list);
			sm->n_trouble--;
			continue;
		}

		if (!memcmp(tr->digest, bo->digest, sizeof tr->digest)) {
			retval = 0;
			break;
		}
	}
	if (threshold <= sm->n_trouble)
		retval = 0;
	pthread_mutex_unlock(&sm->mtx);

	VTAILQ_FOREACH_SAFE(tr, &troublelist, list, tr2)
		FREE_OBJ(tr);

	return (retval ? sm->be->healthy(sm->be, bo, changed) : 0);
}

static const struct director *
resolve(const struct director *dir, struct worker *wrk, struct busyobj *bo) {
	struct vmod_saintmode_saintmode *sm;
	double changed = 0.0;

	CHECK_OBJ_NOTNULL(dir, DIRECTOR_MAGIC);
	CAST_OBJ_NOTNULL(sm, dir->priv, VMOD_SAINTMODE_MAGIC);

	if (!healthy(dir, bo, &changed))
		return (NULL);

	return (sm->be);
}

VCL_VOID
vmod_saintmode__init(VRT_CTX, struct vmod_saintmode_saintmode **smp,
    const char *vcl_name, VCL_BACKEND be, VCL_INT threshold) {
	struct vmod_saintmode_saintmode *sm;

	AN(smp);
	AZ(*smp);
	ALLOC_OBJ(sm, VMOD_SAINTMODE_MAGIC);
	AN(sm);
	*smp = sm;

	sm->threshold = threshold;
	sm->n_trouble = 0;
	AZ(pthread_mutex_init(&sm->mtx, NULL));
	CHECK_OBJ_NOTNULL(be, DIRECTOR_MAGIC);
	sm->be = be;
	VTAILQ_INIT(&sm->troublelist);

	sm->sdir->magic = DIRECTOR_MAGIC;
	sm->sdir->resolve = resolve;
	sm->sdir->healthy = healthy;
	REPLACE(sm->sdir->vcl_name, vcl_name);
	sm->sdir->name = "saintmode";
	sm->sdir->priv = sm;
}

VCL_VOID
vmod_saintmode__fini(struct vmod_saintmode_saintmode **smp) {
	struct trouble *tr, *tr2;
	struct vmod_saintmode_saintmode *sm;

	AN(smp);
	CHECK_OBJ_NOTNULL(*smp, VMOD_SAINTMODE_MAGIC);
	sm = *smp;
	*smp = NULL;

	VTAILQ_FOREACH_SAFE(tr, &sm->troublelist, list, tr2) {
		CHECK_OBJ_NOTNULL(tr, TROUBLE_MAGIC);
		VTAILQ_REMOVE(&sm->troublelist, tr, list);
		FREE_OBJ(tr);
	}

	free(sm->sdir->vcl_name);
	AZ(pthread_mutex_destroy(&sm->mtx));
	FREE_OBJ(sm);
}
