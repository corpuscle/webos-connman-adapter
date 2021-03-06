/* @@@LICENSE
*
*      Copyright (c) 2012 Hewlett-Packard Development Company, L.P.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

/**
 * @file connman_service.c
 *
 * @brief Connman service interface
 *
 */

#include "connman_service.h"

/**
 * @brief  Check if the type of the service is wifi
 *
 * @param  service
 *
 */

gboolean connman_service_type_wifi(connman_service_t *service)
{
	if(NULL == service)
		return FALSE;
	return service->type == CONNMAN_SERVICE_TYPE_WIFI;
}

/**
 * @brief  Check if the type of the service is ethernet
 *
 * @param  service
 *
 */

gboolean connman_service_type_ethernet(connman_service_t *service)
{
	if(NULL == service)
		return FALSE;
	return service->type == CONNMAN_SERVICE_TYPE_ETHERNET;
}

/**
 * @brief  Map the service connection status to corresponding webos state
 * This function is required to send appropriate connection status to the webos world.
 *
 * @param  connman_state
 *
 */

gchar *connman_service_get_webos_state(int connman_state)
{
	switch (connman_state) 
	{
		case CONNMAN_SERVICE_STATE_DISCONNECT:
		case CONNMAN_SERVICE_STATE_IDLE:
			return "notAssociated";
		case CONNMAN_SERVICE_STATE_ASSOCIATION:
			return "associating";
		case CONNMAN_SERVICE_STATE_CONFIGURATION:
			return "associated";
		case CONNMAN_SERVICE_STATE_READY:
		case CONNMAN_SERVICE_STATE_ONLINE:
			return "ipConfigured";
		case CONNMAN_SERVICE_STATE_FAILURE:
			return "ipFailed";
        	break;
    }

    return "notAssociated";
}

/**
 * @brief  Convert the connection state string to its enum value
 *
 * @param  state
 *
 */

int connman_service_get_state(const gchar *state)
{
	int result = CONNMAN_SERVICE_STATE_IDLE;
	
	if(NULL == state)
		return result;

	if (g_str_equal(state, "idle"))
		result = CONNMAN_SERVICE_STATE_IDLE;
	else if (g_str_equal(state, "association"))
		result = CONNMAN_SERVICE_STATE_ASSOCIATION;
	else if (g_str_equal(state, "configuration"))
		result = CONNMAN_SERVICE_STATE_CONFIGURATION;
	else if (g_str_equal(state, "ready"))
		result = CONNMAN_SERVICE_STATE_READY;
	else if (g_str_equal(state, "online"))
		result = CONNMAN_SERVICE_STATE_ONLINE;
	else if (g_str_equal(state, "disconnect"))
		result = CONNMAN_SERVICE_STATE_DISCONNECT;
	else if (g_str_equal(state, "failure"))
		result = CONNMAN_SERVICE_STATE_FAILURE;

	return result;
}

/**
 * @brief  Connect to a remote connman service
 *
 * @param  service
 *
 */

gboolean connman_service_connect(connman_service_t *service)
{
	if(NULL == service)
		return FALSE;

	GError *error = NULL;
	gboolean ret = TRUE;

	connman_interface_service_call_connect_sync(service->remote, NULL, &error);
	if (error)
	{
		g_message("Error: %s", error->message);
		/* If the error is "AlreadyConnected" its not an error */
		if(NULL == g_strrstr(error->message,"AlreadyConnected"))
			ret = FALSE;
		g_error_free(error);
	}
	return ret;
}


/**
 * @brief  Disconnect from a remote connman service
 *
 * @param  service
 *
 */

gboolean connman_service_disconnect(connman_service_t *service)
{
	if(NULL == service)
		return FALSE;

	GError *error = NULL;

	connman_interface_service_call_disconnect_sync(service->remote, NULL, &error);
	if (error)
	{
		g_message("Error: %s", error->message);
		g_error_free(error);
		return FALSE;
	}
	return TRUE;
}


