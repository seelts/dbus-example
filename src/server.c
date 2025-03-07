/*
test with
```sh
dbus-send --session --dest=example.server --print-reply /test example.server.interface.echo string:"Hello, DBus!"
```
*/

#include "constants.h"
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>

static void stop(int exit_code);
static DBusHandlerResult test_handler(DBusConnection *connection, DBusMessage *message, void *user_data);

static DBusConnection *dbus_connection;
static DBusError dbus_error;

int main(void) {
	dbus_error_init(&dbus_error);

	dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
	if (NULL == dbus_connection) {
		fprintf(stderr, "dbus connection failed\n");
		stop(EXIT_FAILURE);
	}

	int result = dbus_bus_request_name(dbus_connection, TEST_BUS_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE, &dbus_error);
	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != result) {
		fprintf(stderr, "can't claim "TEST_BUS_NAME" interface\n");
		stop(EXIT_FAILURE);
	}

	DBusObjectPathVTable object_vtable = {
		.message_function = test_handler,
		// .unregister_function = NULL,
	};
	if (FALSE == dbus_connection_register_object_path(dbus_connection, TEST_OBJECT_PATH, &object_vtable, NULL)) {
		fprintf(stderr, "can't register handler\n");
		stop(EXIT_FAILURE);
	}

	fprintf(stderr, "listening on "TEST_BUS_NAME"."TEST_INTERFACE"...\n");

	while (dbus_connection_read_write_dispatch(dbus_connection, DBUS_TIMEOUT_USE_DEFAULT));
	// DBusMessage *incoming_message = dbus_connection_pop_message(dbus_connection);
	// if (incoming_message) {
	// 	dbus_message_unref(incoming_message);
	// }

	stop(EXIT_SUCCESS);
}

//

static void stop(int exit_code) {
	if (dbus_error_is_set(&dbus_error)) {
		fprintf(stderr, "%s: %s\n", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
	}

	dbus_bus_release_name(dbus_connection, TEST_BUS_NAME, &dbus_error);
	dbus_connection_unref(dbus_connection);

	exit(exit_code);
}

static DBusHandlerResult test_handler(DBusConnection *connection, DBusMessage *message, void *user_data) {
	(void)connection;
	(void)user_data;

	fprintf(stderr, "__________________________\n");
	fprintf(stderr, "received a message on interface %s and object %s\n", dbus_message_get_interface(message), dbus_message_get_path(message));

	if (FALSE == dbus_message_is_method_call(message, TEST_BUS_NAME"."TEST_INTERFACE, TEST_METHOD)) {
		fprintf(stderr, "received message is not a "TEST_METHOD" method call on "TEST_INTERFACE"\n");
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	DBusMessageIter message_iterator;
	if (FALSE == dbus_message_iter_init(message, &message_iterator) || DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&message_iterator)) {
		fprintf(stderr, "received message is not a string\n");
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	const char *received_string = NULL;
	dbus_message_iter_get_basic(&message_iterator, &received_string);
	fprintf(stderr, "received string: %s\n", received_string);

	DBusMessage *reply = dbus_message_new_method_return(message);
	if (NULL == reply) {
		fprintf(stderr, "can't create a reply message\n");
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	DBusMessageIter reply_iterator;
	dbus_message_iter_init_append(reply, &reply_iterator);
	dbus_message_iter_append_basic(&reply_iterator, DBUS_TYPE_STRING, &received_string);

	if (FALSE == dbus_connection_send(dbus_connection, reply, NULL)) {
		fprintf(stderr, "can't send a reply (not enough memory)\n");
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	fprintf(stderr, "request echoed\n");

	dbus_message_unref(reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}
