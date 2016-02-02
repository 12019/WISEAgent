#include "mqtthelper.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <mosquitto.h>

MQTT_CONNECT_CALLBACK on_mqtt_connect_cb = NULL;
MQTT_LOSTCONNECT_CALLBACK on_mqtt_lostconnect_cb = NULL;
MQTT_DISCONNECT_CALLBACK on_mqtt_disconnect_cb = NULL;
MQTT_MESSAGE_CALLBACK on_mqtt_message_func = NULL;

pthread_mutex_t g_publishmutex;

void MQTT_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	//printf("%s\n",mosquitto_connack_string(rc));
	if(rc == mqtt_err_success)
	{
		if(on_mqtt_connect_cb != NULL)
			on_mqtt_connect_cb(rc);
	}
	else
	{	
		if(on_mqtt_lostconnect_cb != NULL)
				on_mqtt_lostconnect_cb(rc);
	}
}

void MQTT_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	//printf("%s\n",mosquitto_connack_string(rc));
	if(rc == mqtt_err_success)
	{
		if(on_mqtt_disconnect_cb != NULL)
			on_mqtt_disconnect_cb(rc);
	}
	else
	{
		if(on_mqtt_lostconnect_cb != NULL)
			on_mqtt_lostconnect_cb(rc);
	}
}

void MQTT_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{	
	if(on_mqtt_message_func != NULL)
		on_mqtt_message_func(msg->topic, msg->payload, msg->payloadlen);
}

int MQTT_lib_version(int *major, int *minor, int *revision)
{
	return mosquitto_lib_version(major, minor, revision);
}

void * MQTT_Initialize(char const * devid)
{
	struct mosquitto *mosq = NULL;
	
	if(pthread_mutex_init(&g_publishmutex, NULL)!=0)
	{
		return mosq;
	}

	mosquitto_lib_init();
	mosq = mosquitto_new(devid, true, NULL);
	if (!mosq)
		return mosq;
	mosquitto_connect_callback_set(mosq, MQTT_connect_callback);
	mosquitto_disconnect_callback_set(mosq, MQTT_disconnect_callback);
	mosquitto_message_callback_set(mosq, MQTT_message_callback);
	
	return mosq;
}

void MQTT_Uninitialize(void *mosq)
{
	pthread_mutex_destroy(&g_publishmutex);

	on_mqtt_connect_cb = NULL;
	on_mqtt_lostconnect_cb = NULL;
	on_mqtt_disconnect_cb = NULL;
	on_mqtt_message_func = NULL;

	if(mosq == NULL)
		return;

	mosquitto_connect_callback_set(mosq, NULL);
	mosquitto_disconnect_callback_set(mosq, NULL);
	mosquitto_message_callback_set(mosq, NULL);

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
}

void MQTT_Callback_Set(void *mosq, MQTT_CONNECT_CALLBACK connect_cb, MQTT_LOSTCONNECT_CALLBACK lostconnect_cb, MQTT_DISCONNECT_CALLBACK disconnect_cb, MQTT_MESSAGE_CALLBACK message_cb)
{
	if(mosq == NULL)
		return;
	on_mqtt_connect_cb = connect_cb;
	on_mqtt_lostconnect_cb = lostconnect_cb;
	on_mqtt_disconnect_cb = disconnect_cb;
	on_mqtt_message_func = message_cb;
}

int MQTT_SetTLS(void *mosq, const char *cafile, const char *capath, const char *certfile, const char *keyfile, int (*pw_callback)(char *buf, int size, int rwflag, void *userdata))
{
	int result = MOSQ_ERR_SUCCESS;
	if(mosq == NULL)
		return MOSQ_ERR_INVAL;
	mosquitto_tls_insecure_set(mosq, true);
	result = mosquitto_tls_set(mosq, cafile, capath, certfile, keyfile, pw_callback);
	return result;
}

int MQTT_SetTLSPSK(void *mosq, const char *psk, const char *identity, const char *ciphers)
{
	int result = MOSQ_ERR_SUCCESS;
	if(mosq == NULL)
		return MOSQ_ERR_INVAL;
	result = mosquitto_tls_psk_set(mosq, psk, identity, ciphers) ;
	return result;
}

int MQTT_Connect(void *mosq, char const * ip, int port, char const * username, char const * password, int keepalive, char* willtopic, const void *willmsg, int msglen )
{
	int result = MOSQ_ERR_SUCCESS;
	if(mosq == NULL)
		return MOSQ_ERR_INVAL;

	if( username!= NULL && password != NULL)
		mosquitto_username_pw_set(mosq,username,password);

	mosquitto_will_clear(mosq);

	if(willmsg != NULL) {
		mosquitto_will_set(mosq, willtopic, msglen, willmsg, 0, false);
	}
	result = mosquitto_connect(mosq, ip, port, keepalive);
	if(result == MOSQ_ERR_SUCCESS)
	{
		mosquitto_loop_start(mosq);
	}
	return result;
}

void MQTT_Disconnect(void *mosq, bool bForce)
{
	if(mosq == NULL)
		return;

	if(!bForce)
		mosquitto_loop(mosq, 0, 1);	

	if(mosquitto_disconnect(mosq) == MOSQ_ERR_SUCCESS)
	{
		printf("MQTT Disconnected\n");
		if(!bForce)
			mosquitto_loop(mosq, 0, 1);	
	}

	mosquitto_loop_stop(mosq, false);
}

int MQTT_Reconnect(void *mosq)
{
	int result = MOSQ_ERR_SUCCESS;

	if(mosq == NULL)
		return MOSQ_ERR_INVAL;
	result = mosquitto_reconnect(mosq);
	if(result == MOSQ_ERR_SUCCESS)
	{
		mosquitto_loop_start(mosq);
	}
	return result;
}

int MQTT_Publish(void *mosq,  char* topic, const void *msg, int msglen, int qos, bool retain)
{
	int result = MOSQ_ERR_SUCCESS;
	if(mosq == NULL)
		return MOSQ_ERR_INVAL;
	pthread_mutex_lock(&g_publishmutex);
	result = mosquitto_publish(mosq, NULL, topic, msglen, msg, qos, retain);
	pthread_mutex_unlock(&g_publishmutex);
	return result;
}

int MQTT_Subscribe(void *mosq,  char* topic, int qos)
{
	int result = MOSQ_ERR_SUCCESS;
	if(mosq == NULL)
		return MOSQ_ERR_INVAL;
	result = mosquitto_subscribe(mosq, NULL, topic, qos);
	return result;
}

void MQTT_Unsubscribe(void *mosq,  char* topic)
{
	if(mosq == NULL)
		return;

	mosquitto_unsubscribe(mosq, NULL, topic);
}

int MQTT_GetSocket(void *mosq)
{
	if(mosq == NULL)
		return -1;
	return mosquitto_socket(mosq);
}

char* MQTT_GetErrorString(int rc)
{
	return mosquitto_strerror(rc);
}