/**
 * @brief  Get all the network related information for a connected service (in online state)
 *
 * @param  service
 *
 */

gboolean connman_service_get_ipinfo(connman_service_t *service)
{
	if(NULL == service)
		return FALSE;

	GError *error = NULL;
	GVariant *properties;
	gsize i;

	connman_interface_service_call_get_properties_sync(service->remote, &properties, NULL, &error);
	if (error)
	{
		g_message("Error: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	for (i = 0; i < g_variant_n_children(properties); i++)
	{
		GVariant *property = g_variant_get_child_value(properties, i);
		GVariant *key_v = g_variant_get_child_value(property, 0);
		const gchar *key = g_variant_get_string(key_v, NULL);

		if (g_str_equal(key, "Ethernet"))
		{
			GVariant *v = g_variant_get_child_value(property, 1);
			GVariant *va = g_variant_get_child_value(v, 0);
			gsize j;
			for(j = 0; j < g_variant_n_children(va); j++)
	  		{
				GVariant *ethernet = g_variant_get_child_value(va, j);
				GVariant *ekey_v = g_variant_get_child_value(ethernet, 0);
				const gchar *ekey = g_variant_get_string(ekey_v, NULL);

				if(g_str_equal(ekey, "Interface"))
				{
					GVariant *ifacev = g_variant_get_child_value(ethernet, 1);
					GVariant *ifaceva = g_variant_get_variant(ifacev);
					service->ipinfo.iface = g_variant_dup_string(ifaceva, NULL);
				}
	  		}
		}
		if(g_str_equal(key, "IPv4"))
		{
			GVariant *v = g_variant_get_child_value(property, 1);
			GVariant *va = g_variant_get_child_value(v, 0);
			gsize j;
			for(j = 0; j < g_variant_n_children(va); j++)
			{
				GVariant *ipv4 = g_variant_get_child_value(va, j);
				GVariant *ikey_v = g_variant_get_child_value(ipv4, 0);
				const gchar *ikey = g_variant_get_string(ikey_v, NULL);

				if(g_str_equal(ikey, "Netmask"))
				{
					GVariant *netmaskv = g_variant_get_child_value(ipv4, 1);
					GVariant *netmaskva = g_variant_get_variant(netmaskv);
					service->ipinfo.netmask = g_variant_dup_string(netmaskva, NULL);
				}
				if(g_str_equal(ikey, "Address"))
				{
					GVariant *addressv = g_variant_get_child_value(ipv4, 1);
					GVariant *addressva = g_variant_get_variant(addressv);
					service->ipinfo.address = g_variant_dup_string(addressva, NULL);
				}
				if(g_str_equal(ikey, "Gateway"))
				{
					GVariant *gatewayv = g_variant_get_child_value(ipv4, 1);
					GVariant *gatewayva = g_variant_get_variant(gatewayv);
					service->ipinfo.gateway = g_variant_dup_string(gatewayva, NULL);
				}
			  }
			}
		if(g_str_equal(key, "Nameservers"))
		{
			GVariant *v = g_variant_get_child_value(property, 1);
			GVariant *va = g_variant_get_child_value(v, 0);
			service->ipinfo.dns = g_variant_dup_strv(va, NULL);
		}
	}
	return TRUE;
}


/**
 * @brief  Callback for service's "property_changed" signal
 *
 * @param  proxy
 * @param  property
 * @param  v
 * @param  service
 *
 */

static void
property_changed_cb(ConnmanInterfaceService *proxy, gchar * property, GVariant *v,
              connman_service_t      *service)
{
	/* Invoke function pointers only for state changed */
	if(g_str_equal(property, "State") == FALSE)
		return;
	g_free(service->state);
	service->state = g_variant_dup_string(g_variant_get_variant(v), NULL);

	if(NULL != service->handle_state_change_fn)
		(service->handle_state_change_fn)((gpointer)service, service->state);
}


void connman_service_register_state_changed_cb(connman_service_t *service, connman_state_changed_cb func)
{
	if(NULL == func)
		return;
        service->handle_state_change_fn = func;
}


/**
 * @brief Update service properties from the supplied variant
 *
 * @param service_v
 */

void connman_service_update_properties(connman_service_t *service, GVariant *service_v)
{
	if(NULL == service || NULL == service_v)
		return;

	GVariant *properties;
	gsize i;
	
	properties = g_variant_get_child_value(service_v, 1);

	for (i = 0; i < g_variant_n_children(properties); i++)
	{
		GVariant *property = g_variant_get_child_value(properties, i);
		GVariant *key_v = g_variant_get_child_value(property, 0);
		GVariant *val_v = g_variant_get_child_value(property, 1);
		GVariant *val = g_variant_get_variant(val_v);
		const gchar *key = g_variant_get_string(key_v, NULL);

		if (g_str_equal(key, "Name"))
			service->name =  g_variant_dup_string(val, NULL);

		if (g_str_equal(key, "Type"))
		{
			const gchar *v = g_variant_get_string(val, NULL);

			if (g_str_equal(v, "wifi"))
				service->type = CONNMAN_SERVICE_TYPE_WIFI;

			if (g_str_equal(v, "ethernet"))
				service->type = CONNMAN_SERVICE_TYPE_ETHERNET;
		}

		if (g_str_equal(key, "State"))
			service->state =  g_variant_dup_string(val, NULL);

		if (g_str_equal(key, "Strength"))
			service->strength = g_variant_get_byte(val);

		if(g_str_equal(key, "Security")) 
			service->security = g_variant_dup_strv(val, NULL);

		if (g_str_equal(key, "AutoConnect"))
			service->auto_connect = g_variant_get_boolean(val);

		if (g_str_equal(key, "Immutable"))
			service->immutable = g_variant_get_boolean(val);

		if (g_str_equal(key, "Favorite"))
			service->favorite = g_variant_get_boolean(val);

	}
}

/**
 * @brief  Create a new connman service instance and set its properties
 *
 * @param  variant
 *
 */

connman_service_t *connman_service_new(GVariant *variant)
{
	if(NULL == variant)
		return NULL;
	
	connman_service_t *service = g_new0(connman_service_t, 1);
	if(service == NULL)
	{
		g_error("Out of memory !!!");
		return NULL;
	}

	service->path = service->name = service->state = NULL;
	
	GVariant *service_v = g_variant_get_child_value(variant, 0);
	service->path = g_variant_dup_string(service_v, NULL);
	
	GError *error = NULL;


	service->remote = connman_interface_service_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
								G_DBUS_PROXY_FLAGS_NONE,
								"net.connman",
								service->path,
								NULL,
								&error);
	if (error)
	{
		g_error("%s", error->message);
		g_error_free(error);
		return NULL;
	}

	service->handle_state_change_fn = NULL;

	service->sighandler_id = g_signal_connect_data(G_OBJECT(service->remote), "property-changed",
		G_CALLBACK(property_changed_cb), service, NULL, 0);

	connman_service_update_properties(service, variant);

	return service;
}


/**
 * @brief  Free the connman service instance 
 *
 * @param  data
 * @param  user_data
 *
 */

void connman_service_free(gpointer data, gpointer user_data)
{
	connman_service_t *service = (connman_service_t *)data;

	if(NULL == service)
		return;

	g_free(service->path);
	g_free(service->name);
	g_free(service->state);
	g_strfreev(service->security);
	g_free(service->ipinfo.iface);
	g_free(service->ipinfo.address);
	g_free(service->ipinfo.netmask);
	g_free(service->ipinfo.gateway);
	g_strfreev(service->ipinfo.dns);

	if(service->sighandler_id)
		g_signal_handler_disconnect(G_OBJECT(service->remote), service->sighandler_id);
	service->handle_state_change_fn = NULL;

	g_free(service);
	service = NULL;
}

