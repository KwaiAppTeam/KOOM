// Author: Qiushi Xue <xueqiushi@kuaishou.com>

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_TAG_MAP_FILE "/system/etc/event-log-tags"

struct EventTagMap;
typedef struct EventTagMap EventTagMap;

/*
 * Open the specified file as an event log tag map.
 *
 * Returns NULL on failure.
 */
EventTagMap *android_openEventTagMap(const char *fileName);

/*
 * Close the map.
 */
void android_closeEventTagMap(EventTagMap *map);

/*
 * Look up a tag by index.  Returns the tag string & string length, or NULL if
 * not found.  Returned string is not guaranteed to be nul terminated.
 */
const char *android_lookupEventTag_len(const EventTagMap *map, size_t *len, unsigned int tag);

/*
 * Look up a format by index. Returns the format string & string length,
 * or NULL if not found. Returned string is not guaranteed to be nul terminated.
 */
const char *android_lookupEventFormat_len(const EventTagMap *map, size_t *len, unsigned int tag);

#ifdef __cplusplus
}
#endif