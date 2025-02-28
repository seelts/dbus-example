#include "constants.h"
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>

static void stop(int exit_code);
static void list_services(void);
static void echo_test(const char *message);
static void echo_test_async(const char *message);

static DBusConnection *dbus_connection;
static DBusError dbus_error;
static DBusMessage *dbus_message;
static bool should_stop = false;

int main() {
	dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
	if (NULL == dbus_connection) {
		fprintf(stderr, "dbus connection failed\n");
		stop(EXIT_FAILURE);
	}

	fprintf(stderr, "available services:\n");
	list_services();

	fprintf(stderr, "\n");

	fprintf(stderr, "sending a message to test service\n");
	echo_test("hey there");

	fprintf(stderr, "sending asynchronous message to test service\n");
	echo_test_async("hey there async");

	fprintf(stderr, "done\n");

	stop(EXIT_SUCCESS);
}

//

static void stop(int exit_code) {
	if (dbus_error_is_set(&dbus_error)) {
		fprintf(stderr, "%s: %s\n", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
	}

	// dbus_message_unref(dbus_message);
	dbus_connection_unref(dbus_connection);

	exit(exit_code);
}

static void list_services(void) {
	dbus_message = dbus_message_new_method_call(DBUS_SERVICE_DBUS, "/", DBUS_INTERFACE_DBUS, "ListNames");
	if (NULL == dbus_message) {
		fprintf(stderr, "dbus message initializatiom failed\n");
		stop(EXIT_FAILURE);
	}

	DBusMessage *reply = dbus_connection_send_with_reply_and_block(dbus_connection, dbus_message, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);
	if (NULL == reply) {
		fprintf(stderr, "dbus message sending failed\n");
		stop(EXIT_FAILURE);
	}

	dbus_message_unref(dbus_message);

	DBusMessageIter message_iterator;
	if (FALSE == dbus_message_iter_init(reply, &message_iterator)) {
		fprintf(stderr, "no services\n");
		stop(EXIT_SUCCESS);
	}

	DBusMessageIter array_iterator;
	dbus_message_iter_recurse(&message_iterator, &array_iterator);
	char *service_name;
	do {
		dbus_message_iter_get_basic(&array_iterator, &service_name);
		printf("%s\n", service_name);
	} while (dbus_message_iter_next(&array_iterator));
}

static void echo_test(const char *message) {
	dbus_message = dbus_message_new_method_call(TEST_BUS_NAME, TEST_OBJECT_PATH, TEST_BUS_NAME"."TEST_INTERFACE, TEST_METHOD);
	if (NULL == dbus_message) {
		fprintf(stderr, "dbus message initializatiom failed\n");
		stop(EXIT_FAILURE);
	}

	DBusMessageIter message_iterator;
	dbus_message_iter_init_append(dbus_message, &message_iterator);
	dbus_message_iter_append_basic(&message_iterator, DBUS_TYPE_STRING, &message);

	DBusMessage *reply = dbus_connection_send_with_reply_and_block(dbus_connection, dbus_message, DBUS_TIMEOUT_USE_DEFAULT, &dbus_error);
	if (NULL == reply) {
		fprintf(stderr, "dbus message sending failed\n");
		stop(EXIT_FAILURE);
	}

	dbus_message_unref(dbus_message);

	DBusMessageIter reply_iterator;
	if (FALSE == dbus_message_iter_init(reply, &reply_iterator) || DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&reply_iterator)) {
		fprintf(stderr, "received reply is not a string\n");
		stop(EXIT_FAILURE);
	}

	const char *received_string = NULL;
	dbus_message_iter_get_basic(&reply_iterator, &received_string);
	fprintf(stderr, "received string: %s\n", received_string);
}

static void handle_dbus_reply(DBusPendingCall *pending_call, void *user_data) {
	(void)user_data;

	DBusMessage *reply_message = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (NULL == reply_message) {
		fprintf(stderr, "dbus pending call: no reply received.\n");
		stop(EXIT_FAILURE);
	}

	char *received_string = NULL;
	if (dbus_message_get_args(reply_message, &dbus_error, DBUS_TYPE_STRING, &received_string, DBUS_TYPE_INVALID)) {
		fprintf(stderr, "received string: %s\n", received_string);
	} else {
		fprintf(stderr, "failed while getting a string from reply\n");
		stop(EXIT_FAILURE);
	}

	dbus_message_unref(reply_message);

	should_stop = true;
}

static void echo_test_async(const char *message) {
	dbus_message = dbus_message_new_method_call(TEST_BUS_NAME, TEST_OBJECT_PATH, TEST_BUS_NAME"."TEST_INTERFACE, TEST_METHOD);
	if (NULL == dbus_message) {
		fprintf(stderr, "dbus message initializatiom failed\n");
		stop(EXIT_FAILURE);
	}

	if (FALSE == dbus_message_append_args(dbus_message, DBUS_TYPE_STRING, &message, DBUS_TYPE_INVALID)) {
		fprintf(stderr, "dbus message payload initializatiom failed\n");
		stop(EXIT_FAILURE);
	}

	DBusPendingCall *pending_call = NULL;
	if (FALSE == dbus_connection_send_with_reply(dbus_connection, dbus_message, &pending_call, DBUS_TIMEOUT_USE_DEFAULT)) {
		fprintf(stderr, "dbus message sending failed\n");
		stop(EXIT_FAILURE);
	}

	dbus_message_unref(dbus_message);

	if (!pending_call) {
		fprintf(stderr, "dbus pending call failed\n");
		stop(EXIT_FAILURE);
	}

	if (FALSE == dbus_pending_call_set_notify(pending_call, handle_dbus_reply, NULL, NULL)) {
		fprintf(stderr, "dbus_pending_call_set_notify failed\n");
		stop(EXIT_FAILURE);
	}

	while (!should_stop && dbus_connection_read_write_dispatch(dbus_connection, DBUS_TIMEOUT_USE_DEFAULT));
}
