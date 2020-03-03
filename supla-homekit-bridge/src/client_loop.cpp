/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "client_loop.h"

#include <string>
#include <vector>

client_config *config;
void *sclient;

void client_loop_on_registererror(void *_suplaclient, void *sthread, int code) {
  if (code == SUPLA_RESULTCODE_BAD_CREDENTIALS) {
    st_app_terminate = 1;
    sthread_terminate(sthread);

  } else {
    usleep(5000000);
  }
}

void client_loop_location_update(void *_suplaclient, void *sthread,
                                 TSC_SuplaLocation *location) {
  supla_log(LOG_DEBUG, "Location #%i: %s EOL=%i", location->Id,
            location->Caption, location->EOL);
}

void client_loop_channel_update(void *_suplaclient, void *sthread,
                                TSC_SuplaChannel_C *channel) {
  supla_log(LOG_DEBUG, "Channel Update %d %d", channel->Id, channel->online);

  client_device_channel *c = channels->add_channel(
      channel->Id, 0, channel->Type, channel->Func, 0, 0, 0, NULL, NULL, NULL,
      false, channel->online == 1, channel->Caption);

  if (c != NULL) {
    c->setHKValue(channel->value.value, channel->value.sub_value,
                  channel->online);
  };
}

void client_loop_channelgroup_update(void *_suplaclient, void *sthread,
                                     TSC_SuplaChannelGroup_B *channel_group) {
  supla_log(LOG_DEBUG, "ChannelGroup #%i %s LocationID=%i, Function=%i EOL=%i",
            channel_group->Id, channel_group->Caption,
            channel_group->LocationID, channel_group->Func, channel_group->EOL);
}

void client_loop_channelgroup_relation_update(
    void *_suplaclient, void *sthread,
    TSC_SuplaChannelGroupRelation *channelgroup_realtion) {
  supla_log(LOG_DEBUG,
            "ChannelGroupRelation GroupId: %i ChannelId: %i, EOL: %i",
            channelgroup_realtion->ChannelGroupID,
            channelgroup_realtion->ChannelID, channelgroup_realtion->EOL);
}

void client_loop_on_event(void *_suplaclient, void *user_data,
                          TSC_SuplaEvent *event) {
  supla_log(LOG_DEBUG, "Event: %i, SenderID: %i, SenderName: %s", event->Event,
            event->SenderID, event->SenderName);
}

void client_loop_channel_value_update(void *_suplaclient, void *sthread,
                                      TSC_SuplaChannelValue *channel_value) {
  client_device_channel *channel = channels->find_channel(channel_value->Id);

  if (channel != NULL) {
    channel->setHKValue(channel_value->value.value,
                        channel_value->value.sub_value, channel_value->online);
  }
}

void client_loop_channel_extendedalue_update(
    void *_suplaclient, void *sthread,
    TSC_SuplaChannelExtendedValue *channel_extendedvalue) {
  client_device_channel *channel =
      channels->find_channel(channel_extendedvalue->Id);

  if (channel != NULL) {
    channel->setExtendedValue(&channel_extendedvalue->value);
  };
}

void client_on_registration_enabled(void *_suplaclient, void *user_data,
                                    TSDC_RegistrationEnabled *reg_enabled) {
  supla_log(LOG_DEBUG, "Client registration enabled to: %u",
            reg_enabled->client_timestamp);
  supla_log(LOG_DEBUG, "I/O Device registration enabled to: %u",
            reg_enabled->iodevice_timestamp);
}

void client_on_superuser_authorization_result(void *_suplaclient,
                                              void *user_data, char authorized,
                                              _supla_int_t code) {
  supla_log(LOG_DEBUG, "Super User %s",
            authorized == 1 ? "authorized" : "unauthorized");
}

void client_on_registered(void *_suplaclient, void *user_data,
                          TSC_SuplaRegisterClientResult_B *result) {
  channels->setInitialized(result->result_code == SUPLA_RESULTCODE_TRUE);
}

void client_on_device_calcfg_result(void *_suplaclient, void *user_data,
                                    TSC_DeviceCalCfgResult *result) {
  supla_log(LOG_DEBUG, "Device calcfg result");
}

void *client_loop_init(void *sthread, client_config *config) {
  TSuplaClientCfg scc;

  supla_client_cfginit(&scc);

  scc.protocol_version = config->getSuplaProtocolVersion();
  supla_log(LOG_DEBUG, "SUPLA PROTOCOL VERSION %d", scc.protocol_version);

  snprintf(scc.Name, SUPLA_CLIENT_NAME_MAXSIZE, "Supla MQTT Proxy");
  snprintf(scc.SoftVer, SUPLA_SOFTVER_MAXSIZE, "1.0-Linux");

  snprintf(scc.Email, SUPLA_EMAIL_MAXSIZE, config->getSuplaEmail().c_str());
  snprintf(scc.AccessIDpwd, SUPLA_ACCESSID_PWD_MAXSIZE,
           config->getSuplaPassword().c_str());

  scc.AccessID = config->getSuplaLocationId();

  memcpy(scc.clientGUID, cfg_client_GUID, SUPLA_GUID_SIZE);
  memcpy(scc.AuthKey, cfg_client_AuthKey, SUPLA_AUTHKEY_SIZE);

  scc.host = strdup(config->getSuplaHost().c_str());
  scc.ssl_port = config->getSuplaPort();
  scc.ssl_enabled = 1;
  scc.user_data = sthread;
  scc.cb_on_registererror = &client_loop_on_registererror;
  scc.cb_location_update = &client_loop_location_update;
  scc.cb_channel_update = &client_loop_channel_update;
  scc.cb_channel_value_update = &client_loop_channel_value_update;
  scc.cb_channel_extendedvalue_update =
      &client_loop_channel_extendedalue_update;
  scc.cb_channelgroup_update = &client_loop_channelgroup_update;
  scc.cb_channelgroup_relation_update =
      &client_loop_channelgroup_relation_update;
  scc.cb_on_event = &client_loop_on_event;
  scc.cb_on_registration_enabled = &client_on_registration_enabled;
  scc.cb_on_superuser_authorization_result =
      &client_on_superuser_authorization_result;
  scc.cb_on_device_calcfg_result = &client_on_device_calcfg_result;
  scc.cb_on_registered = &client_on_registered;

  void *result = supla_client_init(&scc);

  free(scc.host);
  return result;
}

void client_loop(void *user_data, void *sthread) {
  config = new client_config();
  config->load(cfg_config_file);
  sclient = client_loop_init(sthread, config);

  if (sclient == NULL) {
    if (config != NULL) free(config);
    st_app_terminate = 1;
    return;
  }

  if (user_data) *(void **)user_data = sclient;

  while (sthread_isterminated(sthread) == 0) {
    supla_log(LOG_INFO, "Connecting...");

    if (0 == supla_client_connect(sclient)) {
      usleep(2000000);
    } else {
      while (sthread_isterminated(sthread) == 0 &&
             supla_client_iterate(sclient, 10000000) == 1) {
      }
    }
  }

  if (user_data) *(void **)user_data = NULL;

  supla_client_free(sclient);

  if (config != NULL) delete config;
}