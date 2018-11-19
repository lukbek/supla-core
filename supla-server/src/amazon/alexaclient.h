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

#ifndef AMAZON_ALEXACLIENT_H_
#define AMAZON_ALEXACLIENT_H_

/*
 * Indicates that the event was caused by a customer interaction with an
 * application. For example, a customer switches on a light or locks a door
 * using the Alexa app or an app provided by a device vendor.
 */
#define CAUSE_APP_INTERACTION 0

/*
 * Indicates that the event was caused by a physical interaction with an
 * endpoint. For example, manually switching on a light or manually locking a
 * door lock.
 */
#define CAUSE_PHYSICAL_INTERACTION 1

/*
 * Indicates that the event was caused by the periodic poll of an endpoint,
 * which found a change in value. For example, you might poll a temperature
 * sensor every hour and send the updated temperature to Alexa.
 */
#define CAUSE_PERIODIC_POLL 2

/*
 * Indicates that the event was caused by the application of a device rule. For
 * example, a customer configures a rule to switch on a light if a motion sensor
 * detects motion. In this case, Alexa receives an event from the motion sensor,
 * and another event from the light to indicate that its state change was caused
 * by the rule.
 */
#define CAUSE_RULE_TRIGGER 3

/*
 * Indicates that the event was caused by a voice interaction. For example, a
 * user speaking to their Echo device.
 */
#define CAUSE_VOICE_INTERACTION 4

class supla_amazon_alexa;

class supla_alexa_client {
 private:
  supla_amazon_alexa *alexa;
  void refresh_roken(void);

 protected:
  const char *getErrorString(const int code);
  int getErrorCode(const char *code);
  int aeg_post_request(char *data, int *httpResultCode);
  int aeg_post(char *data);

  void *getPowerControllerProperties(bool hi);
  void *getBrightnessControllerProperties(short brightness);
  void *getColorControllerProperties(int color, short brightness);
  void *getContactSensorProperties(bool hi);
  void *getEndpointHealthProperties(bool ok);
  void *getChangeReportHeader(void);
  void *getEndpoint(int channelId);
  void *addProps(void *props_array, void *props);
  void *getChangeReport(int causeType, int channelId, void *context_properties,
                        void *change_properties);
  bool sendChangeReport(int causeType, int channelId, bool online,
                        void *context_props, void *change_props);

 public:
  supla_alexa_client(supla_amazon_alexa *alexa);
  virtual ~supla_alexa_client();

  bool sendPowerChangeReport(int causeType, int channelId, bool hi,
                             bool online);
  bool sendContactChangeReport(int causeType, int channelId, bool hi,
                               bool online);
  bool sendBrightnessChangeReport(int causeType, int channelId, int brightness,
                                  bool online);
  bool sendColorChangeReport(int causeType, int channelId, int color,
                             short colorBrightness, bool online);
};

#endif /* AMAZON_ALEXACLIENT_H_ */
