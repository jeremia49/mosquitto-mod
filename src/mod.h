#ifndef MOD_H
#define MOD_H

#include <stddef.h>

/* Single message */
typedef struct {
    void *data;
    size_t length;
} Message;

/* Topic */
typedef struct {
    char *name;
    Message *messages;
    size_t nmessage;
} Topic;

/* Multi-topic store */
typedef struct {
    Topic *topics;
    size_t count;
} SimpleMsgStore;

/* Store lifecycle */
SimpleMsgStore *store_create(void);
void store_free(SimpleMsgStore *store);

/* Publish API
 * - Creates topic if it does not exist
 * - Copies payload internally
 */
int store_publish(
    SimpleMsgStore *store,
    const char *topic,
    const void *payload,
    size_t payload_length
);

/* Query APIs */
size_t store_get_message_count(
    const SimpleMsgStore *store,
    const char *topic
);

/* JSON output API (uses cJSON)
 * - Returns JSON array string: ["msg1","msg2"]
 * - Caller must free() the returned string
 */
char *store_get_topic_messages_json(
    const SimpleMsgStore *store,
    const char *topic
);

/* Topic removal API
 * - Deletes topic and all its messages
 * - Returns 1 if deleted, 0 if not found
 */
int store_delete_topic(
    SimpleMsgStore *store,
    const char *topic
);

extern SimpleMsgStore *msgstore;

#endif /* MOD_H */