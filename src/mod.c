#include "mod.h"

#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

/* ---------- Internal helpers ---------- */

static void topic_init(Topic *t, const char *name) {
    t->name = malloc(strlen(name) + 1);
    if (!t->name) return;

    strcpy(t->name, name);
    t->messages = NULL;
    t->nmessage = 0;
}

static void topic_destroy(Topic *t) {
    for (size_t i = 0; i < t->nmessage; i++) {
        free(t->messages[i].data);
    }
    free(t->messages);
    free(t->name);
}

static Topic *store_find_topic(SimpleMsgStore *store, const char *name) {
    for (size_t i = 0; i < store->count; i++) {
        if (strcmp(store->topics[i].name, name) == 0) {
            return &store->topics[i];
        }
    }
    return NULL;
}

static const Topic *store_find_topic_const(
    const SimpleMsgStore *store,
    const char *name
) {
    for (size_t i = 0; i < store->count; i++) {
        if (strcmp(store->topics[i].name, name) == 0) {
            return &store->topics[i];
        }
    }
    return NULL;
}

static Topic *store_add_topic(SimpleMsgStore *store, const char *name) {
    Topic *new_topics =
        realloc(store->topics,
                (store->count + 1) * sizeof(Topic));
    if (!new_topics) return NULL;

    store->topics = new_topics;
    topic_init(&store->topics[store->count], name);
    store->count++;

    return &store->topics[store->count - 1];
}

/* ---------- Public API ---------- */

SimpleMsgStore *store_create(void) {
    SimpleMsgStore *s = malloc(sizeof(SimpleMsgStore));
    if (!s) return NULL;

    s->topics = NULL;
    s->count = 0;
    return s;
}

void store_free(SimpleMsgStore *store) {
    if (!store) return;

    for (size_t i = 0; i < store->count; i++) {
        topic_destroy(&store->topics[i]);
    }
    free(store->topics);
    free(store);
}

int store_publish(
    SimpleMsgStore *store,
    const char *topic,
    const void *payload,
    size_t payload_length
) {
    if (!store || !topic || !payload || payload_length == 0) {
        return 0;
    }

    Topic *t = store_find_topic(store, topic);
    if (!t) {
        t = store_add_topic(store, topic);
        if (!t) return 0;
    }

    Message *new_msgs =
        realloc(t->messages,
                (t->nmessage + 1) * sizeof(Message));
    if (!new_msgs) return 0;

    t->messages = new_msgs;

    void *copy = malloc(payload_length+1);
    if (!copy) return 0;

    memcpy(copy, payload, payload_length);
            
	((char *)copy)[payload_length] = '\0';

    t->messages[t->nmessage].data = copy;
    t->messages[t->nmessage].length = payload_length;
    t->nmessage++;

    return 1;
}

size_t store_get_message_count(
    const SimpleMsgStore *store,
    const char *topic
) {
    if (!store || !topic) return 0;

    const Topic *t = store_find_topic_const(store, topic);
    if (!t) return 0;

    return t->nmessage;
}

char *store_get_topic_messages_json(
    const SimpleMsgStore *store,
    const char *topic
) {
    cJSON *array = cJSON_CreateArray();
    if (!array) return NULL;

    if (store && topic) {
        const Topic *t = store_find_topic_const(store, topic);
        if (t) {
            for (size_t i = 0; i < t->nmessage; i++) {
                const char *msg =
                    (const char *)t->messages[i].data;
                cJSON_AddItemToArray(
                    array,
                    cJSON_CreateString(msg)
                );
            }
        }
    }

    char *json = cJSON_PrintUnformatted(array);
    cJSON_Delete(array);
    return json; /* caller must free() */
}

int store_delete_topic(
    SimpleMsgStore *store,
    const char *topic
) {
    if (!store || !topic) return 0;

    for (size_t i = 0; i < store->count; i++) {
        Topic *t = &store->topics[i];

        if (strcmp(t->name, topic) == 0) {
            /* Free topic contents */
            topic_destroy(t);

            /* Move last topic into this slot */
            if (i != store->count - 1) {
                store->topics[i] = store->topics[store->count - 1];
            }

            store->count--;

            /* Shrink array (optional but nice) */
            if (store->count == 0) {
                free(store->topics);
                store->topics = NULL;
            } else {
                Topic *new_topics =
                    realloc(store->topics,
                            store->count * sizeof(Topic));
                if (new_topics) {
                    store->topics = new_topics;
                }
            }

            return 1; /* deleted */
        }
    }

    return 0; /* not found */
}