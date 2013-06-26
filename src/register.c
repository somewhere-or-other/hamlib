/*
 *  Hamlib Interface - provides registering for dynamically loadable backends.
 *  Copyright (c) 2000-2005 by Stephane Fillod
 *
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/**
 * \brief Dynamic registration of backends
 * \file register.c
 *
 * doc todo: Let's explain what's going on here!
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>

/* This is libtool's dl wrapper */
#include <ltdl.h>

#include <hamlib/rig.h>

#include "register.h"

#ifndef PATH_MAX
# define PATH_MAX       1024
#endif

#define RIG_BACKEND_MAX 32

/*
 * RIG_BACKEND_LIST is defined in riglist.h, please keep it up to data,
 * 	ie. each time you give birth to a new backend
 * Also, it should be possible to register "external" backend,
 * that is backend that were not known by Hamlib at compile time.
 * Maybe, riglist.h should reserve some numbers for them? --SF
 */
struct {
	int be_num;
	const char *be_name;
	rig_model_t (* be_probe_all)(hamlib_port_t*, rig_probe_func_t, rig_ptr_t);
} rig_backend_list[RIG_BACKEND_MAX] = RIG_BACKEND_LIST;

/*
 * This struct to keep track of known rig models.
 * It is chained, and used in a hash table, see below.
 */
struct rig_list {
	const struct rig_caps *caps;
	lt_dlhandle handle;			/* handle returned by lt_dlopen() */
	struct rig_list *next;
};

/*----------------------------------------------------------------------
 * This code supports globally registering each backend initializer
 * function (see register.h)
 *----------------------------------------------------------------------
*/
#ifdef BUILD_MONOLITHIC

struct be_linked_list
{
	backend_type           *m_backend ;
	char                    *m_name;
	struct be_linked_list *m_next ;
} be_list_dummy;

static struct be_linked_list *backend_linked_list = NULL ;
static int backend_linked_list_size = 0;
static int backends_initialized = 0;

void be_list_initialize_family( backend_type *func, char *name )
{
	struct be_linked_list *ptr = malloc( sizeof(be_list_dummy) );
	ptr->m_next = backend_linked_list;
	ptr->m_backend = func;
	ptr->m_name = malloc(strlen(name) + 1);
	strcpy(ptr->m_name, name);
	backend_linked_list = ptr;

// uncomment if you need to check for correct creation of linked list
	printf("registering %s: me: %p, (%p)(), next %p\n",
		ptr->m_name, ptr, ptr->m_backend, ptr->m_next);

	backend_linked_list_size++;
}

