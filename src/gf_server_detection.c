#include "gf_server_detection.h"

gfire_server_detector *gfire_server_detector_create(GCallback p_server_callback)
{
	if(!p_server_callback)
		return NULL;

	gfire_server_detector *ret = g_malloc0(sizeof(gfire_server_detector));
	if(!ret)
		return NULL;

	ret->mutex = g_mutex_new();
	ret->server_callback = p_server_callback;

	return ret;
}

void gfire_server_detector_free(gfire_server_detector *p_detector)
{
	if(!p_detector)
		return;

	// Stop the detector first
	gfire_server_detector_stop(p_detector);

	g_mutex_free(p_detector->mutex);

	g_free(p_detector);
}

gboolean gfire_server_detector_running(const gfire_server_detector *p_detector)
{
	if(!p_detector)
		return FALSE;

	g_mutex_lock(p_detector->mutex);
	gboolean ret = p_detector->running;
	g_mutex_unlock(p_detector->mutex);
	return ret;
}

gboolean gfire_server_detector_finished(const gfire_server_detector *p_detector)
{
	if(!p_detector)
		return FALSE;

	g_mutex_lock(p_detector->mutex);
	gboolean ret = p_detector->finished;
	g_mutex_unlock(p_detector->mutex);
	return ret;
}