#define INITIALIZE(rig) \
extern int MAKE_VERSIONED_FN(initrigs, ABI_VERSION, rig(void *be_handle)); \
	be_list_initialize_family(MAKE_VERSIONED_FN(initrigs, ABI_VERSION, rig), #rig);

void init_backend_list()
{
// list of backends of interest to building fldigi
	INITIALIZE(kenwood);
	INITIALIZE(tentec);
	INITIALIZE(yaesu);
	INITIALIZE(adat);
	INITIALIZE(alinco);
	INITIALIZE(aor);
	INITIALIZE(drake);
	INITIALIZE(dummy);
	INITIALIZE(flexradio);
	INITIALIZE(icom);
	INITIALIZE(jrc);
	INITIALIZE(lowe);
	INITIALIZE(pcr);
	INITIALIZE(prm80);
	INITIALIZE(racal);
}

void execute_link_list_initializers()
{
	if (backend_linked_list_size == 0) init_backend_list();

	printf("%d initializers globally registered\n", backend_linked_list_size);
	struct be_linked_list *p_be_linked_list = backend_linked_list;
	while (p_be_linked_list != NULL) {
		printf("Initializing %s using linked initializer list\n", p_be_linked_list->m_name);
		(p_be_linked_list->m_backend)();
		p_be_linked_list = p_be_linked_list->m_next;
	}
	backends_initialized = 1;
	return;
}

#endif /* BUILD_MONOLITHIC */

/*--------------------------------------------------------------------*/

#define RIGLSTHASHSZ 16
#define HASH_FUNC(a) ((a)%RIGLSTHASHSZ)

/*
 * The rig_hash_table is a hash table pointing to a list of next==NULL
 * 	terminated caps.
 */
static struct rig_list *rig_hash_table[RIGLSTHASHSZ];

int rig_lookup_backend(rig_model_t rig_model);

/*
 * Basically, this is a hash insert function that doesn't check for dup!
 */
int HAMLIB_API rig_register(const struct rig_caps *caps)
{
	int hval;
	struct rig_list *p;

	if (!caps)
		return -RIG_EINVAL;

#ifndef DONT_WANT_DUP_CHECK
	if (rig_get_caps(caps->rig_model)!=NULL)
		return -RIG_EINVAL;
#endif

	p = (struct rig_list*)malloc(sizeof(struct rig_list));
	if (!p) {
		rig_debug(RIG_DEBUG_VERBOSE, "rig register, failed to create struct rig_list*");
		return -RIG_ENOMEM;
	}

	hval = HASH_FUNC(caps->rig_model);
	p->caps = caps;
	p->handle = NULL;
	p->next = rig_hash_table[hval];
	rig_hash_table[hval] = p;

//printf("rig_register pointer to rig_has_table = %p\n", rig_hash_table);

	return RIG_OK;
}

/*
 * Get rig capabilities.
 * ie. rig_hash_table lookup
 */

const struct rig_caps * HAMLIB_API rig_get_caps(rig_model_t rig_model)
{
	struct rig_list *p;

	for (p = rig_hash_table[HASH_FUNC(rig_model)]; p; p=p->next) {
		if (p->caps->rig_model == rig_model)
			return p->caps;
	}

	return NULL;	/* sorry, caps not registered! */
}

/*
 * lookup for backend index in rig_backend_list table,
 * according to BACKEND_NUM
 * return -1 if not found.
 */
int rig_lookup_backend(rig_model_t rig_model)
{
#ifdef BUILD_MONOLITHIC
//	if (backends_initialized == 0) execute_link_list_initializers();
#endif

	int i;

	for (i=0; i<RIG_BACKEND_MAX && rig_backend_list[i].be_name; i++) {
		if (RIG_BACKEND_NUM(rig_model) ==
					rig_backend_list[i].be_num)
			return i;
	}

	return -1;
}

/*
 * rig_check_backend
 * check the backend declaring this model has been loaded
 * and if not loaded already, load it!
 * This permits seamless operation in rig_init.
 */
int HAMLIB_API rig_check_backend(rig_model_t rig_model)
{
#ifdef BUILD_MONOLITHIC
	if (backends_initialized == 0) execute_link_list_initializers();
#endif

	const struct rig_caps *caps;
	int be_idx;
	int retval;

	/* already loaded ? */
	caps = rig_get_caps(rig_model);
	if (caps)
		return RIG_OK;

	be_idx = rig_lookup_backend(rig_model);

	/*
	 * Never heard about this backend family!
	 */
	if (be_idx == -1) {
		rig_debug(RIG_DEBUG_VERBOSE, "rig_check_backend: unsupported "
					"backend %d for model %d\n",
					RIG_BACKEND_NUM(rig_model), rig_model);
		return -RIG_ENAVAIL;
	}

	retval = rig_load_backend(rig_backend_list[be_idx].be_name);

	return retval;
}



int HAMLIB_API rig_unregister(rig_model_t rig_model)
{
	int hval;
	struct rig_list *p,*q;

	hval = HASH_FUNC(rig_model);
	q = NULL;
	for (p = rig_hash_table[hval]; p; p=p->next) {
		if (p->caps->rig_model == rig_model) {
			if (q == NULL)
				rig_hash_table[hval] = p->next;
			else
				q->next = p->next;

			free(p);
			return RIG_OK;
		}
		q = p;
	}

	return -RIG_EINVAL;	/* sorry, caps not registered! */
}

/*
 * rig_list_foreach
 * executes cfunc on all the elements stored in the rig hash list
 */
int HAMLIB_API rig_list_foreach(int (*cfunc)(const struct rig_caps*, rig_ptr_t),rig_ptr_t data)
{
#ifdef BUILD_MONOLITHIC
	if (backends_initialized == 0) execute_link_list_initializers();
#endif

	struct rig_list *p;
	int i;

	if (!cfunc)
		return -RIG_EINVAL;

	int j = 0;

	for (i=0; i<RIGLSTHASHSZ; i++) {
		for (p=rig_hash_table[i]; p; p=p->next) {
			j++;
			if ((*cfunc)(p->caps,data) == 0)
				return RIG_OK;
		}
	}

	rig_debug(RIG_DEBUG_VERBOSE, "rig_list_foreach :number entries in hash table %d\n", j);

//printf("rig_list_foreach pointer to rig_has_table = %p\n",rig_hash_table);

	return RIG_OK;
}

int dummy_rig_probe(const hamlib_port_t *p, rig_model_t model, rig_ptr_t data)
{
	rig_debug(RIG_DEBUG_TRACE, "Found rig, model %d\n", model);
	return RIG_OK;
}

/*
 * rig_probe_first
 * called straight by rig_probe
 */
rig_model_t rig_probe_first(hamlib_port_t *p)
{
#ifdef BUILD_MONOLITHIC
	if (backends_initialized == 0) execute_link_list_initializers();
#endif

	int i;
	rig_model_t model;

	for (i=0; i<RIG_BACKEND_MAX && rig_backend_list[i].be_name; i++) {
		if (rig_backend_list[i].be_probe_all) {
			model = (*rig_backend_list[i].be_probe_all)(p, dummy_rig_probe, (rig_ptr_t)NULL);
			/* stop at first one found */
			if (model != RIG_MODEL_NONE)
				return model;
		}
	}
	return RIG_MODEL_NONE;
}

/*
 * rig_probe_all_backends
 * called straight by rig_probe_all
 */
int rig_probe_all_backends(hamlib_port_t *p, rig_probe_func_t cfunc, rig_ptr_t data)
{
#ifdef BUILD_MONOLITHIC
	if (backends_initialized == 0) execute_link_list_initializers();
#endif

	int i;

	for (i=0; i<RIG_BACKEND_MAX && rig_backend_list[i].be_name; i++) {
		if (rig_backend_list[i].be_probe_all) {
			(*rig_backend_list[i].be_probe_all)(p, cfunc, data);
		}
	}
	return RIG_OK;
}


int rig_load_all_backends()
{
#ifdef BUILD_MONOLITHIC
	if (backend_linked_list_size > 0) {
		execute_link_list_initializers();
		return RIG_OK;
	}
	return RIG_EINTERNAL;
#else /* remove all ltdl calls */
	int i;
	for (i = 0; i < RIGLSTHASHSZ; i++) {
		rig_hash_table[i] = NULL;
	}

	for (i=0; i<RIG_BACKEND_MAX && rig_backend_list[i].be_name; i++) {
		rig_load_backend(rig_backend_list[i].be_name);
	}
	return RIG_OK;
#endif /* remove all ltdl calls */
}


#define MAXFUNCNAMELEN 64
typedef int (*backend_init_t)(rig_ptr_t);

/*
 * rig_check_backend_version
 * Check that the versioned init function name for be_name is the correct version.
 */
int rig_check_backend_version(const lt_dlhandle be_handle, const char *be_name,
						backend_init_t *be_init)
{
#ifndef BUILD_MONOLITHIC /* remove all ltdl calls */
	char initfname[MAXFUNCNAMELEN];

	snprintf(initfname, MAXFUNCNAMELEN, "initrigs%d_%s", ABI_VERSION, be_name);
	*be_init = (backend_init_t) lt_dlsym(be_handle, initfname);
	if (!*be_init) {
		rig_debug(RIG_DEBUG_ERR, "rig:  dlsym(%s) failed (%s)\n",
					initfname, lt_dlerror());
		return -RIG_EINVAL;
	}
#endif /* remove all ltdl calls */
	return RIG_OK;
}

/*
 * rig_load_backend
 * Dynamically load a rig backend through dlopen mechanism
 */
int HAMLIB_API rig_load_backend(const char *be_name)
{
#ifndef BUILD_MONOLITHIC /* remove all ltdl calls */
# define PREFIX "hamlib-"

	lt_dlhandle be_handle;
	backend_init_t be_init;
	int status;
	char libname[PATH_MAX];
	char probefname[MAXFUNCNAMELEN];
	int i;

	/*
	 * lt_dlinit may be called several times
	 *
	 * FIXME: make static build seamless
	 */
#if 0
	LTDL_SET_PRELOADED_SYMBOLS();
#endif

	status = lt_dlinit();
	if (status) {
    		rig_debug(RIG_DEBUG_ERR, "rig_backend_load: lt_dlinit for %s "
							"failed: %s\n", be_name, lt_dlerror());
    		return -RIG_EINTERNAL;
	}

	lt_dladdsearchdir(HAMLIB_MODULE_DIR);

	/*
	 * add hamlib directory here
	 */
	snprintf (libname, sizeof (libname), PREFIX"%s", be_name);

	be_handle = lt_dlopenext (libname);
	be_handle = 0;

	/*
	 * external module not found? try dlopenself for backends
	 * compiled in static
	 */
	if (!be_handle || rig_check_backend_version(be_handle, be_name, &be_init) != RIG_OK) {
		rig_debug(RIG_DEBUG_VERBOSE, "rig:  lt_dlopen(\"%s\") failed (%s), "
						"trying static symbols...\n",
						libname, lt_dlerror());
		be_handle = lt_dlopen (NULL);

		if (!be_handle || rig_check_backend_version(be_handle, be_name, &be_init) != RIG_OK) {
			rig_debug(RIG_DEBUG_ERR, "rig:  lt_dlopen(\"%s\") failed (%s)\n",
						libname, lt_dlerror());
			return -RIG_EINVAL;
		}
	}


	/*
	 * register probe function if present
	 * NOTE: rig_load_backend might have been called upon a backend
	 * 	not in riglist.h! In this case, do nothing.
	 */
	for (i=0; i<RIG_BACKEND_MAX && rig_backend_list[i].be_name; i++) {
		if (!strncmp(be_name, rig_backend_list[i].be_name, 64)) {
    			snprintf(probefname, MAXFUNCNAMELEN, "probeallrigs%d_%s", ABI_VERSION, be_name);
    			rig_backend_list[i].be_probe_all =
				(rig_model_t (*)(hamlib_port_t*, rig_probe_func_t, rig_ptr_t))
						lt_dlsym (be_handle, probefname);
				break;
		}
	}

	status = (*be_init)(be_handle);

	rig_debug(RIG_DEBUG_VERBOSE, "rig: %s loaded\n",be_name);

 	return status;
#endif /* remove all ltdl calls */
	return RIG_OK;
}

